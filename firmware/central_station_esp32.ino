#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

#include <WiFi.h>
#include "ThingSpeak.h"

#include <math.h>

// =====================================================
// nRF24
// =====================================================
#define CE_PIN   22
#define CSN_PIN   5
#define SCK_PIN  18
#define MISO_PIN 19
#define MOSI_PIN 23

RF24 radio(CE_PIN, CSN_PIN);
const byte pipeAddress[6] = "RxAAA";

// =====================================================
// Wi-Fi / ThingSpeak
// =====================================================
// Replace these with your own values.
const char* ssid = "YOUR_WIFI_SSID";
const char* pass = "YOUR_WIFI_PASSWORD";

unsigned long myChannelNumber = YOUR_CHANNEL_NUMBER;
const char* myWriteAPIKey = "YOUR_THINGSPEAK_WRITE_API_KEY";

WiFiClient client;

// Free ThingSpeak: minimum 15 s between updates.
const unsigned long THINGSPEAK_MIN_INTERVAL_MS = 15000UL;
unsigned long lastThingSpeakAttempt = 0;

// =====================================================
// Shared packet definition
// Must match transmitter exactly.
// 32 bytes total.
// =====================================================
static const uint32_t PACKET_MAGIC = 0x574E4431UL; // "WND1"
static const uint8_t PACKET_VERSION = 1;

struct __attribute__((packed)) SensorPacket {
  uint32_t magic;
  uint8_t  version;
  uint8_t  reserved;
  uint16_t seq;

  float temperatureC;
  float humidityPct;
  float seaLevelHpa;

  float windInstantMps;
  float windMeanMps;
  float windPeakMps;
};

static_assert(sizeof(SensorPacket) == 32, "SensorPacket must be exactly 32 bytes");

SensorPacket rx;

bool haveLastSeq = false;
uint16_t lastSeq = 0;

// =====================================================
// Helpers
// =====================================================
float mpsToKt(float mps) {
  return mps * 1.94384f;
}

float mpsToKmh(float mps) {
  return mps * 3.6f;
}

uint8_t beaufortFromMps(float mps) {
  if (mps < 0.5f)  return 0;
  if (mps < 1.6f)  return 1;
  if (mps < 3.4f)  return 2;
  if (mps < 5.5f)  return 3;
  if (mps < 8.0f)  return 4;
  if (mps < 10.8f) return 5;
  if (mps < 13.9f) return 6;
  if (mps < 17.2f) return 7;
  if (mps < 20.8f) return 8;
  if (mps < 24.5f) return 9;
  if (mps < 28.5f) return 10;
  if (mps < 32.7f) return 11;
  return 12;
}

uint8_t sshwsEquivalentFromMps(float mps) {
  if (mps < 32.925f) return 0;  // below Cat 1 threshold
  if (mps < 42.700f) return 1;
  if (mps < 49.400f) return 2;
  if (mps < 58.100f) return 3;
  if (mps < 70.000f) return 4;
  return 5;
}

void printFloatOrDash(float value, uint8_t decimals) {
  if (isfinite(value)) {
    Serial.print(value, decimals);
  } else {
    Serial.print("--");
  }
}

void printWindLine(const char* label, float mps) {
  Serial.print(label);

  if (!isfinite(mps)) {
    Serial.println("--");
    return;
  }

  Serial.print(mps, 2);
  Serial.print(" m/s | ");
  Serial.print(mpsToKt(mps), 2);
  Serial.print(" kt | ");
  Serial.print(mpsToKmh(mps), 1);
  Serial.println(" km/h");
}

// =====================================================
// Packet validation / sequence handling
// =====================================================
bool packetIsValid(const SensorPacket& p) {
  if (p.magic != PACKET_MAGIC) {
    Serial.println("Dropped packet: magic mismatch");
    return false;
  }

  if (p.version != PACKET_VERSION) {
    Serial.println("Dropped packet: version mismatch");
    return false;
  }

  // Wind values should always be valid numbers.
  if (!isfinite(p.windInstantMps) ||
      !isfinite(p.windMeanMps) ||
      !isfinite(p.windPeakMps)) {
    Serial.println("Dropped packet: invalid wind value");
    return false;
  }

  // Basic sanity check. This is receiver-side protection only.
  // The transmitter already performs the real filtering.
  if (p.windInstantMps < 0.0f ||
      p.windMeanMps < 0.0f ||
      p.windPeakMps < 0.0f) {
    Serial.println("Dropped packet: negative wind value");
    return false;
  }

  return true;
}

bool isDuplicatePacket(const SensorPacket& p) {
  return haveLastSeq && p.seq == lastSeq;
}

void checkSequenceGap(const SensorPacket& p) {
  if (!haveLastSeq) {
    return;
  }

  uint16_t expected = (uint16_t)(lastSeq + 1);

  if (p.seq != expected) {
    Serial.print("Sequence gap: expected ");
    Serial.print(expected);
    Serial.print(", got ");
    Serial.println(p.seq);
  }
}

// =====================================================
// Wi-Fi / ThingSpeak
// =====================================================
bool connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) {
    return true;
  }

  Serial.print("Connecting to Wi-Fi");

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);

  unsigned long startAttempt = millis();

  while (WiFi.status() != WL_CONNECTED &&
         millis() - startAttempt < 15000UL) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Wi-Fi connected, IP: ");
    Serial.println(WiFi.localIP());
    return true;
  }

  Serial.println("Wi-Fi connection failed");
  return false;
}

// =====================================================
// Printing
// =====================================================
void printPacket(const SensorPacket& p) {
  uint8_t beaufortMean = beaufortFromMps(p.windMeanMps);
  uint8_t sshwsEqMean  = sshwsEquivalentFromMps(p.windMeanMps);

  Serial.println("----------------------------------------");

  Serial.print("Seq: ");
  Serial.println(p.seq);

  Serial.print("Temperature: ");
  printFloatOrDash(p.temperatureC, 1);
  Serial.println(" C");

  Serial.print("Humidity: ");
  printFloatOrDash(p.humidityPct, 1);
  Serial.println(" %");

  Serial.print("Sea-level pressure: ");
  printFloatOrDash(p.seaLevelHpa, 1);
  Serial.println(" hPa");

  printWindLine("Wind current: ", p.windInstantMps);
  printWindLine("Wind mean15:  ", p.windMeanMps);
  printWindLine("Wind peak3:   ", p.windPeakMps);

  Serial.print("Beaufort mean15: ");
  Serial.println(beaufortMean);

  Serial.print("SSHWS-eq mean15: ");
  Serial.println(sshwsEqMean);

  Serial.println("----------------------------------------");
  Serial.println();
}

// =====================================================
// ThingSpeak upload
// =====================================================
void uploadToThingSpeak(const SensorPacket& p) {
  if (!connectWiFi()) {
    Serial.println("Skipping ThingSpeak upload: no Wi-Fi");
    return;
  }

  uint8_t beaufortMean = beaufortFromMps(p.windMeanMps);
  uint8_t sshwsEqMean  = sshwsEquivalentFromMps(p.windMeanMps);

  // Field mapping:
  // field1 = temperature C
  // field2 = humidity %
  // field3 = sea-level pressure hPa
  // field4 = current/live wind m/s
  // field5 = 15 s mean wind m/s
  // field6 = 3 s peak/gust wind m/s
  // field7 = Beaufort from mean15
  // field8 = SSHWS equivalent from mean15

  ThingSpeak.setField(1, p.temperatureC);
  ThingSpeak.setField(2, p.humidityPct);
  ThingSpeak.setField(3, p.seaLevelHpa);
  ThingSpeak.setField(4, p.windInstantMps);
  ThingSpeak.setField(5, p.windMeanMps);
  ThingSpeak.setField(6, p.windPeakMps);
  ThingSpeak.setField(7, (long)beaufortMean);
  ThingSpeak.setField(8, (long)sshwsEqMean);

  String status = "seq=" + String(p.seq);
  ThingSpeak.setStatus(status);

  int httpCode = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);

  if (httpCode == 200) {
    Serial.println("ThingSpeak update OK");
  } else {
    Serial.print("ThingSpeak update failed, HTTP code: ");
    Serial.println(httpCode);
  }

  Serial.println();
}

// =====================================================
// Packet handler
// =====================================================
void handlePacket(const SensorPacket& p) {
  if (!packetIsValid(p)) {
    return;
  }

  if (isDuplicatePacket(p)) {
    Serial.print("Duplicate packet ignored, seq=");
    Serial.println(p.seq);
    return;
  }

  checkSequenceGap(p);

  lastSeq = p.seq;
  haveLastSeq = true;

  printPacket(p);

  // TX already sends every 15 seconds, but keep this guard.
  unsigned long now = millis();

  if (now - lastThingSpeakAttempt >= THINGSPEAK_MIN_INTERVAL_MS) {
    lastThingSpeakAttempt = now;
    uploadToThingSpeak(p);
  } else {
    Serial.println("ThingSpeak interval guard active, skipping upload.");
    Serial.println();
  }
}

// =====================================================
// Setup
// =====================================================
void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("RX v1.1 - nRF24 + ThingSpeak");

  Serial.print("Expected packet size: ");
  Serial.print(sizeof(SensorPacket));
  Serial.println(" bytes");

  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, CSN_PIN);

  if (!radio.begin()) {
    Serial.println("nRF24 begin failed");
    while (true) {
      delay(100);
    }
  }

  radio.setDataRate(RF24_250KBPS);
  radio.setPayloadSize(sizeof(SensorPacket));
  radio.openReadingPipe(1, pipeAddress);
  radio.startListening();

  ThingSpeak.begin(client);
  connectWiFi();

  Serial.println("Receiver ready");
  Serial.println();
}

// =====================================================
// Loop
// =====================================================
void loop() {
  while (radio.available()) {
    radio.read(&rx, sizeof(rx));
    handlePacket(rx);
  }
}

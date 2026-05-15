#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

#include <WiFi.h>
#include "ThingSpeak.h"

#include <math.h>

// =====================================================
// USER SETTINGS
// =====================================================
const bool ENABLE_THINGSPEAK = true;

const unsigned long THINGSPEAK_REPORT_MS = 15000UL;

// Gust/peak is the highest rolling 3-second average
// inside the current 15-second report window.
const unsigned long GUST_WINDOW_MS = 3000UL;

// Serial heartbeat interval.
const unsigned long HEARTBEAT_MS = 5000UL;

// =====================================================
// nRF24
// =====================================================
// Use the receiver CE/CSN pins that worked in your radio-only test.
// These do not need to match the transmitter CE/CSN pins.
#define CE_PIN   22
#define CSN_PIN   5
#define SCK_PIN  18
#define MISO_PIN 19
#define MOSI_PIN 23

const uint8_t RADIO_CHANNEL = 76;

RF24 radio(CE_PIN, CSN_PIN);
const byte pipeAddress[6] = "RxAAA";

// =====================================================
// Wi-Fi / ThingSpeak
// =====================================================
// Put your own values here.
const char* ssid = "SSID";
const char* pass = "PASS";

unsigned long myChannelNumber = 123456;
const char* myWriteAPIKey = "API";

WiFiClient client;

// =====================================================
// COMPACT PACKET FROM TRANSMITTER
// Must match transmitter exactly.
// Fixed 14-byte payload.
// =====================================================
struct __attribute__((packed)) WeatherPacket {
  uint32_t txMs;             // transmitter millis()
  uint16_t sampleMs;         // RF sample duration
  uint16_t windMpsX10;       // wind m/s * 10
  int16_t  temperatureCx10;  // temperature C * 10
  uint16_t humidityX10;      // RH % * 10
  uint16_t seaLevelHpaX10;   // sea-level pressure hPa * 10
};

WeatherPacket rx;

// =====================================================
// GLOBAL STATE
// =====================================================
uint32_t rxPacketCount = 0;
uint32_t uploadCount = 0;

unsigned long lastHeartbeatMs = 0;

float latestTemperatureC = 20.0f;
float latestHumidityPct = 50.0f;
float latestSeaLevelHpa = 1013.25f;

// Latest received wind value.
// This should match the transmitter OLED wind value.
float latestWindMps = 0.0f;

// 15-second report accumulator.
float reportWindWeightedSum = 0.0f;  // wind m/s * milliseconds
unsigned long reportElapsedMs = 0;
float reportPeak3Mps = 0.0f;

// =====================================================
// GUST RING BUFFER
// =====================================================
const uint8_t GUST_RING_SIZE = 20;

struct WindSample {
  uint16_t sampleMs;
  uint16_t windMpsX10;
};

WindSample gustRing[GUST_RING_SIZE];
uint8_t gustWriteIndex = 0;
uint8_t gustCount = 0;

// =====================================================
// CONVERSIONS / SCALE
// =====================================================
float unpackMpsX10(uint16_t value) {
  return value / 10.0f;
}

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
  if (mps < 32.925f) return 0;
  if (mps < 42.700f) return 1;
  if (mps < 49.400f) return 2;
  if (mps < 58.100f) return 3;
  if (mps < 70.000f) return 4;
  return 5;
}

// =====================================================
// GUST RING HELPERS
// =====================================================
void clearGustRing() {
  gustWriteIndex = 0;
  gustCount = 0;

  for (uint8_t i = 0; i < GUST_RING_SIZE; i++) {
    gustRing[i].sampleMs = 0;
    gustRing[i].windMpsX10 = 0;
  }
}

void addGustSample(uint16_t sampleMs, uint16_t windMpsX10) {
  gustRing[gustWriteIndex].sampleMs = sampleMs;
  gustRing[gustWriteIndex].windMpsX10 = windMpsX10;

  gustWriteIndex++;

  if (gustWriteIndex >= GUST_RING_SIZE) {
    gustWriteIndex = 0;
  }

  if (gustCount < GUST_RING_SIZE) {
    gustCount++;
  }
}

bool meanMpsFromRecentGustSamples(
  unsigned long targetWindowMs,
  float &meanMpsOut
) {
  unsigned long totalMs = 0;
  float weightedSum = 0.0f;

  for (uint8_t n = 0; n < gustCount && totalMs < targetWindowMs; n++) {
    int index = (int)gustWriteIndex - 1 - n;

    while (index < 0) {
      index += GUST_RING_SIZE;
    }

    uint16_t sampleMs = gustRing[index].sampleMs;

    if (sampleMs == 0) {
      continue;
    }

    unsigned long remainingMs = targetWindowMs - totalMs;
    unsigned long usedMs = sampleMs;

    if (usedMs > remainingMs) {
      usedMs = remainingMs;
    }

    float sampleMps = unpackMpsX10(gustRing[index].windMpsX10);

    weightedSum += sampleMps * (float)usedMs;
    totalMs += usedMs;
  }

  if (totalMs < targetWindowMs || totalMs == 0) {
    meanMpsOut = 0.0f;
    return false;
  }

  meanMpsOut = weightedSum / (float)totalMs;
  return true;
}

// =====================================================
// REPORT HELPERS
// =====================================================
float reportMeanMps() {
  if (reportElapsedMs == 0) {
    return 0.0f;
  }

  return reportWindWeightedSum / (float)reportElapsedMs;
}

void resetReport() {
  reportWindWeightedSum = 0.0f;
  reportElapsedMs = 0;
  reportPeak3Mps = 0.0f;
  clearGustRing();
}

// =====================================================
// WIFI / THINGSPEAK
// =====================================================
void connectWiFi() {
  if (!ENABLE_THINGSPEAK) {
    return;
  }

  if (WiFi.status() == WL_CONNECTED) {
    return;
  }

  Serial.print("Connecting to Wi-Fi");

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);

  unsigned long startAttempt = millis();

  while (WiFi.status() != WL_CONNECTED &&
         millis() - startAttempt < 10000UL) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Wi-Fi connected, IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("Wi-Fi connection failed");
  }
}

void uploadReport(float mean15Mps, float peak3Mps) {
  if (!ENABLE_THINGSPEAK) {
    Serial.println("ThingSpeak disabled");
    return;
  }

  connectWiFi();

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("ThingSpeak skipped: no Wi-Fi");
    return;
  }

  uint8_t beaufortMean = beaufortFromMps(mean15Mps);
  uint8_t sshwsEqMean = sshwsEquivalentFromMps(mean15Mps);

  // Field mapping:
  // field1 = temperature C
  // field2 = humidity %
  // field3 = sea-level pressure hPa
  // field4 = latest/current wind m/s
  // field5 = 15 s mean wind m/s
  // field6 = 3 s peak/gust wind m/s
  // field7 = Beaufort from mean15
  // field8 = SSHWS equivalent from mean15

  ThingSpeak.setField(1, latestTemperatureC);
  ThingSpeak.setField(2, latestHumidityPct);
  ThingSpeak.setField(3, latestSeaLevelHpa);
  ThingSpeak.setField(4, latestWindMps);
  ThingSpeak.setField(5, mean15Mps);
  ThingSpeak.setField(6, peak3Mps);
  ThingSpeak.setField(7, (long)beaufortMean);
  ThingSpeak.setField(8, (long)sshwsEqMean);

  String status = "rx=" + String(rxPacketCount) +
                  " elapsed=" + String(reportElapsedMs);
  ThingSpeak.setStatus(status);

  int httpCode = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);

  if (httpCode == 200) {
    uploadCount++;
    Serial.println("ThingSpeak update OK");
  } else {
    Serial.print("ThingSpeak update failed, HTTP code: ");
    Serial.println(httpCode);
  }
}

// =====================================================
// SERIAL PRINTING
// =====================================================
void printLiveLine(uint16_t sampleMs, uint32_t txMs) {
  float meanNow = reportMeanMps();

  Serial.print("RX ");
  Serial.print(rxPacketCount);

  Serial.print(" txMs=");
  Serial.print(txMs);

  Serial.print(" sampleMs=");
  Serial.print(sampleMs);

  Serial.print(" wind=");
  Serial.print(latestWindMps, 1);
  Serial.print(" m/s | ");
  Serial.print(mpsToKt(latestWindMps), 1);
  Serial.print(" kt");

  Serial.print(" mean15=");
  Serial.print(meanNow, 1);
  Serial.print(" m/s | ");
  Serial.print(mpsToKt(meanNow), 1);
  Serial.print(" kt");

  Serial.print(" peak3=");
  Serial.print(reportPeak3Mps, 1);
  Serial.print(" m/s | ");
  Serial.print(mpsToKt(reportPeak3Mps), 1);
  Serial.print(" kt");

  Serial.print(" T=");
  Serial.print(latestTemperatureC, 1);

  Serial.print(" H=");
  Serial.print(latestHumidityPct, 1);

  Serial.print(" P=");
  Serial.println(latestSeaLevelHpa, 1);
}

void printReport(float mean15Mps, float peak3Mps) {
  Serial.println("----------------------------------------");

  Serial.print("REPORT mean15=");
  Serial.print(mean15Mps, 1);
  Serial.print(" m/s | ");
  Serial.print(mpsToKt(mean15Mps), 1);
  Serial.print(" kt | ");
  Serial.print(mpsToKmh(mean15Mps), 1);
  Serial.println(" km/h");

  Serial.print("REPORT peak3=");
  Serial.print(peak3Mps, 1);
  Serial.print(" m/s | ");
  Serial.print(mpsToKt(peak3Mps), 1);
  Serial.print(" kt | ");
  Serial.print(mpsToKmh(peak3Mps), 1);
  Serial.println(" km/h");

  Serial.print("Beaufort mean15=");
  Serial.println(beaufortFromMps(mean15Mps));

  Serial.print("SSHWS-eq mean15=");
  Serial.println(sshwsEquivalentFromMps(mean15Mps));

  Serial.println("----------------------------------------");
}

// =====================================================
// PACKET PROCESSING
// =====================================================
void processPacket(const WeatherPacket &p) {
  rxPacketCount++;

  uint16_t sampleMs = p.sampleMs;

  if (sampleMs == 0) {
    sampleMs = 1;
  }

  latestWindMps = unpackMpsX10(p.windMpsX10);
  latestTemperatureC = p.temperatureCx10 / 10.0f;
  latestHumidityPct = p.humidityX10 / 10.0f;
  latestSeaLevelHpa = p.seaLevelHpaX10 / 10.0f;

  // Build 15-second mean from incoming wind samples.
  reportWindWeightedSum += latestWindMps * (float)sampleMs;
  reportElapsedMs += sampleMs;

  // Build 3-second rolling peak/gust from incoming wind samples.
  addGustSample(sampleMs, p.windMpsX10);

  float gust3Mps = 0.0f;

  if (meanMpsFromRecentGustSamples(GUST_WINDOW_MS, gust3Mps)) {
    if (gust3Mps > reportPeak3Mps) {
      reportPeak3Mps = gust3Mps;
    }
  }

  printLiveLine(sampleMs, p.txMs);

  if (reportElapsedMs >= THINGSPEAK_REPORT_MS) {
    float mean15Mps = reportMeanMps();
    float peak3Mps = reportPeak3Mps;

    printReport(mean15Mps, peak3Mps);
    uploadReport(mean15Mps, peak3Mps);
    resetReport();
  }
}

// =====================================================
// HEARTBEAT
// =====================================================
void serviceHeartbeat() {
  unsigned long now = millis();

  if (now - lastHeartbeatMs < HEARTBEAT_MS) {
    return;
  }

  lastHeartbeatMs = now;

  Serial.print("RX alive packets=");
  Serial.print(rxPacketCount);

  Serial.print(" uploads=");
  Serial.print(uploadCount);

  Serial.print(" reportMs=");
  Serial.print(reportElapsedMs);

  Serial.print(" radio=");
  Serial.print(radio.isChipConnected() ? "OK" : "NOT_CONNECTED");

  Serial.print(" WiFi=");
  Serial.println(WiFi.status() == WL_CONNECTED ? "OK" : "OFF");
}

// =====================================================
// SETUP
// =====================================================
void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("RX compact fixed-payload wind x10 + ACK radio + ThingSpeak");

  resetReport();

  // nRF24
  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, CSN_PIN);

  if (!radio.begin()) {
    Serial.println("nRF24 begin failed");
    while (true) {
      delay(100);
    }
  }

  radio.setChannel(RADIO_CHANNEL);
  radio.setDataRate(RF24_250KBPS);
  radio.setPALevel(RF24_PA_LOW);

  // Fixed payload + ACK enabled.
  // Must match transmitter.
  radio.setAutoAck(true);
  radio.setPayloadSize(sizeof(WeatherPacket));

  radio.openReadingPipe(1, pipeAddress);
  radio.startListening();
  radio.flush_rx();

  // ThingSpeak
  ThingSpeak.begin(client);

  if (ENABLE_THINGSPEAK) {
    connectWiFi();
  } else {
    Serial.println("ThingSpeak disabled");
  }

  Serial.println();
  Serial.println("Radio:");
  Serial.print("Radio chip: ");
  Serial.println(radio.isChipConnected() ? "OK" : "NOT_CONNECTED");

  Serial.print("WeatherPacket bytes: ");
  Serial.println(sizeof(WeatherPacket));

  Serial.print("Channel: ");
  Serial.println(RADIO_CHANNEL);

  Serial.println();
  Serial.println("Receiver ready");
}

// =====================================================
// LOOP
// =====================================================
void loop() {
  while (radio.available()) {
    radio.read(&rx, sizeof(rx));
    processPacket(rx);
  }

  serviceHeartbeat();
}

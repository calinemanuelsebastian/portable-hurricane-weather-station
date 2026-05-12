#include <Wire.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

#include <math.h>

// =====================================================
// USER SETTINGS
// =====================================================
const unsigned long WIND_SAMPLE_MS   = 100;     // wind bin interval
const unsigned long OLED_UPDATE_MS   = 500;     // stable display refresh
const unsigned long SENSOR_UPDATE_MS = 1000;    // AHT20 + BMP280 refresh
const unsigned long SEND_INTERVAL_MS = 15000;   // transmit after 15 s of sampled wind time

const float cupRadiusMeters = 0.146f;           // shaft center -> cup center
const int pulsesPerRevolution = 3;              // 3 magnets
const float calibrationFactor = 4.2833f;        // geometry-derived first estimate
const float calibrationOffsetMps = 0.0f;        // keep 0 unless later calibrated
const float altitudeM = 0.0f;                   // set real altitude if needed

// =====================================================
// WIND SETTINGS
// =====================================================
const float TWO_PI_F = 6.28318530718f;

const float WIND_MPS_PER_PULSE_HZ =
  calibrationFactor * TWO_PI_F * cupRadiusMeters / (float)pulsesPerRevolution;

const float WIND_KT_PER_PULSE_HZ =
  WIND_MPS_PER_PULSE_HZ * 1.94384f;

// Live display wind is now intentionally stable.
// With 3 pulses/rev, short windows are naturally jumpy.
const unsigned long WIND_CURRENT_WINDOW_MS = 3000UL;
const unsigned long WIND_CURRENT_MIN_MS    = 2000UL;

// Gust/peak is highest rolling 3-second wind inside the 15 s TX interval.
const unsigned long WIND_GUST_WINDOW_MS = 3000UL;

// 60 bins at 100 ms = about 6 seconds of history.
const uint8_t WIND_RING_SIZE = 60;

// Display smoothing.
// Lower = smoother. Higher = more responsive.
const float WIND_DISPLAY_ALPHA_RISE = 0.35f;
const float WIND_DISPLAY_ALPHA_FALL = 0.22f;

// Electrical chatter rejection.
const unsigned long ELECTRICAL_DEBOUNCE_US = 5000UL;

// Input validation gate.
// This is not a display cap; it rejects impossible Hall edge spacing.
// Use 60-80 kt depending on your station.
// Lower = more stable / stricter.
const float MAX_VALID_WIND_KT = 70.0f;
const float MAX_VALID_WIND_MPS = MAX_VALID_WIND_KT / 1.94384f;

const unsigned long MIN_PHYSICAL_EDGE_INTERVAL_US =
  (unsigned long)((1000000.0f * WIND_MPS_PER_PULSE_HZ) / MAX_VALID_WIND_MPS);

const unsigned long EDGE_GATE_US =
  (MIN_PHYSICAL_EDGE_INTERVAL_US > ELECTRICAL_DEBOUNCE_US)
    ? MIN_PHYSICAL_EDGE_INTERVAL_US
    : ELECTRICAL_DEBOUNCE_US;

// =====================================================
// AHT20
// =====================================================
#define AHT20_ADDRESS 0x38

// =====================================================
// BMP280
// =====================================================
#define BMP280_ADDRESS 0x77   // change to 0x76 if needed
Adafruit_BMP280 bmp;

// =====================================================
// OLED
// =====================================================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define OLED_ADDRESS 0x3C     // change to 0x3D if needed

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// =====================================================
// nRF24
// =====================================================
#define CE_PIN    22
#define CSN_PIN    5
#define SCK_PIN   18
#define MISO_PIN  19
#define MOSI_PIN  23

RF24 radio(CE_PIN, CSN_PIN);
const byte pipeAddress[6] = "RxAAA";

// =====================================================
// KY-003 Hall
// =====================================================
const int hallPin = 4;

volatile uint32_t windAcceptedPulses = 0;
volatile uint32_t windRejectedEdges = 0;
volatile unsigned long lastAcceptedEdgeUs = 0;
volatile unsigned long shortestAcceptedPeriodUs = 0xFFFFFFFFUL;

// =====================================================
// SHARED PACKET DEFINITION
// Must match receiver exactly.
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

// =====================================================
// GLOBAL STATE
// =====================================================
unsigned long lastWindSampleMs = 0;
unsigned long lastOledUpdateMs = 0;
unsigned long lastSensorUpdateMs = 0;

uint16_t seq = 0;

float humidityPercent = NAN;
float temperatureCelsius = NAN;
float seaLevelHpa = NAN;

float windDisplayMps = 0.0f;

// 15-second TX accumulator.
// Updated only by the 100 ms wind sampler.
uint32_t txAccumPulses = 0;
uint32_t txAccumRejected = 0;
unsigned long txAccumElapsedMs = 0;
float txPeakMps = 0.0f;

uint32_t lastSampleAcceptedPulses = 0;
uint32_t lastSampleRejectedEdges = 0;

// =====================================================
// WIND RING BUFFER
// =====================================================
struct WindBin {
  uint16_t elapsedMs;
  uint16_t pulses;
  uint16_t rejected;
};

WindBin windRing[WIND_RING_SIZE];
uint8_t windWriteIndex = 0;
uint8_t windValidCount = 0;

// =====================================================
// SCALE HELPERS
// =====================================================
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
// WIND HELPERS
// =====================================================
void snapshotWindCounters(uint32_t &acceptedOut, uint32_t &rejectedOut) {
  noInterrupts();
  acceptedOut = windAcceptedPulses;
  rejectedOut = windRejectedEdges;
  interrupts();
}

unsigned long getShortestAcceptedPeriodUs() {
  noInterrupts();
  unsigned long value = shortestAcceptedPeriodUs;
  interrupts();
  return value;
}

float windMpsFromPulseHz(float pulseHz) {
  float mps = calibrationOffsetMps + pulseHz * WIND_MPS_PER_PULSE_HZ;
  return (mps < 0.0f) ? 0.0f : mps;
}

float windKtFromMps(float mps) {
  return mps * 1.94384f;
}

void clearWindRing() {
  for (uint8_t i = 0; i < WIND_RING_SIZE; i++) {
    windRing[i].elapsedMs = 0;
    windRing[i].pulses = 0;
    windRing[i].rejected = 0;
  }

  windWriteIndex = 0;
  windValidCount = 0;
}

void addWindBin(uint16_t elapsedMs, uint16_t pulses, uint16_t rejected) {
  windRing[windWriteIndex].elapsedMs = elapsedMs;
  windRing[windWriteIndex].pulses = pulses;
  windRing[windWriteIndex].rejected = rejected;

  windWriteIndex++;

  if (windWriteIndex >= WIND_RING_SIZE) {
    windWriteIndex = 0;
  }

  if (windValidCount < WIND_RING_SIZE) {
    windValidCount++;
  }
}

bool pulseHzFromRecentBins(
  unsigned long targetWindowMs,
  unsigned long minimumWindowMs,
  float &pulseHzOut
) {
  unsigned long totalMs = 0;
  uint32_t totalPulses = 0;

  for (uint8_t n = 0; n < windValidCount && totalMs < targetWindowMs; n++) {
    int index = (int)windWriteIndex - 1 - n;

    while (index < 0) {
      index += WIND_RING_SIZE;
    }

    totalMs += windRing[index].elapsedMs;
    totalPulses += windRing[index].pulses;
  }

  if (totalMs < minimumWindowMs || totalMs == 0) {
    pulseHzOut = 0.0f;
    return false;
  }

  pulseHzOut = ((float)totalPulses * 1000.0f) / (float)totalMs;
  return true;
}

// =====================================================
// ISR
// =====================================================
void IRAM_ATTR onMagnetDetected() {
  unsigned long nowUs = micros();
  unsigned long prevUs = lastAcceptedEdgeUs;

  if (prevUs == 0) {
    lastAcceptedEdgeUs = nowUs;
    windAcceptedPulses++;
    return;
  }

  unsigned long dtUs = nowUs - prevUs;

  if (dtUs < EDGE_GATE_US) {
    windRejectedEdges++;
    return;
  }

  lastAcceptedEdgeUs = nowUs;
  windAcceptedPulses++;

  if (dtUs < shortestAcceptedPeriodUs) {
    shortestAcceptedPeriodUs = dtUs;
  }
}

// =====================================================
// SEA LEVEL REDUCTION
// =====================================================
float reduceToSeaLevelWMO(
  float stationPressureHpa,
  float tempC,
  float rhPercent,
  float altitudeMeters
) {
  const float Kp = 0.0148275f;
  const float a  = 0.0065f;
  const float Ch = 0.12f;

  float Ts = 273.15f + tempC;
  float esat = 6.112f * exp((17.67f * tempC) / (tempC + 243.5f));
  float es = (rhPercent / 100.0f) * esat;
  float Tmv = Ts + (a * altitudeMeters / 2.0f) + (es * Ch);
  float exponent = (Kp * altitudeMeters) / Tmv;

  return stationPressureHpa * pow(10.0f, exponent);
}

// =====================================================
// AHT20
// =====================================================
void initAHT20() {
  Wire.beginTransmission(AHT20_ADDRESS);
  Wire.write(0xBE);
  Wire.write(0x08);
  Wire.write(0x00);
  Wire.endTransmission();
  delay(10);
}

bool readAHT20(float &humidityOut, float &temperatureOut) {
  Wire.beginTransmission(AHT20_ADDRESS);
  Wire.write(0xAC);
  Wire.write(0x33);
  Wire.write(0x00);

  if (Wire.endTransmission() != 0) {
    return false;
  }

  delay(80);

  Wire.requestFrom(AHT20_ADDRESS, 6);

  if (Wire.available() != 6) {
    return false;
  }

  uint8_t data[6];

  for (int i = 0; i < 6; i++) {
    data[i] = Wire.read();
  }

  uint32_t humidity =
    ((uint32_t)data[1] << 12) |
    ((uint32_t)data[2] << 4)  |
    (data[3] >> 4);

  uint32_t temperature =
    ((uint32_t)(data[3] & 0x0F) << 16) |
    ((uint32_t)data[4] << 8) |
    data[5];

  humidityOut = humidity * 100.0f / 1048576.0f;
  temperatureOut = (temperature * 200.0f / 1048576.0f) - 50.0f;

  if (humidityOut < 0.0f) {
    humidityOut = 0.0f;
  }

  if (humidityOut > 100.0f) {
    humidityOut = 100.0f;
  }

  return true;
}

// =====================================================
// ENVIRONMENTAL SENSOR SERVICE
// =====================================================
void updateEnvironmentalSensorsNow() {
  float hTmp;
  float tTmp;

  if (readAHT20(hTmp, tTmp)) {
    humidityPercent = hTmp;
    temperatureCelsius = tTmp;
  }

  float pressureHpa = bmp.readPressure() / 100.0f;

  seaLevelHpa = reduceToSeaLevelWMO(
    pressureHpa,
    isnan(temperatureCelsius) ? 20.0f : temperatureCelsius,
    isnan(humidityPercent) ? 50.0f : humidityPercent,
    altitudeM
  );
}

void serviceEnvironmentalSensors() {
  unsigned long now = millis();

  if (now - lastSensorUpdateMs < SENSOR_UPDATE_MS) {
    return;
  }

  updateEnvironmentalSensorsNow();
  lastSensorUpdateMs = millis();
}

// =====================================================
// WIND SAMPLER SERVICE
// =====================================================
void serviceWindSampler() {
  unsigned long now = millis();

  if (now - lastWindSampleMs < WIND_SAMPLE_MS) {
    return;
  }

  uint32_t acceptedNow;
  uint32_t rejectedNow;
  snapshotWindCounters(acceptedNow, rejectedNow);

  uint32_t deltaPulses32 = acceptedNow - lastSampleAcceptedPulses;
  uint32_t deltaRejected32 = rejectedNow - lastSampleRejectedEdges;
  unsigned long elapsedLong = now - lastWindSampleMs;

  lastSampleAcceptedPulses = acceptedNow;
  lastSampleRejectedEdges = rejectedNow;
  lastWindSampleMs = now;

  uint16_t elapsedMs =
    (elapsedLong > 65535UL) ? 65535 : (uint16_t)elapsedLong;

  uint16_t deltaPulses =
    (deltaPulses32 > 65535UL) ? 65535 : (uint16_t)deltaPulses32;

  uint16_t deltaRejected =
    (deltaRejected32 > 65535UL) ? 65535 : (uint16_t)deltaRejected32;

  addWindBin(elapsedMs, deltaPulses, deltaRejected);

  // 15-second report accumulation.
  txAccumPulses += deltaPulses32;
  txAccumRejected += deltaRejected32;
  txAccumElapsedMs += elapsedLong;

  // Stable live wind from rolling 4-second window.
  float currentPulseHz = 0.0f;
  float targetMps = 0.0f;

  if (pulseHzFromRecentBins(
        WIND_CURRENT_WINDOW_MS,
        WIND_CURRENT_MIN_MS,
        currentPulseHz
      )) {
    targetMps = windMpsFromPulseHz(currentPulseHz);
  }

  float alpha = (targetMps > windDisplayMps)
    ? WIND_DISPLAY_ALPHA_RISE
    : WIND_DISPLAY_ALPHA_FALL;

  windDisplayMps += alpha * (targetMps - windDisplayMps);

  if (windDisplayMps < 0.05f) {
    windDisplayMps = 0.0f;
  }

  // 3-second gust/peak inside current TX interval.
  if (txAccumElapsedMs >= WIND_GUST_WINDOW_MS) {
    float gustPulseHz = 0.0f;

    if (pulseHzFromRecentBins(
          WIND_GUST_WINDOW_MS,
          WIND_GUST_WINDOW_MS,
          gustPulseHz
        )) {
      float gustMps = windMpsFromPulseHz(gustPulseHz);

      if (gustMps > txPeakMps) {
        txPeakMps = gustMps;
      }
    }
  }
}

float txMeanMps() {
  if (txAccumElapsedMs == 0) {
    return 0.0f;
  }

  float pulseHz =
    ((float)txAccumPulses * 1000.0f) / (float)txAccumElapsedMs;

  return windMpsFromPulseHz(pulseHz);
}

void resetTxAccumulator() {
  txAccumPulses = 0;
  txAccumRejected = 0;
  txAccumElapsedMs = 0;
  txPeakMps = 0.0f;
}

// =====================================================
// OLED
// =====================================================
void updateDisplay(
  float humidityVal,
  float temperatureVal,
  float pressureVal,
  float windValMps,
  float meanPreviewMps,
  uint16_t seqVal
) {
  float windKt = windKtFromMps(windValMps);
  float meanKt = windKtFromMps(meanPreviewMps);

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);

  display.print("Pkg: ");
  display.println(seqVal);

  display.print("W: ");
  display.print(windKt, 1);
  display.println(" kt");

  display.print("T: ");

  if (isnan(temperatureVal)) {
    display.println("--.- C");
  } else {
    display.print(temperatureVal, 1);
    display.println(" C");
  }

  display.print("H: ");

  if (isnan(humidityVal)) {
    display.println("--.- %");
  } else {
    display.print(humidityVal, 1);
    display.println(" %");
  }

  display.print("P: ");

  if (isnan(pressureVal)) {
    display.println("----.- hPa");
  } else {
    display.print(pressureVal, 1);
    display.println(" hPa");
  }

  if (meanKt < 64.0f) {
    display.print("Scale: Bft ");
    display.println(beaufortFromMps(meanPreviewMps));
  } else {
    display.print("Scale: S-S-eq ");
    display.println(sshwsEquivalentFromMps(meanPreviewMps));
  }

  display.display();
}

void serviceDisplay() {
  unsigned long now = millis();

  if (now - lastOledUpdateMs < OLED_UPDATE_MS) {
    return;
  }

  updateDisplay(
    humidityPercent,
    temperatureCelsius,
    seaLevelHpa,
    windDisplayMps,
    txMeanMps(),
    seq
  );

  lastOledUpdateMs = millis();
}

// =====================================================
// TRANSMIT SERVICE
// =====================================================
void serviceTransmit() {
  if (txAccumElapsedMs < SEND_INTERVAL_MS) {
    return;
  }

  float reportMeanMps = txMeanMps();
  float reportPeakMps = txPeakMps;
  float reportCurrentMps = windDisplayMps;

  uint32_t reportPulses = txAccumPulses;
  uint32_t reportRejected = txAccumRejected;
  unsigned long reportElapsedMs = txAccumElapsedMs;

  SensorPacket tx{};

  tx.magic = PACKET_MAGIC;
  tx.version = PACKET_VERSION;
  tx.reserved = 0;
  tx.seq = seq++;

  tx.temperatureC = temperatureCelsius;
  tx.humidityPct = humidityPercent;
  tx.seaLevelHpa = seaLevelHpa;

  tx.windInstantMps = reportCurrentMps;
  tx.windMeanMps = reportMeanMps;
  tx.windPeakMps = reportPeakMps;

  resetTxAccumulator();

  bool ok = radio.write(&tx, sizeof(tx));

  Serial.print("TX seq=");
  Serial.print(tx.seq);

  Serial.print(" current=");
  Serial.print(tx.windInstantMps, 2);
  Serial.print(" m/s ");
  Serial.print(windKtFromMps(tx.windInstantMps), 1);
  Serial.print(" kt");

  Serial.print(" mean15=");
  Serial.print(tx.windMeanMps, 2);
  Serial.print(" m/s ");
  Serial.print(windKtFromMps(tx.windMeanMps), 1);
  Serial.print(" kt");

  Serial.print(" peak3=");
  Serial.print(tx.windPeakMps, 2);
  Serial.print(" m/s ");
  Serial.print(windKtFromMps(tx.windPeakMps), 1);
  Serial.print(" kt");

  Serial.print(" elapsedMs=");
  Serial.print(reportElapsedMs);

  Serial.print(" pulses=");
  Serial.print(reportPulses);

  Serial.print(" rejected=");
  Serial.print(reportRejected);

  Serial.print(" gateUs=");
  Serial.print(EDGE_GATE_US);

  unsigned long shortestUs = getShortestAcceptedPeriodUs();

  if (shortestUs != 0xFFFFFFFFUL) {
    Serial.print(" shortestAcceptedUs=");
    Serial.print(shortestUs);
  }

  Serial.print(" ");
  Serial.println(ok ? "OK" : "FAIL");
}

// =====================================================
// SETUP
// =====================================================
void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("TX v3.3 - stable efficient wind sampler");

  Wire.begin(16, 17);
  Wire.setClock(400000);

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    Serial.println("SSD1306 init failed");
    while (true) {
      delay(100);
    }
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("TX booting...");
  display.display();

  if (!bmp.begin(BMP280_ADDRESS)) {
    Serial.println("BMP280 not found");

    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("BMP280 not found");
    display.display();

    while (true) {
      delay(100);
    }
  }

  bmp.setSampling(
    Adafruit_BMP280::MODE_NORMAL,
    Adafruit_BMP280::SAMPLING_X2,
    Adafruit_BMP280::SAMPLING_X16,
    Adafruit_BMP280::FILTER_X16,
    Adafruit_BMP280::STANDBY_MS_500
  );

  initAHT20();

  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, CSN_PIN);

  if (!radio.begin()) {
    Serial.println("nRF24 begin failed");

    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("nRF24 failed");
    display.display();

    while (true) {
      delay(100);
    }
  }

  radio.setDataRate(RF24_250KBPS);
  radio.setPayloadSize(sizeof(SensorPacket));
  radio.setRetries(5, 15);
  radio.openWritingPipe(pipeAddress);
  radio.stopListening();

  clearWindRing();

  pinMode(hallPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(hallPin), onMagnetDetected, FALLING);

  unsigned long startMs = millis();

  uint32_t acceptedStart;
  uint32_t rejectedStart;
  snapshotWindCounters(acceptedStart, rejectedStart);

  lastSampleAcceptedPulses = acceptedStart;
  lastSampleRejectedEdges = rejectedStart;

  lastWindSampleMs = startMs;
  lastOledUpdateMs = startMs;
  lastSensorUpdateMs = startMs;

  resetTxAccumulator();

  updateEnvironmentalSensorsNow();
  lastSensorUpdateMs = millis();

  Serial.print("Packet size: ");
  Serial.print(sizeof(SensorPacket));
  Serial.println(" bytes");

  Serial.print("Wind coefficient: ");
  Serial.print(WIND_MPS_PER_PULSE_HZ, 6);
  Serial.println(" m/s per pulse/s");

  Serial.print("Wind coefficient: ");
  Serial.print(WIND_KT_PER_PULSE_HZ, 6);
  Serial.println(" kt per pulse/s");

  Serial.print("Wind sample interval: ");
  Serial.print(WIND_SAMPLE_MS);
  Serial.println(" ms");

  Serial.print("Live wind window: ");
  Serial.print(WIND_CURRENT_WINDOW_MS);
  Serial.println(" ms");

  Serial.print("Gust wind window: ");
  Serial.print(WIND_GUST_WINDOW_MS);
  Serial.println(" ms");

  Serial.print("OLED update interval: ");
  Serial.print(OLED_UPDATE_MS);
  Serial.println(" ms");

  Serial.print("TX interval: ");
  Serial.print(SEND_INTERVAL_MS);
  Serial.println(" ms");

  Serial.print("Max valid wind gate: ");
  Serial.print(MAX_VALID_WIND_KT, 1);
  Serial.println(" kt");

  Serial.print("Final edge gate: ");
  Serial.print(EDGE_GATE_US);
  Serial.println(" us");

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("TX ready");
  display.print("Gate: ");
  display.print(EDGE_GATE_US);
  display.println(" us");
  display.display();
}

// =====================================================
// LOOP
// =====================================================
void loop() {
  serviceWindSampler();
  serviceTransmit();
  serviceEnvironmentalSensors();
  serviceDisplay();
}

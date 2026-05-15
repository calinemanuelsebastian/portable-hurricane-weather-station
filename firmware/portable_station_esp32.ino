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
const unsigned long WIND_BIN_MS       = 100UL;    // pulse-count bin interval
const unsigned long TX_INTERVAL_MS    = 1000UL;   // RF packet interval
const unsigned long OLED_UPDATE_MS    = 500UL;    // OLED refresh
const unsigned long SENSOR_UPDATE_MS  = 1000UL;   // AHT20 + BMP280 refresh

// =====================================================
// ANEMOMETER GEOMETRY
// =====================================================
// Magnet radius is informational only.
// It is NOT used in wind-speed conversion.
const float magnetRadiusMeters = 0.020f;          // 20 mm

// Important wind-conversion radius:
// shaft center -> cup center.
const float cupRadiusMeters = 0.146f;             // 146 mm

const float cupAssemblyDiameterMeters = cupRadiusMeters * 2.0f;

// Three magnets at 120 degrees.
// Must equal accepted Hall pulses per real rotor revolution.
const int pulsesPerRevolution = 3;

// Starting calibration estimate.
// This is not pure geometry. Adjust after comparing to a reference anemometer.
const float calibrationFactor = 2.7f;
const float calibrationOffsetMps = 0.0f;

// Set this to real station altitude above mean sea level.
const float altitudeM = 0.0f;

// =====================================================
// WIND MEASUREMENT SETTINGS
// =====================================================
const float TWO_PI_F = 6.28318530718f;

const float CUP_CIRCUMFERENCE_M =
  TWO_PI_F * cupRadiusMeters;

const float WIND_MPS_PER_RPS =
  calibrationFactor * CUP_CIRCUMFERENCE_M;

const float WIND_MPS_PER_RPM =
  WIND_MPS_PER_RPS / 60.0f;

const float WIND_MPS_PER_PULSE_HZ =
  WIND_MPS_PER_RPS / (float)pulsesPerRevolution;

// Rolling RPM/RPS window.
// Shorter = faster but noisier.
// Longer = smoother but slower.
const unsigned long WIND_WINDOW_MS = 3000UL;
const unsigned long WIND_MIN_WINDOW_MS = 1500UL;

// 50 bins x 100 ms = around 5 seconds of history.
const uint8_t WIND_RING_SIZE = 50;

// Small electrical debounce only.
// If pulse count is too high, try 2000 or 3000.
// If pulse count is too low at high RPM, try 0.
const unsigned long HALL_DEBOUNCE_US = 1000UL;

// Local 15 s OLED mean preview.
const unsigned long WIND_MEAN_WINDOW_MS = 15000UL;
const unsigned long WIND_MEAN_MIN_MS    = 3000UL;
const uint8_t WIND_MEAN_RING_SIZE = 20;

// =====================================================
// AHT20 / BMP280
// =====================================================
#define AHT20_ADDRESS 0x38
#define BMP280_ADDRESS 0x77   // change to 0x76 if needed

Adafruit_BMP280 bmp;
bool bmpOk = false;

// =====================================================
// OLED
// =====================================================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define OLED_ADDRESS 0x3C     // change to 0x3D if needed

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
bool oledOk = false;

// =====================================================
// nRF24
// =====================================================
// Use the CE/CSN pins that worked in your radio-only test.
#define CE_PIN    22
#define CSN_PIN    5
#define SCK_PIN   18
#define MISO_PIN  19
#define MOSI_PIN  23

const uint8_t RADIO_CHANNEL = 76;

RF24 radio(CE_PIN, CSN_PIN);
const byte pipeAddress[6] = "RxAAA";

// =====================================================
// KY-003 Hall sensor
// =====================================================
const int hallPin = 4;

volatile uint32_t hallPulseTotal = 0;
volatile uint32_t hallRejectedBounce = 0;
volatile unsigned long lastHallPulseUs = 0;

// =====================================================
// COMPACT RF PACKET
// Fixed 14-byte payload.
// Must match receiver exactly.
// =====================================================
struct __attribute__((packed)) WeatherPacket {
  uint32_t txMs;             // transmitter millis()
  uint16_t sampleMs;         // RF sample duration
  uint16_t windMpsX10;       // wind m/s * 10
  int16_t  temperatureCx10;  // temperature C * 10
  uint16_t humidityX10;      // RH % * 10
  uint16_t seaLevelHpaX10;   // sea-level pressure hPa * 10
};

// =====================================================
// GLOBAL STATE
// =====================================================
unsigned long lastWindBinMs = 0;
unsigned long lastTxMs = 0;
unsigned long lastOledUpdateMs = 0;
unsigned long lastSensorUpdateMs = 0;
unsigned long lastTxWindSampleMs = 0;

uint32_t lastWindBinPulseTotal = 0;

float temperatureC = 20.0f;
float humidityPct = 50.0f;
float stationPressureHpa = 1013.25f;
float seaLevelHpa = 1013.25f;

// Single wind value used by OLED and RF.
float latestWindMps = 0.0f;
float latestMean15Mps = 0.0f;

// Debug values
float latestRotorRps = 0.0f;
float latestRotorRpm = 0.0f;
uint32_t latestWindowPulses = 0;
unsigned long latestWindowMs = 0;

// =====================================================
// WIND RING BUFFER
// =====================================================
struct WindBin {
  uint16_t elapsedMs;
  uint16_t pulses;
};

WindBin windRing[WIND_RING_SIZE];
uint8_t windWriteIndex = 0;
uint8_t windValidCount = 0;

// =====================================================
// LOCAL 15 s MEAN HISTORY
// =====================================================
struct WindSample {
  uint16_t sampleMs;
  uint16_t windMpsX10;
};

WindSample windMeanRing[WIND_MEAN_RING_SIZE];
uint8_t windMeanWriteIndex = 0;
uint8_t windMeanCount = 0;

// =====================================================
// CONVERSIONS / SCALES
// =====================================================
float mpsToKt(float mps) {
  return mps * 1.94384f;
}

float mpsToKmh(float mps) {
  return mps * 3.6f;
}

float windMpsFromRotorRps(float rotorRps) {
  float windMps = WIND_MPS_PER_RPS * rotorRps + calibrationOffsetMps;

  if (windMps < 0.0f) {
    windMps = 0.0f;
  }

  return windMps;
}

float windMpsFromRotorRpm(float rotorRpm) {
  return windMpsFromRotorRps(rotorRpm / 60.0f);
}

uint16_t packMpsX10(float value) {
  if (value < 0.0f) {
    value = 0.0f;
  }

  float packed = value * 10.0f;

  if (packed > 65535.0f) {
    packed = 65535.0f;
  }

  return (uint16_t)lroundf(packed);
}

float unpackMpsX10(uint16_t value) {
  return value / 10.0f;
}

int16_t packTempCx10(float value) {
  return (int16_t)lroundf(value * 10.0f);
}

uint16_t packUnsignedX10(float value) {
  if (value < 0.0f) {
    value = 0.0f;
  }

  float packed = value * 10.0f;

  if (packed > 65535.0f) {
    packed = 65535.0f;
  }

  return (uint16_t)lroundf(packed);
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
// HALL ISR
// =====================================================
void IRAM_ATTR onHallPulse() {
  unsigned long nowUs = micros();

  if (HALL_DEBOUNCE_US > 0 && lastHallPulseUs != 0) {
    unsigned long dtUs = nowUs - lastHallPulseUs;

    if (dtUs < HALL_DEBOUNCE_US) {
      hallRejectedBounce++;
      return;
    }
  }

  lastHallPulseUs = nowUs;
  hallPulseTotal++;
}

// =====================================================
// HALL HELPERS
// =====================================================
uint32_t getHallPulseTotal() {
  noInterrupts();
  uint32_t value = hallPulseTotal;
  interrupts();

  return value;
}

void snapshotHallCounters(uint32_t &pulsesOut, uint32_t &rejectedOut) {
  noInterrupts();
  pulsesOut = hallPulseTotal;
  rejectedOut = hallRejectedBounce;
  interrupts();
}

// =====================================================
// WIND RING HELPERS
// =====================================================
void clearWindRing() {
  windWriteIndex = 0;
  windValidCount = 0;

  for (uint8_t i = 0; i < WIND_RING_SIZE; i++) {
    windRing[i].elapsedMs = 0;
    windRing[i].pulses = 0;
  }
}

void addWindBin(uint16_t elapsedMs, uint16_t pulses) {
  windRing[windWriteIndex].elapsedMs = elapsedMs;
  windRing[windWriteIndex].pulses = pulses;

  windWriteIndex++;

  if (windWriteIndex >= WIND_RING_SIZE) {
    windWriteIndex = 0;
  }

  if (windValidCount < WIND_RING_SIZE) {
    windValidCount++;
  }
}

bool rotorRpsFromRing(float &rpsOut) {
  unsigned long totalMs = 0;
  uint32_t totalPulses = 0;

  for (uint8_t n = 0; n < windValidCount; n++) {
    int index = (int)windWriteIndex - 1 - n;

    while (index < 0) {
      index += WIND_RING_SIZE;
    }

    uint16_t binMs = windRing[index].elapsedMs;

    if (binMs == 0) {
      continue;
    }

    // Use complete bins only.
    // This avoids counting all pulses from a bin while using only part of its time.
    if (totalMs >= WIND_WINDOW_MS) {
      break;
    }

    totalMs += binMs;
    totalPulses += windRing[index].pulses;
  }

  latestWindowMs = totalMs;
  latestWindowPulses = totalPulses;

  if (totalMs < WIND_MIN_WINDOW_MS || totalMs == 0) {
    rpsOut = 0.0f;
    return false;
  }

  float seconds = (float)totalMs / 1000.0f;
  float rotations = (float)totalPulses / (float)pulsesPerRevolution;

  rpsOut = rotations / seconds;
  return true;
}

// =====================================================
// 15 s MEAN HELPERS
// =====================================================
void clearWindMeanRing() {
  windMeanWriteIndex = 0;
  windMeanCount = 0;

  for (uint8_t i = 0; i < WIND_MEAN_RING_SIZE; i++) {
    windMeanRing[i].sampleMs = 0;
    windMeanRing[i].windMpsX10 = 0;
  }
}

void addWindMeanSample(uint16_t sampleMs, uint16_t windMpsX10) {
  windMeanRing[windMeanWriteIndex].sampleMs = sampleMs;
  windMeanRing[windMeanWriteIndex].windMpsX10 = windMpsX10;

  windMeanWriteIndex++;

  if (windMeanWriteIndex >= WIND_MEAN_RING_SIZE) {
    windMeanWriteIndex = 0;
  }

  if (windMeanCount < WIND_MEAN_RING_SIZE) {
    windMeanCount++;
  }
}

bool meanMpsFromRecentSamples(
  unsigned long targetWindowMs,
  unsigned long minimumWindowMs,
  float &meanMpsOut
) {
  unsigned long totalMs = 0;
  float weightedSum = 0.0f;

  for (uint8_t n = 0; n < windMeanCount && totalMs < targetWindowMs; n++) {
    int index = (int)windMeanWriteIndex - 1 - n;

    while (index < 0) {
      index += WIND_MEAN_RING_SIZE;
    }

    uint16_t sampleMs = windMeanRing[index].sampleMs;

    if (sampleMs == 0) {
      continue;
    }

    unsigned long remainingMs = targetWindowMs - totalMs;
    unsigned long usedMs = sampleMs;

    if (usedMs > remainingMs) {
      usedMs = remainingMs;
    }

    float mps = unpackMpsX10(windMeanRing[index].windMpsX10);

    weightedSum += mps * (float)usedMs;
    totalMs += usedMs;
  }

  if (totalMs < minimumWindowMs || totalMs == 0) {
    meanMpsOut = 0.0f;
    return false;
  }

  meanMpsOut = weightedSum / (float)totalMs;
  return true;
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

  if (Wire.available() < 6) {
    return false;
  }

  uint8_t data[6];

  for (int i = 0; i < 6; i++) {
    data[i] = Wire.read();
  }

  uint32_t humidityRaw =
    ((uint32_t)data[1] << 12) |
    ((uint32_t)data[2] << 4)  |
    (data[3] >> 4);

  uint32_t temperatureRaw =
    ((uint32_t)(data[3] & 0x0F) << 16) |
    ((uint32_t)data[4] << 8) |
    data[5];

  humidityOut = humidityRaw * 100.0f / 1048576.0f;
  temperatureOut = temperatureRaw * 200.0f / 1048576.0f - 50.0f;

  if (humidityOut < 0.0f) {
    humidityOut = 0.0f;
  }

  if (humidityOut > 100.0f) {
    humidityOut = 100.0f;
  }

  return true;
}

// =====================================================
// SENSOR SERVICE
// =====================================================
void readSensorsNow() {
  float h;
  float t;

  if (readAHT20(h, t)) {
    humidityPct = h;
    temperatureC = t;
  }

  if (bmpOk) {
    stationPressureHpa = bmp.readPressure() / 100.0f;
  }

  seaLevelHpa = reduceToSeaLevelWMO(
    stationPressureHpa,
    temperatureC,
    humidityPct,
    altitudeM
  );
}

void serviceSensors() {
  unsigned long now = millis();

  if (now - lastSensorUpdateMs >= SENSOR_UPDATE_MS) {
    readSensorsNow();
    lastSensorUpdateMs = millis();
  }
}

// =====================================================
// WIND SERVICE
// =====================================================
void serviceWind() {
  unsigned long nowMs = millis();

  if (nowMs - lastWindBinMs < WIND_BIN_MS) {
    return;
  }

  uint32_t pulseTotalNow = getHallPulseTotal();

  uint32_t deltaPulses32 = pulseTotalNow - lastWindBinPulseTotal;
  unsigned long elapsedMsLong = nowMs - lastWindBinMs;

  lastWindBinPulseTotal = pulseTotalNow;
  lastWindBinMs = nowMs;

  uint16_t elapsedMs =
    (elapsedMsLong > 65535UL) ? 65535 : (uint16_t)elapsedMsLong;

  uint16_t pulses =
    (deltaPulses32 > 65535UL) ? 65535 : (uint16_t)deltaPulses32;

  addWindBin(elapsedMs, pulses);

  float rotorRps = 0.0f;

  if (rotorRpsFromRing(rotorRps)) {
    latestRotorRps = rotorRps;
    latestRotorRpm = rotorRps * 60.0f;
    latestWindMps = windMpsFromRotorRps(rotorRps);
  } else {
    latestRotorRps = 0.0f;
    latestRotorRpm = 0.0f;
    latestWindMps = 0.0f;
  }

  if (latestWindMps < 0.05f) {
    latestWindMps = 0.0f;
  }
}

// =====================================================
// RADIO SERVICE
// =====================================================
void sendPacket() {
  unsigned long nowMs = millis();
  unsigned long sampleMsLong = nowMs - lastTxWindSampleMs;

  if (sampleMsLong == 0) {
    sampleMsLong = 1;
  }

  lastTxWindSampleMs = nowMs;

  uint16_t sampleMs16 =
    (sampleMsLong > 65535UL) ? 65535 : (uint16_t)sampleMsLong;

  uint16_t windMpsX10 = packMpsX10(latestWindMps);

  addWindMeanSample(sampleMs16, windMpsX10);

  float meanPreview = 0.0f;

  if (meanMpsFromRecentSamples(
        WIND_MEAN_WINDOW_MS,
        WIND_MEAN_MIN_MS,
        meanPreview
      )) {
    latestMean15Mps = meanPreview;
  } else {
    latestMean15Mps = unpackMpsX10(windMpsX10);
  }

  WeatherPacket tx;

  tx.txMs = nowMs;
  tx.sampleMs = sampleMs16;
  tx.windMpsX10 = windMpsX10;
  tx.temperatureCx10 = packTempCx10(temperatureC);
  tx.humidityX10 = packUnsignedX10(humidityPct);
  tx.seaLevelHpaX10 = packUnsignedX10(seaLevelHpa);

  bool ok = radio.write(&tx, sizeof(tx));

  uint32_t pulsesNow;
  uint32_t bounceNow;
  snapshotHallCounters(pulsesNow, bounceNow);

  Serial.print("TX sampleMs=");
  Serial.print(tx.sampleMs);

  Serial.print(" wind=");
  Serial.print(unpackMpsX10(tx.windMpsX10), 1);
  Serial.print(" m/s");

  Serial.print(" rpm=");
  Serial.print(latestRotorRpm, 1);

  Serial.print(" rps=");
  Serial.print(latestRotorRps, 3);

  Serial.print(" winP=");
  Serial.print(latestWindowPulses);

  Serial.print(" winMs=");
  Serial.print(latestWindowMs);

  Serial.print(" mean15=");
  Serial.print(latestMean15Mps, 1);
  Serial.print(" m/s");

  Serial.print(" pulses=");
  Serial.print(pulsesNow);

  Serial.print(" bounce=");
  Serial.print(bounceNow);

  Serial.print(" T=");
  Serial.print(temperatureC, 1);

  Serial.print(" H=");
  Serial.print(humidityPct, 1);

  Serial.print(" Psl=");
  Serial.print(seaLevelHpa, 1);

  Serial.print(" radio=");
  Serial.println(ok ? "OK" : "FAIL");
}

void serviceRadio() {
  unsigned long now = millis();

  if (now - lastTxMs >= TX_INTERVAL_MS) {
    sendPacket();
    lastTxMs = millis();
  }
}

// =====================================================
// OLED
// =====================================================
void serviceDisplay() {
  if (!oledOk) {
    return;
  }

  unsigned long now = millis();

  if (now - lastOledUpdateMs < OLED_UPDATE_MS) {
    return;
  }

  float displayedWindMps = unpackMpsX10(packMpsX10(latestWindMps));

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);

  display.print("W: ");
  display.print(displayedWindMps, 1);
  display.println(" m/s");

  display.print("RPM: ");
  display.print(latestRotorRpm, 0);
  display.println();

  display.print("M15: ");
  display.print(latestMean15Mps, 1);
  display.println(" m/s");

  display.print("T: ");
  display.print(temperatureC, 1);
  display.println(" C");

  display.print("H: ");
  display.print(humidityPct, 1);
  display.println(" %");

  display.print("P: ");
  display.print(seaLevelHpa, 1);
  display.println(" hPa");

  if (mpsToKt(latestMean15Mps) < 64.0f) {
    display.print("Scale: Bft ");
    display.println(beaufortFromMps(latestMean15Mps));
  } else {
    display.print("Scale: SS ");
    display.println(sshwsEquivalentFromMps(latestMean15Mps));
  }

  display.display();

  lastOledUpdateMs = millis();
}

// =====================================================
// SETUP
// =====================================================
void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("TX RPM-based wind x10 + OLED + ACK radio");

  // I2C
  Wire.begin(16, 17);
  Wire.setClock(400000);

  // OLED
  oledOk = display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS);

  if (oledOk) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("TX booting...");
    display.display();
  } else {
    Serial.println("OLED not found");
  }

  // AHT20
  initAHT20();

  // BMP280
  bmpOk = bmp.begin(BMP280_ADDRESS);

  if (bmpOk) {
    bmp.setSampling(
      Adafruit_BMP280::MODE_NORMAL,
      Adafruit_BMP280::SAMPLING_X2,
      Adafruit_BMP280::SAMPLING_X16,
      Adafruit_BMP280::FILTER_X16,
      Adafruit_BMP280::STANDBY_MS_500
    );
  } else {
    Serial.println("BMP280 not found");
  }

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
  radio.setAutoAck(true);
  radio.setRetries(5, 15);
  radio.setPayloadSize(sizeof(WeatherPacket));

  radio.openWritingPipe(pipeAddress);
  radio.stopListening();
  radio.flush_tx();

  // Wind
  clearWindRing();
  clearWindMeanRing();

  pinMode(hallPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(hallPin), onHallPulse, FALLING);

  // Initial sensor read
  readSensorsNow();

  unsigned long startMs = millis();
  uint32_t startPulses = getHallPulseTotal();

  lastWindBinMs = startMs;
  lastWindBinPulseTotal = startPulses;

  lastTxMs = startMs;
  lastTxWindSampleMs = startMs;
  lastOledUpdateMs = startMs;
  lastSensorUpdateMs = startMs;

  Serial.println();
  Serial.println("Geometry:");
  Serial.print("Magnet radius m: ");
  Serial.println(magnetRadiusMeters, 4);

  Serial.print("Cup radius m: ");
  Serial.println(cupRadiusMeters, 4);

  Serial.print("Cup assembly diameter m: ");
  Serial.println(cupAssemblyDiameterMeters, 4);

  Serial.print("Cup circumference m: ");
  Serial.println(CUP_CIRCUMFERENCE_M, 4);

  Serial.print("Pulses per revolution: ");
  Serial.println(pulsesPerRevolution);

  Serial.println();
  Serial.println("Calibration:");
  Serial.print("Calibration factor: ");
  Serial.println(calibrationFactor, 4);

  Serial.print("Wind m/s per rotor RPS: ");
  Serial.println(WIND_MPS_PER_RPS, 4);

  Serial.print("Wind m/s per rotor RPM: ");
  Serial.println(WIND_MPS_PER_RPM, 5);

  Serial.print("Wind m/s per pulse Hz: ");
  Serial.println(WIND_MPS_PER_PULSE_HZ, 5);

  Serial.println();
  Serial.println("Measurement:");
  Serial.print("Wind bin ms: ");
  Serial.println(WIND_BIN_MS);

  Serial.print("Wind window ms: ");
  Serial.println(WIND_WINDOW_MS);

  Serial.print("Wind minimum window ms: ");
  Serial.println(WIND_MIN_WINDOW_MS);

  Serial.print("Hall debounce us: ");
  Serial.println(HALL_DEBOUNCE_US);

  Serial.println();
  Serial.println("Radio:");
  Serial.print("Radio chip: ");
  Serial.println(radio.isChipConnected() ? "OK" : "NOT_CONNECTED");

  Serial.print("WeatherPacket bytes: ");
  Serial.println(sizeof(WeatherPacket));

  Serial.print("Channel: ");
  Serial.println(RADIO_CHANNEL);

  if (oledOk) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("TX ready");
    display.print("K=");
    display.println(calibrationFactor, 2);
    display.print("Pkt ");
    display.print(sizeof(WeatherPacket));
    display.println(" B");
    display.display();
  }

  Serial.println();
  Serial.println("TX ready");
}

// =====================================================
// LOOP
// =====================================================
void loop() {
  serviceWind();
  serviceRadio();
  serviceSensors();
  serviceDisplay();
}

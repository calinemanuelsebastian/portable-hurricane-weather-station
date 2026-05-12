# Assembly Notes

These notes describe the assembly and first-test procedure for the current ESP32-based portable weather monitoring prototype. They are based on the uploaded firmware files:

- `portable_station_esp32.ino` — portable measurement unit / transmitter;
- `central_station_esp32.ino` — central receiving station / ThingSpeak uploader.

The current firmware configuration uses an **AHT20 sensor for temperature and relative humidity** and a **BMP280 sensor for atmospheric pressure**. It does not use a DHT22 in the uploaded code.

---

## 1. System Overview

The system consists of two main nodes:

1. **Portable measurement unit**
   - ESP32 development board;
   - AHT20 temperature and relative humidity sensor;
   - BMP280 pressure sensor;
   - OLED display;
   - custom Hall-effect cup anemometer;
   - nRF24L01 radio module;
   - 18650-based battery supply and DC-DC converters;
   - PETG enclosure and printed anemometer parts.

2. **Central receiving station**
   - ESP32 development board;
   - nRF24L01 radio module;
   - Wi-Fi connection;
   - ThingSpeak channel for online visualisation.

The general data chain is:

```text
AHT20 + BMP280 + Hall-effect anemometer
        -> portable ESP32
        -> nRF24L01 wireless link
        -> central ESP32
        -> Wi-Fi
        -> ThingSpeak
        -> remote user
```

---

## 2. Required Arduino Libraries

Install the following libraries before compiling the firmware:

### Portable measurement unit

- `Wire`
- `SPI`
- `RF24`
- `Adafruit BMP280`
- `Adafruit GFX`
- `Adafruit SSD1306`

### Central receiving station

- `SPI`
- `RF24`
- `WiFi`
- `ThingSpeak`

Use an ESP32 board package compatible with the selected ESP32 development boards.

---

## 3. Portable Measurement Unit Pinout

The uploaded transmitter firmware uses the following pin configuration.

| Function | ESP32 pin | Notes |
|---|---:|---|
| I2C SDA | GPIO 16 | Used by OLED, AHT20 and BMP280 |
| I2C SCL | GPIO 17 | Used by OLED, AHT20 and BMP280 |
| Hall sensor output | GPIO 4 | KY-003 pulse input, interrupt on falling edge |
| nRF24L01 CE | GPIO 22 | Radio control pin |
| nRF24L01 CSN | GPIO 5 | SPI chip select |
| nRF24L01 SCK | GPIO 18 | SPI clock |
| nRF24L01 MISO | GPIO 19 | SPI MISO |
| nRF24L01 MOSI | GPIO 23 | SPI MOSI |
| OLED address | `0x3C` | Change to `0x3D` in code if required |
| BMP280 address | `0x77` | Change to `0x76` in code if required |
| AHT20 address | `0x38` | Temperature and relative humidity |

Important: the code uses **GPIO 16 and GPIO 17 for I2C**, not the common ESP32 default pins GPIO 21 and GPIO 22. This is important because GPIO 22 is already used as the nRF24L01 CE pin.

---

## 4. Central Station Pinout

The uploaded receiver firmware uses the same nRF24L01 SPI pin configuration.

| Function | ESP32 pin | Notes |
|---|---:|---|
| nRF24L01 CE | GPIO 22 | Radio control pin |
| nRF24L01 CSN | GPIO 5 | SPI chip select |
| nRF24L01 SCK | GPIO 18 | SPI clock |
| nRF24L01 MISO | GPIO 19 | SPI MISO |
| nRF24L01 MOSI | GPIO 23 | SPI MOSI |

The central station firmware does not use an OLED display in the uploaded code. It receives packets from the portable station, prints information to the Serial Monitor, and uploads the received values to ThingSpeak through Wi-Fi.

---

## 5. nRF24L01 Wireless Link

Both ESP32 boards must use the same radio configuration:

| Setting | Value in firmware |
|---|---|
| Pipe address | `RxAAA` |
| Data rate | `RF24_250KBPS` |
| Payload size | 32 bytes |
| Packet magic | `0x574E4431UL` |
| Packet version | `1` |

The transmitter and receiver use the same packed `SensorPacket` structure. The packet is exactly **32 bytes**, which is the maximum payload size of the nRF24L01. If fields are added later, the packet size must be checked again on both the transmitter and receiver.

The current packet contains:

```text
magic
version
reserved
sequence number
temperature C
humidity %
sea-level pressure hPa
current/live wind speed m/s
15 s mean wind speed m/s
3 s peak/gust wind speed m/s
```

Recommended radio assembly notes:

- Power each nRF24L01 module from a stable **3.3 V** supply.
- Do not power the nRF24L01 from 5 V.
- Place a capacitor of approximately **10 uF to 100 uF** across VCC and GND close to each nRF24L01 module.
- Keep radio wiring short where possible.
- Make sure both ESP32 boards and all modules share a common ground.

---

## 6. Environmental Sensor Assembly

Connect the AHT20, BMP280 and OLED display to the same I2C bus:

```text
ESP32 GPIO 16 -> SDA
ESP32 GPIO 17 -> SCL
3.3 V         -> sensor/display VCC, if the modules support 3.3 V operation
GND           -> sensor/display GND
```

The firmware expects:

```text
AHT20 address:  0x38
BMP280 address: 0x77
OLED address:   0x3C
```

If the BMP280 is not detected, try changing this line in the transmitter firmware:

```cpp
#define BMP280_ADDRESS 0x77
```

to:

```cpp
#define BMP280_ADDRESS 0x76
```

If the OLED is not detected, try changing:

```cpp
#define OLED_ADDRESS 0x3C
```

to:

```cpp
#define OLED_ADDRESS 0x3D
```

The transmitter firmware stops during boot if the BMP280 or nRF24L01 fails to initialise. The OLED displays a short error message in these cases.

---

## 7. Anemometer Assembly

The wind-speed sensor is a custom cup anemometer using three magnets and a KY-003 Hall-effect sensor module.

### Mechanical assembly

1. Print the anemometer hub, arms and cup supports in PETG.
2. Mount the cups symmetrically to reduce imbalance and vibration.
3. Install the rotating shaft using the selected M5 screw or threaded rod.
4. Install the bearings and check that the rotor spins freely.
5. Mount three neodymium magnets on the rotating assembly.
6. Place the magnets at equal angular spacing, approximately 120 degrees apart.
7. Secure the magnets mechanically so they cannot detach during rotation.
8. Use threadlocker where appropriate to prevent loosening due to vibration.

### Hall sensor positioning

1. Fix the KY-003 Hall-effect sensor close to the path of the magnets.
2. Adjust the gap so that each magnet passage is detected reliably.
3. Make sure the rotating magnets do not touch the sensor.
4. Route the sensor wires so that they cannot interfere with the rotating assembly.
5. Test the rotor manually before powering the complete system outdoors.

The uploaded firmware uses:

```cpp
const int hallPin = 4;
const int pulsesPerRevolution = 3;
```

Therefore, the assembled anemometer must produce **three pulses per revolution**. If the number of magnets is changed, `pulsesPerRevolution` must also be changed.

---

## 8. Wind-Speed Calculation Settings

The current wind-speed conversion is a provisional functional implementation. The uploaded transmitter firmware uses:

```cpp
const float cupRadiusMeters = 0.146f;
const int pulsesPerRevolution = 3;
const float calibrationFactor = 4.2833f;
const float calibrationOffsetMps = 0.0f;
```

The basic conversion used by the firmware is:

```text
pulseHz = detected pulses per second
windSpeedMps = calibrationOffsetMps
             + pulseHz * calibrationFactor * 2 * PI * cupRadiusMeters / pulsesPerRevolution
```

The code applies an electrical debounce filter to reduce chatter from the Hall sensor input. The previous wind-speed-based `70 kt` edge gate has been removed, so the remaining gate is only:

```cpp
const unsigned long ELECTRICAL_DEBOUNCE_US = 5000UL;
const unsigned long EDGE_GATE_US = ELECTRICAL_DEBOUNCE_US;
```

The wind-speed algorithm must be calibrated against a reference anemometer before the system is used for quantitative meteorological measurements.

---

## 9. Firmware Timing

The transmitter firmware uses the following timing values:

| Function | Firmware value |
|---|---:|
| Wind sampling interval | 100 ms |
| OLED refresh interval | 500 ms |
| AHT20 + BMP280 sensor refresh interval | 1000 ms |
| Wireless transmission interval | 15000 ms |
| Live wind display window | 3000 ms |
| Gust/peak window | 3000 ms |

The receiver firmware also includes a 15000 ms ThingSpeak upload guard:

```cpp
const unsigned long THINGSPEAK_MIN_INTERVAL_MS = 15000UL;
```

In the current implementation, the transmitter sends one data packet every 15 s of accumulated wind-sampling time, and the receiver uploads accepted packets to ThingSpeak subject to the interval guard.

---

## 10. Pressure Reading and Altitude Setting

The BMP280 measures station pressure. The transmitter firmware converts this value to sea-level equivalent pressure using temperature, relative humidity and a configured altitude value.

The current code setting is:

```cpp
const float altitudeM = 0.0f;
```

Before field testing, set this value to the approximate altitude of the deployment location in metres. If it remains at `0.0f`, the sea-level reduction will effectively use sea-level altitude.

This correction is useful for display and comparison, but it should not be treated as a certified pressure measurement without comparison against a reference instrument.

---

## 11. OLED Display Information

The portable station OLED displays:

- packet sequence number;
- wind speed in knots;
- temperature in degrees Celsius;
- relative humidity in percent;
- sea-level pressure in hPa;
- Beaufort scale value, or Saffir-Simpson-equivalent category when the 15 s mean wind speed reaches the configured threshold.

The Saffir-Simpson value displayed by the prototype is only a firmware-derived equivalent based on the current wind-speed estimate. It should not be treated as an official hurricane classification, especially before anemometer calibration.

---

## 12. ThingSpeak Setup

Create a ThingSpeak channel and configure the field labels so that they match the receiver firmware.

The uploaded receiver firmware currently maps the fields as follows:

| ThingSpeak field | Firmware value |
|---|---|
| Field 1 | Temperature, degrees Celsius |
| Field 2 | Relative humidity, percent |
| Field 3 | Sea-level pressure, hPa |
| Field 4 | Current/live wind speed, m/s |
| Field 5 | 15 s mean wind speed, m/s |
| Field 6 | 3 s peak/gust wind speed, m/s |
| Field 7 | Beaufort value from 15 s mean wind speed |
| Field 8 | Saffir-Simpson-equivalent value from 15 s mean wind speed |

Important consistency check: if the ThingSpeak dashboard labels Field 1 as humidity and Field 2 as temperature, then either the ThingSpeak labels or the receiver code should be corrected. The uploaded receiver code sends temperature to Field 1 and humidity to Field 2.

Before uploading the receiver firmware, replace the placeholder values:

```cpp
const char* ssid = "YOUR_WIFI_SSID";
const char* pass = "YOUR_WIFI_PASSWORD";
unsigned long myChannelNumber = YOUR_CHANNEL_NUMBER;
const char* myWriteAPIKey = "YOUR_THINGSPEAK_WRITE_API_KEY";
```

Do not commit real Wi-Fi passwords or ThingSpeak API keys to a public GitHub repository. Use a separate `config.h` or `secrets.h` file for private credentials and add it to `.gitignore`.

---

## 13. Power Supply Assembly

The portable station is intended to be powered by two 18650 lithium-ion cells connected in series and protected by a 2S BMS module.

Recommended checks before connecting the electronics:

1. Verify the battery pack polarity.
2. Verify the 2S BMS wiring.
3. Adjust and measure each DC-DC converter output using a multimeter.
4. Confirm the 3.3 V rail before connecting the nRF24L01, AHT20, BMP280 and OLED.
5. Confirm the 5 V rail before connecting the ESP32 development board, if powering it through the 5 V/VIN input.
6. Confirm that all grounds are connected together.
7. Do not connect 5 V directly to ESP32 GPIO pins or to modules that require 3.3 V logic.

The nRF24L01 is sensitive to voltage drops. If communication is unstable, first check the 3.3 V rail, the decoupling capacitor and the common ground connection.

Lithium-ion batteries must be handled carefully. Use protected cells or a suitable BMS, avoid short circuits, and do not charge or operate damaged cells.

---

## 14. Enclosure and Weather Protection

The prototype enclosure, anemometer arms and hub are manufactured from PETG. The front panel is made from plexiglass and sealed using a silicone gasket. Cable and component ingresses are sealed using hot-melt adhesive in the current prototype.

Recommended assembly practices:

- use heat-shrink tubing on exposed solder joints;
- provide strain relief for external cables;
- avoid placing electronics directly below possible water ingress points;
- keep the pressure and humidity sensing area ventilated but protected from direct water;
- avoid direct solar heating of the temperature and humidity sensor where possible;
- consider a radiation shield for improved temperature measurements;
- use corrosion-resistant fasteners and connectors for outdoor or salt-air environments;
- replace hot-melt adhesive with more robust sealing methods for long-term outdoor deployment.

The current enclosure should be considered prototype-level protection, not a certified waterproof or storm-rated enclosure.

---

## 15. First Power-Up Procedure

Use this order for first testing.

### Step 1: Test the central station alone

1. Upload `central_station_esp32.ino` with the correct Wi-Fi and ThingSpeak settings.
2. Open the Serial Monitor at 115200 baud.
3. Confirm that the nRF24L01 initialises successfully.
4. Confirm that the ESP32 connects to Wi-Fi.
5. Wait for packets from the portable station.

Expected Serial Monitor messages include:

```text
RX v1.1 - nRF24 + ThingSpeak
Expected packet size: 32 bytes
Wi-Fi connected, IP: ...
Receiver ready
```

### Step 2: Test the portable station indoors

1. Upload `portable_station_esp32.ino`.
2. Open the Serial Monitor at 115200 baud.
3. Confirm that the OLED starts correctly.
4. Confirm that the BMP280 is detected.
5. Confirm that the nRF24L01 initialises successfully.
6. Confirm that temperature, humidity and pressure values appear on the OLED.
7. Rotate the anemometer manually and check that wind values respond.

Expected Serial Monitor messages include:

```text
TX v3.3 - stable efficient wind sampler
Packet size: 32 bytes
Wind coefficient: ...
TX interval: 15000 ms
TX ready
```

### Step 3: Test wireless communication

1. Keep both ESP32 boards powered.
2. Place the units close to each other for the first test.
3. Wait at least 15 s for the first transmitted packet.
4. Confirm that the receiver prints the packet sequence number and sensor values.
5. Confirm that ThingSpeak updates the configured fields.

### Step 4: Test the anemometer

1. Rotate the cups slowly by hand.
2. Confirm that the OLED wind value changes.
3. Confirm that the receiver receives non-zero wind values.
4. Check for unstable values caused by poor sensor alignment, magnet spacing or electrical noise.

---

## 16. Pre-Deployment Checklist

Before outdoor testing, verify the following:

- the AHT20, BMP280 and OLED are connected to GPIO 16 and GPIO 17;
- the BMP280 address matches the physical module;
- the OLED address matches the physical display;
- the Hall sensor output is connected to GPIO 4;
- the anemometer has three magnets if `pulsesPerRevolution = 3`;
- the magnet-to-sensor gap is reliable but does not cause contact;
- the nRF24L01 modules are powered from stable 3.3 V rails;
- decoupling capacitors are installed near the nRF24L01 modules;
- both nRF24L01 modules use the same pipe address and payload structure;
- the central station Wi-Fi credentials are correct;
- the ThingSpeak channel fields match the receiver firmware mapping;
- the altitude value in the transmitter firmware is set correctly for the test location;
- the battery pack and DC-DC converter outputs have been checked with a multimeter;
- all grounds are common;
- the enclosure is closed and cables have strain relief;
- the station is mounted securely and the anemometer can rotate freely.

---

## 17. Known Limitations

The current system is a functional prototype. The following limitations should be considered during assembly, testing and documentation:

- the wind-speed conversion is not yet calibrated against a reference anemometer;
- the pressure correction depends on the altitude value configured in the firmware;
- temperature and humidity readings may be affected by enclosure heating, poor ventilation and direct sunlight;
- the PETG enclosure is not an IP-rated weatherproof enclosure;
- the nRF24L01 link is intended for short-range testing and may be affected by 2.4 GHz interference;
- battery autonomy has not yet been fully characterised;
- the system depends on the central station Wi-Fi connection for online publication;
- the system is not intended for official warnings, navigation decisions, certified measurements or safety-critical decisions.

Future improvements may include calibrated wind-speed measurement, comparison with reference instruments, improved waterproofing, corrosion-resistant connectors, solar charging, local data logging, battery monitoring and longer-range communication using LoRa modules.

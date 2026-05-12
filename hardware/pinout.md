# Pinout

This document describes the wiring used by the current ESP32-based portable weather monitoring prototype.

The system has two electronic units:

1. `portable_station_esp32` – the measurement unit with sensors, OLED display, Hall-effect anemometer input and nRF24L01 transmitter;
2. `central_station_esp32` – the receiving unit with nRF24L01 receiver, Wi-Fi connection and ThingSpeak publication.

> Firmware reference: `portable_station_esp32_no_70kt_edge_filter.ino` and `central_station_esp32.ino`.

---

## 1. Portable station ESP32 pinout

### I2C bus

The portable station uses one I2C bus for the OLED display, AHT20 sensor and BMP280 pressure module.

| Function | ESP32 pin | Notes |
|---|---:|---|
| I2C SDA | GPIO 16 | Shared by OLED, AHT20 and BMP280 |
| I2C SCL | GPIO 17 | Shared by OLED, AHT20 and BMP280 |
| 3.3 V | 3V3 rail | Recommended supply for I2C modules |
| GND | GND | Common ground required |

Current I2C addresses used in the firmware:

| Device | Address | Notes |
|---|---:|---|
| AHT20 | `0x38` | Temperature and relative humidity |
| BMP280 | `0x77` | Change to `0x76` in firmware if the module uses that address |
| OLED display | `0x3C` | Change to `0x3D` in firmware if required |

Firmware line:

```cpp
Wire.begin(16, 17);
```

---

### AHT20 temperature and humidity sensor

| AHT20 pin | ESP32 / rail | Notes |
|---|---|---|
| VCC | 3.3 V | Use 3.3 V logic level with ESP32 |
| GND | GND | Common ground |
| SDA | GPIO 16 | I2C data |
| SCL | GPIO 17 | I2C clock |

---

### BMP280 pressure sensor

| BMP280 pin | ESP32 / rail | Notes |
|---|---|---|
| VCC / VIN | 3.3 V | Use 3.3 V if the board supports direct 3.3 V supply |
| GND | GND | Common ground |
| SDA | GPIO 16 | I2C data |
| SCL | GPIO 17 | I2C clock |

Current firmware address:

```cpp
#define BMP280_ADDRESS 0x77
```

If the sensor is not detected, try changing it to:

```cpp
#define BMP280_ADDRESS 0x76
```

---

### OLED display

| OLED pin | ESP32 / rail | Notes |
|---|---|---|
| VCC | 3.3 V | Use 3.3 V unless the OLED board specifically supports 5 V input |
| GND | GND | Common ground |
| SDA | GPIO 16 | I2C data |
| SCL | GPIO 17 | I2C clock |

Current firmware address:

```cpp
#define OLED_ADDRESS 0x3C
```

---

### KY-003 Hall-effect sensor for anemometer

The KY-003 Hall-effect sensor detects the three magnets installed on the rotating anemometer assembly.

| KY-003 pin | ESP32 / rail | Notes |
|---|---:|---|
| VCC | 3.3 V | Recommended for ESP32 logic safety |
| GND | GND | Common ground |
| OUT / Signal | GPIO 4 | Hall pulse input |

Firmware configuration:

```cpp
const int hallPin = 4;
pinMode(hallPin, INPUT_PULLUP);
attachInterrupt(digitalPinToInterrupt(hallPin), onMagnetDetected, FALLING);
```

Important notes:

- the firmware expects a falling edge when a magnet is detected;
- the current anemometer uses three magnets, so the firmware uses `pulsesPerRevolution = 3`;
- the current firmware uses only an electrical debounce gate for pulse filtering;
- no wind-speed-based 70 kt edge rejection threshold is used in the updated firmware.

---

### nRF24L01 module on portable station

| nRF24L01 pin | ESP32 pin / rail | Notes |
|---|---:|---|
| VCC | 3.3 V | Do not power the nRF24L01 from 5 V |
| GND | GND | Common ground |
| CE | GPIO 22 | Radio control pin |
| CSN / CS | GPIO 5 | SPI chip select |
| SCK | GPIO 18 | SPI clock |
| MISO | GPIO 19 | SPI MISO |
| MOSI | GPIO 23 | SPI MOSI |
| IRQ | Not connected | Not used in the current firmware |

Firmware configuration:

```cpp
#define CE_PIN    22
#define CSN_PIN    5
#define SCK_PIN   18
#define MISO_PIN  19
#define MOSI_PIN  23

SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, CSN_PIN);
RF24 radio(CE_PIN, CSN_PIN);
const byte pipeAddress[6] = "RxAAA";
```

Power recommendation:

- place a capacitor of approximately 10 µF to 100 µF across the nRF24L01 VCC and GND pins;
- keep radio wiring short;
- use a stable 3.3 V supply, especially when the ESP32 and radio transmit at the same time.

---

## 2. Central station ESP32 pinout

The central station currently uses an ESP32 board, one nRF24L01 module, Wi-Fi and ThingSpeak publication. It does not use local environmental sensors in the current firmware.

### nRF24L01 module on central station

| nRF24L01 pin | ESP32 pin / rail | Notes |
|---|---:|---|
| VCC | 3.3 V | Do not power the nRF24L01 from 5 V |
| GND | GND | Common ground |
| CE | GPIO 22 | Radio control pin |
| CSN / CS | GPIO 5 | SPI chip select |
| SCK | GPIO 18 | SPI clock |
| MISO | GPIO 19 | SPI MISO |
| MOSI | GPIO 23 | SPI MOSI |
| IRQ | Not connected | Not used in the current firmware |

Firmware configuration:

```cpp
#define CE_PIN   22
#define CSN_PIN   5
#define SCK_PIN  18
#define MISO_PIN 19
#define MOSI_PIN 23

SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, CSN_PIN);
RF24 radio(CE_PIN, CSN_PIN);
const byte pipeAddress[6] = "RxAAA";
```

---

## 3. Shared radio configuration

Both ESP32 boards must use the same nRF24L01 communication settings.

| Setting | Current value |
|---|---:|
| Pipe address | `RxAAA` |
| Data rate | `RF24_250KBPS` |
| Payload size | 32 bytes |
| Packet magic | `0x574E4431UL` / `WND1` |
| Packet version | `1` |

The packet structure must remain identical in both firmware files.

Current packet fields:

```cpp
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
```

The firmware checks that the payload is exactly 32 bytes:

```cpp
static_assert(sizeof(SensorPacket) == 32, "SensorPacket must be exactly 32 bytes");
```

If new fields are added, the packet size must be checked carefully because the nRF24L01 maximum payload size is 32 bytes.

---

## 4. ThingSpeak field mapping

The receiver firmware currently uploads the received values to ThingSpeak using the following field order:

| ThingSpeak field | Data uploaded by receiver firmware | Unit / meaning |
|---:|---|---|
| Field 1 | `temperatureC` | Temperature in °C |
| Field 2 | `humidityPct` | Relative humidity in % |
| Field 3 | `seaLevelHpa` | Sea-level equivalent pressure in hPa |
| Field 4 | `windInstantMps` | Instantaneous / displayed wind speed in m/s |
| Field 5 | `windMeanMps` | Mean wind speed over the transmission interval in m/s |
| Field 6 | `windPeakMps` | Peak / gust wind speed over the transmission interval in m/s |
| Field 7 | `beaufortMean` | Beaufort number calculated from mean wind speed |
| Field 8 | `sshwsEqMean` | Saffir-Simpson equivalent category calculated from mean wind speed |

Receiver firmware lines:

```cpp
ThingSpeak.setField(1, p.temperatureC);
ThingSpeak.setField(2, p.humidityPct);
ThingSpeak.setField(3, p.seaLevelHpa);
ThingSpeak.setField(4, p.windInstantMps);
ThingSpeak.setField(5, p.windMeanMps);
ThingSpeak.setField(6, p.windPeakMps);
ThingSpeak.setField(7, (long)beaufortMean);
ThingSpeak.setField(8, (long)sshwsEqMean);
```

Important: the ThingSpeak channel labels should match this field order. If the online channel is labelled differently, either relabel the ThingSpeak fields or change the receiver firmware field mapping.

---

## 5. Power wiring notes

### Portable station

The portable station is powered from two 18650 lithium-ion cells connected in series and protected by a 2S BMS.

Recommended power arrangement:

| Rail / module | Purpose |
|---|---|
| 2 × 18650 cells in series | Main battery pack |
| 2S BMS | Battery protection |
| Main switch | Turns portable unit on and off |
| MP1584EN buck converter | Regulated 3.3 V rail for sensors and nRF24L01 |
| MT3608 boost converter | 5 V supply for ESP32 development board, if used in the current build |

Important safety notes:

- verify all voltages with a multimeter before connecting the ESP32, sensors or radio modules;
- all modules must share a common ground;
- do not connect 5 V signals to ESP32 GPIO pins;
- the nRF24L01 must be powered from 3.3 V, not 5 V;
- lithium-ion cells require careful handling, correct polarity and a suitable BMS/charging arrangement.

### Central station

The central station can be powered from USB or a stable 5 V supply suitable for the ESP32 development board. The nRF24L01 module must still be supplied from 3.3 V.

---

## 6. Quick wiring checklist

Before powering the system:

- check battery polarity;
- check 3.3 V and 5 V rails with a multimeter;
- check that all grounds are connected together;
- check that the nRF24L01 modules receive 3.3 V only;
- check that SDA and SCL are connected to GPIO 16 and GPIO 17 on the portable station;
- check that both nRF24L01 modules use the same CE, CSN, SCK, MISO and MOSI wiring;
- check that the KY-003 output is connected to GPIO 4;
- check that the ThingSpeak field labels match the receiver firmware mapping;
- check that the packet structure is identical in both firmware files.

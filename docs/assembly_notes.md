
---

## `docs/assembly_notes.md`

```markdown
# Assembly Notes

## General overview

The system consists of two main parts:

1. Portable measurement unit
2. Central receiving station

The portable unit measures wind speed, temperature, relative humidity and atmospheric pressure. The central station receives the data wirelessly and publishes them online through ThingSpeak.

## Portable measurement unit

The portable unit is based on an ESP32 development board and includes:

- ESP32 microcontroller;
- AHT20 temperature and humidity sensor;
- BMP280 atmospheric pressure sensor;
- custom cup anemometer;
- KY-003 Hall-effect sensor module;
- nRF24L01 wireless module;
- OLED display;
- 2 × 18650 lithium-ion cells;
- 2S battery management/protection module;
- DC-DC voltage converters;
- 3D-printed PETG enclosure;
- plexiglass front panel with silicone gasket.

## Anemometer assembly

The anemometer is a custom-designed cup anemometer. The arms and hub are 3D printed from PETG.

Assembly steps:

1. Print the anemometer hub, arms and cup supports from PETG.
2. Mount the three cups symmetrically around the central shaft.
3. Install three neodymium magnets on the rotating assembly.
4. Ensure that the magnets are positioned at the same distance from the shaft centre.
5. Mount the KY-003 Hall-effect sensor near the rotating magnets.
6. Adjust the sensor position so that each magnet passage is detected reliably.
7. Rotate the anemometer manually and check that pulses are detected by the ESP32.

The current prototype uses three magnets, therefore the firmware is configured with:

```cpp
const int pulsesPerRevolution = 3;

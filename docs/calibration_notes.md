# Calibration Notes

## Current calibration status

The wind-speed measurement is currently based on a functional, prototype-level conversion algorithm. The anemometer uses three neodymium magnets mounted on the rotating assembly and a KY-003 Hall-effect sensor module fixed near the rotor. Each time a magnet passes in front of the Hall sensor, a digital pulse is generated and counted by the ESP32.

The current system is able to detect anemometer rotation and convert the pulse frequency into a wind-speed value. However, the conversion factor is provisional and must be refined through calibration against a reference anemometer.

## Wind-speed calculation principle

The current firmware uses a period-based method for estimating instantaneous wind speed.

The main parameters are:

```cpp
const float cupRadiusMeters = 0.146f;
const int pulsesPerRevolution = 3;
const float calibrationFactor = 4.2833f;
const float calibrationOffsetMps = 0.0f;

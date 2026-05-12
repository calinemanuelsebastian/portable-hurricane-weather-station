# Calibration Notes

These notes describe the calibration and adjustment procedure for the current ESP32-based portable weather monitoring prototype. They are based on the uploaded firmware files:

- `portable_station_esp32.ino` — portable measurement unit / transmitter;
- `central_station_esp32.ino` — central receiving station / ThingSpeak uploader.

The current prototype is suitable for **functional testing**, but it should not be treated as a certified meteorological instrument until its measurements are compared with appropriate reference instruments.

---

## 1. Current Calibration Status

The uploaded firmware contains a provisional wind-speed conversion algorithm. It uses the anemometer geometry, the number of magnets, and a provisional calibration factor to convert Hall-effect pulses into wind speed.

Current transmitter settings:

```cpp
const float cupRadiusMeters = 0.146f;           // shaft centre -> cup centre
const int pulsesPerRevolution = 3;              // 3 magnets
const float calibrationFactor = 4.2833f;        // geometry-derived first estimate
const float calibrationOffsetMps = 0.0f;        // keep 0 unless later calibrated
const float altitudeM = 0.0f;                   // set real altitude if needed
```

Important notes:

- the current wind-speed factor is a **first estimate**, not a certified calibration;
- the anemometer uses **three magnets**, so the firmware expects **three pulses per revolution**;
- the pressure value transmitted by the system is a **sea-level equivalent pressure**, calculated using the configured altitude value;
- the current altitude value is `0.0f`, so it should be changed before field testing if the station is not at sea level;
- temperature, humidity, pressure and wind-speed readings should all be compared with suitable reference instruments before being used as quantitative meteorological measurements.

---

## 2. Wind-Speed Measurement Principle

The wind sensor is a custom Hall-effect cup anemometer.

Mechanical and electrical principle:

1. The rotating anemometer assembly carries three neodymium magnets.
2. A KY-003 Hall-effect sensor is fixed close to the magnet path.
3. Each magnet passage generates one digital pulse.
4. The ESP32 counts accepted pulses using an interrupt on GPIO 4.
5. Pulse frequency is converted into wind speed by the firmware.

The uploaded code uses:

```cpp
const int hallPin = 4;
attachInterrupt(digitalPinToInterrupt(hallPin), onMagnetDetected, FALLING);
```

The conversion is based on the following relationship:

```text
pulseHz = accepted pulses per second
revPerSecond = pulseHz / pulsesPerRevolution
cupSpeedMps = 2 * PI * cupRadiusMeters * revPerSecond
windSpeedMps = calibrationOffsetMps + calibrationFactor * cupSpeedMps
```

Equivalent firmware expression:

```text
windSpeedMps = calibrationOffsetMps
             + pulseHz * calibrationFactor * 2 * PI * cupRadiusMeters / pulsesPerRevolution
```

With the current constants, the firmware coefficient is approximately:

```text
WIND_MPS_PER_PULSE_HZ = 1.309755 m/s per pulse/s
WIND_KT_PER_PULSE_HZ  = 2.545954 kt per pulse/s
```

This means that, in the current firmware, a measured pulse frequency of `1 pulse/s` corresponds to approximately `1.31 m/s`, before considering the limitations of the provisional calibration.

---

## 3. Wind Values Produced by the Firmware

The transmitter calculates and sends three wind-speed values:

| Packet value | Meaning | Firmware basis |
|---|---|---|
| `windInstantMps` | current/live displayed wind speed | smoothed value from recent pulse bins |
| `windMeanMps` | mean wind speed over the 15 s transmission interval | accumulated accepted pulses over the report interval |
| `windPeakMps` | peak/gust value inside the 15 s interval | highest rolling 3 s wind value during the interval |

The receiver uploads these values to ThingSpeak using the following field mapping:

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

For calibration, the most useful value is usually **Field 5: 15 s mean wind speed**, because it is less noisy than the live value and better represents the full transmission interval.

---

## 4. Pulse Filtering and Edge Rejection

The transmitter uses a simple electrical debounce filter for the Hall-effect sensor interrupt. The previous wind-speed-based edge gate, which rejected edges above an estimated `70 kt` threshold, has been removed.

Relevant settings:

```cpp
const unsigned long ELECTRICAL_DEBOUNCE_US = 5000UL;
const unsigned long EDGE_GATE_US = ELECTRICAL_DEBOUNCE_US;
```

Any Hall edge arriving sooner than `5000 us` after the previous accepted edge is rejected as probable electrical chatter. This is a debounce filter only; it is not intended to impose a wind-speed limit or reject pulses based on a Saffir-Simpson or Beaufort threshold.

Important notes:

- removing the `70 kt` edge gate avoids artificially limiting high wind-speed pulse detection;
- the remaining `5000 us` debounce still rejects extremely fast repeated edges caused by noise or unstable triggering;
- with the current provisional wind coefficient, a `5000 us` interval corresponds to a much higher theoretical wind speed than the intended prototype test range, so it should not affect normal functional testing;
- if false pulses appear, improve wiring, grounding, sensor mounting and magnet alignment before increasing the debounce value.

During testing, the transmitter Serial Monitor prints useful diagnostic information, including:

```text
pulses=...
rejected=...
gateUs=...
shortestAcceptedUs=...
```

A high number of rejected edges may indicate electrical noise, poor magnet/sensor alignment, excessive vibration, or an unsuitable debounce setting.

---

## 5. Preliminary Checks Before Calibration

Before comparing the anemometer with a reference instrument, complete these checks.

### 5.1 Mechanical checks

- Confirm that the anemometer rotates freely.
- Confirm that the cups are mounted symmetrically.
- Confirm that the shaft is straight and the bearings are not binding.
- Confirm that the magnets are firmly fixed and equally spaced.
- Confirm that the KY-003 sensor does not touch the rotating assembly.
- Confirm that the sensor gap is small enough for reliable detection but large enough to avoid contact.

### 5.2 Pulse-counting checks

The firmware expects:

```cpp
const int pulsesPerRevolution = 3;
```

Therefore, one complete revolution of the rotor should produce three accepted pulses.

Manual rotation is useful for checking pulse detection, but it is **not** a valid wind-speed calibration method because it does not reproduce real aerodynamic behaviour.

### 5.3 Zero-wind check

With the anemometer stationary:

- the displayed wind speed should settle close to `0.0 m/s`;
- the accepted pulse count should not increase continuously;
- the rejected edge count should not increase rapidly;
- if non-zero wind is shown while the rotor is stationary, check wiring, grounding, pull-up behaviour, sensor placement and electrical noise.

---

## 6. Recommended Wind Calibration Procedure

A proper wind calibration requires comparison against a reference anemometer. The reference instrument should be placed as close as practical to the prototype anemometer while avoiding wake effects and physical interference.

### 6.1 Required equipment

Recommended equipment:

- reference anemometer;
- stable airflow source, such as a controlled fan setup or wind tunnel;
- laptop connected to the ESP32 Serial Monitor;
- access to the ThingSpeak channel, if online logging is used;
- notebook or CSV file for recording calibration data.

A wind tunnel or controlled airflow setup is preferred. Outdoor calibration is possible, but it is more difficult because wind speed changes rapidly and the reference and prototype may not experience identical airflow.

### 6.2 Test setup

1. Mount the prototype anemometer securely.
2. Mount the reference anemometer close to it, at a similar height and orientation.
3. Avoid placing one instrument directly behind the other.
4. Keep the anemometer away from walls, corners, table edges or other objects that distort airflow.
5. Start the transmitter and receiver.
6. Confirm that the transmitter sends packets every 15 s.
7. Confirm that the receiver displays and/or uploads the wind values.

### 6.3 Data collection

For each airflow level:

1. Allow the airflow and rotor speed to stabilise.
2. Record the reference wind speed over the same interval as the prototype report.
3. Record the prototype `windMeanMps` value.
4. Record the number of accepted pulses and elapsed time if using Serial Monitor data.
5. Repeat each test point at least three times.
6. Use several wind-speed levels, including low, medium and higher values within the safe range of the test setup.

Suggested CSV structure:

```text
timestamp,test_point,reference_wind_mps,prototype_mean_mps,prototype_current_mps,prototype_peak_mps,report_pulses,report_elapsed_ms,computed_pulse_hz,rejected_edges,notes
```

The pulse frequency can be calculated from Serial Monitor data as:

```text
computed_pulse_hz = report_pulses / (report_elapsed_ms / 1000)
```

For best results, use the 15 s mean wind value rather than the live display value.

---

## 7. Calculating New Wind Calibration Constants

There are two practical ways to update the wind calibration constants.

### Method A: Calibration from raw pulse frequency

This is the cleanest method if `report_pulses` and `report_elapsed_ms` are recorded.

The base coefficient without the calibration factor is:

```text
baseCoefficient = 2 * PI * cupRadiusMeters / pulsesPerRevolution
```

With the current geometry:

```text
baseCoefficient = 0.305782 m/s per pulse/s
```

Fit a linear relationship between the reference wind speed and the measured pulse frequency:

```text
referenceWindMps = offset + slopePulse * pulseHz
```

Then update the firmware constants as:

```text
calibrationFactor = slopePulse / baseCoefficient
calibrationOffsetMps = offset
```

If the fitted offset is very small and not physically meaningful, it may be preferable to keep:

```cpp
const float calibrationOffsetMps = 0.0f;
```

and use a zero-intercept fit.

### Method B: Calibration from the current firmware wind output

This method is easier if only the current firmware output is available.

Fit a linear relationship between the reference wind speed and the current reported 15 s mean wind speed:

```text
referenceWindMps = offset + slopeReported * prototypeMeanMps
```

Because the current firmware offset is `0.0f`, the updated constants can be estimated as:

```text
newCalibrationFactor = oldCalibrationFactor * slopeReported
newCalibrationOffsetMps = offset
```

For the uploaded firmware:

```text
oldCalibrationFactor = 4.2833
```

Example:

```text
If slopeReported = 0.85,
newCalibrationFactor = 4.2833 * 0.85 = 3.6408
```

After updating the constants, upload the firmware again and repeat the comparison with the reference anemometer.

---

## 8. Error Metrics After Calibration

After applying the new constants, collect a new verification dataset and calculate basic error indicators.

For each test point:

```text
error = prototypeWindMps - referenceWindMps
absoluteError = abs(error)
percentageError = 100 * error / referenceWindMps
```

For the whole dataset:

```text
meanError = average(error)
meanAbsoluteError = average(abs(error))
RMSE = sqrt(average(error^2))
```

Do not calculate percentage error for very low wind speeds close to zero, because the result may become misleading.

A simple calibration summary should include:

- number of test points;
- wind-speed range tested;
- reference instrument used;
- old calibration constants;
- new calibration constants;
- mean error;
- mean absolute error;
- RMSE;
- notes about test conditions.

---

## 9. Pressure Calibration and Altitude Setting

The BMP280 measures pressure at the station. The transmitter firmware then converts this to sea-level equivalent pressure using the configured altitude, temperature and relative humidity.

Current altitude setting:

```cpp
const float altitudeM = 0.0f;
```

Before field testing, set `altitudeM` to the approximate altitude of the deployment location in metres.

Important distinction:

- **station pressure** is the actual pressure measured at the sensor location;
- **sea-level pressure** is a corrected value intended for easier comparison between locations at different altitudes.

The current firmware transmits `seaLevelHpa`, not raw station pressure.

Recommended pressure checks:

1. Set the correct altitude value in the transmitter firmware.
2. Place the station near a trusted reference barometer or weather station.
3. Allow the sensor to stabilise.
4. Compare the reported sea-level pressure with the reference sea-level pressure.
5. If comparing against station pressure instead, modify the firmware to print or transmit the raw BMP280 pressure value.

The pressure correction depends on:

- the accuracy of the configured altitude;
- the BMP280 sensor reading;
- the local temperature and humidity values used by the correction function;
- the representativeness of the reference pressure value.

If a systematic pressure offset is observed after setting the correct altitude, a future firmware version may include a pressure offset constant, for example:

```cpp
const float pressureOffsetHpa = 0.0f;
```

The current uploaded firmware does not include this offset.

---

## 10. Temperature and Relative Humidity Checks

The uploaded transmitter firmware uses an AHT20 sensor for temperature and relative humidity.

Current AHT20 address:

```cpp
#define AHT20_ADDRESS 0x38
```

Recommended checks:

1. Place the prototype near a reference temperature and humidity instrument.
2. Keep both instruments away from direct sunlight, hands, hot electronics and airflow from fans unless that airflow is part of the test.
3. Allow enough time for the sensors to stabilise.
4. Compare readings at several conditions if possible.
5. Record the difference between the prototype and the reference instrument.

Possible sources of error:

- heat from the ESP32 and DC-DC converters;
- direct solar radiation;
- poor ventilation inside the enclosure;
- water ingress or condensation;
- sensor placement too close to the enclosure wall or internal electronics;
- lack of a radiation shield during outdoor use.

If a systematic offset is observed, future firmware versions may include correction constants, for example:

```cpp
const float temperatureOffsetC = 0.0f;
const float humidityOffsetPct = 0.0f;
```

The current uploaded firmware does not include these offsets.

---

## 11. Beaufort and Saffir-Simpson-Equivalent Display

Both firmware files contain helper functions for Beaufort and Saffir-Simpson-equivalent values.

Important limitations:

- these values are calculated from the prototype wind-speed estimate;
- the wind-speed estimate is not yet calibrated;
- the receiver calculates the Saffir-Simpson-equivalent value from the 15 s mean wind speed;
- this is not an official hurricane classification;
- the Saffir-Simpson-equivalent value should be treated only as an indicative firmware display feature.

Until the anemometer is calibrated and tested, the system should not be used to classify hurricane intensity or support safety-critical decisions.

---

## 12. Recommended Calibration Record

Keep a calibration record in the repository, for example in a file named:

```text
docs/calibration_record.md
```

Suggested content:

```text
Calibration date:
Location:
Operator:
Prototype version:
Firmware version:
Reference anemometer:
Reference pressure instrument:
Reference temperature/humidity instrument:
Anemometer radius used in firmware:
Pulses per revolution:
Old calibration factor:
Old calibration offset:
New calibration factor:
New calibration offset:
Altitude setting:
Wind-speed range tested:
Number of calibration points:
Mean error:
Mean absolute error:
RMSE:
Notes:
```

Also keep the raw calibration data where possible, for example:

```text
data/wind_calibration_YYYY-MM-DD.csv
```

Do not delete old calibration records. They are useful for tracking design changes, firmware changes and mechanical modifications.

---

## 13. When Recalibration Is Required

Recalibrate or at least re-check the wind-speed measurement if any of the following changes are made:

- the cup radius changes;
- the number of magnets changes;
- the magnet positions change;
- the cup shape changes;
- the bearing type or shaft friction changes significantly;
- the Hall sensor position changes;
- the pulse filtering settings change;
- the anemometer is repaired after damage;
- the firmware wind-speed calculation is modified.

Re-check pressure, temperature and humidity measurements if:

- the sensor module is replaced;
- the sensor position inside the enclosure changes;
- ventilation or radiation shielding is changed;
- the altitude setting is changed;
- the enclosure is modified in a way that affects heating or airflow.

---

## 14. Current Prototype Limitations

The current prototype should be documented with the following limitations:

- wind-speed conversion is provisional and requires calibration against a reference anemometer;
- pressure output depends on the altitude value configured in firmware;
- temperature and humidity may be affected by enclosure heating and ventilation;
- the PETG enclosure is not a certified weatherproof or storm-rated enclosure;
- the nRF24L01 link is intended for short-range functional testing;
- ThingSpeak values depend on the Wi-Fi connection and the central station upload process;
- the system is not intended for official meteorological reporting, navigation decisions, emergency warnings or safety-critical decisions.

The project should be described as a **functional prototype** and a **complementary local observation tool**, not as a replacement for certified meteorological stations.

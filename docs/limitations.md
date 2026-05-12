
---

## `docs/limitations.md`

```markdown
# Limitations

## Prototype status

This project is currently a functional prototype. It has been developed to demonstrate local meteorological data acquisition, wireless transmission and online visualisation. It is not a certified meteorological instrument and should not be used as a replacement for official weather stations.

## Wind-speed accuracy

The anemometer uses a custom mechanical design and a Hall-effect pulse detection system. The current wind-speed conversion algorithm is provisional and has not yet been calibrated against a reference anemometer.

Main limitations:

- the calibration factor is still experimental;
- rotor geometry affects the wind-speed conversion;
- friction, bearing quality and rotor balance may influence measurements;
- low wind speeds may be affected by starting friction;
- strong gusts may require further testing;
- the current algorithm requires validation under controlled conditions.

## Sensor limitations

The AHT20 and BMP280 sensors are suitable for prototype-level environmental monitoring, but the readings may be affected by enclosure placement, direct sunlight, ventilation and thermal influence from nearby electronics.

Possible sources of error include:

- insufficient airflow around the temperature and humidity sensor;
- heating from the ESP32 or voltage regulators;
- lack of radiation shielding;
- pressure reduction errors if altitude is not set correctly;
- sensor drift over time.

## Enclosure limitations

The enclosure is made from 3D-printed PETG and uses a plexiglass front panel with a silicone gasket. Component ingresses are sealed with hot-melt adhesive.

This provides basic prototype protection, but the enclosure is not formally IP-rated.

Further improvements are needed for:

- long-term rain exposure;
- salt-air environments;
- UV exposure;
- corrosion protection;
- cable gland sealing;
- mechanical impact resistance;
- hurricane-level wind and debris conditions.

## Communication limitations

The current version uses nRF24L01 modules for communication between the portable station and the central station.

Limitations include:

- limited range compared with LoRa-based solutions;
- possible interference in the 2.4 GHz band;
- reduced performance through walls or obstacles;
- dependency on correct antenna positioning;
- possible packet loss in unfavourable conditions.

Future versions may use SX1278 LoRa modules or ESP32 boards equipped with LoRa transceivers for improved range and robustness.

## Power limitations

The portable unit uses two 18650 lithium-ion cells. Battery autonomy has not yet been fully tested under long-term outdoor operation.

Power-related limitations include:

- unknown long-term battery life;
- reduced capacity in cold or harsh conditions;
- possible voltage drops under load;
- charging system requiring further testing;
- lack of integrated solar charging in the current version.

## Online publication limitations

The central station depends on Wi-Fi access and the availability of the ThingSpeak platform. If internet connectivity is lost, online visualisation may be interrupted.

Possible improvements include:

- local data logging;
- offline storage during internet outages;
- automatic upload after reconnection;
- configurable update intervals;
- alert thresholds for dangerous wind speeds or pressure drops.

## Testing limitations

The current tests focused on functional operation, not on full validation.

The prototype has been tested for:

- sensor reading;
- OLED display;
- Hall sensor pulse detection;
- wireless transmission;
- reception by the central station;
- online publication through ThingSpeak.

The prototype has not yet been fully tested for:

- calibrated measurement accuracy;
- long-term outdoor reliability;
- battery autonomy;
- communication range;
- operation during heavy rain;
- resistance to severe wind;
- salt-air exposure;
- actual hurricane conditions.

## Intended use

The system is intended for:

- educational use;
- prototype development;
- experimental local weather observation;
- community-level awareness;
- research and demonstration purposes.

The system is not intended for:

- official meteorological reporting;
- navigation safety decisions;
- emergency warnings without independent verification;
- certified disaster-response operations.

## Future improvements

Recommended future developments include:

- calibration against a reference anemometer;
- comparison with certified temperature, humidity and pressure sensors;
- improved enclosure sealing;
- solar charging;
- longer-range communication;
- local data storage;
- improved power management;
- better radiation shielding for the temperature and humidity sensor;
- configurable warning thresholds;
- publication of 3D-printable parts and assembly documentation.

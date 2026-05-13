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
- the current algorithm requires calibration and characterisation under controlled conditions;
- the firmware currently uses an electrical debounce gate for Hall-sensor pulse detection, but no wind-speed-based edge rejection threshold is applied.

## Sensor limitations

The AHT20 and BMP280 sensors are suitable for prototype-level environmental monitoring, but the readings may be affected by enclosure placement, direct sunlight, ventilation and thermal influence from nearby electronics.

Possible sources of error include:

- insufficient airflow around the temperature and humidity sensor;
- heating from the ESP32 or voltage regulators;
- lack of radiation shielding;
- pressure reduction errors if altitude is not set correctly;
- sensor drift over time.

## Pressure correction limitations

The firmware converts pressure readings to sea-level equivalent pressure using a configured altitude value. If the altitude value is incorrect or left unchanged for a different deployment location, the reported sea-level pressure may be inaccurate.

For more reliable pressure reporting, future versions should:

- set the deployment altitude accurately;
- document the altitude used during each test;
- compare pressure readings with a reference instrument;
- allow altitude configuration without modifying the firmware.

## Enclosure limitations

The enclosure is made from 3D-printed PETG and uses a plexiglass front panel with a silicone gasket. Component ingresses are sealed with hot-melt adhesive.

This provides basic prototype-level protection, but the enclosure is not formally IP-rated.

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
- possible packet loss in unfavourable conditions;
- no quantified packet-loss, range or reliability measurements have been performed yet;
- the transmitted packet is currently designed to fit within the nRF24L01 payload limit, so future changes to the packet structure should be checked carefully.

Future versions may use SX1278 LoRa modules or ESP32 boards equipped with LoRa transceivers for improved range and robustness.

## Power limitations

The portable unit uses two 18650 lithium-ion cells. Battery autonomy has not yet been fully tested under long-term outdoor operation.

The current documented wiring also includes a dedicated switch between the BMS/power distribution output and the input of the 3.3 V DC-DC converter. This allows the 3.3 V peripheral rail to be switched independently, but it also requires careful operation if the ESP32 5 V supply remains active.

Power-related limitations include:

- unknown long-term battery life;
- reduced capacity in cold or harsh conditions;
- possible voltage drops under load;
- charging system requiring further testing;
- lack of integrated solar charging in the current version;
- possible unintended back-powering of peripheral modules if the ESP32 is powered while the switched 3.3 V rail is off.

## Online publication limitations

The central station depends on Wi-Fi access and the availability of the ThingSpeak platform. If internet connectivity is lost, online visualisation may be interrupted.

Possible limitations include:

- dependency on Wi-Fi coverage at the central station;
- dependency on the ThingSpeak platform;
- interruption of online visualisation during internet outages;
- possible mismatch between firmware field order and ThingSpeak channel labels if the channel is not configured consistently.

Possible improvements include:

- local data logging;
- offline storage during internet outages;
- automatic upload after reconnection;
- configurable update intervals;
- alert thresholds for dangerous wind speeds or pressure drops.

## Testing limitations

The current tests focused on functional operation, not on formal validation.

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
- packet-loss rate;
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

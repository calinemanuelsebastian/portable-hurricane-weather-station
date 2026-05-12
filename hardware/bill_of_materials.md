```markdown
# Bill of Materials

This bill of materials covers the minimum components required for the current prototype, consisting of one portable ESP32 weather station and one ESP32 central receiving station.

> Note: The current firmware uses an AHT20 + BMP280 configuration. If the physical module is actually DHT22 + BMP280, the firmware and documentation should be adjusted accordingly.

---

## Minimum Components

| Component | Quantity | Used for | Notes |
|---|---:|---|---|
| ESP32 development board | 2 | Portable unit and central station | One ESP32 is used for sensor acquisition; the second ESP32 receives the data and publishes them online |
| AHT20 + BMP280 module / DHT22 + BMP280 module | 1 | Portable unit | Temperature, relative humidity and atmospheric pressure measurement |
| KY-003 Hall-effect sensor module | 1 | Portable unit | Pulse detection for the anemometer |
| Neodymium magnets | 3 | Anemometer rotor | Mounted on the rotating assembly; the firmware is configured for 3 pulses per revolution |
| nRF24L01 module | 2 | Portable unit and central station | Wireless communication between the two ESP32 boards |
| OLED display | 1 | Portable unit | Local visualisation of measured values |
| 18650 lithium-ion cells | 2 | Portable unit | Battery power supply |
| 2S BMS module | 1 | Portable unit | Battery protection for the two 18650 cells connected in series |
| MP1584EN step-down converter | 1 | Portable unit | 3.3 V regulated supply for sensors and low-voltage modules |
| MT3608 DC-DC boost converter | 2 | Portable unit | 5 V supply for the ESP32 and 8.4 V charging stage |
| PETG printed parts | Several | Mechanical structure | Enclosure, anemometer hub, arms and internal supports |
| Plexiglass sheet | 1 | Enclosure | Front panel |
| Silicone gasket material | 1 | Enclosure | Sealing between the enclosure and the plexiglass front panel |
| Bearings | 2 | Anemometer shaft | Bearing size should be selected according to the shaft and hub design |
| M3 screws | As required | Enclosure and internal mounting | Used for securing electronic modules, brackets and smaller printed parts |
| M4 screws | As required | Enclosure and mechanical structure | Used for stronger mechanical connections, depending on the printed design |
| M3 heat-set threaded inserts | As required | PETG printed parts | Recommended for repeated assembly/disassembly of small components |
| M4 heat-set threaded inserts | As required | PETG printed parts | Recommended for stronger mechanical fixing points |
| M3/M4 nuts and washers | As required | Mechanical assembly | Useful where threaded inserts are not used |
| Wires and connectors | As required | Electrical connections | Dupont wires, soldered wires, JST connectors or similar |
| Heat-shrink tubing | As required | Electrical protection | Used for insulation and strain relief |
| Hot-melt adhesive or sealant | As required | Enclosure sealing | Used to seal cable/component ingresses in the prototype |

---

## Optional Components and Future Improvements

| Component | Quantity | Used for | Notes |
|---|---:|---|---|
| SX1278 LoRa module | 2 | Alternative wireless communication | Possible replacement for nRF24L01 where longer communication range is required |
| ESP32 board with integrated LoRa transceiver | 2 | Alternative system architecture | Could simplify long-range communication in future versions |
| Additional 2S BMS modules | Several | Future units | Useful if multiple portable stations are built |
| Additional MP1584EN step-down converters | Several | Future units | Useful for separate regulated voltage rails |
| Additional MT3608 DC-DC boost converters | Several | Future units | Useful for alternative power or charging configurations |
| Larger OLED display | 1–2 | Local visualisation | Optional improvement for better readability |
| Additional 18650 lithium-ion cells | Several | Extended autonomy | Useful for larger battery packs or backup power |
| Solar charging module | 1 | Future power system | Could improve autonomy during field deployment |
| MicroSD card module | 1 | Local data logging | Useful for storing measurements during internet outages |
| Weatherproof cable glands | As required | Enclosure improvement | Recommended for more robust outdoor use |
| Corrosion-resistant screws | As required | Outdoor deployment | Stainless steel screws are recommended for humid or salt-air environments |
| PETG filament | 1 kg | 3D printing | Required if printing the enclosure, hub, arms and supports locally |
```

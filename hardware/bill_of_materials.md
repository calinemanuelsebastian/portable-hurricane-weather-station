# Bill of Materials

This bill of materials covers the current functional prototype of the portable ESP32-based weather station. The system consists of two main units:

1. **portable_station_esp32** – the portable measurement unit;
2. **central_station_esp32** – the receiving station connected to Wi-Fi and ThingSpeak.
---

## 1. Control and Communication Components

### ESP32 development board
- **Quantity:** 2
- **Used for:** portable measurement unit and central receiving station
- **Notes:** one ESP32 reads the sensors and transmits data; the second ESP32 receives the data and publishes them online.

### nRF24L01 wireless module
- **Quantity:** 2
- **Used for:** wireless communication between the portable unit and the central station
- **Notes:** used in the current functional prototype.

### SX1278 LoRa module
- **Quantity:** 2
- **Status:** optional / future improvement
- **Notes:** may be used in future versions where longer communication range is required.

### ESP32 board with integrated LoRa transceiver
- **Quantity:** 2
- **Status:** optional / alternative configuration
- **Notes:** may be used in future versions to simplify the long-range communication architecture.

---

## 2. Sensors and Display

### AHT20 + BMP280 module
- **Quantity:** 1
- **Used for:** temperature, relative humidity and atmospheric pressure measurement

### KY-003 Hall-effect sensor module
- **Quantity:** 1
- **Used for:** anemometer pulse detection

### OLED display
- **Quantity:** 1
- **Used for:** local visualisation on the portable unit
- **Notes:** displays temperature, humidity, pressure, wind speed and system status information.

### Larger OLED display
- **Quantity:** 1–2
- **Status:** optional
- **Notes:** may be used in future versions for improved readability.

---

## 3. Anemometer Assembly

### Neodymium magnets
- **Quantity:** 3
- **Used for:** Hall-effect pulse generation
- **Notes:** mounted on the rotating anemometer assembly. The firmware is currently configured for three pulses per revolution.

### 625 bearings
- **Quantity:** 2
- **Used for:** anemometer shaft support
- **Notes:** open bearings are preferred. In the prototype, 625ZZ bearings were modified by removing the metal shields and cleaning the grease to reduce rotational resistance.

### M5 threaded rod or M5 screw
- **Quantity:** 1
- **Used for:** anemometer shaft assembly
- **Notes:** cut to approximately **50 mm**.

### PETG printed anemometer parts
- **Quantity:** several
- **Used for:** anemometer hub, arms and cup supports
- **Notes:** printed from PETG for better mechanical and environmental resistance than PLA.

---

## 4. Power Supply Components

### 18650 lithium-ion cells
- **Quantity:** 2
- **Used for:** portable unit power supply
- **Notes:** connected in series. Other configurations could be used.

### Additional 18650 lithium-ion cells
- **Quantity:** several
- **Status:** optional
- **Notes:** useful for extended autonomy, backup power or additional prototype units.

### 2S BMS module
- **Quantity:** 1
- **Used for:** battery protection
- **Notes:** protects the two 18650 cells connected in series.

### Additional 2S BMS modules
- **Quantity:** several
- **Status:** optional
- **Notes:** useful if multiple portable units are built.

### MP1584EN step-down converter
- **Quantity:** 1
- **Used for:** regulated 3.3 V supply
- **Notes:** supplies low-voltage modules such as sensors and radio modules.

### Additional MP1584EN step-down converters
- **Quantity:** several
- **Status:** optional
- **Notes:** useful for future versions with separate regulated voltage rails.

### MT3608 DC-DC boost converter
- **Quantity:** 2
- **Used for:** 5 V ESP32 supply and charging stage
- **Notes:** one converter can be used for the ESP32 supply, while another can be configured for the charging stage.

### Additional MT3608 DC-DC boost converters
- **Quantity:** several
- **Status:** optional
- **Notes:** useful for alternative power or charging configurations.

### USB cable / USB power adapter
- **Quantity:** as required
- **Used for:** programming and powering the ESP32 boards
- **Notes:** especially useful for the central receiving station.

### Solar charging module
- **Quantity:** 1
- **Status:** optional / future improvement
- **Notes:** may improve autonomy during outdoor deployment.

---

## 5. Enclosure and Printed Parts

### PETG printed enclosure
- **Quantity:** 1 complete set
- **Used for:** housing the portable measurement unit
- **Notes:** includes the main enclosure body and any internal supports.

### PETG printed internal supports
- **Quantity:** several
- **Used for:** mounting electronic modules inside the enclosure
- **Notes:** quantity depends on the final internal layout.

### PETG filament
- **Quantity:** approximately 1 kg
- **Used for:** printing the enclosure, hub, arms and supports
- **Notes:** required if the parts are printed locally.

### Plexiglass sheet
- **Quantity:** 1
- **Used for:** front panel
- **Notes:** allows visibility of the OLED display and internal layout.

### Silicone gasket material
- **Quantity:** 1
- **Used for:** front panel sealing
- **Notes:** placed between the plexiglass panel and the printed enclosure.

### Weatherproof cable glands
- **Quantity:** as required
- **Status:** optional / future improvement
- **Notes:** recommended for more robust outdoor use.

---

## 6. Mechanical Fasteners and Assembly Hardware

### M5 threaded inserts
- **Quantity:** 11
- **Used for:** main enclosure and stronger mechanical fixing points.

### M3 threaded inserts
- **Quantity:** 9
- **Used for:** smaller printed parts, electronic supports and internal mounting points.

### M6 threaded insert
- **Quantity:** 1
- **Used for:** larger mechanical fixing point in the printed structure.

### M3 × 30 mm Phillips screws
- **Quantity:** 3
- **Used for:** deeper fastening points or parts requiring longer screws.

### M3 × 10 mm Phillips screws
- **Quantity:** 6
- **Used for:** smaller brackets, internal supports or electronic module fixing.

### M5 × 16 mm screws
- **Quantity:** 2
- **Notes:** approximate length; cut to size during assembly.

### M5 × 16 mm butterfly screws
- **Quantity:** 4
- **Notes:** approximate length; cut to size during assembly.
- **Used for:** removable access points, such as the plexiglass front panel.

### Wide washers for butterfly screws
- **Quantity:** 4
- **Used for:** distributing pressure over the plexiglass/front panel area.

### M5 nuts
- **Quantity:** 3
- **Used for:** securing the shaft and mechanical assembly.

### M5 washers
- **Quantity:** 6
- **Used for:** spacing, alignment and load distribution around the rotating assembly.

### Corrosion-resistant screws
- **Quantity:** as required
- **Status:** optional / future improvement
- **Notes:** stainless steel screws are recommended for humid or salt-air environments.

---

## 7. Wiring and Electrical Assembly Materials

### Wires and connectors
- **Quantity:** as required
- **Used for:** electrical connections
- **Notes:** Dupont wires, soldered wires, JST connectors or similar.

### Header pins
- **Quantity:** as required
- **Used for:** module connections
- **Notes:** useful for ESP32, sensor modules and radio modules.

### Prototype PCB / perfboard
- **Quantity:** as required
- **Status:** optional
- **Notes:** useful for making the internal wiring cleaner and more reliable.

### Heat-shrink tubing
- **Quantity:** as required
- **Used for:** insulation and strain relief.

### Capacitor for nRF24L01 module
- **Quantity:** 1–2
- **Recommended value:** 10 µF to 100 µF
- **Used for:** stabilising the nRF24L01 power supply.
- **Notes:** recommended because nRF24L01 modules can be sensitive to voltage drops.

---

## 8. Adhesives, Sealants and Auxiliary Materials

### Threadlocker, e.g. Loctite
- **Quantity:** as required
- **Used for:** preventing loosening of nuts and screws caused by vibration or rotation.

### Double-sided adhesive tape
- **Quantity:** as required
- **Used for:** temporary or semi-permanent mounting of lightweight internal components.

### Hot-melt adhesive or sealant
- **Quantity:** as required
- **Used for:** prototype-level sealing of cable and component ingresses.

### Silicone sealant
- **Quantity:** optional
- **Status:** future improvement
- **Notes:** may provide better sealing than hot-melt adhesive for outdoor use.

---

## 9. Optional Data and Autonomy Improvements

### MicroSD card module
- **Quantity:** 1
- **Status:** optional / future improvement
- **Purpose:** local data logging during internet outages.

### Real-time clock module
- **Quantity:** 1
- **Status:** optional / future improvement
- **Purpose:** timestamping locally stored measurements.

### Battery voltage monitoring circuit
- **Quantity:** 1
- **Status:** optional / future improvement
- **Purpose:** monitoring battery level and improving power management.

---

## Notes

Some screw lengths are approximate because they were cut to size during prototype assembly. Future versions should define standardised screw lengths after the final 3D-printed parts are fixed.

The prototype is intended for functional testing and further development. For long-term outdoor deployment, the enclosure, sealing, connectors and mechanical fasteners should be improved and tested under realistic environmental conditions.

# Bill of Materials

This bill of materials covers the current functional prototype of the portable ESP32-based weather monitoring system. The system consists of two main units:

1. **portable_station_esp32** — the portable measurement unit;
2. **central_station_esp32** — the receiving station connected to Wi-Fi and ThingSpeak.

> Note: the current firmware uses an **AHT20 + BMP280** configuration. If the physical module is changed, the firmware, documentation and ThingSpeak labels should be updated accordingly.

---

## 1. Control and communication components

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

## 2. Sensors and display

### AHT20 sensor module
- **Quantity:** 1
- **Used for:** temperature and relative humidity measurement
- **Notes:** connected to the portable ESP32 through the I2C bus.

### BMP280 module
- **Quantity:** 1
- **Used for:** atmospheric pressure measurement
- **Notes:** connected to the portable ESP32 through the I2C bus. The firmware can use address `0x77` or `0x76`, depending on the physical module.

### KY-003 Hall-effect sensor module
- **Quantity:** 1
- **Used for:** anemometer pulse detection
- **Notes:** detects the passage of the magnets mounted on the rotating anemometer assembly.

### OLED display
- **Quantity:** 1
- **Used for:** local visualisation on the portable unit
- **Notes:** displays temperature, humidity, pressure, wind speed and system status information.

---

## 3. Anemometer assembly

### Neodymium magnets
- **Quantity:** 3
- **Used for:** Hall-effect pulse generation
- **Notes:** mounted on the rotating anemometer assembly. The firmware is currently configured for three pulses per revolution.

### 625 bearings
- **Quantity:** 2
- **Used for:** anemometer shaft support
- **Notes:** open bearings are preferred. In the prototype, 625ZZ bearings may be modified by removing the metal shields and cleaning the grease to reduce rotational resistance.

### M5 threaded rod or M5 screw
- **Quantity:** 1
- **Used for:** anemometer shaft assembly
- **Notes:** cut to the length required by the printed anemometer structure.

### PETG printed anemometer parts
- **Quantity:** several
- **Used for:** anemometer hub, arms and cup supports
- **Notes:** printed from PETG for better mechanical and environmental resistance than PLA.

---

## 4. Power supply components

### 18650 lithium-ion cells
- **Quantity:** 2
- **Used for:** portable unit power supply
- **Notes:** connected in series as a 2S battery pack.

### 2S BMS module
- **Quantity:** 1
- **Used for:** battery protection
- **Notes:** protects the two 18650 cells connected in series.

### Main power switch
- **Quantity:** 1
- **Used for:** main portable-station power control, if installed immediately after the BMS output
- **Notes:** use this switch to disconnect the complete portable unit from the battery supply if a full master on/off function is required.

### 3.3 V rail switch
- **Quantity:** 1
- **Used for:** switching the input of the 3.3 V DC-DC converter
- **Notes:** in the current documented wiring, this switch is placed between the BMS/power distribution positive output and the MP1584EN input. It controls the 3.3 V rail used by the AHT20, BMP280, OLED display, KY-003 Hall sensor and nRF24L01 module.

### MP1584EN step-down converter
- **Quantity:** 1
- **Used for:** regulated 3.3 V supply
- **Notes:** supplies low-voltage modules such as the AHT20, BMP280, OLED display, KY-003 and nRF24L01. Verify the output with a multimeter before connecting the modules.

### 5 V regulator / boost converter
- **Quantity:** 1
- **Used for:** 5 V ESP32 development-board supply, if the ESP32 is powered through VIN/5V
- **Notes:** choose a regulator suitable for the 2S lithium-ion input range. A fully charged 2S pack can reach approximately 8.4 V.

### Additional DC-DC converters
- **Quantity:** as required
- **Status:** optional
- **Notes:** useful for alternative power architectures, separate rails or charging experiments.

### USB cable / USB power adapter
- **Quantity:** as required
- **Used for:** programming and powering the ESP32 boards
- **Notes:** especially useful for the central receiving station.

### Solar charging module
- **Quantity:** 1
- **Status:** optional / future improvement
- **Notes:** may improve autonomy during outdoor deployment.

---

## 5. Enclosure and printed parts

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

## 6. Mechanical fasteners and assembly hardware

### Threaded inserts
- **Quantity:** as required
- **Used for:** printed enclosure and mechanical fixing points
- **Notes:** use the sizes required by the final printed design.

### Screws, nuts and washers
- **Quantity:** as required
- **Used for:** enclosure, front panel, supports and anemometer assembly
- **Notes:** stainless steel or corrosion-resistant fasteners are recommended for humid or salt-air environments.

### Butterfly screws and wide washers
- **Quantity:** as required
- **Used for:** removable access points, such as the plexiglass front panel
- **Notes:** useful where repeated access to the internal electronics is needed.

---

## 7. Wiring and electrical assembly materials

### Wires and connectors
- **Quantity:** as required
- **Used for:** electrical connections
- **Notes:** Dupont wires, soldered wires, JST connectors or similar may be used. Soldered or locking connectors are preferred for field use.

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
- **Used for:** insulation and strain relief
- **Notes:** recommended for soldered joints and exposed conductors.

### Capacitor for nRF24L01 module
- **Quantity:** 1–2
- **Recommended value:** 10 µF to 100 µF
- **Used for:** stabilising the nRF24L01 power supply
- **Notes:** place the capacitor close to the nRF24L01 VCC and GND pins.

---

## 8. Adhesives, sealants and auxiliary materials

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

## 9. Optional data and autonomy improvements

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

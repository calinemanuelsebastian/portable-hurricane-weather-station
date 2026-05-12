# Bill of Materials

---

## 1. Control and Communication

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

---

## 2. Sensors and Local Display

### AHT20 + BMP280 module / DHT22 + BMP280 module
- **Quantity:** 1
- **Used for:** temperature, relative humidity and atmospheric pressure measurement
- **Notes:** use the sensor type that matches the firmware.

### KY-003 Hall-effect sensor module
- **Quantity:** 1
- **Used for:** anemometer pulse detection
- **Notes:** detects the passage of the magnets mounted on the rotating anemometer assembly.

### OLED display
- **Quantity:** 1
- **Used for:** local visualisation on the portable unit
- **Notes:** displays temperature, humidity, pressure, wind speed and status information.

---

## 3. Anemometer Assembly

### Neodymium magnets
- **Quantity:** 3
- **Used for:** pulse generation
- **Notes:** mounted on the rotating anemometer assembly. The firmware is currently configured for three pulses per revolution.

### Bearings
- **Quantity:** 2
- **Used for:** anemometer shaft support
- **Notes:** bearing size should match the shaft and hub design.

### PETG printed parts
- **Quantity:** several
- **Used for:** anemometer arms, hub and enclosure parts
- **Notes:** printed from PETG for improved mechanical and environmental resistance compared with PLA.

---

## 4. Power Supply

### 18650 lithium-ion cells
- **Quantity:** 2
- **Used for:** portable unit power supply
- **Notes:** connected in series.

### 2S BMS module
- **Quantity:** 1
- **Used for:** battery protection
- **Notes:** protects the two 18650 cells connected in series.

### MP1584EN step-down converter
- **Quantity:** 1
- **Used for:** regulated 3.3 V supply
- **Notes:** supplies low-voltage modules such as sensors and radio modules.

### MT3608 DC-DC boost converter
- **Quantity:** 2
- **Used for:** 5 V ESP32 supply and charging stage
- **Notes:** one converter can be used for the ESP32 supply, while another can be configured for the charging stage.

---

## 5. Enclosure and Mechanical Parts

### Plexiglass sheet
- **Quantity:** 1
- **Used for:** front panel
- **Notes:** allows visibility of the OLED display and internal layout.

### Silicone gasket material
- **Quantity:** 1
- **Used for:** front panel sealing
- **Notes:** placed between the plexiglass panel and the enclosure.

### M3 screws
- **Quantity:** as required
- **Used for:** electronic modules, brackets and smaller printed parts

### M4 screws
- **Quantity:** as required
- **Used for:** stronger mechanical connections and enclosure fixing points

### M3 heat-set threaded inserts
- **Quantity:** as required
- **Used for:** repeated assembly and disassembly of smaller PETG parts

### M4 heat-set threaded inserts
- **Quantity:** as required
- **Used for:** stronger mechanical fixing points in printed parts

### M3/M4 nuts and washers
- **Quantity:** as required
- **Used for:** mechanical fastening where threaded inserts are not used

### Wires and connectors
- **Quantity:** as required
- **Used for:** electrical connections
- **Notes:** Dupont wires, soldered wires, JST connectors or similar.

### Heat-shrink tubing
- **Quantity:** as required
- **Used for:** insulation and strain relief

### Hot-melt adhesive or sealant
- **Quantity:** as required
- **Used for:** prototype sealing
- **Notes:** used to seal cable and component ingresses.

---

## 6. Optional Future Additions

### Larger OLED display
- **Quantity:** 1–2
- **Purpose:** improved readability

### Additional 18650 lithium-ion cells
- **Quantity:** several
- **Purpose:** extended autonomy or additional prototype units

### Solar charging module
- **Quantity:** 1
- **Purpose:** improved autonomy during outdoor deployment

### MicroSD card module
- **Quantity:** 1
- **Purpose:** local data logging during internet outages

### Weatherproof cable glands
- **Quantity:** as required
- **Purpose:** improved enclosure sealing

### Corrosion-resistant screws
- **Quantity:** as required
- **Purpose:** improved durability in humid or salt-air environments

### PETG filament
- **Quantity:** approximately 1 kg
- **Purpose:** printing the enclosure, hub, arms and supports

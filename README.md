# Portable Weather Station for Hurricane-Prone Island Environments

This repository contains the firmware, hardware documentation and development notes for a portable, low-cost meteorological monitoring system designed for local weather observation in hurricane-prone island environments.

The prototype measures wind speed, temperature, relative humidity and atmospheric pressure using an ESP32-based measurement unit, a custom Hall-effect cup anemometer, a DHT22 sensor and a BMP280 pressure sensor. Data are transmitted wirelessly to a central ESP32 station using nRF24L01 modules and published online through the ThingSpeak platform for remote visualisation.

## Links

Printables: https://www.printables.com/model/1719175-portable-esp32-weather-station-for-hurricane-prone 
ThingSpeak: https://thingspeak.mathworks.com/channels/3365147

## Project Status

This project is currently a functional prototype. The system has been tested for sensor acquisition, wireless transmission and online data publication. The wind-speed conversion algorithm is not callibrated against a reference anemometer.

## Main Features

- ESP32-based portable measurement unit
- Custom cup anemometer with Hall-effect pulse detection
- AHT22 temperature and relative humidity measurement
- BMP280 atmospheric pressure measurement
- nRF24L01 wireless communication between the portable unit and central station
- ESP32-based central station with Wi-Fi connection
- Online data publication through ThingSpeak
- OLED display for local data visualisation
- 3D-printed PETG enclosure and anemometer components

## System Architecture

The system is organised as follows:

```text
AHT22 + BMP280 + Hall-effect anemometer
                ↓
        ESP32 portable unit
                ↓
        nRF24L01 wireless link
                ↓
        ESP32 central station
                ↓
              Wi-Fi
                ↓
           ThingSpeak
                ↓
        Remote visualisation

# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

MeteoCharts-ESP32 is an ESP32-based weather station that collects sensor data (temperature, humidity, pressure) from a BME280 sensor and sends it to a MeteoCharts server via HTTPS. The device supports an OLED display (SSD1306), battery monitoring, and uses deep sleep for power efficiency.

## Project Structure

```
src/
├── ESP32_Arduino/           # Arduino framework implementation (main/active)
│   ├── MeteoCharts/         # Main sketch and headers
│   │   ├── MeteoCharts.ino  # Main application code
│   │   └── images.h         # Display icons (generated via image2cpp)
│   └── Configuration/
│       └── conf.h.example   # Configuration template
└── ESP32_Espidf/            # ESP-IDF framework (stub/WIP)
    └── MeteoCharts/
        └── platformio.ini   # PlatformIO config for ESP32-C3
```

## Configuration

Copy `src/ESP32_Arduino/Configuration/conf.h.example` to `src/ESP32_Arduino/MeteoCharts/conf.h` and configure:
- WiFi credentials (SSID, PASSWORD)
- Hardware flags (HAS_LED, HAS_BATTERY, HAS_DISPLAY)
- I2C pins and addresses
- Server endpoint

## Build Commands

### Arduino IDE
Open `src/ESP32_Arduino/MeteoCharts/MeteoCharts.ino` in Arduino IDE with ESP32 board support installed.

### PlatformIO (ESP-IDF)
```bash
cd src/ESP32_Espidf/MeteoCharts
pio run                    # Build
pio run -t upload          # Upload to device
pio device monitor         # Serial monitor
```

## Hardware Dependencies

- **Board**: ESP32-C3 (airm2m_core_esp32c3) or compatible ESP32
- **Sensor**: BME280 (I2C address 0x76)
- **Display**: SSD1306 OLED 128x32 or 128x64 (I2C address 0x3C)

## Arduino Libraries Required

- Adafruit_BME280
- Adafruit_SSD1306
- Arduino_JSON
- WiFi (ESP32 core)
- HTTPClient (ESP32 core)

## API Communication

Device sends POST to `/api/devices/sensors` with JSON payload:
```json
{"mac":"...", "temperature":"...", "humidity":"...", "pressure":"...", "battery":"...", "token":"..."}
```

Response codes: 0=OK, 9=Invalid payload, 10=Missing MAC, 11=MAC not found, 12=Wrong token, 50=Token registration

// Device config: ESP32-C3 template
#ifndef CONF_DEVICE_H
#define CONF_DEVICE_H

// Hardware features
const bool HAS_LED = false;
const bool HAS_BATTERY = false;
const bool HAS_DISPLAY = false;

// Pin configuration
#define LED_PIN 3
#define SDA 8
#define SCL 9
#define BATTERY_PIN 0

// BME280 sensor
#define BME280_BUS 0x76

// Display SSD1306
#define SSD1306_BUS 0x3C
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32

// Sleep duration (microseconds)
#define SLEEP_DURATION_US (15 * 60 * 1000000ULL)

#endif

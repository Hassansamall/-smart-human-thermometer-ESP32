# Smart Human Thermometer (ESP32)

A contactless smart thermometer project based on the ESP32 microcontroller. It features real-time temperature monitoring using an infrared sensor, a local OLED display for immediate feedback, and a WiFi Access Point with a modern web dashboard for remote viewing.

## Features
- **Non-Contact Measurement**: Uses the MLX90614 IR sensor for hygienic readings.
- **Visual Feedback**: 0.96" OLED display shows temperature and color-coded health status (Low, Normal, Elevated, Fever).
- **Web Dashboard**: Hosts a WiFi Access Point (`Smart_Thermometer_AP`) serving a responsive web page for monitoring on smartphones or laptops.
- **Real-time Updates**: The web interface updates automatically via AJAX.

## Hardware Requirements

| Component | Interface | Description |
|-----------|-----------|-------------|
| **ESP32 Dev Module** | - | Main Microcontroller |
| **MLX90614** | I2C | Non-contact Infrared Temperature Sensor |
| **SSD1331 OLED** | SPI | 0.96" Color OLED Display (96x64) |

## Pinout Configuration

### MLX90614 Sensor (I2C)
| Sensor Pin | ESP32 Pin | Note |
|------------|-----------|------|
| VIN        | 3.3V      | -    |
| GND        | GND       | -    |
| SCL        | D22       | Standard I2C SCL |
| SDA        | D21       | Standard I2C SDA |

### SSD1331 OLED Display (SPI)
| Display Pin | ESP32 Pin | Function |
|-------------|-----------|----------|
| CS          | D8        | Chip Select |
| DC          | D10       | Data/Command |
| RES         | D9        | Reset |
| SDA (MOSI)  | D23       | Hardware MOSI (Standard) |
| SCL (SCK)   | D18       | Hardware SCK (Standard) |
| VCC         | 3.3V      | - |
| GND         | GND       | - |

*> **Note:** Pin mappings can vary between specific ESP32 boards. Check your board's pinout diagram.*

## Installation & Usage

1. **Wiring**: Connect all components according to the pinout table above.
2. **Setup**:
   - Install the required libraries in Arduino IDE:
     - `Adafruit MLX90614 Library`
     - `Adafruit SSD1331 OLED Driver Library`
     - `Adafruit GFX Library`
     - `ESPAsyncWebServer` & `AsyncTCP`
3. **Upload**: Open `code.ino`, select your board and COM port, and upload the sketch.
4. **Operation**:
   - The OLED will display the current temperature reading.
   - Connect your phone/laptop to the WiFi network: **`Smart_Thermometer_AP`**.
   - Navigate to `http://192.168.4.1` (or the IP shown on the OLED) to view the dashboard.

## Health Status Indication

- **Low (< 25°C)**: Blue - Too cold (or background reading)
- **Normal (25°C - 28°C)**: Green - Healthy
- **Elevated (28°C - 31°C)**: Orange - Caution
- **Fever (> 31°C)**: Red - High temperature warning

*> **Disclaimer:** This project is for educational purposes only. It is not a certified medical device.*

/**
 * Smart Contactless Thermometer
 * 
 * Target Board: ESP32
 * Sensors: MLX90614 (I2C) - Temperature
 * Display: SSD1331 (SPI) - 0.96" Color OLED
 * Features:
 *  - Contactless temperature reading
 *  - Color-coded health status indicator (OLED)
 *  - WiFi Access Point with Web Dashboard
 *  - Real-time temperature updates via AJAX
 * 
 * Author: Repo Owner
 * Maintained by: Antigravity
 */

// --- Include necessary libraries ---
#include <Wire.h>               // I2C communication
#include <Adafruit_MLX90614.h>  // MLX90614 temperature sensor
#include <Adafruit_GFX.h>       // Core graphics library
#include <Adafruit_SSD1331.h>   // SSD1331 OLED driver
#include <WiFi.h>               // ESP32 WiFi capabilities
#include <ESPAsyncWebServer.h>  // Asynchronous Web Server
#include <AsyncTCP.h>           // Asynchronous TCP library

// --- Include Custom Web Page ---
#include "web_page.h"

// --- Configuration ---

// WiFi Access Point Credentials
const char* AP_SSID = "Smart_Thermometer_AP";
const char* AP_PASSWORD = NULL; // No password for open AP

/**
 * Temperature Thresholds (Celsius)
 * NOTE: These values are calibrated for demonstration/testing purposes.
 * Adjust as necessary for Medical Grade accuracy.
 */ 
const float THRESHOLD_LOW = 25.0;      // Below this is "LOW"
const float THRESHOLD_NORMAL = 28.0;   // Up to this is "NORMAL"
const float THRESHOLD_ELEVATED = 31.0; // Up to this is "ELEVATED"
// Anything above THRESHOLD_ELEVATED is "FEVER"

// --- Pin Definitions ---

// OLED Display (SPI)
#define OLED_CS    8
#define OLED_DC    10
#define OLED_RES   9
// Note: SCL(D13) and SDA(D11) are default HSPI pins on some boards, 
// but pin definitions can vary by ESP32 board variant.

// I2C Pins (MLX90614)
#define I2C_SDA    21
#define I2C_SCL    22

// --- Color Definitions (RGB565) ---
#define COLOR_BLACK  0x0000
#define COLOR_GREEN  0x07E0
#define COLOR_RED    0xF800
#define COLOR_WHITE  0xFFFF
#define COLOR_BLUE   0x001F
#define COLOR_ORANGE 0xFD20

// --- Global Objects ---
Adafruit_MLX90614 mlx = Adafruit_MLX90614();
Adafruit_SSD1331 display = Adafruit_SSD1331(OLED_CS, OLED_DC, OLED_RES);
AsyncWebServer server(80);

// --- Global Variables ---
float currentTemperature = 0.0;
String healthStatus = "NORMAL";
unsigned long lastReadTime = 0;
const unsigned long READ_INTERVAL_MS = 3000; // Update every 3 seconds

// --- Function Prototypes ---
void setupWiFi();
void setupDisplay();
void setupSensor();
void setupWebServer();
void updateDisplay(float temp, String status, uint16_t color);
String getHealthStatus(float temp, uint16_t &color);

void setup() {
  Serial.begin(115200);
  delay(1000); // Give serial monitor time to catch up
  Serial.println("\n--- Smart Thermometer Booting Up ---");

  setupDisplay();
  setupSensor();
  setupWiFi();
  setupWebServer();
  
  Serial.println("System Initialized and Ready.");
}

void loop() {
  // Non-blocking timer
  if (millis() - lastReadTime >= READ_INTERVAL_MS) {
    lastReadTime = millis();
    
    // Read object temperature
    float tempC = mlx.readObjectTempC();
    
    Serial.print("Raw Sensor Reading (Celsius): ");
    Serial.println(tempC);

    if (isnan(tempC)) {
      Serial.println("Error: Failed to read from MLX sensor!");
      // Show error on display
      display.fillScreen(COLOR_BLACK);
      display.setTextColor(COLOR_RED);
      display.setCursor(5, 25);
      display.setTextSize(1);
      display.println("SENSOR ERROR");
    } else {
      // Simple rolling average filter
      if (currentTemperature == 0.0) {
        currentTemperature = tempC;
      } else {
        currentTemperature = (tempC * 0.5) + (currentTemperature * 0.5);
      }
      
      uint16_t statusColor;
      healthStatus = getHealthStatus(currentTemperature, statusColor);
      
      updateDisplay(currentTemperature, healthStatus, statusColor);
    }
  }
}

// --- Helper Functions ---

void setupWiFi() {
  Serial.print("Starting Access Point: ");
  Serial.println(AP_SSID);
  
  WiFi.softAP(AP_SSID, AP_PASSWORD);
  
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP Address: ");
  Serial.println(myIP);

  // Show IP on display temporarily
  display.fillScreen(COLOR_BLACK);
  display.setCursor(5, 10);
  display.setTextColor(COLOR_GREEN);
  display.println("AP Ready!");
  display.setCursor(5, 30);
  display.setTextSize(1);
  display.setTextColor(COLOR_WHITE);
  display.println(myIP);
  delay(2000); // Small delay to let user see IP
}

void setupDisplay() {
  display.begin();
  display.fillScreen(COLOR_BLACK);
  display.setCursor(5, 10);
  display.setTextColor(COLOR_WHITE);
  display.setTextSize(1);
  display.println("Initializing...");
}

void setupSensor() {
  // Initialize I2C with defined pins
  Wire.begin(I2C_SDA, I2C_SCL);
  
  if (!mlx.begin()) {
    Serial.println("FATAL: Could not connect to MLX90614 sensor! Check wiring.");
    display.fillScreen(COLOR_BLACK);
    display.setCursor(5, 30);
    display.setTextColor(COLOR_RED);
    display.println("Sensor Error!");
    while (1); // Halt execution
  }
}

void setupWebServer() {
  // Serve the main HTML page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });
  
  // JSON API endpoint for temperature data
  server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request){
    String json = "{\"temperature\":\"" + String(currentTemperature, 1) + "\",";
    json += "\"status\":\"" + healthStatus + "\"}";
    request->send(200, "application/json", json);
  });
  
  server.begin();
  Serial.println("Web server started.");
}

String getHealthStatus(float temp, uint16_t &color) {
  if (temp < THRESHOLD_LOW) {
    color = COLOR_BLUE;
    return "LOW";
  } else if (temp <= THRESHOLD_NORMAL) {
    color = COLOR_GREEN;
    return "NORMAL";
  } else if (temp <= THRESHOLD_ELEVATED) {
    color = COLOR_ORANGE;
    return "ELEVATED";
  } else {
    color = COLOR_RED;
    return "FEVER";
  }
}

void updateDisplay(float temp, String status, uint16_t color) {
  display.fillScreen(COLOR_BLACK);
  
  // Draw Temperature
  display.setTextColor(color);
  display.setCursor(10, 15);
  display.setTextSize(2);
  display.print(temp, 1);
  
  // Draw Degree Symbol and 'C'
  display.print(" ");
  display.drawCircle(display.getCursorX() + 3, 18, 3, color);
  display.setCursor(display.getCursorX() + 10, 15);
  display.print("C");
  
  // Draw Status Text
  display.setCursor(18, 45);
  display.setTextSize(1);
  display.println(status);
}

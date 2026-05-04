// --- Final Code for Contactless Smart Thermometer V3 (Single Reading) ---
// This code takes ONE accurate reading after a person stands still for 2 seconds,
// uploads it to Supabase once, and waits for them to move away before resetting.

// --- Include necessary libraries ---
#include <Wire.h>
#include <Adafruit_MLX90614.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1331.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

// --- Configuration ---
// WiFi Credentials (MUST BE UPDATED)
const char* WIFI_SSID = "TP-Link_D300";
const char* WIFI_PASSWORD = "2581991abm";

// Supabase Credentials (MUST BE UPDATED)
const char* SUPABASE_URL = "https://hoqpukivoaialazemrwg.supabase.co/rest/v1/temperature_readings";
const char* SUPABASE_KEY = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6ImhvcXB1a2l2b2FpYWxhemVtcndnIiwicm9sZSI6ImFub24iLCJpYXQiOjE3NzQ0NTkwMzAsImV4cCI6MjA5MDAzNTAzMH0.jgNohwkytZTXY9ZP3dcYh1SYyvDgaEdH4d6vhGkn-_Y";

// --- Temperature Thresholds ---
const float LOW_TEMP_THRESHOLD = 35.0; 
const float NORMAL_TEMP_UPPER_THRESHOLD = 37.5; 
const float ELEVATED_TEMP_UPPER_THRESHOLD = 38.3; 

// --- Pin Definitions for OLED Display (SPI) ---
#define OLED_CS    8
#define OLED_DC    10
#define OLED_RES   9

// --- Color Definitions for OLED ---
#define COLOR_BLACK  0x0000
#define COLOR_GREEN  0x07E0
#define COLOR_RED    0xF800
#define COLOR_WHITE  0xFFFF
#define COLOR_BLUE   0x001F
#define COLOR_ORANGE 0xFD20
#define COLOR_YELLOW 0xFFE0

// --- Object Initialization ---
Adafruit_MLX90614 mlx = Adafruit_MLX90614();
Adafruit_SSD1331 display = Adafruit_SSD1331(OLED_CS, OLED_DC, OLED_RES);

// --- State Machine Variables ---
unsigned long lastLoopTime = 0;
const unsigned long loopInterval = 200; // Check sensor every 200ms

bool isReading = false;
bool readingLocked = false;
unsigned long readingStartTime = 0;
const unsigned long STABILIZATION_TIME = 8000; // Require 8 seconds of steady reading

float smoothedTemperature = 0.0;
String healthStatus = "NORMAL";

void uploadToSupabase(float temp, String status);

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n--- Smart Thermometer Booting Up ---");

  display.begin();
  display.fillScreen(COLOR_BLACK);
  display.setCursor(5, 10);
  display.setTextColor(COLOR_WHITE);
  display.setTextSize(1);
  display.println("Initializing...");

  // Explicitly initialize the I2C bus on standard ESP32 pins (SDA=21, SCL=22)
  Wire.begin(21, 22);
  
  if (!mlx.begin()) {
    Serial.println("FATAL: Error connecting to MLX sensor. Check wiring.");
    display.fillScreen(COLOR_BLACK);
    display.setCursor(5, 30);
    display.setTextColor(COLOR_RED);
    display.println("Sensor Error!");
    while (1);
  }
  
  delay(500);
  
  // Connect to WiFi
  Serial.print("Connecting to WiFi: ");
  Serial.println(WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  display.fillScreen(COLOR_BLACK);
  display.setCursor(5, 10);
  display.setTextColor(COLOR_WHITE);
  display.println("Connecting WiFi...");

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  display.fillScreen(COLOR_BLACK);
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected.");
    display.setCursor(5, 10);
    display.setTextColor(COLOR_GREEN);
    display.println("WiFi Connected!");
  } else {
    Serial.println("\nWiFi connection failed!");
    display.setCursor(5, 10);
    display.setTextColor(COLOR_RED);
    display.println("WiFi Failed");
  }

  delay(2000);
  
  // Show Initial Ready State
  display.fillScreen(COLOR_BLACK);
  display.setCursor(15, 30);
  display.setTextColor(COLOR_WHITE);
  display.setTextSize(2);
  display.println("READY");
  
  Serial.println("Setup complete.");
}

void loop() {
  if (millis() - lastLoopTime >= loopInterval) {
    lastLoopTime = millis();
    
    float Ta = mlx.readAmbientTempC();
    float To = mlx.readObjectTempC();
    
    // Check if a person is in range (Object temp significantly higher than ambient, e.g. > 30C)
    // Adjust 30.0 if the sensor is too sensitive or not sensitive enough
    bool personInRange = (To > 30.0 && To < 40.0);

    if (personInRange) {
       // ADAPTIVE CALIBRATION ALGORITHM
       float adaptiveOffset = ((25.0 - Ta) * 0.3) + 3.5; 
       if (adaptiveOffset < 1.0) adaptiveOffset = 1.0;
       if (adaptiveOffset > 7.0) adaptiveOffset = 7.0;
       
       float tempC = To + adaptiveOffset;

       // 1. New Person Detected
       if (!isReading && !readingLocked) {
           isReading = true;
           readingStartTime = millis();
           smoothedTemperature = tempC;
           
           display.fillScreen(COLOR_BLACK);
           display.setCursor(5, 30);
           display.setTextColor(COLOR_YELLOW);
           display.setTextSize(2);
           display.println("READING...");
           Serial.println("Target acquired. Stabilizing...");
       } 
       // 2. Stabilizing reading over time
       else if (isReading && !readingLocked) {
           // Roll average to smooth out fluctuations
           smoothedTemperature = (tempC * 0.4) + (smoothedTemperature * 0.6);
           
           // If 2 seconds have passed, LOCK the reading!
           if (millis() - readingStartTime >= STABILIZATION_TIME) {
               readingLocked = true;
               isReading = false;
               
               // Determine Health Status
               uint16_t textColor;
               if (smoothedTemperature < LOW_TEMP_THRESHOLD) {
                 healthStatus = "LOW"; textColor = COLOR_BLUE;
               } else if (smoothedTemperature <= NORMAL_TEMP_UPPER_THRESHOLD) {
                 healthStatus = "NORMAL"; textColor = COLOR_GREEN;
               } else if (smoothedTemperature <= ELEVATED_TEMP_UPPER_THRESHOLD) {
                 healthStatus = "ELEVATED"; textColor = COLOR_ORANGE;
               } else {
                 healthStatus = "FEVER"; textColor = COLOR_RED;
               }
               
               // Show Locked Result on OLED
               display.fillScreen(COLOR_BLACK);
               display.setTextColor(textColor);
               display.setCursor(10, 15);
               display.setTextSize(2);
               display.print(smoothedTemperature, 1);
               display.print(" ");
               display.drawCircle(display.getCursorX() + 3, 18, 3, textColor);
               display.setCursor(display.getCursorX() + 10, 15);
               display.print("C");
               
               display.setCursor(18, 45);
               display.setTextSize(1);
               display.println(healthStatus);
               
               Serial.print("Final Locked Reading: ");
               Serial.print(smoothedTemperature);
               Serial.print(" - ");
               Serial.println(healthStatus);
               
               // FIRE DATABASE UPLOAD ONCE!
               uploadToSupabase(smoothedTemperature, healthStatus);
           }
       }
    } else {
       // No person in range (To < 30.0)
       
       // If we already finished a reading, wait for them to walk away
       if (readingLocked && To < 28.0) {
           readingLocked = false;
           smoothedTemperature = 0.0;
           
           Serial.println("Target left. Resetting to READY state.");
           display.fillScreen(COLOR_BLACK);
           display.setCursor(15, 30);
           display.setTextColor(COLOR_WHITE);
           display.setTextSize(2);
           display.println("READY");
       } 
       // If they walked away BEFORE it finished stabilizing
       else if (isReading) {
           isReading = false;
           smoothedTemperature = 0.0;
           
           Serial.println("Target lost before lock. Resetting...");
           display.fillScreen(COLOR_BLACK);
           display.setCursor(15, 30);
           display.setTextColor(COLOR_WHITE);
           display.setTextSize(2);
           display.println("READY");
       }
    }
  }
}

// --- Helper function to keep loop() clean ---
void uploadToSupabase(float temp, String status) {
  if (WiFi.status() == WL_CONNECTED && String(SUPABASE_KEY).indexOf("YOUR_") == -1) {
    WiFiClientSecure *client = new WiFiClientSecure;
    if(client) {
      client->setInsecure(); // No certificate verification
      HTTPClient https;
      
      if (https.begin(*client, SUPABASE_URL)) {
        https.addHeader("Content-Type", "application/json");
        https.addHeader("apikey", SUPABASE_KEY);
        https.addHeader("Authorization", String("Bearer ") + SUPABASE_KEY);
        https.addHeader("Prefer", "return=minimal");

        String jsonPayload = "{\"temperature\":" + String(temp, 1) + 
                             ", \"status\":\"" + status + 
                             "\", \"person_name\":\"Unknown\"}";

        int httpCode = https.POST(jsonPayload);
        if (httpCode > 0) {
          Serial.printf("Supabase POST success. Code: %d\n", httpCode);
        } else {
          Serial.printf("Supabase POST failed, error: %s\n", https.errorToString(httpCode).c_str());
        }
        https.end();
      } else {
        Serial.println("Unable to connect to Supabase (HTTPS setup failed)");
      }
      delete client;
    }
  }
}

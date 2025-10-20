// --- Final Code for Contactless Smart Thermometer ---
// This code includes a WiFi Access Point and a modern web dashboard for real-time display.

// --- Include necessary libraries ---
#include <Wire.h>
#include <Adafruit_MLX90614.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1331.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>

// --- Configuration ---
// Access Point (AP) Credentials
const char* ap_ssid = "Smart_Thermometer_AP";
const char* ap_password = NULL;

// --- !! TEMPORARY TEST VALUES !! ---
// These values are lowered to make it easy to test the color-changing logic.
// A normal room temperature should now read as "NORMAL" (Green).
// Holding your finger near the sensor should change it to "ELEVATED" (Orange).
const float LOW_TEMP_THRESHOLD = 25.0; // Was 36.5
const float NORMAL_TEMP_UPPER_THRESHOLD = 28.0; // Was 37.5
const float ELEVATED_TEMP_UPPER_THRESHOLD = 31.0; // Was 38.3

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

// --- Object Initialization ---
Adafruit_MLX90614 mlx = Adafruit_MLX90614();
Adafruit_SSD1331 display = Adafruit_SSD1331(OLED_CS, OLED_DC, OLED_RES);
AsyncWebServer server(80);

// --- Global Variables ---
float currentTemperature = 0.0;
String healthStatus = "NORMAL";
unsigned long lastReadTime = 0;
const unsigned long readInterval = 3000; // 3 seconds between readings

// --- Web Dashboard HTML, CSS, and JavaScript ---
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>Smart Thermometer</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    @import url('https://fonts.googleapis.com/css2?family=Inter:wght@400;700;900&display=swap');
    :root {
      --bg-color: #1a1a2e; --card-color: #16213e; --secondary-text: #a7a9be;
      --color-low: #00bfff; --color-normal: #2ecc71; --color-elevated: #f39c12; --color-fever: #e74c3c;
    }
    body {
      font-family: 'Inter', sans-serif; display: flex; flex-direction: column; align-items: center; justify-content: center; min-height: 100vh;
      margin: 0; padding: 20px; color: var(--secondary-text); box-sizing: border-box;
      background: linear-gradient(-45deg, #1a1a2e, #16213e, #0f3460, #1a1a2e);
      background-size: 400% 400%;
      animation: gradientBG 15s ease infinite;
    }
    @keyframes gradientBG {
        0% { background-position: 0% 50%; }
        50% { background-position: 100% 50%; }
        100% { background-position: 0% 50%; }
    }
    .container {
      width: 100%; max-width: 500px; text-align: center;
      background: rgba(0,0,0,0.2);
      padding: 30px;
      border-radius: 20px;
      backdrop-filter: blur(10px);
      border: 1px solid rgba(255,255,255,0.1);
    }
    .temp-display {
      width: 250px; height: 250px; border-radius: 50%; margin: 0 auto 30px;
      display: flex; flex-direction: column; justify-content: center; align-items: center;
      background: radial-gradient(circle, var(--card-color) 60%, transparent 80%);
      border: 5px solid; transition: border-color 0.5s ease, box-shadow 0.5s ease;
    }
    .temp-value { font-size: 5rem; font-weight: 900; line-height: 1; transition: color 0.5s ease; }
    .temp-status { font-size: 1.5rem; font-weight: 700; margin-top: 10px; transition: color 0.5s ease; }
    .health-key { display: flex; flex-wrap: wrap; justify-content: center; gap: 15px; margin-top: 30px; padding-top: 20px; border-top: 1px solid rgba(255,255,255,0.1); }
    .key-item { display: flex; align-items: center; gap: 8px; font-size: 0.9rem;}
    .key-color { width: 15px; height: 15px; border-radius: 50%; }
  </style>
</head>
<body>
<div class="container">
    <div class="temp-display" id="temp-display">
        <div class="temp-value" id="temp-value">--.-&deg;C</div>
        <div class="temp-status" id="temp-status">LOADING...</div>
    </div>
    <div class="health-key">
        <div class="key-item"><div class="key-color" style="background-color: var(--color-low);"></div> Low</div>
        <div class="key-item"><div class="key-color" style="background-color: var(--color-normal);"></div> Normal</div>
        <div class="key-item"><div class="key-color" style="background-color: var(--color-elevated);"></div> Elevated</div>
        <div class="key-item"><div class="key-color" style="background-color: var(--color-fever);"></div> Fever</div>
    </div>
</div>

<script>
function updateTemperatureUI(temp, status) {
    const tempValueEl = document.getElementById('temp-value');
    const tempStatusEl = document.getElementById('temp-status');
    const displayCircle = document.getElementById('temp-display');
    
    tempValueEl.innerHTML = `${parseFloat(temp).toFixed(1)}&deg;C`;
    tempStatusEl.textContent = status;

    let colorVar;
    if (status === 'LOW') colorVar = 'var(--color-low)';
    else if (status === 'NORMAL') colorVar = 'var(--color-normal)';
    else if (status === 'ELEVATED') colorVar = 'var(--color-elevated)';
    else if (status === 'FEVER') colorVar = 'var(--color-fever)';
    
    displayCircle.style.borderColor = colorVar;
    displayCircle.style.boxShadow = `0 0 25px ${colorVar}`;
    tempStatusEl.style.color = colorVar;
    tempValueEl.style.color = colorVar;
}

function fetchData() {
    fetch('/data')
        .then(response => response.json())
        .then(data => {
            const temp = parseFloat(data.temperature);
            updateTemperatureUI(temp, data.status);
        }).catch(error => console.error('Error fetching data:', error));
}

window.onload = () => {
    fetchData(); // Fetch initial data
    setInterval(fetchData, 3000); // Refresh every 3 seconds
};
</script>
</body>
</html>
)rawliteral";


void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n--- Smart Thermometer Booting Up ---");

  // Initialize OLED Display
  display.begin();
  display.fillScreen(COLOR_BLACK);
  display.setCursor(5, 10);
  display.setTextColor(COLOR_WHITE);
  display.setTextSize(1);
  display.println("Initializing...");

  // Explicitly initialize the I2C bus on standard ESP32 pins (SDA=21, SCL=22)
  Wire.begin(21, 22);
  
  // Initialize MLX90614 Sensor
  if (!mlx.begin()) {
    Serial.println("FATAL: Error connecting to MLX sensor. Check wiring.");
    display.fillScreen(COLOR_BLACK);
    display.setCursor(5, 30);
    display.setTextColor(COLOR_RED);
    display.println("Sensor Error!");
    while (1);
  }
  
  delay(500);
  
  // Start WiFi Access Point
  Serial.print("Starting Access Point: ");
  Serial.println(ap_ssid);
  WiFi.softAP(ap_ssid, ap_password);
  
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);

  display.fillScreen(COLOR_BLACK);
  display.setCursor(5, 10);
  display.setTextColor(COLOR_GREEN);
  display.println("AP Ready!");
  display.setCursor(5, 30);
  display.setTextSize(1);
  display.setTextColor(COLOR_WHITE);
  display.println(myIP);

  // --- Web Server Routes ---
  // Main page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });
  
  // Data endpoint
  server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request){
    String json = "{\"temperature\":\"" + String(currentTemperature, 1) + "\",";
    json += "\"status\":\"" + healthStatus + "\"}";
    request->send(200, "application/json", json);
  });
  
  server.begin();
  Serial.println("Web server started. Setup complete.");
}


void loop() {
  if (millis() - lastReadTime >= readInterval) {
    lastReadTime = millis();
    float tempC = mlx.readObjectTempC();
    
    // Print the raw sensor reading for debugging
    Serial.print("Raw Sensor Reading (Celsius): ");
    Serial.println(tempC);

    if (!isnan(tempC)) {
      // Use a simple rolling average to smooth readings
      currentTemperature = (currentTemperature == 0.0) ? tempC : (tempC * 0.5) + (currentTemperature * 0.5);
      
      uint16_t textColor;
      if (currentTemperature < LOW_TEMP_THRESHOLD) {
        healthStatus = "LOW"; textColor = COLOR_BLUE;
      } else if (currentTemperature <= NORMAL_TEMP_UPPER_THRESHOLD) {
        healthStatus = "NORMAL"; textColor = COLOR_GREEN;
      } else if (currentTemperature <= ELEVATED_TEMP_UPPER_THRESHOLD) {
        healthStatus = "ELEVATED"; textColor = COLOR_ORANGE;
      } else {
        healthStatus = "FEVER"; textColor = COLOR_RED;
      }

      // Update OLED Display
      display.fillScreen(COLOR_BLACK);
      display.setTextColor(textColor);
      display.setCursor(10, 15);
      display.setTextSize(2);
      display.print(currentTemperature, 1);
      display.print(" ");
      display.drawCircle(display.getCursorX() + 3, 18, 3, textColor);
      display.setCursor(display.getCursorX() + 10, 15);
      display.print("C");
      display.setCursor(18, 45);
      display.setTextSize(1);
      display.println(healthStatus);
    } else {
      Serial.println("Failed to read from MLX sensor!");
      display.fillScreen(COLOR_BLACK);
      display.setTextColor(COLOR_RED);
      display.setCursor(5, 25);
      display.setTextSize(1);
      display.println("SENSOR ERROR");
    }
  }
}


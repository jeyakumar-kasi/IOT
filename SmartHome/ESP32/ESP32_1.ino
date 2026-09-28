#include <WiFi.h>
#include <HTTPClient.h>
#include <WebServer.h>

// ==================== CONFIGURATION SECTOR ====================
const char* ssid     = "YOUR_WIFI_SSID";     // Your Wi-Fi Router Name
const char* password = "YOUR_WIFI_PASSWORD"; // Your Wi-Fi Router Password

// Static IP Address of the 2nd-Floor Rooftop ESP8266
const String roofESP_IP = "http://192.168.1.50"; 

const int IRRIGATION_TIMER_MINS = 15; // mins
// ==============================================================

// Pin Definitions for 30-Pin ESP32
const int P43_FLOAT_PIN = 4;   // GPIO 4 for the ground tank P43 float switch
const int PUMP_RELAY_PIN = 19; // GPIO 19 for the Optocoupled Pump Relay (Pump #1: 55W Irrigation Pump)
const int PUMP2_RELAY_PIN = 21; // GPIO 21 for the High-Power Pump Relay (Pump #2: 1.5 HP Water Pump)

// Host a local server on port 80 so your 1st-floor PC can read data if it's awake
WebServer server(80);

unsigned long lastNetworkCheck = 0;
const long checkInterval = 20000; // Pull data from the roof every 20 seconds
int overheadTankDistance = 0;

// === DAILY TIMER CONFIGURATION FOR PUMP #1 (55W IRRIGATION PUMP) ===
unsigned long lastDailyPumpRun = 0;
unsigned long dailyPumpStartTime = 0;
bool isDailyTimerActive = false;

// 24 Hours in milliseconds (86,400,000 ms)
const unsigned long ONE_DAY_MS = 86400000; 
// 15 Minutes run duration in milliseconds (900,000 ms)
const unsigned long PUMP_RUN_DURATION_MS = IRRIGATION_TIMER_MINS * 60000; //15mins => 900000; 

void setup() {
  Serial.begin(115200);
  
  pinMode(P43_FLOAT_PIN, INPUT_PULLUP);
  pinMode(PUMP_RELAY_PIN, OUTPUT);
  pinMode(PUMP2_RELAY_PIN, OUTPUT);
  
  // Default Both Pumps OFF at Boot (Active-Low Relay standard boards)
  digitalWrite(PUMP_RELAY_PIN, HIGH); // Default Pump OFF (Active-Low Relay)
  digitalWrite(PUMP2_RELAY_PIN, HIGH); // Default Pump 2 OFF (Active-Low Relay)

  // Connect to your Wi-Fi Router
  WiFi.begin(ssid, password);
  Serial.print("Connecting Master ESP32 to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nMaster ESP32 Connected!");
  Serial.print("Ground Floor IP Address (Ensure Router Locks This): ");
  Serial.println(WiFi.localIP()); // Should be locked to 192.168.1.60

  server.on("/", handleRoot);
  server.begin();
}

void loop() {
  server.handleClient(); // Listen for requests from the desktop PC
  unsigned long currentMillis = millis();

  // Task 1: Periodically fetch water level data from the 2nd-floor roof over Wi-Fi
  if (currentMillis - lastNetworkCheck >= checkInterval) {
    lastNetworkCheck = currentMillis;
    fetchRooftopData();
  }

  // --- LOGIC BLOCK A: PUMP #1 (55W IRRIGATION) DAILY 15-MINUTE TIMER ---
  // If 24 hours have passed since the last run (or at boot), initiate the cycle
  if (!isDailyTimerActive && (currentMillis - lastDailyPumpRun >= ONE_DAY_MS || lastDailyPumpRun == 0)) {
    int groundSourceWaterCheck = digitalRead(P43_FLOAT_PIN);
    
    // Only start if the ground water float switch confirms water is present
    if (groundSourceWaterCheck == LOW) {
      Serial.println("⏰ TIMER TRIGGER: Starting 15-minute daily irrigation cycle (Pump #1).");
      isDailyTimerActive = true;
      dailyPumpStartTime = currentMillis;
      digitalWrite(PUMP_RELAY_PIN, LOW); // Turn relay ON
    } else {
      Serial.println("⏰ TIMER BLOCKED: Cannot start daily irrigation because ground tank is empty.");
      // Retry checking in 5 minutes instead of waiting a full day
      lastDailyPumpRun = currentMillis - ONE_DAY_MS + 300000; 
    }
  }

  // If the daily timer loop is active, monitor the 15-minute cutoff threshold
  if (isDailyTimerActive) {
    if (currentMillis - dailyPumpStartTime >= PUMP_RUN_DURATION_MS) {
      Serial.println("⏰ TIMER SUCCESS: 15 minutes complete. Stopping daily irrigation (Pump #1).");
      isDailyTimerActive = false;
      lastDailyPumpRun = currentMillis;
      digitalWrite(PUMP_RELAY_PIN, HIGH); // Turn relay OFF
    }
  }

  // Task 2: Core Hardware Interlock Protection Logic
  int groundSourceWater = digitalRead(P43_FLOAT_PIN);

  // If the ground-floor backup tank goes dry (Float switch triggers HIGH)
  if (groundSourceWater == HIGH) { 
    Serial.println("🚨 CRITICAL FAULT: Ground Source Tank Empty! Forcing ALL Pumps OFF.");
    digitalWrite(PUMP_RELAY_PIN, HIGH); 
    digitalWrite(PUMP2_RELAY_PIN, HIGH); // Emergency shut off Pump #2
    
    // Reset timer states cleanly if dry-run occurs during execution
    if (isDailyTimerActive) {
      isDailyTimerActive = false;
      lastDailyPumpRun = currentMillis; 
    }
  } 
  else if (overheadTankDistance > 0 && overheadTankDistance < 30) {
    // Water is closer than 30cm to rooftop sensor = Overhead tank is full!
    Serial.println("✅ Overhead Tank Full. Turning ALL Pumps OFF.");
    digitalWrite(PUMP_RELAY_PIN, HIGH);
    digitalWrite(PUMP2_RELAY_PIN, HIGH); // Stop high-power refill pump
    
    if (isDailyTimerActive) {
      isDailyTimerActive = false;
      lastDailyPumpRun = currentMillis;
    }
  }
  // --- LOGIC BLOCK B: PUMP #2 (1.5 HP REFILL) AUTOMATIC LOW LEVEL SYSTEM ---
  else if (overheadTankDistance > 120 && groundSourceWater == LOW) {
    // Tank is low (water far away from sensor) and ground water is available
    Serial.println("💧 Overhead Tank Low. Activating 1.5 HP Refill Pump (Pump #2).");
    digitalWrite(PUMP2_RELAY_PIN, LOW); // Turn relay ON
  }
  else {
    // If overhead tank distance is normal (between 30cm and 120cm), keep Pump #2 off
    if (overheadTankDistance >= 30 && overheadTankDistance <= 120) {
      digitalWrite(PUMP2_RELAY_PIN, HIGH); // Turn relay OFF
    }
  }
}

// Function to handle the desktop PC checking the master status
void handleRoot() {
  String response = "Ground Floor Master Status:\n";
  response += "Overhead Tank Distance: " + String(overheadTankDistance) + " cm\n";
  response += "Ground Tank Status: " + String(digitalRead(P43_FLOAT_PIN) == LOW ? "OK" : "EMPTY") + "\n";
  response += "Pump 1 (55W) Status: " + String(digitalRead(PUMP_RELAY_PIN) == LOW ? "RUNNING" : "STOPPED") + "\n";
  response += "Pump 2 (1.5 HP) Status: " + String(digitalRead(PUMP2_RELAY_PIN) == LOW ? "RUNNING" : "STOPPED") + "\n";
  response += "Daily Timer Running: " + String(isDailyTimerActive ? "YES" : "NO");
  server.send(200, "text/plain", response);
}

// Wireless background network request to pull data from the roof ESP8266
void fetchRooftopData() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(roofESP_IP);
    int httpCode = http.GET();
    
    if (httpCode > 0) {
      String payload = http.getString();

      // Look for the "Water Tank Distance:" text inside the roof response
      int index = payload.indexOf("Water Tank Distance:");
      if (index != -1) {
        int start = index + String("Water Tank Distance:").length();
        int end = payload.indexOf("cm", start);
        String distanceStr = payload.substring(start, end);
        distanceStr.trim();
        overheadTankDistance = distanceStr.toInt();
        Serial.printf("Live Wireless Data -> Roof Tank Distance: %d cm\n", overheadTankDistance);
      }
    } else {
      Serial.println("⚠️ Connection Error: Failed to contact Rooftop ESP8266.");
    }
    http.end();
  }
}

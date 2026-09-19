#include <WiFi.h>
#include <HTTPClient.h>
#include <WebServer.h>

// ==================== CONFIGURATION SECTOR ====================
const char* ssid     = "YOUR_WIFI_SSID";     // Your Wi-Fi Router Name
const char* password = "YOUR_WIFI_PASSWORD"; // Your Wi-Fi Router Password

// Static IP Address of the 2nd-Floor Rooftop ESP8266
const String roofESP_IP = "http://192.168.1"; 
// ==============================================================

// Pin Definitions for 30-Pin ESP32
const int P43_FLOAT_PIN = 4;   // GPIO 4 for the ground tank P43 float switch
const int PUMP_RELAY_PIN = 19; // GPIO 19 for the Optocoupled Pump Relay

// Host a local server on port 80 so your 1st-floor PC can read data if it's awake
WebServer server(80);

unsigned long lastNetworkCheck = 0;
const long checkInterval = 20000; // Pull data from the roof every 20 seconds
int overheadTankDistance = 0;

void setup() {
  Serial.begin(115200);
  
  pinMode(P43_FLOAT_PIN, INPUT_PULLUP);
  pinMode(PUMP_RELAY_PIN, OUTPUT);
  digitalWrite(PUMP_RELAY_PIN, HIGH); // Default Pump OFF (Active-Low Relay)

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

  // Task 2: Core Hardware Interlock Protection Logic
  int groundSourceWater = digitalRead(P43_FLOAT_PIN);

  // If the ground-floor backup tank goes dry (Float switch triggers HIGH)
  if (groundSourceWater == HIGH) { 
    Serial.println("🚨 CRITICAL FAULT: Ground Source Tank Empty! Forcing Pump OFF.");
    digitalWrite(PUMP_RELAY_PIN, HIGH); 
  } 
  else if (overheadTankDistance > 0 && overheadTankDistance < 30) {
    // Water is closer than 30cm to rooftop sensor = Overhead tank is full!
    Serial.println("✅ Overhead Tank Full. Turning Pump OFF.");
    digitalWrite(PUMP_RELAY_PIN, HIGH);
  }
  else if (overheadTankDistance > 120 && groundSourceWater == LOW) {
    // Tank is low (water far away from sensor) and ground water is available
    Serial.println("💧 Overhead Tank Low. Activating 55W Pump.");
    digitalWrite(PUMP_RELAY_PIN, LOW); // Turn relay ON
  }
}

// Function to handle the desktop PC checking the master status
void handleRoot() {
  String response = "Ground Floor Master Status:\n";
  response += "Overhead Tank Distance: " + String(overheadTankDistance) + " cm\n";
  response += "Ground Tank Status: " + String(digitalRead(P43_FLOAT_PIN) == LOW ? "OK" : "EMPTY") + "\n";
  response += "Pump Status: " + String(digitalRead(PUMP_RELAY_PIN) == LOW ? "RUNNING" : "STOPPED");
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

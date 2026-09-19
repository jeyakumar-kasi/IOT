#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

// ==================== CONFIGURATION SECTOR ====================
const char* ssid     = "YOUR_WIFI_SSID";     // Put your Wi-Fi name here
const char* password = "YOUR_WIFI_PASSWORD"; // Put your Wi-Fi password here

// ESP8266 Pin Maps (Matches your waterproof JSN-SR04T layout)
const int TRIG_PIN = 5;  // Board Label D1 (GPIO 5)
const int ECHO_PIN = 4;  // Board Label D2 (GPIO 4) - Note: Use the 1k/2k resistor safety divider here!
// ==============================================================

// Set up the local web server on standard port 80
ESP8266WebServer server(80);

// Global state variables
int currentTankDistance = 0;
unsigned long lastSensorRead = 0;
const long sensorInterval = 2000; // Read the water tank distance every 2 seconds

void setup() {
  Serial.begin(115200);
  delay(10);
  
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // Connect to your home Wi-Fi Router
  Serial.println();
  Serial.print("Connecting to Wi-Fi Network: ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  // Blink serial dots until network handshake is complete
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\nWi-Fi Connected successfully!");
  Serial.print("Rooftop Node Static Local IP Address: ");
  Serial.println(WiFi.localIP()); // Ensure this matches the IP set in your ground floor master code!

  // Define URLs (Routes)
  server.on("/", handleRoot);
  server.on("/trigger_alarm", handleAlarmRequest);
  
  // Launch the server engine
  server.begin();
  Serial.println("Rooftop Local Web Server Started.");
}

void loop() {
  // Handle incoming background queries from the Ground Floor ESP32 Master
  server.handleClient(); 

  unsigned long currentMillis = millis();

  // Non-blocking timer to update sensor readings independently without crashing Wi-Fi
  if (currentMillis - lastSensorRead >= sensorInterval) {
    lastSensorRead = currentMillis;
    currentTankDistance = readWaterDistance();
    
    // Debugging printouts for your bench-testing setup
    Serial.print("Current Tank Distance: ");
    Serial.print(currentTankDistance);
    Serial.println(" cm");
  }
}

// Function that sends the water level data when the ground floor master hits "http://192.168.1"
void handleRoot() {
  String responseText = "Water Tank Distance: " + String(currentTankDistance) + " cm\n";
  responseText += "Status: ROOFTOP_NODE_ONLINE\n";
  
  server.send(200, "text/plain", responseText);
}

// Optional: Handles the security alarm routing triggered by your 1st-floor desktop PC
void handleAlarmRequest() {
  server.send(200, "text/plain", "Rooftop alert acknowledged.");
  Serial.println("🚨 SECURITY ALERT: Human spotted on CCTV! (Trigger flashlights/buzzers here if wired).");
}

// Low-level helper function to pulse the ultrasonic sensor safely
int readWaterDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  
  // 30,000 microsecond timeout ensures the single-core CPU won't lock up if the probe gets unplugged
  long pulseDuration = pulseIn(ECHO_PIN, HIGH, 30000); 
  
  // Calculate raw distance in centimeters based on sound speed
  int calculatedDistance = pulseDuration * 0.034 / 2;
  
  // Error trap: If reading is out of range, default to 0 to prevent erratic pump activations
  if (calculatedDistance <= 0 || calculatedDistance > 450) {
    return 0; 
  }
  
  return calculatedDistance;
}

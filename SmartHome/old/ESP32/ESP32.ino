#include <WiFi.h>
#include <WebServer.h>

// Wi-Fi Credentials
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// Pin Definitions
const int TRIG_PIN = 5;
const int ECHO_PIN = 18;
const int RELAY_VALVE_PIN = 19;
const int ROOFTOP_ALARM_PIN = 21; // Optional: Connect a light/buzzer for night security

// Web Server on port 80
WebServer server(80);

// Timing Variables (Non-blocking)
unsigned long lastSensorRead = 0;
const long sensorInterval = 10000; // Read tank every 10 seconds

unsigned long irrigationStartTime = 0;
const long irrigationDuration = 900000; // 15 minutes in milliseconds
bool isIrrigating = false;

int currentTankDistance = 0;
const int MIN_TANK_LEVEL_CM = 150; // Safety cutoff threshold

void setup() {
  Serial.begin(115200);
  
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(RELAY_VALVE_PIN, OUTPUT);
  pinMode(ROOFTOP_ALARM_PIN, OUTPUT);
  
  // Set relays to default OFF state (Assuming active-low relay boards)
  digitalWrite(RELAY_VALVE_PIN, HIGH);
  digitalWrite(ROOFTOP_ALARM_PIN, HIGH);

  // Connect to your Wi-Fi Router
  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected!");
  Serial.print("ESP32 Static Local IP: ");
  Serial.println(WiFi.localIP()); // Paste this IP address into your desktop Python script

  // Define Web Server Routes
  server.on("/", handleRoot);
  server.on("/trigger_alarm", handleAlarm);
  server.on("/start_irrigation", handleStartIrrigation);
  
  server.begin();
}

void loop() {
  server.handleClient(); // Handle incoming network requests from your PC
  unsigned long currentMillis = millis();

  // TASK 1: Read Water Tank Level Every 10 Seconds
  if (currentMillis - lastSensorRead >= sensorInterval) {
    lastSensorRead = currentMillis;
    currentTankDistance = readUltrasonic();
    
    // SAFETY CHECK: Force close valve if water tank is too low
    if (currentTankDistance > MIN_TANK_LEVEL_CM && isIrrigating) {
      stopIrrigation();
    }
  }

  // TASK 2: Manage Automatic Irrigation Cutoff
  if (isIrrigating && (currentMillis - irrigationStartTime >= irrigationDuration)) {
    stopIrrigation();
  }
}

// Function to handle the home page URL (e.g., http://192.168.1)
void handleRoot() {
  String statusMsg = "Water Tank Distance: " + String(currentTankDistance) + " cm\n";
  statusMsg += "Irrigation Status: " + String(isIrrigating ? "ON" : "OFF");
  server.send(200, "text/plain", statusMsg);
}

// Function triggered by Python PC when a human is spotted at night
void handleAlarm() {
  server.send(200, "text/plain", "Rooftop alarm activated!");
  digitalWrite(ROOFTOP_ALARM_PIN, LOW); // Turn on rooftop light/alarm (Active Low)
  delay(3000);                          // Brief delay for the alarm trigger
  digitalWrite(ROOFTOP_ALARM_PIN, HIGH); // Turn off
}

// Function to trigger irrigation via a browser or local network call
void handleStartIrrigation() {
  if (currentTankDistance < MIN_TANK_LEVEL_CM) {
    isIrrigating = true;
    irrigationStartTime = millis();
    digitalWrite(RELAY_VALVE_PIN, LOW); // Open valve
    server.send(200, "text/plain", "Irrigation started successfully.");
  } else {
    server.send(200, "text/plain", "Error: Tank level too low to irrigate.");
  }
}

void stopIrrigation() {
  isIrrigating = false;
  digitalWrite(RELAY_VALVE_PIN, HIGH); // Close valve
}

int readUltrasonic() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  
  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  return duration * 0.034 / 2;
}

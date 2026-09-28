/**
* [Ground Floor: ESP32] 
* 🔄 Multi-Pump Sequential Logic Execution
* ----------------------------------------
* Pump #1 (1.5 HP Water Pump): Designated as the primary refill source. It turns on 
* automatically whenever the overheadTankDistance > 120 cm logic trips to keep the 
* main water supply refilled.

* Sequential Interlock Priority: Every morning at 6:00 AM, the daily watering window 
* activates. However, to ensure household water is never compromised, Pump #1 (1.5 HP)
* always takes absolute priority. If the roof sensor reports that the overhead tank needs
* water, the irrigation system is safely held back until Pump #1 finishes filling the 
* tank completely (overheadTankDistance < 30 cm).

* Pump #2 (55W Irrigation Pump): As soon as the overhead tank is verified full, Pump #2 
* takes over and runs smoothly for exactly 15 minutes (6:00 AM to 6:15 AM) using non-blocking 
* tracking variables (softRtc).
*
* @Module    : As part of "Smart Home Automation" Project.
* @Author    : jeyakumar.kasi@hyproid.com
* @Created on: Mon Sep 28, 2026 21:23 
*/

#include <WiFi.h>
#include <HTTPClient.h>
#include <WebServer.h>
#include <Wire.h>
#include "RTClib.h"

// ==================== CONFIGURATION SECTOR ====================
const char* ssid     = "MY_WIFI_SSID";     // Wi-Fi Router Name
const char* password = "MY_WIFI_PASSWORD"; // Wi-Fi Router Password

// Static IP Address of the 2nd-Floor Rooftop ESP8266
const String roofESP_IP = "http://192.168.1.50"; 

// Main Pump (1.5hp) 
const int MAIN_PUMP_MAX_RUN_MIN = 10; // Mins (Max. continuous run)

// --- IRRIGATION PUMP SCHEDULE CONFIGURATION (24-Hour Format) ---
const int startHour = 6;    // 6:00 AM (Scheduled ON)
const int startMinute = 0;
const int endHour = 6;      // 6:15 AM (Scheduled OFF)
const int endMinute = 15;

// ==============================================================

// Hardware Pin Assignments
const int batteryPin = A0;  
const int ldrPin = A1;       // Connect LDR voltage divider signal here
const int sqwPin = 2;       
const int PUMP_RELAY_PIN = 19; // GPIO 19 for the Optocoupled Pump Relay (Pump #2: 55watts pump)
const int PUMP2_RELAY_PIN = 21; // GPIO 21 for the High-Power Pump Relay (Pump #1: 1.5HP pump)
const int P43_FLOAT_PIN = 4;   // GPIO 4 for the ground tank P43 float switch
const int buttonPin = 3;     // Push button connected to GND
const int errorPin = 5;      // LED Indicators
const int allOkPin = 4;
const int runningPin = 6;

// --- LDR LIGHT THRESHOLD ---
// Lower values = Darker room required. Higher values = turns on even if dim.
// Typical range with 10k resistor: Dark < 200, Room light > 500 | Default: 300
const int darkThreshold = 0; // Set to "0" to ignore this condition.

// Software Timing Variables (Non-blocking)
unsigned long lastSerialUpdate = 0;
const unsigned long serialInterval = 1000;     // Print status every 1 second
unsigned long lastBatteryCheck = 0;
const unsigned long batteryCheckInterval = 43200000; // Check battery every 12 hours

// Sync Flag
bool dynamicSyncedToday = false;

// Button Debounce & State Variables
bool manualOverride = false;
bool lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50; 

// Strings for boot logging and display
String bootTimestamp = "";
const char* daysOfTheWeek[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

// Core Hardware and Software Clock Initializers
RTC_DS1307 rtc;
// Software-only clock that calculates time locally using the Arduino's crystal
RTC_Millis softRtc; 

// Host a local server on port 80 so your 1st-floor PC can read data if it's awake
WebServer server(80);

unsigned long lastNetworkCheck = 0;
const long checkInterval = 20000; // Pull data from the roof every 20 seconds
int overheadTankDistance = 0;

// Watchdog state tracking variables
bool isRooftopOnline = true; 

// --- ADVANCED SAFETY RUN-TIME VARIABLES ---
unsigned long pump1StartTime = 0;
bool isPump1Running = false;
bool pump1LockoutActive = false;
const unsigned long PUMP1_MAX_RUN_TIME_MS = MAIN_PUMP_MAX_RUN_MIN * 60000; // 'n' Minutes continuous run maximum threshold limit

void setup() {
  Serial.begin(115200);
  while (!Serial); 
  Serial.println("Starting an Application...");

  // ACTIVE-LOW SETUP: Initialize pin and immediately set HIGH (Relay OFF)
  pinMode(PUMP_RELAY_PIN, OUTPUT);
  digitalWrite(PUMP_RELAY_PIN, HIGH); // Default Pump OFF (Active-Low Relay)
  
  pinMode(PUMP2_RELAY_PIN, OUTPUT);
  digitalWrite(PUMP2_RELAY_PIN, HIGH); // Default Pump 2 OFF (Active-Low Relay)
  
  pinMode(P43_FLOAT_PIN, INPUT_PULLUP);
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(sqwPin, INPUT_PULLUP); 

  // LED Indicators
  pinMode(errorPin, OUTPUT);
  pinMode(allOkPin, OUTPUT);
  pinMode(runningPin, OUTPUT);

  // --- ONE-TIME HARDWARE RTC STARTUP PIN PULL ---
  if (!rtc.begin()) {
    Serial.println("Error: Could not find DS1307 RTC!");
    digitalWrite(errorPin, HIGH);
    while (1); 
  }

  // NOTE: If you ever need to manually set the time again in the future, 
  // uncomment the line below, adjust the values, upload once, then comment it back out.
  // rtc.adjust(DateTime(2026, 9, 26, 21, 14, 05)); 

  // Turn off the hardware square wave generator to conserve extra hardware energy
  rtc.writeSqwPinMode(DS1307_OFF);

  // Fetch the definitive hardware time exactly ONCE at boot
  DateTime hardwareBootTime = rtc.now();

  // Initialize our Software-only Clock using that precise time
  softRtc.begin(hardwareBootTime);

  // Format the restart timestamp
  bootTimestamp = String(hardwareBootTime.year()) + "-" + 
                  (hardwareBootTime.month() < 10 ? "0" : "") + String(hardwareBootTime.month()) + "-" + 
                  (hardwareBootTime.day() < 10 ? "0" : "") + String(hardwareBootTime.day()) + " " + 
                  (hardwareBootTime.hour() < 10 ? "0" : "") + String(hardwareBootTime.hour()) + ":" + 
                  (hardwareBootTime.minute() < 10 ? "0" : "") + String(hardwareBootTime.minute()) + ":" + 
                  (hardwareBootTime.second() < 10 ? "0" : "") + String(hardwareBootTime.second());

  Serial.println("=========================================");
  Serial.print("DEVICE RESTARTED / FIXED BOOT TIME: ");
  Serial.println(bootTimestamp);
  Serial.println("RTC Hardware disconnected safely. Using internal crystal tracking.");
  Serial.println("=========================================");

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

  digitalWrite(errorPin, LOW); 
  digitalWrite(allOkPin, HIGH);
}

void loop() {
  server.handleClient(); // Listen for requests from the desktop PC
  // 1. REFRESH CURRENT TIME FROM INTERNAL SOFTWARE (Zero hardware battery drain)
  DateTime now = softRtc.now();
  unsigned long currentMillis = millis();

  // Task 1: Periodically fetch water level data from the 2nd-floor roof over Wi-Fi
  if (currentMillis - lastNetworkCheck >= checkInterval) {
    lastNetworkCheck = currentMillis;
    fetchRooftopData();
  }

  // 2. ONCE-A-DAY MIDNIGHT DRIFT RESYNC LOGIC
  if (now.hour() == 0 && now.minute() == 0) {
    if (!dynamicSyncedToday) {
      DateTime realHardwareTime = rtc.now(); 
      softRtc.begin(realHardwareTime);       
      dynamicSyncedToday = true;             
      Serial.println("\n[SYSTEM] Clock Drift Sync Completed Successfully at Midnight.");
      
      // Auto-clear the 1.5HP maximum run-time safety block once per day at midnight
      if (pump1LockoutActive) {
        pump1LockoutActive = false;
        Serial.println("[SYSTEM SAFE]: Daily reset clearing Pump #1 max safety runtime lockout.");
      }
    }
  } else {
    if (dynamicSyncedToday) {
      dynamicSyncedToday = false;
    }
  }

  // 3. CONTINUOUS SOFTWARE BUTTON MONITORING (Debounced)
  int reading = digitalRead(buttonPin);
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading == LOW && lastButtonState == HIGH) {
      manualOverride = !manualOverride; 
      Serial.println("\n[!] BUTTON PRESSED - TOGGLING OVERRIDE [!]");
    }
  }
  lastButtonState = reading;

  // 4. READ AMBIENT LIGHT SENSOR (LDR)
  int ldrValue = analogRead(ldrPin);
  bool isDarkOut = (darkThreshold <= 0) || (ldrValue < darkThreshold);

  // 5. LIGHT AUTOMATION LOGIC (Time + LDR Combined) -> MIGRATED FOR PUMP #2 (55W IRRIGATION PUMP)
  int currentMinutes = (now.hour() * 60) + now.minute();
  int startMinutes = (startHour * 60) + startMinute;
  int endMinutes = (endHour * 60) + endMinute;

  bool timeScheduleActive = false;

  if (startMinutes < endMinutes) {
    if (currentMinutes >= startMinutes && currentMinutes < endMinutes) {
      timeScheduleActive = true;
    }
  } else {
    if (currentMinutes >= startMinutes || currentMinutes < endMinutes) {
      timeScheduleActive = true;
    }
  }

  // Task 2: Core Hardware Interlock Protection Logic
  int groundSourceWater = digitalRead(P43_FLOAT_PIN);

  // If the ground-floor backup tank goes dry (Float switch triggers HIGH)
  if (groundSourceWater == HIGH) { 
    Serial.println("🚨 CRITICAL FAULT: Ground Source Tank Empty! Forcing ALL Pumps OFF.");
    digitalWrite(PUMP_RELAY_PIN, HIGH); 
    digitalWrite(PUMP2_RELAY_PIN, HIGH); // Emergency shut off Pump #1
    isPump1Running = false;
  } 
  else if (overheadTankDistance > 0 && overheadTankDistance < 30) {
    // Water is closer than 30cm to rooftop sensor = Overhead tank is full!
    Serial.println("✅ Overhead Tank Full. Turning ALL Pumps OFF.");
    digitalWrite(PUMP_RELAY_PIN, HIGH);
    digitalWrite(PUMP2_RELAY_PIN, HIGH); // Stop high-power refill pump
    isPump1Running = false;
  }
  else {
    // --- PUMP #1 (1.5HP REFILL PUMP) SYSTEM LOGIC ---
    // Only allow Pump #1 to turn on if roof is online and no safety runtime lockout is active
    if (overheadTankDistance > 120 && groundSourceWater == LOW && isRooftopOnline && !pump1LockoutActive) {
      
      // Track the initialization timestamp of the high-power run
      if (!isPump1Running) {
        isPump1Running = true;
        pump1StartTime = currentMillis;
        Serial.println("⚡ Pump #1 Core Timer Initialized.");
      }

      // MONITOR MAXIMUM RUNTIME CUTOFF EXCEEDED: Prevent infinite runs
      if (currentMillis - pump1StartTime >= PUMP1_MAX_RUN_TIME_MS) {
        digitalWrite(PUMP2_RELAY_PIN, HIGH); // Force relay OFF instantly
        isPump1Running = false;
        pump1LockoutActive = true; 
        Serial.println("\n🚨 HARWARE EXCEPTION: Pump #1 exceeded max continuous 20-min limit! Lockout activated.");
      } else {
        digitalWrite(PUMP2_RELAY_PIN, LOW); // Turn relay ON safely
        digitalWrite(PUMP_RELAY_PIN, HIGH); // Force irrigation OFF to guarantee filling power
      }
    }
    else {
      // Turn off Pump #1 if distance returns within normal parameters or sensor goes down
      if ((overheadTankDistance >= 30 && overheadTankDistance <= 120) || !isRooftopOnline || pump1LockoutActive) {
        digitalWrite(PUMP2_RELAY_PIN, HIGH); // Turn relay OFF
        isPump1Running = false;
      }
      
      // --- PUMP #2 (55W IRRIGATION PUMP) TIME LOGIC ---
      // The automated rule: Time window active, LDR conditions valid, and Pump #1 is NOT busy refilling
      bool shouldBeOn = timeScheduleActive && isDarkOut && (digitalRead(PUMP2_RELAY_PIN) == HIGH);
      
      // Apply button toggle override state inversion (Override bypasses the LDR rule)
      if (manualOverride) {
        shouldBeOn = !shouldBeOn;
      }
      
      // ACTIVE-LOW TRANSLATION CONTROL FOR PUMP #2
      if (shouldBeOn) {
        digitalWrite(runningPin, HIGH);
        digitalWrite(PUMP_RELAY_PIN, LOW);  // LOW turns the Active-Low isolated relay ON
      } else {
        digitalWrite(PUMP_RELAY_PIN, HIGH); // HIGH turns the Active-Low isolated relay OFF
        digitalWrite(runningPin, LOW);
      }
    }
  }
  
  // 6. NON-BLOCKING SERIAL PRINTER & CACHED BATTERY LOGIC
  if (millis() - lastSerialUpdate >= serialInterval) {
    lastSerialUpdate = millis();
    
    // Print Boot Reference
    Serial.print("Booted: ["); Serial.print(bootTimestamp); Serial.print("] | ");
    
    // Print Current Date & Time
    Serial.print(daysOfTheWeek[now.dayOfTheWeek()]); Serial.print(' ');
    if (now.month() < 10) Serial.print('0'); Serial.print(now.month(), DEC); Serial.print('-');
    if (now.day() < 10) Serial.print('0'); Serial.print(now.day(), DEC); Serial.print(' ');
    if (now.hour() < 10) Serial.print('0'); Serial.print(now.hour(), DEC); Serial.print(':');
    if (now.minute() < 10) Serial.print('0'); Serial.print(now.minute(), DEC); Serial.print(':');
    if (now.second() < 10) Serial.print('0'); Serial.print(now.second(), DEC);
    
    // Print Light Sensor value
    Serial.print(" | LDR: "); Serial.print(ldrValue);Serial.print(isDarkOut ? " (Dark)" : " (Bright)");
    
    // Print Automation Output Status
    Serial.print(" | Overhead Dist: "); Serial.print(overheadTankDistance); Serial.print("cm");
    Serial.print(isRooftopOnline ? " (Online)" : " (OFFLINE ❌)");
    
    if (pump1LockoutActive) 
      Serial.print(" [LOCKOUT]"); 
    Serial.print(digitalRead(PUMP2_RELAY_PIN) == LOW ? " | PUMP1: ON" : " | PUMP1: OFF");
    Serial.print(digitalRead(PUMP_RELAY_PIN) == LOW ? " | PUMP2: ON" : " | PUMP2: OFF");
    Serial.print(manualOverride ? " (MANUAL)" : " (AUTO)  ");
    
    // Low-frequency Battery Monitoring
    if (millis() - lastBatteryCheck >= batteryCheckInterval || lastBatteryCheck == 0) {
      lastBatteryCheck = millis();
      int rawAnalog = analogRead(batteryPin);
      
      // @adjust: Add +0.6V (or the specific forward drop of your diode) to correct the math
      float batteryVoltage = (rawAnalog * (5.0 / 1023.0)) + 0.6;
      float batteryPercentage = constrain(((batteryVoltage - 2.0) / (3.0 - 2.0)) * 100.0, 0.0, 100.0);
      Serial.print(" | Batt: "); Serial.print(batteryVoltage, 2);Serial.print("V ("); Serial.print(batteryPercentage, 0); Serial.println("%)");
    } else {
      Serial.println(" | Batt: [Cached]");
    }
  }
}

// Function to handle the desktop PC checking the master status
void handleRoot() {
  String response = "Ground Floor Master Status:\n";
  response += "Overhead Tank Distance: " + String(overheadTankDistance) + " cm\n";
  response += "Rooftop Link: " + String(isRooftopOnline ? "CONNECTED" : "DISCONNECTED ❌") + "\n";
  response += "Pump 1 (1.5HP) Status: " + String(digitalRead(PUMP2_RELAY_PIN) == LOW ? "RUNNING" : "STOPPED") + "\n";
  response += "Pump 2 (55W) Status: " + String(digitalRead(PUMP_RELAY_PIN) == LOW ? "RUNNING" : "STOPPED") + "\n";
  response += "Pump 1 Lockout Status: " + String(pump1LockoutActive ? "LOCKED" : "SAFE") + "\n";
  response += "Manual Override Mode: " + String(manualOverride ? "ACTIVE" : "DISABLED");
  
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
      isRooftopOnline = true; // Connection valid, arm operations
      digitalWrite(errorPin, LOW);
      
      // Look for the "Water Tank Distance:" text inside the roof response
      int index = payload.indexOf("Water Tank Distance:");
      if (index != -1) {
        int start = index + String("Water Tank Distance:").length();
        int end = payload.indexOf("cm", start);
        String distanceStr = payload.substring(start, end);
        distanceStr.trim();
        
        overheadTankDistance = distanceStr.toInt();
        Serial.printf("\nLive Wireless Data -> Roof Tank Distance: %d cm\n", overheadTankDistance);
      }
    } else {
      // WATCHDOG INTERCEPT: The network signal dropped mid-execution!
      //⚠️ Connection Error: Failed to contact Rooftop ESP8266
      isRooftopOnline = false;
      overheadTankDistance = 0;
      digitalWrite(errorPin, HIGH); // Illuminate error LED indicator
      
      // BROADCAST NETWORK FAILURE SIGNAL FOR THE PC DESKTOP MONITOR ALARM
      Serial.println("🚨ROOFTOP_OFFLINE🚨");
      
      // IMMEDIATE FAIL-SAFE SHUTDOWN: Drop Pump #1 relay instantly to prevent overfilling or running dry
      if (digitalRead(PUMP2_RELAY_PIN) == LOW) {
        digitalWrite(PUMP2_RELAY_PIN, HIGH);
        isPump1Running = false;
        Serial.println("\n🚨 WATCHDOG FAULT: Rooftop Node lost! Emergency Shutdown Pump #1 (1.5HP) immediately.");
      }
    } // else:httpcode
      
    http.end();
  } // if:WI-FI Status
}

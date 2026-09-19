/**
* Switch ON/OFF the Lights - daily (@6PM to 6 AM)
*
* @Module    : As part of "Smart Home Automation" Project.
* @Author    : jeyakumar.kasi@hyproid.com
* @Created on: Fri Sep 09, 2026 23:53 
*/

#include <Wire.h>
#include "RTClib.h"

RTC_DS1307 rtc;
// Software-only clock that calculates time locally using the Arduino's crystal
RTC_Millis softRtc; 

// Hardware Pin Assignments
const int batteryPin = A0;  
const int ldrPin = A1;       // Connect LDR voltage divider signal here
const int sqwPin = 2;       
const int lightRelayPin = 7; 
const int buttonPin = 3;     // Push button connected to GND
const int errorPin = 5;      // LED Indicators
const int allOkPin = 4;
const int runningPin = 6;

// --- OVERNIGHT SCHEDULE CONFIGURATION (24-Hour Format) ---
const int startHour = 18;   // 6:00 PM (Scheduled ON)
const int startMinute = 0;
const int endHour = 6;      // 6:00 AM Next Day (Scheduled OFF)
const int endMinute = 0;

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

void setup() {
  Serial.begin(9600);
  while (!Serial); 

  // ACTIVE-LOW SETUP: Initialize pin and immediately set HIGH (Relay OFF)
  pinMode(lightRelayPin, OUTPUT);
  digitalWrite(lightRelayPin, HIGH); 
  
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(sqwPin, INPUT_PULLUP); 

  # LED Indicators
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
  // rtc.adjust(DateTime(2026, 9, 18, 23, 28, 05)); 


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

  digitalWrite(errorPin, LOW); 
  digitalWrite(allOkPin, HIGH);
}

void loop() {
  // 1. REFRESH CURRENT TIME FROM INTERNAL SOFTWARE (Zero hardware battery drain)
  DateTime now = softRtc.now();

  // 2. ONCE-A-DAY MIDNIGHT DRIFT RESYNC LOGIC
  if (now.hour() == 0 && now.minute() == 0) {
    if (!dynamicSyncedToday) {
      DateTime realHardwareTime = rtc.now(); 
      softRtc.begin(realHardwareTime);       
      dynamicSyncedToday = true;             
      Serial.println("\n[SYSTEM] Clock Drift Sync Completed Successfully at Midnight.");
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

  // 5. LIGHT AUTOMATION LOGIC (Time + LDR Combined)
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

  // The automated rule: Time window must be active AND it must be dark outside
  bool shouldBeOn = timeScheduleActive && isDarkOut;

  // Apply button toggle override state inversion (Override bypasses the LDR rule)
  if (manualOverride) {
    shouldBeOn = !shouldBeOn; 
  }

  // ACTIVE-LOW TRANSLATION CONTROL
  if (shouldBeOn) {
    digitalWrite(runningPin, HIGH);
    digitalWrite(lightRelayPin, LOW);  // LOW turns the Active-Low isolated relay ON
  } else {
    digitalWrite(lightRelayPin, HIGH); // HIGH turns the Active-Low isolated relay OFF
    digitalWrite(runningPin, LOW);
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
    Serial.print(" | LDR: "); Serial.print(ldrValue);
    Serial.print(isDarkOut ? " (Dark)" : " (Bright)");

    // Print Automation Output Status
    Serial.print(shouldBeOn ? " | LIGHT: ON" : " | LIGHT: OFF");
    Serial.print(manualOverride ? " (MANUAL)" : " (AUTO)  ");

    // Low-frequency Battery Monitoring
    if (millis() - lastBatteryCheck >= batteryCheckInterval || lastBatteryCheck == 0) {
      lastBatteryCheck = millis();
      int rawAnalog = analogRead(batteryPin);
      // float batteryVoltage = rawAnalog * (5.0 / 1023.0);
      
      // @Issue: After removing the "usb cable", still the red led (labeled as: ON, next to "R17") is blinking in arduino uno board.
      // Solution: Add a Signal Diode (Recommended)
      // Place a small standard diode (like a 1N4148 or 1N4007) in series along the jumper wire running from your battery to the A0 pin.
      // 1. Orient the diode so the anode (non-band side) connects to the battery, and the cathode (striped-band side) connects to the A0 pin.
      // 2. This allows the Arduino to read the voltage when it is powered on, but physically blocks current from flowing backward into the Arduino 
      //    when the main power is turned off.
      // @adjust: Add +0.6V (or the specific forward drop of your diode) to correct the math
      float batteryVoltage = (rawAnalog * (5.0 / 1023.0)) + 0.6;
      float batteryPercentage = constrain(((batteryVoltage - 2.0) / (3.0 - 2.0)) * 100.0, 0.0, 100.0);
      
      Serial.print(" | Batt: "); Serial.print(batteryVoltage, 2); 
      Serial.print("V ("); Serial.print(batteryPercentage, 0); Serial.println("%)");
    } else {
      Serial.println(" | Batt: [Cached]");
    }
  }
}

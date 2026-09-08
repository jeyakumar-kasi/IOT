

#include <Wire.h>
#include <RTClib.h>
#include "helpers.h"

const int RELAY_PIN = 8;

void setup() {
  _setup();
  pinMode(RELAY_PIN, OUTPUT);  
  digitalWrite(RELAY_PIN, HIGH); // OFF

  checkRTCStatus();
  setInitialRTCDateTime();
  printDateTime(getRTCNow()); Serial.println(" | Welcome!");  
  buzzer(3 * 1000); 
}

void loop() {
  // checkRTCStatus();
  DateTime now = getRTCNow();

  int hour = now.hour();
  if (hour >= 18 || hour <= 6) {
    print("Lights ON");
    digitalWrite(RED_LED, LOW);
    digitalWrite(GREEN_LED, HIGH);
    digitalWrite(RELAY_PIN, LOW); // ON

  } else {
    digitalWrite(RED_LED, HIGH);
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(RELAY_PIN, HIGH); // OFF
    print("Lights OFF");          
  }  
    
  delay(5000); // wait for 5 secs
}    

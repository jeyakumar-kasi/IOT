#include <RTClib.h>
#include <uEEPROMLib.h>

RTC_DS1307 rtc;


void write(const String &msg)
{
  Serial.print("Writing into RTC memory... \t");
  if (!eeprom.eeprom_write(0, (byte *) msg, strlen(msg))) {
    Serial.println("Failed.");    
  } else {
    Serial.println("Done.");        
  }  
}

void setup() {
  Serial.begin(9600);
  while (! Serial);

  write("100_2023-02-04_21:00");  

}

void loop() {
  // put your main code here, to run repeatedly:

}

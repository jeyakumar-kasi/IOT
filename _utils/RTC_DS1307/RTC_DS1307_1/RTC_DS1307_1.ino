#include "RTClib.h"

RTC_DS1307 rtc;

char daysOfTheWeek[7][12] = {
  "Sunday",
  "Monday",
  "Tuesday",
  "Wednesday",
  "Thursday",
  "Friday",
  "Saturday"
};

// String functions

char* substr(char* str, signed int totalChars) 
{
  char* newStr, * p1, *p2;
  newStr = (char*)malloc(totalChars);
  p1 = str;
  p2 = newStr;

  for (int i = 0; i < totalChars; i++) {
    *p2++ = *p1++;
  }

  *p2 = '\0';
  return newStr;
}
void printDateTime(DateTime d) 
{
  Serial.print(substr(daysOfTheWeek[d.dayOfTheWeek()], 3)); Serial.print(" ");
  Serial.print(d.year()); Serial.print("/"); Serial.print(d.month()); Serial.print("/"); Serial.print(d.day());Serial.print(" ");
  Serial.print(d.hour()); Serial.print(":"); Serial.print(d.minute()); Serial.print(":"); Serial.print(d.second());
}

void syncDateTime() 
{
  Serial.print("Sync. the time... ["); 
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  // OR
  //rtc.adjust(DateTime(2022, 6, 16, 22, 22, 0));
  printDateTime(rtc.now()); Serial.println("]");
}

void isSetupOk()
{
  // RTC
  bool isSetupOk = false;
  bool isAlerted = false;
  while (! isSetupOk) {
    if (! rtc.begin()) {
      if (! isAlerted) {
        isAlerted = true;
        Serial.println("RTC is not starting...");
      }
    } else if (! rtc.isrunning()) {
      if (! isAlerted) {
        isAlerted = true;
        Serial.println("RTC is not running...");
      }
    } else {
      isSetupOk = true;
    }

    // Check again after 100 ms.
    delay(100);
  }
  
  isAlerted = false;
  Serial.print("RTC is OK. ");
  printDateTime(rtc.now()); Serial.println("");
}


void setup() {
  
  Serial.begin(9600);
  while (!Serial);

  isSetupOk();
  syncDateTime();

}

void loop() {
  // put your main code here, to run repeatedly:
  delay(5000);
  isSetupOk();
}

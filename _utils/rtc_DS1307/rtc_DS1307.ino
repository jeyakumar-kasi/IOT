#include <Wire.h>
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

void syncDateTime() {
  Serial.println("Sync. the time...");
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  // --(OR)--
  //rtc.adjust(DateTime(2022, 5, 23, 13, 55, 50));
}

void printDateTime(DateTime d) {
  Serial.print(d.year(), DEC);  Serial.print("/");
  Serial.print(d.month(), DEC); Serial.print("/");
  Serial.print(d.day(), DEC);   Serial.print("(");
  Serial.print(daysOfTheWeek[d.dayOfTheWeek()]); Serial.print(") ");
  Serial.print(d.hour(), DEC);  Serial.print(":");
  Serial.print(d.minute(), DEC); Serial.print(":");
  Serial.println(d.second(), DEC);
}

DateTime getDateTime(signed int *addDays) {
  if (addDays) {
    return rtc.now() + TimeSpan(addDays, 0, 0, 0);
  }
  return rtc.now();
}

void setup() {
  //while (!Serial) {
    Serial.begin(9600);
    while (!Serial); // Wait for a Serial to begin
    Serial.println("Starting Serial...");
  //}

  Serial.println("RTC Program!");

  // RTC
  if (! rtc.begin()) {
    Serial.println("Couldn't start RTC!");
    while (1);
  }

  if (! rtc.isrunning()) {
    Serial.println("RTC is not running!");

    //! syncDateTime();
  }

}

void loop() {
  Serial.println("Looping...!");
  DateTime now = getDateTime(0);
  printDateTime(now);

  // Unix Time
  Serial.print("Since Mid-night: 1/1/1970 = "); Serial.print(now.unixtime());
  Serial.print("\t(In Secs): "); Serial.println(now.unixtime() / 86400L);

  // Next 7 days & 30 Secs?
  //DateTime d = now + TimeSpan(7, 12, 30, 6);
  DateTime d = now + TimeSpan(7, 0, 0, 0);
  printDateTime(d);
  delay(1000);
}

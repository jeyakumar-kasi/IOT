
#include <EEPROM.h>
#include <RTClib.h>
#include "uEEPROMLib.h"

// uEEPROMLib eeprom;
uEEPROMLib eeprom(0x50);

// RTC_DS3231 rtc;
RTC_DS1307 rtc;
DateTime initialRTCDateTime;
char daysOfTheWeek[7][12] = {
  "Sunday",
  "Monday",
  "Tuesday",
  "Wednesday",
  "Thursday",
  "Friday",
  "Saturday"
};

const int RED_LED = 7; // ERR
const int GREEN_LED = 6; // ALL OK
const int BLUE_LED = 5; // RUNNING
const int BUZZER_PIN = 4;
const int LIGHT_SENSOR_PIN = A0;

// Threshold for darkness (0 = pure darkness, 1023 = blinding light)
// Note: LDR => Leg 1 (5v), Leg 2 (A0) | 10k Resistor => Leg 1 (A0), Leg 2 (GND)
const int DARKNESS_THRESHOLD = 300; // @tune

// --------------------------- Strings ---------------------------
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

String split(String str, char delimiter, unsigned int pos)
{
  // Get matched string length
  unsigned int matchStrLen = 0;
  unsigned int matchStrStartPos = 0;
  unsigned int matchDelimiterCount = 0;
  for (int i = 0; i < str.length(); i++) {
    if (str[i] == delimiter) {
      if (++matchDelimiterCount == pos) {
        break;
      }

      // Reset match string info.
      matchStrLen = 0;
      matchStrStartPos = i+1; // Ignore current delimiter position.
    }
    else {
      matchStrLen++;
    }
  }

  // Create the matched String
  char* matchStr;
  matchStr = (char*)malloc(matchStrLen); // Include for "trailing space" as well.
  //printf("Start Position: %d\t", matchStrStartPos);
  //printf("Length: %d\n", matchStrLen);
  for (int i = matchStrStartPos, j=0; i < (matchStrStartPos+matchStrLen); i++,j++) {
    matchStr[j] = str[i];
  }

  // Add trailing space
  matchStr[matchStrLen] = '\0';
  return matchStr;
}


// --------------------------- EEPROM ---------------------------

void write(const String &msg, unsigned int pos=0)
{
  // Write the Length
  byte len = msg.length();
  EEPROM.write(pos, len);
  pos += 1;

  // Write the message
  for (int i = 0; i < len; i++) {
    EEPROM.write(pos + i, msg[i]);
  }
}

String read(int pos=0) 
{
  int len = EEPROM.read(pos);
  char msg[len+1]; // include 'pos' value

  // Read the message
  pos += 1;
  for (int i = 0; i < len; i++) {
    msg[i] = EEPROM.read(pos+i);
  }

  // Add trailing space
  msg[len] = '\0';
  return msg;
}

// --------------------------- DATE/TIME ---------------------------

String dateToStr(DateTime d)
{
  return String(d.year()) + "-" + String(d.month()) + "-" + String(d.day());
}

String dateTimeToStr(DateTime d) 
{
  /*DateTime now = getRTCNow();
  char format[] = "hh:mm:ss";          // or "hh:mm"
  char tm = now.toString(format);
  Serial.println(tm);*/

  return String(d.year()) + "-" + String(d.month()) + "-" + String(d.day()) + "_" + String(d.hour()) + ":" + String(d.minute()) + ":" + String(d.second());
}

DateTime strToDateTime(String s)
{
  String dateStr = split(s, '_', 1);
  String timeStr = split(s, '_', 2);
  return DateTime(
    split(dateStr, '-', 1).toInt(), // Year
    split(dateStr, '-', 2).toInt(), // Month
    split(dateStr, '-', 3).toInt(), // Day
    split(timeStr, ':', 1).toInt(), // Hour
    split(timeStr, ':', 2).toInt(), // Minute
    split(timeStr, ':', 3).toInt()  // Second
  );
}

TimeSpan millisToTimeSpan(long ms)
{
  int d = ms/ 86400000;
  int h = ms/ 3600000;  h %= 24;
  int m = ms/ 60000;    m %= 60;
  int s = ms/ 1000;     s %= 60;
  return TimeSpan(d, h, m, s);
}

void printDateTime(DateTime d) 
{
  Serial.print(substr(daysOfTheWeek[d.dayOfTheWeek()], 3)); Serial.print(" ");
  Serial.print(d.year()); Serial.print("/"); Serial.print(d.month()); Serial.print("/"); Serial.print(d.day());Serial.print(" ");
  Serial.print(d.hour()); Serial.print(":"); Serial.print(d.minute()); Serial.print(":"); Serial.print(d.second());
}

// --------------------------- RTC ---------------------------
void setInitialRTCDateTime()
{
    // Set the current RTC time in the variable to avoid pollig every time to RTC later.
    Serial.println("Setting up the RTC time...");
    initialRTCDateTime = rtc.now();//.unixtime();
}

DateTime getRTCNow() 
{
  long _millis = millis();
  long gracePeriodMillis = (long) 61 * 1000; // 61 secs

  if (_millis >= (long) (1294967295 - gracePeriodMillis)) { // 4294967295
    // millis() Rollover, Reset the time with current RTC time.
    setInitialRTCDateTime();
  }

  // Avoid pollig to RTC and calculate using millis.
  return initialRTCDateTime + millisToTimeSpan(_millis);
}

void checkRTCStatus()
{
  bool isSetupOk = false;
  bool isAlerted = false;
  while (! isSetupOk) {
    if (! rtc.begin()) {
      digitalWrite(RED_LED, HIGH);
      if (! isAlerted) {
        isAlerted = true;
        Serial.println("RTC is not starting...");
      }
    } else if (! rtc.isrunning()) {
      digitalWrite(RED_LED, HIGH);
      if (! isAlerted) {
        isAlerted = true;
        Serial.println("RTC is not running/lost power, setting the time...");
        // Sets the RTC to the exact date & time this sketch was compiled
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));        
      }
    } else if ((int) rtc.now().year() < 2022 ) {
      // Blink danger light
      digitalWrite(RED_LED, HIGH);
      if (! isAlerted) {
        isAlerted = true;
        Serial.println("RTC is not sending a valid date.");
      }
    } else {
      isSetupOk = true;
      digitalWrite(RED_LED, LOW);
      Serial.println("RTC is OK.");
    }

    // Check again after 100 ms.
    delay(100);
  }
  isAlerted = false;
}

// void writeRTC(const char c_string[]) {
void writeRTC(const String &msg) {
  int string_length = msg.length() + 1;
  char c_string[string_length];
  msg.toCharArray(c_string, string_length);

  // char c_string[] = "100_2023-02-04_12:0:0"; 
  // int string_length = strlen(c_string);  
  if (! eeprom.eeprom_write(0, string_length)) {
    Serial.println("Failed to store the Length.");
  } else {
    // Write a long string of chars FROM position 10 which isn't aligned to the 32 byte pages of the EEPROM
    if (! eeprom.eeprom_write(10, (byte *) c_string, string_length)) {
      Serial.println("Failed to store string.");
    }
  }  
}

String readRTC()
{
  int len = 0; 
  eeprom.eeprom_read(0, &len);
  // Serial.println(len);

  // Serial.print("string: ");
  char s[len];
  eeprom.eeprom_read(10, (byte *) s, len); 
  char msg[len];
  for (int i = 0; i < len; i++) {
    msg[i] = s[i];
  }

  // Add trailing space
  msg[len] = '\0';
  return msg;
}

// --------------------------- Sound ---------------------------

void buzzer(long ms)
{
  delay(1000);
  tone(BUZZER_PIN, 1000); // Send 1kHz sound signal
  delay(ms);
  noTone(BUZZER_PIN);
  delay(1000);
}

// --------------------------- LIGHT ---------------------------

int _getLightLevel() {
  return analogRead(LIGHT_SENSOR_PIN);
}

bool isDark() {
  int lightLevel = _getLightLevel();
  return lightLevel < DARKNESS_THRESHOLD; 
}

// --------------------------- Others ---------------------------
void _setup() {
  Serial.begin(9600);
  while (! Serial);

  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);
}

void print(String message) {
  printDateTime(getRTCNow()); Serial.println(":" + message);  
}


#include <Servo.h>
#include <EEPROM.h>
#include "RTClib.h"

bool isResetEEPROM = false;        // Important: Must be "false" at production time.

bool isForceRun = false;
bool isMotorRunning = false;
const long interval = (long) 3 * 24 * 60 * 60 * 1000; // in millis (3 days)
const long motorRunningTime = (long) 60 * 1000; //(long) 2 * 60 * 60 * 1000; // in millis (2 Hrs)
const float lastRanThresholdPercent = 60.0; // Percent to Re-run check after arduino restart.

// --------------------- Declarations -------------------------

Servo servo1;
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

const int redLedPin = 7;
const int greenLedPin = 6; 
const int motorCtrlPin = 8;

// ------------------- Helper functions -----------------------

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

String split(String str, char* delimiter, unsigned int pos)
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


void syncDateTime() 
{
  Serial.print("Sync. the time... ["); printDateTime(rtc.now()); Serial.println("]");
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  // OR
  //rtc.adjust(DateTime(2022, 6, 16, 22, 22, 0));
}

DateTime getDateTime(signed int *days) 
{
  if (days) {
    // Add or Reduce the days.
    return rtc.now() + TimeSpan(days, 0, 0, 0);
  } else {
    return rtc.now();
  }
}

int getDaysDiff(DateTime d1, DateTime d2) 
{
  return ceil((d2.unixtime() - d1.unixtime())/60/60/24); 
}

long dateTimeToMillis(DateTime d, signed long *ms) 
{
  if (ms) {
    // Add or subtract millis
    return d.unixtime() + ms;
  }
  return d.unixtime();
}

void printDateTime(DateTime d) 
{
  Serial.print(substr(daysOfTheWeek[d.dayOfTheWeek()], 3)); Serial.print(" ");
  Serial.print(d.year()); Serial.print("/"); Serial.print(d.month()); Serial.print("/"); Serial.print(d.day());Serial.print(" ");
  Serial.print(d.hour()); Serial.print(":"); Serial.print(d.minute()); Serial.print(":"); Serial.print(d.second());
}

// EEPROM
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

String dateTimeToStr(DateTime d) 
{
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

// ------------------- Custom functions -----------------------

void resetEEPROM() {
  Serial.println("Resetting the EEPROM...");
  write("", 0);
}

void startMotor() 
{
  // Reset force run state.
  isForceRun = false; 
  if (! isMotorRunning) {
    isMotorRunning = true;
    DateTime now = getDateTime(0);
    //printDateTime(now);
    write("0_" + dateTimeToStr(now), 0); // Last Started time.
    Serial.println(" Running the Motor...");
    digitalWrite(redLedPin, LOW);
    digitalWrite(greenLedPin, HIGH);
    servo1.write(90);
  } else {
    Serial.println("Motor is already running now...");
  }
}

void stopMotor() 
{
  if (isMotorRunning) {
    isMotorRunning = false;
    servo1.write(0);

    digitalWrite(greenLedPin, LOW);
    digitalWrite(redLedPin, HIGH);
    DateTime now = getDateTime(0);
    //printDateTime(now); 
    write("1_" + dateTimeToStr(now), 0); // Last Completed time.
    Serial.println(" Motor is stopped.");
  } else {
    Serial.println("Motor is not running now!");
  }
}


void runMotor() 
{
  startMotor();
  delay(motorRunningTime);
  stopMotor();
  delay(interval); 
}

float lastRanPercent(DateTime lastRanDateTime) {
  //Serial.print("Last Ran TIme(ms): "); Serial.println(lastRanDateTime.unixtime()); //printDateTime(lastRanDateTime);
  int modTime = ((rtc.now().unixtime() - lastRanDateTime.unixtime()) % 86400000) % motorRunningTime; // 1300
  //Serial.println(modTime); 
  return (100.00/(float) motorRunningTime) * (float) (motorRunningTime - modTime);
}

bool isElapsedInterval(DateTime lastRanDateTime)
{
  return (rtc.now().unixtime() - lastRanDateTime.unixtime()) >= (interval/1000);
}

bool isNeedForceRun() 
{
  // Read the last ran state from EEPROM.
  String lastRanStateStr = (String) "";//0_2022-06-22_12:0:10"; //!read(0);
  if (lastRanStateStr && lastRanStateStr != "") {
    String lastRanDateStr = split(lastRanStateStr, '_', 2);
    String lastRanTimeStr = split(lastRanStateStr, '_', 3);
    DateTime lastRanDateTime = strToDateTime(lastRanDateStr + "_" + lastRanTimeStr);

    if(isElapsedInterval(lastRanDateTime)) {
      // it's too long from its last ran time.
      Serial.println("Already elaspsed the time from its last run.");
      return true;
    } else if (split(lastRanStateStr, '_', 1) == "0") {
      // Last run was not successful.
      // Compare the last running time with threshold level.
      float lastRanPercentVal = lastRanPercent(lastRanDateTime);
      Serial.print("Last Ran Percent: "); Serial.println(lastRanPercentVal);
      if (lastRanPercentVal < lastRanThresholdPercent) {
        // Need to run again.
        return true;
      }
    }
  }
  return false;
}

void setup() 
{
  Serial.begin(9600);
  while (!Serial);

  if (isResetEEPROM) {
    resetEEPROM();
    Serial.println("EEPROM is resetted successfully. Exiting...!");
    while(1);
  } 

  // RTC
  if (! rtc.begin()) {
    Serial.println("RTC is not starting, Exiting...!");
    while(1);
  } else if (! rtc.isrunning()) {
    Serial.println("RTC is not running!");
  }
  // Sync Datetime
  syncDateTime();
  
  Serial.println("Welcome to Watering Project!");
  pinMode(redLedPin, OUTPUT);
  pinMode(greenLedPin, OUTPUT);
  pinMode(motorCtrlPin, OUTPUT);
  servo1.attach(motorCtrlPin);
  if (! isForceRun) {
    // Check the prev. running state
    if(isNeedForceRun()) {
      runMotor();
    } else {
      // wait for next routine.
      delay(interval);
    }
  }
}

void loop() 
{
  runMotor(); 
}

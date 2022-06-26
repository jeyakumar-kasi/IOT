#include <EEPROM.h>
#include <Servo.h>
#include <RTClib.h>

bool isResetApp = false;        // Important: Must be "false" at production time.

const String runEveryDayAt = "22:06"; // HH:MM
const long intervalTime = (long) 1 * 60 * 60 * 1000; //(long) 3 * 24 * 60 * 60 * 1000; // in millis (3 days)
const long motorRunningTime = (long) 10 * 60 * 1000; //(long) 2 * 60 * 60 * 1000; // in millis (2 Hrs)
const float lastRanThresholdPercent = 60.0; // Percent to Re-run check after arduino restart.
// --------------------------------------------------------

RTC_DS1307 rtc;
DateTime nextRunAtDateTime;
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

const int redLedPin = 7;
const int greenLedPin = 6; 
const int motorCtrlPin = 8;


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

// Time
TimeSpan millisToTimeSpan(long ms)
{
  int d = ms/ (24 * 60 * 60 * 1000);
  int h = ms/ (60 * 60 * 1000);       h %= 24;
  int m = ms/ (60 * 1000);            m %= 60;
  int s = ms/ (1000);                 s %= 60;
  return TimeSpan(d, h, m, s);
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

void syncDateTime() 
{
  Serial.print("Sync. the time... ["); 
  //!rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  // OR
  //rtc.adjust(DateTime(2022, 6, 16, 22, 22, 0));
  printDateTime(rtc.now()); Serial.println("]");
}

// ----------------------------------------------------------------

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

void setInitialRTCDateTime()
{
    // Set the current RTC time in the variable to avoid pollig every time to RTC later.
    initialRTCDateTime = rtc.now();//.unixtime();
}

DateTime getRTCNow() 
{
  long _millis = millis();
  long gracePeriodMillis = (long) 61 * 1000; // 61 secs

  if (_millis >= (long) (4294967295 - gracePeriodMillis)) {
    // millis() Rollover, Reset the time with current RTC time.
    setInitialRTCDateTime();
  }

  // Avoid pollig to RTC and calculate using millis.
  /*int d = floor(_millis / 86400 * 1000);
  int h = floor(d / 3600 * 1000);
  int m = floor(h / 60 * 1000);
  int s = floor(m / 60 * 1000);*/
  return initialRTCDateTime + millisToTimeSpan(_millis);
}

bool isTodayRunPossible()
{
  // Check current time is exceeded or not.
  int H = split(runEveryDayAt, ':', 1).toInt();
  int M = split(runEveryDayAt, ':', 2).toInt();
  //int S = split(runEveryDayAt, ':', 3).toInt();

  return ((int) getRTCNow().hour() < H) || 
         ((int) getRTCNow().hour() == H && (int) getRTCNow().minute() <= M); //@check
}

DateTime nextPossibleDay()
{
  if (isTodayRunPossible()) {
    // Run on today.
    return strToDateTime(dateToStr(getRTCNow()) + "_" + runEveryDayAt);
  } else {
    // Take it on tomorrow.
    DateTime tomorrowDateTime = getRTCNow() + TimeSpan(1, 0, 0, 0);
    return strToDateTime(dateToStr(tomorrowDateTime) + "_" + runEveryDayAt);
  }
}


DateTime getNextRunDateTime()
{
  // Read the last ran state from EEPROM.
  String lastRanStateStr = (String) read(0); //"59.1_2022-06-28_12:0:10"; 

  if (lastRanStateStr && lastRanStateStr != "") {
    float lastRanStateLevel = split(lastRanStateStr, '_', 1).toFloat(); // 0 -> 0.5 -> 1

    if (lastRanStateLevel <= (float) lastRanThresholdPercent) {
      Serial.print(lastRanStateLevel); Serial.println(" | Partially Ran, Run it again on next immedidate possible date.");
      return nextPossibleDay();
    } else {
      String lastRanDateStr = split(lastRanStateStr, '_', 2);
      //String lastRanTimeStr = split(lastRanStateStr, '_', 3);
      DateTime lastRanDateTime = strToDateTime(lastRanDateStr + "_" + runEveryDayAt);
      return lastRanDateTime + millisToTimeSpan(intervalTime); //TimeSpan(3, 0, 0, 0);
    }
  } else {
    // Check today is possible or take it on tomorrow.
    return nextPossibleDay();
  }
}

// ---------------------------------------------------------------

void printDateTime(DateTime d) 
{
  Serial.print(substr(daysOfTheWeek[d.dayOfTheWeek()], 3)); Serial.print(" ");
  Serial.print(d.year()); Serial.print("/"); Serial.print(d.month()); Serial.print("/"); Serial.print(d.day());Serial.print(" ");
  Serial.print(d.hour()); Serial.print(":"); Serial.print(d.minute()); Serial.print(":"); Serial.print(d.second());
}


// --------------------------------------------------------------

void resetEEPROM() {
  Serial.println("Resetting the EEPROM...");
  write("", 0);
}

void await_motorRunningTime()
{
  long _t = millis();
  bool isThresholdUpdated = false;
  while (1) {
    if ((millis() - _t) >= motorRunningTime) {
      // Exit the loop
      break;
    }

    // Motor is runnig...

    // Update motor's current running percentage time in EEPROM.
    if (! isThresholdUpdated) {
      float tillRanPercent = (float) (100.0/motorRunningTime) * (millis() - _t);
      if (tillRanPercent >= lastRanThresholdPercent) {
        // Update in EEPROM.
        Serial.print(tillRanPercent); Serial.println(" | Updating the threshold percent..."); 
        write(String(tillRanPercent) + "_" + dateTimeToStr(getRTCNow()), 0);
        isThresholdUpdated = true;
      }
    }

    delay(1000);
  }
}

void runMotor()
{
  Serial.println("Motor - ON");
  DateTime now = getRTCNow();
  write("0_" + dateTimeToStr(now), 0);

  //@servo1.write(90);
  digitalWrite(motorCtrlPin, LOW);
  digitalWrite(redLedPin, LOW);
  digitalWrite(greenLedPin, HIGH);

  // Wait for the motor completes its running state.
  await_motorRunningTime();

  //@servo1.write(0);
  digitalWrite(motorCtrlPin, HIGH);
  digitalWrite(greenLedPin, LOW);
  digitalWrite(redLedPin, HIGH);

  Serial.println("Motor - OFF");
  now = getRTCNow();
  write("1_" + dateTimeToStr(now), 0);

  // Update "Next Run" date & time
  nextRunAtDateTime = getNextRunDateTime();
}

void setup() {
  Serial.begin(9600);
  while (!Serial);

  // RTC
  if (! rtc.begin()) {
    Serial.println("RTC is not starting...");
    while (1);
  } else if (! rtc.isrunning()) {
    Serial.println("RTC is not running...");
    while (1);
  }

  if (isResetApp) {
    resetEEPROM();

    // Sync Datetime
    syncDateTime();
    Serial.println("Reset is done. Exiting...!");
    while(1);
  }

  // Set initial RTC time
  setInitialRTCDateTime();

  printDateTime(getRTCNow()); Serial.println(" | Welcome!");

  // Update "Next Run" date & time
  nextRunAtDateTime = getNextRunDateTime();

  pinMode(redLedPin, OUTPUT);
  pinMode(greenLedPin, OUTPUT);
  pinMode(motorCtrlPin, OUTPUT);
  //@servo1.attach(motorCtrlPin);
}

void loop() {
  printDateTime(nextRunAtDateTime); Serial.println(" | Next Run Time..");

  if ((int) getRTCNow().day() == (int) nextRunAtDateTime.day()) {
    // Day
    if ((int) getRTCNow().hour() == (int) nextRunAtDateTime.hour()) {
      // Hour
      if ((int) getRTCNow().minute() == (int) nextRunAtDateTime.minute()) { 
        // Minute
        runMotor();
      } 
    } else {
      Serial.println("Wait for a minute...");
      delay(57 * 1000); 
    }
  } else {
    Serial.println("Wait for an hour...");
    delay(57 * 60 * 1000); 
  }

  // Default timeout.
  delay(27 * 1000); // Wait for at least 30 secs.
}

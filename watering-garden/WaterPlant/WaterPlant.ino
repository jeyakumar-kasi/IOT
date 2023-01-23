
#include <EEPROM.h>
#include <RTClib.h>


const int redLedPin = 7;
const int blueLedPin = 6; 
const int motorCtrlPin = 8;
const String runEveryDayAt = "08:00"; // HH:MM
const long intervalTime = (long) 1 * 60 * 60 * 1000; //(long) 3 * 24 * 60 * 60 * 1000; // in millis (3 days)
const long motorRunningTime = (long) 10 * 60 * 1000; //(long) 2 * 60 * 60 * 1000; // in millis (2 Hrs)
const float lastRanThresholdPercent = 60.0; // Percent to Re-run check after arduino restart.

DateTime initialRTCDateTime;
DateTime nextRunAtDateTime;

// ------------------------------------------------------

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

// ----------------------[ Utils ]--------------------------------
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

// Strings
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

// ------------------------[ helpers ]------------------------------

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

void printDateTime(DateTime d) 
{
  Serial.print(substr(daysOfTheWeek[d.dayOfTheWeek()], 3)); Serial.print(" ");
  Serial.print(d.year()); Serial.print("/"); Serial.print(d.month()); Serial.print("/"); Serial.print(d.day());Serial.print(" ");
  Serial.print(d.hour()); Serial.print(":"); Serial.print(d.minute()); Serial.print(":"); Serial.print(d.second());
}


// ----------------------[ functions ]--------------------------------

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
    float lastRanStateLevel = split(lastRanStateStr, '_', 1).toFloat(); // 0 -> 50.0 -> 100

    if (lastRanStateLevel <= (float) lastRanThresholdPercent) {
      Serial.print(lastRanStateLevel); Serial.println(" | Partially Ran, Run it again on next immedidate possible date.");
      return nextPossibleDay();
    } else {
      String lastRanDateStr = split(lastRanStateStr, '_', 2);
      String lastRanTimeStr = split(lastRanStateStr, '_', 3);
      DateTime lastRanDateTime = strToDateTime(lastRanDateStr + "_" + lastRanTimeStr);
      DateTime nextPossibleRanDateTime = lastRanDateTime + millisToTimeSpan(intervalTime); //TimeSpan(3, 0, 0, 0);
      return strToDateTime(dateToStr(nextPossibleRanDateTime) + "_" + runEveryDayAt);
    }
  } else {
    // Check today is possible or take it on tomorrow.
    return nextPossibleDay();
  }
}

// ---------------------[ app ]---------------------------------

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
  digitalWrite(blueLedPin, HIGH);

  // Wait for the motor completes its running state.
  await_motorRunningTime();

  //@servo1.write(0);
  digitalWrite(motorCtrlPin, HIGH);
  digitalWrite(blueLedPin, LOW);

  Serial.println("Motor - OFF");
  now = getRTCNow();
  write("100_" + dateTimeToStr(now), 0);

  // Update "Next Run" date & time
  nextRunAtDateTime = getNextRunDateTime();
}


// ------------------------------------------------------

void checkRTCStatus()
{
  bool isSetupOk = false;
  bool isAlerted = false;
  while (! isSetupOk) {
    if (! rtc.begin()) {
      digitalWrite(redLedPin, HIGH);      
      if (! isAlerted) {
        isAlerted = true;
        Serial.println("RTC is not starting...");
      }
    } else if (! rtc.isrunning()) {
      digitalWrite(redLedPin, HIGH);
      if (! isAlerted) {
        isAlerted = true;
        Serial.println("RTC is not running...");
      }
    } else if ((int) rtc.now().year() < 2022 ) {
      // Blink danger light
      digitalWrite(redLedPin, HIGH);
      if (! isAlerted) {
        isAlerted = true;
        Serial.println("RTC is not sending a valid date.");
      }
    } else {
      isSetupOk = true;
      digitalWrite(redLedPin, LOW);
      Serial.println("RTC is OK.");
    }

    // Check again after 100 ms.
    delay(100);
  }
  isAlerted = false;
}

void setup() {
  Serial.begin(9600);
  while (!Serial);
  
  pinMode(redLedPin, OUTPUT);
  pinMode(blueLedPin, OUTPUT);
  pinMode(motorCtrlPin, OUTPUT);

  // RTC
  rtc.begin();
  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  // rtc.adjust(DateTime(2023,1,23, 23, 59, 55)); //!
  checkRTCStatus();
  setInitialRTCDateTime();
  printDateTime(getRTCNow()); Serial.println(" | Welcome!");  

  // Update "Next Run" date & time
  nextRunAtDateTime = getNextRunDateTime();
}



void loop() {
    // checkRTCStatus();
    printDateTime(getRTCNow()); Serial.print(" | Next Run: "); 
    printDateTime(nextRunAtDateTime); Serial.println();  

    if ((int) getRTCNow().day() == (int) nextRunAtDateTime.day()) {
      if ((int) getRTCNow().hour() == (int) nextRunAtDateTime.hour()) {
        if ((int) getRTCNow().minute() == (int) nextRunAtDateTime.minute()) {
          Serial.println("Start to run the Motor...");
          runMotor();
        }
      }
    }

    delay(1000);
}

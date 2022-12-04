#include <RTClib.h>

RTC_DS1307 rtc;

char buf1[20];

const int redLedPin = 7;

void checkRTCStatus()
{
  bool isSetupOk = false;
  bool isAlerted = false;
  while (! isSetupOk) {
    if (! rtc.begin()) {
      // Blink danger light
      digitalWrite(redLedPin, HIGH);
      if (! isAlerted) {
        // isAlerted = true;
        Serial.println("RTC is not starting...");
      }
    } else if (! rtc.isrunning()) {
      // Blink danger light
      digitalWrite(redLedPin, HIGH);
      if (! isAlerted) {
        // isAlerted = true;
        Serial.println("RTC is not running...");
      }
    } else if ((int) rtc.now().year() < 2022 ) {
      // Blink danger light
      digitalWrite(redLedPin, HIGH);
      if (! isAlerted) {
        isAlerted = true;
        Serial.println("RTC is not sending a valid date.");
        rtc.adjust(DateTime(F(__DATE__),F(__TIME__)));
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
  pinMode(redLedPin, OUTPUT);
  Serial.begin(9600);
  while (!Serial) {
    digitalWrite(redLedPin, HIGH);
    delay(1000);
    digitalWrite(redLedPin, LOW);
    delay(500);
  };

  
  checkRTCStatus();

  
}


void loop()
{

  checkRTCStatus();
  DateTime now = rtc.now();

  sprintf(buf1, "%02d:%02d:%02d %02d/%02d/%02d",  now.hour(), now.minute(), now.second(), now.day(), now.month(), now.year());  
  
  Serial.print(F("Date/Time: "));
  Serial.println(buf1);

  delay(1000);
}


// void loop() {
//   // put your main code here, to run repeatedly:
//   checkRTCStatus();
//   delay(1000);
// }

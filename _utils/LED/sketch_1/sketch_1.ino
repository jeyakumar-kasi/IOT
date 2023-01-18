
#include <RTClib.h>

RTC_DS1307 rtc;

const int redLedPin = 7;
const int blueLedPin = 5;

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
    // } else if ((int) rtc.now().year() < 2022 ) {
    //   // Blink danger light
    //   digitalWrite(redLedPin, HIGH);
    //   if (! isAlerted) {
    //     isAlerted = true;
    //     Serial.println("RTC is not sending a valid date.");
    //     rtc.adjust(DateTime(F(__DATE__),F(__TIME__)));
    //   }
    // } 
    }else {
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
  digitalWrite(redLedPin, HIGH);
  Serial.println("Welcome");
}

void loop() {
  delay(1000);
  Serial.println("LED OFF");
  digitalWrite(redLedPin, HIGH);
  digitalWrite(blueLedPin, LOW);
  delay(1000);
  Serial.println("LED ON");
  digitalWrite(redLedPin, LOW);
  digitalWrite(blueLedPin, HIGH);
  checkRTCStatus();
}

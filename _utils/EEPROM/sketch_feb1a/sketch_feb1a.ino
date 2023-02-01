#include <EEPROM.h>
#include <RTClib.h>

const int redLedPin = 7;

RTC_DS1307 rtc;
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


void setup() {
  Serial.begin(9600);
  while (! Serial);

  // RTC
  rtc.begin();
  //rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  
  Serial.print("Writing ...");
  write("100_2023-01-01_21:30:00", 0);
  Serial.println(" Done.");

  Serial.print("Reading ...");
  String msg = (String) read(0);
  Serial.println(msg);

}

void loop() {
  // put your main code here, to run repeatedly:

}

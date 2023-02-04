#include "uEEPROMLib.h"
#include <EEPROM.h>

// uEEPROMLib eeprom;
uEEPROMLib eeprom(0x50);



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

// void writeRTC(const char c_string[]) {
void writeRTC(const String &msg) {
  int string_length = msg.length() + 1;
  char c_string[string_length];
  msg.toCharArray(c_string, string_length);
    
  // char c_string[] = "100_2023-02-04_12:0:0"; //23
  // char c_string[string_length] = msg.c_str();
  if (eeprom.eeprom_write(0, string_length)) {
    Serial.println("int was stored correctly.");
    
    // Write a long string of chars FROM position 33 which isn't aligned to the 32 byte pages of the EEPROM
    if (eeprom.eeprom_write(10, (byte *) c_string, string_length)) {
      Serial.println("string was stored correctly.");
    } else {
      Serial.println("Failed to store string.");
    }
    
  } else {
    Serial.println("Failed to store int.");
    
  }  
}
String readRTC()
{
  int len = 0; 
  eeprom.eeprom_read(0, &len);
  Serial.println(len);

  Serial.print("string: ");
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

void setup() {
  Serial.begin(9600);
  delay(2500);

  // Wire.begin();

  int inttmp = 23;
  float floattmp = 3.1416;
  char chartmp = 'A';
  char c_string[] = "100_2023-02-04_12:0:0"; //23
  int string_length = strlen(c_string);

  // Serial.println("Writing into memory...");
  
  // // Write single char at address 
  // if (!eeprom.eeprom_write(8, chartmp)) {
  // Serial.println("Failed to store char.");
  // } else {
  // Serial.println("char was stored correctly.");
  // }

  //! int len = strlen(c_string);
  // eeprom.eeprom_write(0, 23);
  // Serial.print("Length: "); Serial.println(len); 
    
  

  // Write an int
  // if (eeprom.eeprom_write(0, string_length)) {
  //   Serial.println("int was stored correctly.");
    
  //   // Write a long string of chars FROM position 33 which isn't aligned to the 32 byte pages of the EEPROM
  //   if (eeprom.eeprom_write(10, (byte *) c_string, string_length)) {
  //     Serial.println("string was stored correctly.");
  //   } else {
  //     Serial.println("Failed to store string.");
  //   }
    
  // } else {
  //   Serial.println("Failed to store int.");
    
  // }

  

  // // write a float
  // if (!eeprom.eeprom_write(4, floattmp)) {
  //   Serial.println("Failed to store float.");
  // } else {
  //   Serial.println("float was stored correctly.");
  // }

  // Serial.println("");
  // Serial.println("Reading memory...");
  
  //! Serial.print("int: ");
  // eeprom.eeprom_read(0, &len);
  // Serial.println((int) len);

  // writeRTC("60.1_2023-02-04_12:0:0");
  writeRTC(String(100) + "_2023-02-04_12:30:00");
  String data = (String) readRTC(); 
  Serial.println(data);
 
 
  // Serial.print("float: ");
  // eeprom.eeprom_read(4, &floattmp);
  // Serial.println((float) floattmp);

  // Serial.print("char: ");
  // eeprom.eeprom_read(8, &chartmp);
  // Serial.println(chartmp);

    
  // Serial.print("string: ");
  // char s[string_length];
  // eeprom.eeprom_read(1, (byte *) s, string_length);
  // Serial.println(s);
  // readRTC();
  // Serial.println();

  // Serial.println("EEPROM: ");
  // msg = (String) read(0);  
}

void loop() {
}
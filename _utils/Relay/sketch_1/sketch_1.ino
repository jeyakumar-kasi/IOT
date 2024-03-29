

const int motorCtrlPin = 8;

void setup() {
  Serial.begin(9600);
  while (! Serial);

  pinMode(motorCtrlPin, OUTPUT);
  digitalWrite(motorCtrlPin, HIGH); // OFF
}

void loop() {
  delay(30 * 1000);
  Serial.println("Motor - ON");
  digitalWrite(motorCtrlPin, LOW);
  //delay((long) 30000); // 10 mins
  delay(15 * 1000);
  
  Serial.println("Motor - OFF");
  digitalWrite(motorCtrlPin, HIGH);
  //delay((long) 1 * 60000); // 10 mins
  delay(30 * 1000);
}

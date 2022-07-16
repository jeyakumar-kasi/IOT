

const int motorCtrlPin = 8;

void setup() {
  Serial.begin(9600);
  while (! Serial);

  pinMode(motorCtrlPin, OUTPUT);
}

void loop() {
  Serial.println("Motor - ON");
  digitalWrite(motorCtrlPin, LOW);
  delay((long) 30000); // 10 mins

  
  digitalWrite(motorCtrlPin, HIGH);
  Serial.println("Motor - OFF");
  delay((long) 1 * 60000); // 10 mins
}

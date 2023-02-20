

const int motorCtrlPin = 8;

void setup() {
  Serial.begin(9600);
  while (!Serial);

  
  pinMode(motorCtrlPin, OUTPUT);
  Serial.println("Welcome | Testing 12v Relay.");
  delay(1000);
}

void loop() {
  
  Serial.println("Motor - ON");
  digitalWrite(motorCtrlPin, LOW);
  delay(5000);
  Serial.println("Motor - OFF");
  digitalWrite(motorCtrlPin, HIGH);
  delay(5000);  


}

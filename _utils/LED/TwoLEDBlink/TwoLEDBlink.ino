
const int redLedPin = 7;
const int blueLedPin = 6;


void setup() {
  pinMode(redLedPin, OUTPUT);
  pinMode(blueLedPin, OUTPUT);
}

void loop() {
  digitalWrite(redLedPin, LOW);
  digitalWrite(blueLedPin, HIGH);
  delay(1000);
  digitalWrite(blueLedPin, LOW);
  digitalWrite(redLedPin, HIGH);
  delay(1000);
}

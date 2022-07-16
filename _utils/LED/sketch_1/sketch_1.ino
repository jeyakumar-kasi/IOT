

const int greenLedPin = 7;

void setup() {
  Serial.begin(9600);
  while (!Serial);

  pinMode(greenLedPin, OUTPUT);
  digitalWrite(greenLedPin, HIGH);
  Serial.println("Welcome");
}

void loop() {
  delay(1000);
  Serial.println("LED OFF");
  digitalWrite(greenLedPin, LOW);
  delay(1000);
  Serial.println("LED ON");
  digitalWrite(greenLedPin, HIGH);
}

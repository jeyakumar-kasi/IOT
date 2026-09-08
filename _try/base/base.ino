

const int redLedPin = 7;
const int greenLedPin = 6;
const int buzzerPin = 5;


void setup() {
  // Serial.begin(9600);
  // while (!Serial);

  // put your setup code here, to run once:
  pinMode(redLedPin, OUTPUT);
  pinMode(greenLedPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);

  // Serial.println("Started.");
  delay(1000);
}

void alarm(long ms) {
  tone(buzzerPin, 1000); // Send 1KHz sound signal
  delay(ms);
  noTone(buzzerPin);
}

void loop() {
  // put your main code here, to run repeatedly:
  digitalWrite(greenLedPin, HIGH);
  digitalWrite(redLedPin, LOW);
  //alarm(2000);
  delay(500);
  digitalWrite(greenLedPin, LOW);
  digitalWrite(redLedPin, HIGH);
  delay(500);
}

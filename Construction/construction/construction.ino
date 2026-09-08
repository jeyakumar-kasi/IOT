

const int LedPin = 7; 
const int RelayPin = 8;
const int buzzerPin = 5;
const long interval = 10 * 1000; // 10 mins
const long runningTime =  5 * 1000; // 1 min


void setup() {
  pinMode(LedPin, OUTPUT);

  // put your setup code here, to run once:
  Serial.begin(9600);
  
  // Wait for Serial to begin
  while(! Serial) {;} 
}

void buzzer(long ms)
{
  delay(1000);
  tone(buzzerPin, 1000); // Send 1kHz sound signal
  delay(ms);
  noTone(buzzerPin);
  delay(1000);
}

void run() {
  Serial.println("Running...");
  buzzer(1000);
  digitalWrite(LedPin, HIGH);
  digitalWrite(RelayPin, LOW);  // ON
  
  // Running
  delay(runningTime);

  Serial.println("Stopped.");
  buzzer(1000);
  digitalWrite(RelayPin, HIGH); // OFF
  digitalWrite(LedPin, LOW);
}

void loop() {
  // put your main code here, to run repeatedly:
  run();
  delay(interval);
}

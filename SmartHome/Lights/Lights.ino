
const int LDR_PIN = A0;
const int RED_LED = 7;
const int GREEN_LED = 6;
const int RELAY_PIN = 8;

const int LIGHT_THRESHOLD = 300;

int _getLightLevel() {
  return analogRead(LDR_PIN);
}

bool isDark() {
  int lightLevel = _getLightLevel();
  Serial.print("Light: "); Serial.println(lightLevel);
  return lightLevel < LIGHT_THRESHOLD;
}

void setup() {
  Serial.begin(9600);
  while (!Serial);

  digitalWrite(RELAY_PIN, HIGH); // OFF
}

void loop() {
  if (isDark()) {
    // Night - Switch ON
    digitalWrite(RED_LED, LOW);
    digitalWrite(GREEN_LED, HIGH);
    digitalWrite(RELAY_PIN, LOW); // ON        
  } else {
    digitalWrite(RED_LED, HIGH);
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(RELAY_PIN, HIGH); // OFF
  }
  delay(1000);
}

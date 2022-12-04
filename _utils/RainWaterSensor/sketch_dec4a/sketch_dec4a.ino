
//typedef enum {false=0, true} bool;

bool isRaining() {
  return analogRead(A0) < 1023;
}

float getRainingPercent() {
  Serial.print(analogRead(A0)); Serial.print(" => \t");
  if (analogRead(A0) < 1023) {
    // Raining.
    int sum = 0;
    for (int i = 0; i < 5; i++) {
      delay(500);
      sum += analogRead(A0);
    }
    float mean = (float) sum / 5.0;
    // Find the percent.
    return 100.0 - ((100.0/1023.0) * mean);
  }
  return 0.0;
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  while (!Serial);
}

void loop() {
  float rainPercent = getRainingPercent();
  
  if (rainPercent > 75) {
    Serial.print("Heavy Raining.");
  } else if (rainPercent > 50) {
    Serial.print("Medium");
  } else if (rainPercent > 25) {
    Serial.print("Low");
  } else {
    Serial.print("NO rain!!!");
  }
  Serial.println(" (" + (String) rainPercent +" %)");

  delay(1000);
}

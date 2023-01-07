#include <Arduino.h>

void setup() {
  Serial.begin(9600);
}

void loop() {
  for (int i=0; i<8; i++) {
//    analogReference(DEFAULT); //sets the voltage reference to the internal reference (~5.0v)
    analogReference(EXTERNAL); //sets the voltage reference to the internal reference (~3.3v)
    int value = analogRead(A0+i);
    Serial.print(value);
    Serial.print("\t");
  }
  Serial.print("\n");
  delay(100);
}

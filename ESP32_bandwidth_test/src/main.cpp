#include <Wire.h>
#include <Arduino.h>

unsigned long duration  = 60000;  //milliseconds
int OUTPUT_PIN          = 25;     //DAC_1

void setup(void) {

  Serial.begin(115200);
  delay(500);
  Serial.println("Frequency,Amplitude ");

  //setup an array of sine values to be output
  int arraySize = 360;
  float values[arraySize];

  for (int i = 0; i < arraySize; i++) {
    float omega = 2.0 * PI * ((float)i);
    int value = 255 * sin(2 * PI * omega) + 128;
    values[i] = value;
  }

  unsigned long start = millis();
  unsigned long now = millis();
  while ((now - start) < duration) {
    for (int i = 0; i < arraySize; i++) {
      dacWrite(OUTPUT_PIN, values[i]);
      now = millis();
      delay(10);
    }
  }
}

void loop(void) {
  //do nothing
}
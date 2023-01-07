//Blinking the yellow onboard LED (Pin 13) on the Arduino Nano Every

#include <Arduino.h>

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT); //pin 13
}

void loop() {
  // put your main code here, to run repeatedly:
  digitalWrite(LED_BUILTIN, HIGH);
  Serial.print("LED is on ");
  Serial.println(LED_BUILTIN);
  delay(1000);
  digitalWrite(LED_BUILTIN, LOW);
  Serial.println("LED is off");
  delay(1000);
}

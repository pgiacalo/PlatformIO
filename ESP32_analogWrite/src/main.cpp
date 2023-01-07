/*
 * 01/02/2023
 * ChatGPT prompt: Write the code for an ESP32 to output a voltage that rises from 1 volt to 3 volts in 10 seconds.
 * I modified the code slightly so that the for() loop goes from zero to 255. 
 * 
 * This code uses the analogWrite() function to output a voltage on the specified pin. 
 * The for loop increments the output voltage by 1 volt each iteration between delay() calls. 
 * This causes the voltage to rise from 0 volt to 3 volts over the course of 10 seconds.
 * It's important to note that the ESP32 can only output a limited range of voltages, and the actual voltage output may differ from the desired value. The ESP32 can output a maximum of 3.3 volts on its digital output pins, and the resolution of the analogWrite() function is 8 bits, so the output voltage will be in increments of 3.3/255 = 0.0129 volts.
*/
#include <Arduino.h>

const int outputPin = 0; // output pin for the voltage

void setup() {
  Serial.begin(115200);
  pinMode(outputPin, OUTPUT);
}

void loop() {
  for (int i = 0; i <= 255; i++) {
//    Serial.println(i);
    analogWrite(outputPin, i); // output the voltage
    delay(10);
  }
}

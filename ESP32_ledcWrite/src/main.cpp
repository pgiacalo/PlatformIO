/*
 * Code example from the Espressif documentation for ledcWrite().
 * The documentation warns that the analog output is 
*/
#include <Arduino.h>

const int ledPin1 = 0;
const int ledPin2 = 4;
const int ledPin3 = 16;

// PWM channel 0 parameter
const int freq = 1000; // PWM frequency (not sure what this means)
const int ledChannel = 0; // the LED channel number 
const int resolution = 12; // resolution can be 1 to 20 bits for ESP32

void setup(){
    // Configure LED channel 0
    ledcSetup(ledChannel, freq, resolution);

    // Attach 3 LED pins to the LED channel
    ledcAttachPin(ledPin1, ledChannel);
    ledcAttachPin(ledPin2, ledChannel);
    ledcAttachPin(ledPin3, ledChannel);
}

void loop(){
    // Increase the brightness of the led in the loop
    for(int dutyCycle = 0; dutyCycle <= 4096; dutyCycle++){
        ledcWrite(ledChannel, dutyCycle);
        delay(1);
    }
}

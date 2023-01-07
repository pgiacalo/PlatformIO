#include <Arduino.h>

#define MAX_4_BIT_NUM   15
#define MAX_6_BIT_NUM   63
#define MAX_8_BIT_NUM   255
#define MAX_10_BIT_NUM  1023
#define MAX_12_BIT_NUM  4095

void setup() {
  Serial.begin(115200);

  //Now setup 12 Arduino analog pins for output

  //=================================================================
  //Use Data Direction Registers (DDRs) to set bits on the Ports.
  //This is 60 times faster than calling pinMode() and analogWrite().
  //Yields execution times in nanoseconds rather than microseconds.
  //=================================================================

  //set Arduino Uno digital pins 0-7 to OUTPUT (Port D)
  //Port D Pins = digital pins 7, 6, 5, 4, 3, 2, 1, 0
  DDRD = B11111111; // 1 for OUTPUT, 0 for INPUT

  //set Arduino Uno Pins 8-13 to OUTPUT (Port B)
  //Port B Pins: NA, NA, 13, 12, 11, 10, 9, 8
  DDRB = B001111; //only 6 Pins on Port B 

  //Tips:
  // 1) Use write DDR pin values, use boolean operators 
  //  - to set a pin to zero, use &=      (only 0 & 0 wins so pins set to 1 aren't effected)
  //  - to set a pin to one, use the |=   (1 always wins)
  //
  // 2) To read DDR pin values, use PIN Registers (PINB, PINC, PIND)
  //    - if (PINB & B00100000) {...} //same as digitalRead(5)
}

void loop() {
  int valSine = 0;
  for (int i = 0; i<360; i++){
    //sin(x) returns values between +/-1
    //so we add 1 to the sin() result to keep all values positive
    //divide by 2?! 
    valSine = ((sin(i * DEG_TO_RAD) + 1) * MAX_12_BIT_NUM) / 2;
  }

    //set the DDR pins high (1) or low (0) 
    PORTD = valSine;      //the low bits
    PORTB = valSine >> 8; //the high bits

    delay(10);

    Serial.print(valSine / 4);
    Serial.print(", ");
    Serial.print(analogRead(A0));

}

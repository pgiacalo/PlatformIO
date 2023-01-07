/*
  How to maximize the accuracy of an Arduino's analog-to-digital conversions. 
    1) Connect a precise, external voltage reference to the Arduino's AREF pin. 
    2) Use an averaging filter (i.e., a low pass filter) on values returned by analogRead().
*/

#include <Arduino.h>

// Change the following hard-coded values to match the your actual values 
float const EXTERNAL_VREF = 4.99877;  //the reference voltage (measured at the AREF pin)
uint8_t adcInputPin = A3;
int const adcBitDepth = 10;
long adcSteps = pow(2, adcBitDepth); //i.e., 1024 for a bit depth of 10

void setup() {
  Serial.begin(115200);
  analogReference(EXTERNAL); //Arduino will use an external voltage reference (at AREF pin)
  Serial.println("============================================");
  Serial.print(  "Using an EXTERNAL voltage reference (volts): ");
  Serial.println(EXTERNAL_VREF, 3);
  Serial.print("ADC bit depth: ");
  Serial.println(adcBitDepth);
  Serial.print("ADC steps: ");
  Serial.println(adcSteps);
  Serial.print("ADC minimum step size (volts): ");
  Serial.println(EXTERNAL_VREF/((float)adcSteps), 4);
  Serial.println("============================================");
}

float volts = 0.0;
int const count = 100;
long total = 0;
float avgValue = 0.0;

void loop() {
    delay(50);
    total = 0;
    avgValue = 0.0;
    //read many values and average them as a low pass noise filter
    for (int i=0; i<count; i++){
      //analogRead() returns an int (but the total can grow to a long)
      total = total + analogRead(adcInputPin); 
    }
    avgValue = (float)total/(float)count;
    volts = avgValue / (float)adcSteps * EXTERNAL_VREF;
    Serial.println(volts, 4); //print with 4 decimal digits
}

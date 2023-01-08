/**
This class is design to help in the calibration of ESP32 ADCs. 
When run, it will also output the voltage of the ESP32's internal voltage reference. 

Calibration will potentially make the values returned by the ADC more precise. 
However, my own testing has shown that the ADC values jump around from moment to moment. 
So this calibration might not provide any benefit (I have not done a statistical analysis
to find out the difference between a calibrated and uncalibrated ADC). 

Note that you will need to include the esp_adc_cal.h header file and link the esp32-adc-cal 
library in your project in order to use these functions. This file is found in the esp-idf/components
directory. The required files are installed with the ESP-IDF framework -- Espressif's official 
IoT Development Framework for the ESP32.

How to use this class:

1) Connect a precise 3.000 volt source to ESP32 pin GPIO36 and ground. 
2) Run this code.
3) It will print the ADC adjusted and non-adjusted voltage measurements to the terminal.
4) It will also print the adjusted and non-adjusted voltage error (in percent).
5) Adjust the value of CAL_ADJUSTMENT in the code below to minimize the adjused voltage errors. 
    - reduce the CAL_ADJUSTMENT value, if the adjusted voltage is too high
    - increase the CAL_ADJUSTMENT value, if the adjusted voltage is too low

Once you have established the optimum calibration value for a particular ESP32 board, you can use 
the value in your other projects to improve its ADC accuracy. 


===============================
General Notes on the ESP32 ADCs
===============================

The ESP32 is a microcontroller that features a number of analog-to-digital converters (ADCs). 
The exact number of ADCs that are available on an ESP32 chip can vary depending on the specific model.

In general, the ESP32 has two 12-bit ADCs that can be used to measure analog signals. 
These ADCs have a resolution of up to 0.5% and can operate at a maximum sampling rate 
of 1 mega sample per second (Msps).

The ESP32 also has an additional 8-bit ADC that can be used to measure the voltage of its internal battery. 
This ADC has a resolution of 1% and can operate at a maximum sampling rate of 200 kilo samples per second (ksps).

Overall, the ESP32 has a total of three ADCs that can be used to measure analog signals. 
These ADCs can be used to measure a variety of different signals, including sensor outputs, 
audio signals, and more.
*/

#include <Arduino.h>
#include "esp_adc_cal.h"
#include "stdio.h"

//=======================================
//Adjust this value to calibrate the ADC
//=======================================
// Start with CAL_ADJUSTMENT at 1.000
#define CAL_ADJUSTMENT          1.028 // If the "adjusted voltage" is too high, reduce this value (and vis-a-versa)

#define TEST_VOLTAGE            3.000           //the voltage applied to the assigned ADC_PIN
#define ADC_PIN                 36              //use GPIO36 to connect the test voltage (i.e., 3.000 volts)
#define ADC_UNIT                ADC_UNIT_1      //calibrate ADC_UNIT_1 and/or ADC_UNIT_2
#define ADC_ATTENUATION         ADC_ATTEN_DB_11 //ESP32 attenuates input voltages to allow higher voltage inputs (max 3.3 volts)
#define DELAY_BETWEEN_TESTS     2000            //milliseconds
//================ 
// Constants 
//================ 
#define ADC_MAX_INPUT_VOLTAGE   3.3   //volts (unrelated to the 3.000 input voltage used for calibration)
#define ADC_NOMINAL_VREF        1100  //millivolts (the actual value varies between ESP32 chips from 1000 to 1200 mV)
#define ADC_BIT_DEPTH           12    //12 bits used to convert analog input to digital output
#define ADC_STEPS               (float)(pow(2, ADC_BIT_DEPTH) - 1) //4095 for 12 bit ADCs

//struct that holds the characteristics of the ADC
esp_adc_cal_characteristics_t adc_chars;

//holds the actual internal voltage reference of the ESP32 chip (a factory setting, e.g., 1135 mV)
float vref;

float readAdjustedVoltage(int adc_pin) {
  return CAL_ADJUSTMENT * (analogRead(adc_pin) / ADC_STEPS) * ADC_MAX_INPUT_VOLTAGE * (ADC_NOMINAL_VREF / vref);
}

/**
 * Utility function to get the internal vRef of the ESP32 chip in millivolts.
 * ESP32 vRef is nominally 1100 millivolts but it varies from chip-to-chip.
 * This function returns the vRef value measured/set at the factory. 
 * 
 * @return float the internal vRef of the ESP32 chip, in millivolts
 */
float getVRef(){
  //populate the ADC struct by selecting: the ADC, input voltage attenuation, ADC bit depth, and nominal vRef 
  esp_adc_cal_characterize(ADC_UNIT, ADC_ATTENUATION, ADC_WIDTH_BIT_12, ADC_NOMINAL_VREF, &adc_chars);
  //set the internal voltage reference from the ESP32 chip (a factory setting, e.g., 1135 mV)
  vref = adc_chars.vref; //millivolts
  Serial.println("---->> actual ESP32 internal vRef, adc_chars.vref=" + String(adc_chars.vref + " mV"));
  return vref;
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  //get the internal vRef from the chip
  vref = getVRef();
  Serial.println("---->> actual ESP32 internal reference voltage (millivolts) = " + String(adc_chars.vref));
}

void loop() {
  float unadjusted_voltage = (analogRead(ADC_PIN) / ADC_STEPS * ADC_MAX_INPUT_VOLTAGE);
  float adjusted_voltage   = readAdjustedVoltage(ADC_PIN);
  
  Serial.println();
  Serial.println("Test with " + String(TEST_VOLTAGE) + " volts on pin GPIO" + String(ADC_PIN));
  Serial.println("------------------------------------");
  Serial.println("Adjusted Voltage    = " + String(adjusted_voltage, 3) + "v  " + String(adjusted_voltage / TEST_VOLTAGE * 100 - 100) + "% error");
  Serial.println("Un-adjusted Voltage = " + String(unadjusted_voltage, 3) + "v  " + String(unadjusted_voltage / TEST_VOLTAGE * 100 - 100) + "% error");
  delay(DELAY_BETWEEN_TESTS);
}

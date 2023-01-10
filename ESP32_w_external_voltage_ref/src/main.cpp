/*
To configure the ESP32's ADC to use an external voltage reference for its measurements, 
you will need to use the ESP32's ADC library, which is a part of the ESP-IDF 
(Espressif IoT Development Framework) or the Arduino library.

Here's an example of how you might configure the ESP32's ADC to use an 
external voltage reference in ESP-IDF:

Connect the external voltage reference to the ESP32's ADC input pin.

Include the ADC driver by adding the following line at the top of your code:
*/

#include <Arduino.h>
#include "driver/adc.h"

void setup() {
  //Configure the ADC to use the external voltage reference by setting the appropriate configuration bits in the adc1_config_t struct:
  adc1_config_t adc_config;
  adc_config.reference = ADC_REF_EXT;  // Set the reference to external

  //Initialize the ADC using the configuration struct and the channel number you want to use:
  adc1_config_width(ADC_WIDTH_BIT_12);
  adc1_init();

  //You can now take ADC readings by calling the adc1_get_voltage() function with the channel number you want to read:
  uint32_t voltage = adc1_get_voltage(channel);
  //Note: in the above examples channel should be a channel you specified for external voltage, for more information on channel please check the documentation of the ESP32.

}

void loop() {
  // put your main code here, to run repeatedly:
}



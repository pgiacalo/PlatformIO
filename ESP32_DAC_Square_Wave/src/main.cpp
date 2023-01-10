/*
  This code is designed to generate a square wave from the ESP32. 
  The square wave FREQUENCY is a settable variable. 
  The minimum square wave voltage is 0 volts and the maximum defaults to 1.65 volts (half of the ESP32 max voltage).

  You can use an oscilloscope or other equipment to measure the waveform output from the assigned DAC_CHANNEL.

  This code includes the esp_system.h and driver/dac.h header files, which contain the 
  necessary definitions and declarations for the DAC functions.

  The code defines a number of constants:

  DAC_CHANNEL specifies the DAC channel to use. The ESP32 has two DAC channels.
  DAC_BIT_DEPTH specifies the bit depth of the DAC. The DAC on the ESP32 has a fixed bit depth of 8 bits.
  MAX_AMPLITUDE specifies the amplitude of the waveform. It is set to 1/2 the maximum value of the DAC's bit depth.

  In the setup function, the DAC is initialized by calling the dac_output_enable() function with the specified DAC channel.
  Also, a timer is setup to periodically callback a the onTimer() function that produces/outputs the square wave. 
  The period of timer callbacks can be controllable in units of microseconds. 

  The loop() function does nothing.

*/

#include <Arduino.h>
#include "esp_system.h"
#include "driver/dac.h"
#include <stdio.h>

#define FREQUENCY 3000
#define DAC_CHANNEL DAC_CHANNEL_1   //i.e., DAC_CHANNEL_1 = GPIO25 and DAC_CHANNEL_2 GPIO26

//================== 
// Constants
//================== 
#define DAC_MAX_VALUE 255
#define DAC_BIT_DEPTH 8
#define MAX_AMPLITUDE (1 << (DAC_BIT_DEPTH - 1))  //keeps the output amplitude at 50% of the maximum voltage (i.e., 1.65 v) 

hw_timer_t * timer = NULL;

// Flag to track the state of the square wave
bool square_wave_state = false;

void IRAM_ATTR onTimer() {
  if (square_wave_state) {
    dac_output_voltage(DAC_CHANNEL, MAX_AMPLITUDE);
  } else {
    dac_output_voltage(DAC_CHANNEL, 0);
  }

  // Toggle the square wave state
  square_wave_state = !square_wave_state;
}

void setup() {
  printf("---------------- setup() called -----------------");
  dac_output_enable(DAC_CHANNEL);

  timer = timerBegin(0, 80, true);
  timerAttachInterrupt(timer, &onTimer, true);
  //we divide frequency by 2 below, since onTimer() must be called twice per cycle
  timerAlarmWrite(timer, 1000000 / FREQUENCY/2, true);
  timerAlarmEnable(timer);
}

void loop() {
  // Do nothing
}


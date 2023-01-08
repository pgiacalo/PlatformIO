/* Using ChatGPT code to generate a sine wave */

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "driver/dac.h"
#include "esp_system.h"
#include <stdio.h>

#include "esp_adc_cal.h"

#include "freertos/FreeRTOS.h"
#include "driver/dac.h"
#include "driver/timer.h"
#include "esp_system.h"

#define DEBUG         false
#define DAC_CHANNEL   DAC_CHANNEL_2
#define FREQUENCY     2000     // the desired frequency of the output waveform
#define SAMPLE_RATE   16384    // per Nyquist, set this to be at least 2 x FREQUENCY
#define TIMER_DIVIDER 80      // 80MHz timer frequency

//do NOT change the following 2 values
#define MAX_DAC_VALUE 255    //the ESP32 DAC bit depth (this is fixed in the hardware)
#define AMPLITUDE     127    //set to half of the MAX_DAC_VALUE to stay within ESP32 voltage range

int sine_wave[SAMPLE_RATE];   // global sine wave array
int wave_index = 0;           // current position in sine wave array

hw_timer_t * timer = NULL;

void onTimer() {
  if (DEBUG){
    Serial.print("called back [" + String(wave_index)); Serial.println("] " + String(sine_wave[wave_index]));
    Serial.print("sine_wave[wave_index] = " + String(sine_wave[wave_index]));
  }

  dac_output_voltage(DAC_CHANNEL, sine_wave[wave_index]);
  wave_index++;
  if (wave_index >= SAMPLE_RATE) {
    wave_index = 0;  // wrap around to start of sine wave array
  }
}

void printArray(){
  for (int i = 0; i < sizeof(sine_wave)/sizeof(sine_wave[0]); i++) {
    Serial.print(i); Serial.print(") ");
    Serial.print(", " + String(sine_wave[i]));
  }
}

void dac_sine_wave_setup(int frequency) {
  // initialize sine wave array
  //note that in order to avoid negative values, AMPLITUDE is added to each value 
  //before inserting into the array
  for (int i = 0; i < SAMPLE_RATE; i++) {
    sine_wave[i] = AMPLITUDE + (AMPLITUDE * sin((2 * PI * i * frequency) / SAMPLE_RATE));
  }

  if (DEBUG){
    printArray();
  }

  timer = timerBegin(0, 80, true);
  timerAttachInterrupt(timer, &onTimer, true);
  timerAlarmWrite(timer, 1000000 / FREQUENCY, true);
  timerAlarmEnable(timer);

  dac_output_enable(DAC_CHANNEL);

  // // configure timer
  // timer_config_t timer_config = {
  //     .alarm_en = TIMER_ALARM_EN,
  //     .counter_en = TIMER_PAUSE,
  //     .intr_type = TIMER_INTR_LEVEL,
  //     .counter_dir = TIMER_COUNT_UP,
  //     .auto_reload = TIMER_AUTORELOAD_EN,
  //     .divider = TIMER_DIVIDER
  // };
  // timer_init(TIMER_GROUP_0, TIMER_0, &timer_config);

  // // set timer period
  // uint64_t timer_period = (TIMER_BASE_CLK / SAMPLE_RATE) * (1000000 / frequency);
  // timer_set_alarm_value(TIMER_GROUP_0, TIMER_0, timer_period);

  // // register timer callback
  // timer_isr_register(TIMER_GROUP_0, TIMER_0, &onTimer, NULL, ESP_INTR_FLAG_IRAM, NULL);

  // // enable timer
  // timer_start(TIMER_GROUP_0, TIMER_0);

  // enable DAC output

}

void setup() {
  Serial.begin(115200); delay(500); //a short delay is req'd to allow ESP32 to finish Serial output setup

  printf("---------------- setup(1) called -----------------\n");
  Serial.println("---------------- setup(2) called -----------------");
  Serial.print("portTICK_PERIOD_MS="); Serial.println(portTICK_PERIOD_MS);

  dac_sine_wave_setup(FREQUENCY);
}

void loop(){
  //do nothing, since the timer and callbacks will handle all of the work 
  //of producing the waveform output. 
}


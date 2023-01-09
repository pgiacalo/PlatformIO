/**
 * Code for the ESP32 that outputs a sine wave via a DAC channel. 
 * 
 * @file main.cpp
 * @author Philip Giacalone
 * @brief 
 * @version 0.1
 * @date 2023-01-08
 */

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "esp_system.h"
#include "driver/dac.h"
#include "driver/timer.h"
#include "clk.h"

//specify the output frequency, sample rate, and attenuation
#define FREQUENCY           500     // the desired frequency (Hz) of the output waveform
#define SAMPLES_PER_SECOND  40000   // ADC samples per second. Per Nyquist, set this to be at least 2 x FREQUENCY
#define ATTENUATION         0.8     // output waveform voltage attenuation (must be 1.0 or less)
#define DEBUG               false

//specify how to generate the sine wave data 
#define STATIC        0       // waveform values are generated once and stored in an array
#define DYNAMIC       1       // waveform values are generated dynamically at runtime
#define GENERATE      DYNAMIC  // STATIC may cause compile failure ("DRAM segment data does not fit") due to memory "overflow"

//specify the DAC channel
#define DAC_CHANNEL   DAC_CHANNEL_1 // the waveform output pin. (e.g., DAC_CHANNEL_1 or DAC_CHANNEL_2)
 
//do NOT change the following values
#define MAX_DAC_VALUE 255    //the maximum ESP32 DAC value (8 bit DAC. this is fixed in the hardware)
#define AMPLITUDE     127    //amplitude is half of peak-to-peak
#define TIMER_DIVIDER 80     // timer frequency divider. timer runs at 80MHz by default. a divider of 2 means it runs at 40MHz. 
#define OMEGA         (2 * PI * FREQUENCY) //the frequency in radians per second

//the array of STATIC sine wave data 
int sine_wave[SAMPLES_PER_SECOND];   // array that holds sine wave values
//holds the index of the array
int wave_index = 0;           // current position in sine wave array
//the timer used to make callbacks to the onTimer() function
hw_timer_t * timer = NULL;

/** 
 * The function generates and outputs the sine wave to the DAC channel.
 * It is called periodically by the timer. The period depends on the SAMPLES_PER_SECOND.
*/
void onTimer() {
  int waveform_value = 0;

  if (GENERATE == STATIC){
    // get the sine wave value from the pre-loaded, static array of values
    waveform_value = sine_wave[wave_index];
  } else {
    // generate the sine wave value dynamically
    // note that in order to avoid negative values, AMPLITUDE is added to each value 
    waveform_value = ATTENUATION * (AMPLITUDE + AMPLITUDE * sin((OMEGA * wave_index) / SAMPLES_PER_SECOND));
  }

  // output the voltage to the DAC_CHANNEL
  dac_output_voltage(DAC_CHANNEL, waveform_value);

  if (DEBUG){
    Serial.print("onTimer() called "); 
    Serial.println("[" + String(wave_index) + "] " + String(waveform_value));
  }

  wave_index++;
  if (wave_index >= SAMPLES_PER_SECOND) {
    wave_index = 0;  // wrap around to start of sine wave array
  }
}

/**
 * @brief Prints the contents of the sine_wave array to the terminal. Used only for debugging.
 */
void printArray(){
  for (int i = 0; i < sizeof(sine_wave)/sizeof(sine_wave[0]); i++) {
    Serial.print(i); Serial.print(") "); 
    Serial.print(String(sine_wave[i]));
    Serial.print("\n");
  }
}

/**
 * @brief Create a Sine Wave Data object
 * 
 * @param frequency 
 */
void createSineWaveData(int frequency){
  //populate an array with waveform data
  //note that in order to avoid negative values, AMPLITUDE is added to each value 
  for (int i = 0; i < SAMPLES_PER_SECOND; i++) {
    sine_wave[i] = ATTENUATION * (AMPLITUDE + AMPLITUDE * sin((OMEGA * i) / SAMPLES_PER_SECOND));
  }

  if (DEBUG){
    printArray();
  }
}

/**
 * @brief Configures the callback timer. 
 * The frequency of the callbacks is determined by the given samples_per_second.
 * 
 * @param samples_per_second - the rate at which the waveform is sampled
 */
void setupCallbackTimer(long samples_per_second) {
  // set up timer 0 to generate a callback to onTimer() every 1 microsecond
  int timer_id = 0; //the ESP32 has several timers. Just use 0. 
  boolean countUp = true;
  long ONE_SECOND_IN_MICROSECONDS = 1000000; //the timer has a resolution of 1 microsecond

  timer = timerBegin(timer_id, TIMER_DIVIDER, countUp);
  timerAttachInterrupt(timer, &onTimer, true);
  //timerAlarmWrite() sets up callbacks with a resolution of microseconds
  timerAlarmWrite(timer, ONE_SECOND_IN_MICROSECONDS / samples_per_second, true);
  timerAlarmEnable(timer);
}

/**
 * @brief Prints the settings to the terminal
 * 
 */
void printSettings(){
  Serial.println();
  Serial.println("=======================================================");
  Serial.print("Waveform generation is ");
  if (GENERATE == STATIC){
    Serial.println("STATIC");
  } else if (GENERATE == DYNAMIC){
    Serial.println("DYNAMIC");
  } else {
    Serial.println("(ERROR) UNDEFINED");
  }
  
  Serial.println("Frequency  : " + String(FREQUENCY) + " Hz");
  Serial.println("Sample Rate: " + String(SAMPLES_PER_SECOND) + " sample per second");
  uint32_t clock_speed = esp_clk_cpu_freq() / 1000000;  //MHz  
  Serial.println("Clock_Speed: " + String(clock_speed) + " MHz");
  Serial.println("=======================================================");
  Serial.println();
}

void printClockSpeed(){
  //
}

void setup() {
  Serial.begin(115200); 
  delay(500); //a short delay to allow ESP32 to finish Serial output setup

  printSettings();

  if (GENERATE == STATIC){
    createSineWaveData(FREQUENCY);
  }

  setupCallbackTimer(SAMPLES_PER_SECOND);

  dac_output_enable(DAC_CHANNEL);
}

/**
 * @brief Does nothing, since all the work is handled by the timer and onTimer()
 * 
 */
void loop(){
  //do nothing, since the timer and its callbacks to onTimer() handle ALL of the work 
  delay(60000);
}

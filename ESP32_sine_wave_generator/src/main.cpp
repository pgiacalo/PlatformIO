/**
 * Code for the ESP32 that outputs a sine wave via a DAC channel. 
 * 
 * Note on a waveform's amplitude vs its peak-to-peak value:
 * 
 * In mathematics, the amplitude of a sine wave is typically defined as the 
 * maximum absolute value of the waveform. This is the distance from the 
 * midpoint of the waveform to the maximum or minimum value of the waveform. 
 * 
 * For example, if the sine wave has a maximum value of 3 and a minimum value
 * of -3, the peak-to-peak value is 6 and the amplitude is 3. 
 * 
 * The peak-to-peak value of a waveform is the difference between the maximum 
 * and minimum values of the waveform, so it is twice the amplitude. 
 * 
 * @file main.cpp
 * @author Philip Giacalone
 * @brief 
 * @version 0.1
 * @date 2023-01-11
 */

#include <Arduino.h>
#include "driver/dac.h"
#include "driver/timer.h"
#include "clk.h"

//Configurable items: specify the output frequency, sample rate, attenuation and DAC Channel
#define FREQUENCY           3000    // the desired frequency (Hz) of the output waveform
#define SAMPLES_PER_SECOND  180000  // ADC samples per second. Per Nyquist, set this at least 2 x FREQUENCY
#define ATTENUATION         0.8     // output waveform voltage attenuation (must be 1.0 or less)
#define DAC_CHANNEL         DAC_CHANNEL_1 // the waveform output pin. (e.g., DAC_CHANNEL_1 or DAC_CHANNEL_2)

//These items should probably be left as-is
#define DAC_BIT_DEPTH       8       // ESP32: 8 bits (fixed within ESP32 hardware)
#define DEBUG               false
 
//Do NOT change the following 
#define SAMPLES_PER_CYCLE   SAMPLES_PER_SECOND/FREQUENCY 
#define MAX_DAC_VALUE       255     // (255) the maximum ESP32 DAC value, peak-to-peak (8 bit DAC fixed in hardware)
#define AMPLITUDE           127     // (127) amplitude is half of peak-to-peak
#define TIMER_DIVIDER       80      // (80) timer frequency divider. timer runs at 80MHz by default. 

//the array that will hold all of the digital waveform values
int waveValues[SAMPLES_PER_CYCLE];
//holds the current index to the waveValues array 
int currentWaveSample = 0;

unsigned long previousMillis = 0UL;
unsigned long interval = 120000UL; //120 seconds

//the timer used to make callbacks to the onTimer() function
hw_timer_t * timer = NULL;

// Node structure for a circular linked list.
// This will hold the waveform values for one complete cycle.
struct node {
  int data;
  struct node *next;
};
struct node* currentNode;

// memory information struct
typedef struct {
    size_t free_heap;
    size_t minimum_free_heap;
    size_t used_heap;
} heap_info_t;

heap_info_t heap_info;

//utility function
void get_heap_info(heap_info_t *info)
{
    info->free_heap = esp_get_free_heap_size();
    info->minimum_free_heap = esp_get_minimum_free_heap_size();
    info->used_heap = info->free_heap - info->minimum_free_heap;
}

//utility function
void printHeapInfo(){
  get_heap_info(&heap_info);
  Serial.println("------Heap Info------");
  Serial.println("Free heap        : " + String(heap_info.free_heap));
  Serial.println("Min Free heap    : " + String(heap_info.minimum_free_heap));
  Serial.println("Used Heap        : " + String(heap_info.used_heap));
}

/** 
 * Prints out the contents of the linked list 
 */
template <typename T, std::size_t N> 
void printArray(const T (&arr)[N]) { 
    for (std::size_t i = 0; i < N; i++) {
        Serial.println(arr[i]);
    }
}

/**
 * @brief Populates the given circular linked list with one complete cycle of sinusoid data
 * 
 * Since we know the SAMPLES_PER_CYCLE of the waveform, we'll put that many values into the linked list.
 * The timer will call function onTimer() at the precise rate needed to produce the desired output frequency. 
 * 
 * @param head pointer to the circular linked list to be populated with one complete cycle of sinusoid data
 * @return void
 */
void populateWaveArray() {
  int count = sizeof(waveValues) / sizeof(waveValues[0]);
  for (int i = 0; i < count; i++) {
    float angleInDegrees = ((float)i) * (360.0/((float)SAMPLES_PER_CYCLE));
    float angleInRadians = 2.0 * PI * angleInDegrees / 360.0;
    if (DEBUG){
      Serial.println("i : degrees : radians " + String(i) + " : " + String(angleInDegrees) + " : " + String(angleInRadians));
    }
    long value = ATTENUATION * (AMPLITUDE + AMPLITUDE * sin(angleInRadians));
    waveValues[i] = value;
  }
  if (DEBUG){
    printArray(waveValues);
  }
}

/** 
 * The function generates and outputs the sine wave to the DAC channel.
 * It is called periodically by the timer. 
 * This function:
 *  1) gets values of the waveform from the circular linked list
 *  2) outputs the value to the DAC channel
 *  3) advances the linked list to the next node 
*/
void onTimer() {

  // get the waveform value from the linked list
  int waveform_value = waveValues[currentWaveSample];
  // output the voltage to the DAC_CHANNEL
  dac_output_voltage(DAC_CHANNEL, waveform_value);
  // advance the index or reset to zero
  currentWaveSample++;
  if (currentWaveSample >= SAMPLES_PER_CYCLE){
    currentWaveSample = 0;
  } 
}

/**
 * @brief Configures the callback timer. 
 * The frequency of the callbacks is determined by the SAMPLES_PER_SECOND.
 */
void setupCallbackTimer() {
  // set up timer 0 to generate a callback to onTimer() every 1 microsecond
  int timer_id = 0; //the ESP32 has several timers. Just use 0. 
  boolean countUp = true;

  long MICROSECONDS_PER_SECOND = 1000000; //the timer has a resolution of 1 microsecond (nice!) 
  long MICROSECONDS_PER_SAMPLE = MICROSECONDS_PER_SECOND / SAMPLES_PER_SECOND;

  timer = timerBegin(timer_id, TIMER_DIVIDER, countUp);
  timerAttachInterrupt(timer, &onTimer, true);
  //timerAlarmWrite() sets up callbacks with a resolution of microseconds
  timerAlarmWrite(timer, MICROSECONDS_PER_SAMPLE, true);
  timerAlarmEnable(timer);
}

/**
 * @brief Prints the settings to the terminal
 * 
 */
void printSettings(){
  Serial.println();
  Serial.println();
  Serial.println("=======================================================");  
  Serial.println("Frequency        : " + String(FREQUENCY) + " Hz");
  Serial.println("Sample Rate      : " + String(SAMPLES_PER_SECOND) + " samples per second");
  Serial.println("Samples Per Cycle: " + String(SAMPLES_PER_CYCLE) + " samples per cycle");
  uint32_t clock_speed = esp_clk_cpu_freq() / 1000000;  //MHz  
  Serial.println("Clock_Speed      : " + String(clock_speed) + " MHz");

  printHeapInfo();

  Serial.println("=======================================================");
  Serial.println();
}

void checkConfig(){
  if (FREQUENCY < 0.0){
    throw "ERROR: checkConfig() FREQUENCY must be positive. Found FREQUENCY=" + String(FREQUENCY);
  }

  if (SAMPLES_PER_SECOND < 0.0){
    throw "ERROR: checkConfig() SAMPLES_PER_SECOND must be positive. Found SAMPLES_PER_SECOND=" + String(FREQUENCY);
  }

  if (ATTENUATION > 1.0 || ATTENUATION < 0.0){
    throw "ERROR: checkConfig() ATTENUATION must be positive and <= 1. Found ATTENUATION=" + String(ATTENUATION);
  }
}

void setup() {
  try {

    Serial.begin(115200); 
    delay(500); //a short delay to allow ESP32 to finish Serial output setup

    printSettings();

    checkConfig();

    populateWaveArray();

    dac_output_enable(DAC_CHANNEL); //do this before setupCallbackTimer() so the output channel is ready

    setupCallbackTimer(); 

  } catch (const std::exception &exc) {
    Serial.println(exc.what());
  }
}

// /**
//  * @brief Does nothing, since all the work is handled by the timer and onTimer()
//  */
// void loop()
// {
//   unsigned long currentMillis = millis();
//   if(currentMillis - previousMillis > interval)
//   {
//    	previousMillis = currentMillis;
//   }
// }

/**
 * @brief Does nothing, since all the work is handled by the timer and onTimer()
 * 
 */
void loop(){
  //do nothing, since the timer and its callbacks to onTimer() handle ALL of the work 
  delay(60000);
}

/**
 * Code for the ESP32 that outputs a sine wave via a DAC channel. 
 * 
 * Espressif ESP32-WROOM-32D
 * -------------------------
 * Chip is ESP32-D0WD-V3 (revision v3.0) 
 * Features: WiFi, BT, Dual Core, 240MHz, VRef calibration in efuse, Coding Scheme None
 * Crystal is 40MHz
 * --------
 * See the notes posted at the bottom of this file for information about improving the performance
 * of this code. 
 * --------
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
 * --------
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
#include <stdio.h>

// If using the FreeRTOS timer (which is faster than the built-in timer peripherals)
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"

//Configurable items: specify the output frequency, sample rate, attenuation and DAC Channel
#define FREQUENCY           200    // the desired frequency (Hz) of the output waveform
#define SAMPLES_PER_SECOND  150000  // ADC samples per second. Per Nyquist, set this at least 2 x FREQUENCY
#define ATTENUATION         0.5     // output waveform voltage attenuation (must be 1.0 or less)
#define DAC_CHANNEL         DAC_CHANNEL_1 // the waveform output pin. (e.g., DAC_CHANNEL_1 or DAC_CHANNEL_2)
#define STATIC              0
#define DYNAMIC             1
#define GENERATE_WAVES      STATIC 

double frequencies[] = {100.0};   // Hz, frequencies of the sine waves
double amplitudes[] = {0.5};       // amplitudes of the sine waves (range is from 0.0 to 1.0)
double phases[] = {0.0};      //{0.0, PI/4.0}     radians, phase angles of the sine waves in radians
double decay = 0.99;          // decay coefficient

//These items should probably be left as-is
#define DAC_BIT_DEPTH       8       // ESP32: 8 bits (fixed within ESP32 hardware)
#define DEBUG               false
 
//Do NOT change the following 
#define SAMPLES_PER_CYCLE   SAMPLES_PER_SECOND/FREQUENCY 
#define MAX_DAC_VALUE       255     // (255) the maximum ESP32 DAC value, peak-to-peak (8 bit DAC fixed in hardware)
#define MAX_DAC_AMPLITUDE   127     // (127) amplitude is half of peak-to-peak
#define TIMER_DIVIDER       80      // (80) timer frequency divider. timer runs at 80MHz by default. 

//FreeRTOS testing
typedef struct {
  int counter;
} timer_data_t;


//FreeRTOS testing - used in newer version of api (that I don't have!)
#define TIMER_STACK_SIZE (1024 * 4)
StackType_t timer_stack[4096];
StaticTask_t timer_task;


//the following are set by the system at runtime
double now = 0.0;                     // seconds. keeps track of the time (time since start-up in seconds)
double sampleCount = 0.0;               // keeps track of time steps, when dynamically generating the waveforms
int numberOfWaves = 0;              // set automatically at runtime. 
double MICROSECONDS_PER_SAMPLE = 0.0;  //set automatically at runtime.
double SECONDS_PER_SAMPLE = 0.0;       //set automatically at runtime.
const double MICROSECONDS_PER_SECOND = 1000000.0; //the timer has a resolution of 1 microsecond (nice!) 

//the array that will hold all of the digital waveform values
int waveValues[SAMPLES_PER_CYCLE];
//holds the current index to the waveValues array 
int waveSampleIndex = 0;

int dynamic_value = 0;  //TODO remove eventually. just for testing DYNAMIC.
int waveform_value = 0;

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
    long value = ATTENUATION * (MAX_DAC_AMPLITUDE + MAX_DAC_AMPLITUDE * sin(angleInRadians));
    waveValues[i] = value;
  }
}

// utility function to get count of elements in any given array
template <typename T, size_t N>
size_t getElementCount(T (&array)[N]) {
    return N;
}

/* 
The following equation represents a wave that oscillates sinusoidally 
with a frequency of f while its amplitude decays exponentially over time 
with a decay constant of a.

y(t) = A * e^(-at) * sin(2πft + φ)

where:

y(t) is the amplitude of the wave at time t
A is the initial amplitude of the wave
e is the base of the natural logarithm (approximately 2.718)
a is the decay constant, which determines how quickly the amplitude of the wave decays over time
t is time
f is the frequency of the wave in cycles per second
π (pi) is the mathematical constant (approximately 3.14)
φ is the phase angle in radians
*/
struct waveform {
    //frequency in cycles/second
    float frequency;
    //amplitude between 0.0 and 1.0
    float amplitude;
    //phase_angle in radians
    float phase_angle;
    //determines how quickly the amplitude of the wave decays over time
    float decay_constant;
};

struct waveform waveform1 = { 2.0, 0.8, 1.57, 0.1 };
struct waveform waveform2 = { 10.0, 0.2, 3.14, 0.1 };

waveform waves[] = {waveform1, waveform2};


/** 
 * The function generates and outputs the sine wave to the DAC channel.
 * It is called periodically by the timer. 
 * This function:
 *  1) gets values of the waveform from the circular linked list
 *  2) outputs the value to the DAC channel
 *  3) advances the linked list to the next node 
*/
void onTimer() {

  if (GENERATE_WAVES == STATIC){ //------STATIC GENERATION OF WAVEFORMS------

    // get the waveform value from the linked list
    waveform_value = waveValues[waveSampleIndex];
    // output the voltage to the DAC_CHANNEL
    dac_output_voltage(DAC_CHANNEL, waveform_value);
    // advance the index or reset to zero
    waveSampleIndex++;
    if (waveSampleIndex >= SAMPLES_PER_CYCLE){
      waveSampleIndex = 0;
    } 

  } else { //------DYNAMIC GENERATION OF WAVEFORMS------

    int64_t time_since_boot = esp_timer_get_time(); //microseconds
    double _t_ = time_since_boot/1000000.0;
    // ESP_LOGI(TAG, "Periodic timer called, time since boot: %lld us", time_since_boot);

    float y = 0;  //the final value to be sent to the ESP32 output pin (between 0 and 255)
    int waveCount = getElementCount(waves);
    for (int i=0; i<waveCount; i++){
      waveform w = waves[i];
      // y(t) = A * e^(-at) * sin(2πft + φ)
      float exponential = pow(M_E, (-1) * w.decay_constant * _t_);
      float angle = M_TWOPI * w.frequency * _t_ + w.phase_angle;
      //sum all the y values as floats in order to maintain resolution during the calculations
      y = y + ( w.amplitude * exponential * ( 127.0 + ( 127.0 * sin(angle) )));
    }
    dac_output_voltage(DAC_CHANNEL_1, ((int)y));

    // //------------ old way --------------
    // //data for each waveform (frequency, amplitude, phase angle, attenuation)
    // float wave1[] = {100.0, 0.5, 0.0, 0.5};
    // float wave2[] = {1000.0, 0.1, 0.0, 0.5};

    // // formula for a sine wave with frequency (ω) amplitude (A) and phase angle (φ)
    // //   f(t) = A sin(ωt + φ)    
    // float waveform1 = (wave1[3] * ( ((float)127) + ( ((float)127) * (wave1[1] * (sin(M_TWOPI * wave1[0] * _t_ + wave1[2])))))); 
    // float waveform2 = (wave2[3] * ( ((float)127) + ( ((float)127) * (wave2[1] * (sin(M_TWOPI * wave2[0] * _t_ + wave2[2])))))); 
    // // printf("%.9f\t", now);
    // // printf("%.3f\t", waveform1);
    // // printf("%.3f\t", waveform2);
    // int output = (int)(waveform1 + waveform2);
    // // printf("%d\n", output);
    
    // dac_output_voltage(DAC_CHANNEL_1, output);

//    } //end for numberOfWaves

  } //end else DYNAMIC
}

/**
 * @brief Configures the callback timer. 
 * The frequency of the callbacks is determined by the SAMPLES_PER_SECOND.
 */
void setupCallbackTimer() { //TODO my version of function 
  // set up timer 0 to generate a callback to onTimer() every 1 microsecond
  int timer_id = 1; //the ESP32 has several timers. 
  boolean count_up = true;

  timer = timerBegin(timer_id, TIMER_DIVIDER, count_up);
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
  Serial.println("Frequency            : " + String(FREQUENCY) + " Hz");
  Serial.println("Sample Rate          : " + String(SAMPLES_PER_SECOND) + " samples per second");
  Serial.println("Samples Per Cycle    : " + String(SAMPLES_PER_CYCLE) + " samples per cycle");
  Serial.printf( "Seconds Per Sample   : %.8lf seconds \n", SECONDS_PER_SAMPLE);
  Serial.printf( "Microsecs Per Sample : %.3lf usec \n", MICROSECONDS_PER_SAMPLE);

  int apb_freq = esp_clk_apb_freq();
  Serial.printf( "APB Timer Period     : %.3lf usec\n", apb_freq);

  uint32_t clock_speed = esp_clk_cpu_freq() / 1000000;  //MHz  
  Serial.println("Clock_Speed          : " + String(clock_speed) + " MHz");

  printHeapInfo();

  Serial.println("=======================================================");
  Serial.println();
}

void checkConfig(){
  if (FREQUENCY < 0.0){
    String msg = "ERROR: checkConfig() FREQUENCY value must be positive. Found FREQUENCY=" + String(FREQUENCY);
    Serial.println(msg);
    throw msg;
  }

  if (SAMPLES_PER_SECOND < 0.0){
    String msg = "ERROR: checkConfig() SAMPLES_PER_SECOND value must be positive. Found SAMPLES_PER_SECOND=" + String(FREQUENCY);
    Serial.println(msg);
    throw msg;
  }

  if (ATTENUATION > 1.0 || ATTENUATION < 0.0){
    String msg = "ERROR: checkConfig() ATTENUATION value must be between zero and one. Found ATTENUATION=" + String(ATTENUATION);
    Serial.println(msg);
    throw msg;
  }
}

void setup() {
  try {

    Serial.begin(115200); 
    delay(500); //a short delay to allow ESP32 to finish Serial output setup

    MICROSECONDS_PER_SAMPLE = MICROSECONDS_PER_SECOND / SAMPLES_PER_SECOND;
    SECONDS_PER_SAMPLE = MICROSECONDS_PER_SAMPLE / 1000000;

    printSettings();

    checkConfig();

    populateWaveArray();

    dac_output_enable(DAC_CHANNEL); //do this before setupCallbackTimer() so the output channel is ready

    setupCallbackTimer(); 

  } catch (const std::exception &exc) {
    Serial.println("ERROR caught in setup()...");
    Serial.println(exc.what());
  }
}

/**
 * @brief Does nothing, since all the work is handled by the timer and onTimer()
 */
void loop()
{
  unsigned long currentMillis = millis();
  if(currentMillis - previousMillis > interval)
  {
   	previousMillis = currentMillis;
  }
}

// /**
//  * @brief Does nothing, since all the work is handled by the timer and onTimer()
//  * 
//  */
// void loop(){
//   //do nothing, since the timer and its callbacks to onTimer() handle ALL of the work 
//   delay(60000);
// }

/*
NOTES on optimizing/improving the performance of the callback timer. 

--- The maximum rate of callbacks from the timer is what limits the maximum sample rate of this code ---

There are several ways to implement a timer callback on the ESP32, each with its own advantages and disadvantages 
in terms of performance.

1) One method is to use the ESP32's built-in timer peripheral, which allows you to configure a timer to generate an 
interrupt at a specific interval. This method is relatively simple to set up, but the performance may be limited 
by the overhead of the interrupt handler.

2) Another method is to use a FreeRTOS software timer, which allows you to create a timer task that runs in the 
background and is scheduled by the FreeRTOS kernel. This method can provide higher performance than using the 
built-in timer peripheral, as it allows you to offload the interrupt handling to a separate task.

3) A third option is to use a Direct Memory Access (DMA) controller to transfer samples from memory to the DAC. 
This method can provide the highest performance and the lowest CPU usage, but it requires more complex programming. 
With DMA, the CPU does not need to be involved in the data transfer, allowing it to be used for other tasks.

Regarding the maximum sample rate it can achieve for DAC output, it depends on how you implement the timer callback. 
With DMA and a proper configuration, it can achieve high sample rate, such as tens of mega samples per second. 
On the other hand, using the built-in timer peripheral or a FreeRTOS software timer, the maximum sample rate will be lower.

Keep in mind that the actual achievable sample rate will also depend on factors such as the clock frequency of the 
ESP32 and the accuracy of the timer.



*/
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
 * @date 2023-01-08
 */

#include <Arduino.h>
#include "driver/dac.h"
#include "driver/timer.h"
#include "clk.h"

//Configurable items: specify the output frequency, sample rate, attenuation and DAC Channel
#define FREQUENCY           10    // the desired frequency (Hz) of the output waveform
#define SAMPLES_PER_SECOND  1000  // (140000 max) ADC samples per second. Per Nyquist, set this at least 2 x FREQUENCY
#define ATTENUATION         1.0     // output waveform voltage attenuation (must be 1.0 or less)
#define DAC_CHANNEL         DAC_CHANNEL_1 // the waveform output pin. (e.g., DAC_CHANNEL_1 or DAC_CHANNEL_2)
#define DAC_BIT_DEPTH       8       // ESP32: 8 bits (fixed within ESP32 hardware)
#define DEBUG               true
#define STATIC              0
#define DYNAMIC             1
#define GENERATE_WAVES      DYNAMIC 

//Do NOT change the following 
#define SAMPLES_PER_CYCLE   SAMPLES_PER_SECOND/FREQUENCY 
#define DAC_PEAK_TO_PEAK    255     // (255) the maximum ESP32 DAC value, peak-to-peak (8 bit DAC fixed in hardware)
#define DAC_AMPLITUDE       127     // (127) amplitude is half of peak-to-peak
#define TIMER_DIVIDER       80      // (80) timer frequency divider. timer runs at 80MHz by default. 

float frequencies[] = {10.0, 100.0};   // Hz, frequencies of the sine waves
float amplitudes[] = {1.0, 0.1};       // amplitudes of the sine waves (range is from 0.0 to 1.0)
float phases[] = {0.0, PI/4.0};      //       radians, phase angles of the sine waves in radians
float decay = 0.99;          // decay coefficient

//the following are 
uint64_t now = 0;            // microseconds. keeps track of the time, when dynamically generating the waveforms
uint64_t sampleCount = 0;      // keeps track of time steps, when dynamically generating the waveforms
int numberOfWaves = sizeof(frequencies);  // set automatically at runtime. 
int MICROSECONDS_PER_SAMPLE = 0;    //set automatically at runtime.

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

void debug(String s){
  if (DEBUG) {
    Serial.println(s);
  }
}

void get_heap_info(heap_info_t *info)
{
    info->free_heap = esp_get_free_heap_size();
    info->minimum_free_heap = esp_get_minimum_free_heap_size();
    info->used_heap = info->free_heap - info->minimum_free_heap;
}

void printHeapInfo(){
  get_heap_info(&heap_info);
  Serial.println("------Heap Info------");
  Serial.println("Free heap        : " + String(heap_info.free_heap));
  Serial.println("Min Free heap    : " + String(heap_info.minimum_free_heap));
  Serial.println("Used Heap        : " + String(heap_info.used_heap));
}

void printLinkedList(){
  struct node *current = currentNode;
  Serial.println("-----Linked List Contents-----");
  do {
    int value = current->data;
    Serial.println(String(value));
    current = current->next;
  } while (current != currentNode);
}

// Function to create a circular linked list with a specified size
struct node* createCircularLinkedList(int size) {
  debug("Start createCircularLinkedList()");

  struct node *head, *current, *temp;

  // Create the first node
  head = (struct node*) malloc(sizeof(struct node));
  head->data = 1;
  current = head;

  // Create the rest of the nodes
  for (int i = 0; i < size-1; i++) {
    temp = (struct node*) malloc(sizeof(struct node));
    temp->data = i;
    current->next = temp;
    current = temp;
  }

  // Link the last node to the head to create the circular linked list
  current->next = head;

  debug("Finished createCircularLinkedList()");

  return head;
}

/**
 * @brief Populates the given circular linked list with one complete cycle of sinusoid data
 * 
 * Since we know the SAMPLES_PER_CYCLE of the waveform, we'll put that many values into the linked list.
 * The timer will call function onTimer() at the precise rate needed to produce the desired output frequency. 
 * 
 * @param head the start of the Populates the given circular linked list with one complete cycle of sinusoid data
 * @return void
 */
 
void populateCircularLinkedList(struct node *head) {

  debug("Start populateCircularLinkedList()");

  struct node *current = head;

  for (int i = 0; i < SAMPLES_PER_CYCLE; i++) {
    float angleInDegrees = ((float)i) * (360.0/((float)SAMPLES_PER_CYCLE));
    float angleInRadians = 2.0 * PI * angleInDegrees / 360.0;
    if (DEBUG){
      Serial.println("i : degrees : radians " + String(i) + " : " + String(angleInDegrees) + " : " + String(angleInRadians));
    }
    long value = ATTENUATION * (DAC_AMPLITUDE + DAC_AMPLITUDE * sin(angleInRadians));
    current->data = value;
    current = current->next;
  }
  if (DEBUG){
    printLinkedList();
  }
  debug("Finished populateCircularLinkedList()");
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

  int waveform_value = 0;

  if (GENERATE_WAVES == STATIC){ //------STATIC GENERATION OF WAVEFORMS------

    // get the waveform value from the linked list (value of 0 to 255)
    waveform_value = currentNode->data;
    // output the voltage to the DAC_CHANNEL
    dac_output_voltage(DAC_CHANNEL, waveform_value);
    // advance to the value in the linked list
    currentNode = currentNode->next; 

  } else { //------DYNAMIC GENERATION OF WAVEFORMS------

    now = sampleCount * MICROSECONDS_PER_SAMPLE / 1000000; //seconds since start

    for (int i=0; i<numberOfWaves; i++){

      float omega_t = TWO_PI * frequencies[i] * ((float)now);
      float phi = phases[i];  //already in radians
      // formula for a sine wave with frequency (ω) amplitude (A) and phase angle (φ)
      // f(t) = A sin(ωt + φ)
      float sine_value = sin(omega_t + phi);

      float single_wave_value = amplitudes[i] * sine_value;
      //waveform_value ranges from 0 to 255
      waveform_value = waveform_value + (ATTENUATION * (DAC_AMPLITUDE + (DAC_AMPLITUDE * single_wave_value))); //<--bug

    } //end for
    dac_output_voltage(DAC_CHANNEL, waveform_value);
    sampleCount++;

  } //end else
}

/**
 * @brief Configures the callback timer. 
 * The frequency of the callbacks is determined by the SAMPLES_PER_SECOND.
 */
void setupCallbackTimer() {
  debug("Begin setupCallbackTimer()");
  // set up timer 0 to generate a callback to onTimer() every 1 microsecond
  int timer_id = 0; //the ESP32 has several timers. Just use 0. 
  boolean countUp = true;

  long MICROSECONDS_PER_SECOND = 1000000; //the timer has a resolution of 1 microsecond (nice!) 
  MICROSECONDS_PER_SAMPLE = MICROSECONDS_PER_SECOND / SAMPLES_PER_SECOND; //sets the global variable

  timer = timerBegin(timer_id, TIMER_DIVIDER, countUp);
  timerAttachInterrupt(timer, &onTimer, true);
  //timerAlarmWrite() sets up callbacks with a resolution of microseconds
  timerAlarmWrite(timer, MICROSECONDS_PER_SAMPLE, true);
  timerAlarmEnable(timer);
//  debug("Finished setupCallbackTimer()");
}

/**
 * @brief Prints the settings to the terminal
 * 
 */
void printSettings(){
  debug("Start printSettings()");

  Serial.println();
  Serial.println();
  Serial.println("=======================================================");  
  if (GENERATE_WAVES == STATIC){
    Serial.println("Generate         : STATIC waveform data");
  } else {
    Serial.println("Generate         : DYNAMIC waveform data");
  }
  Serial.println("Frequency        : " + String(FREQUENCY) + " Hz");
  Serial.println("Sample Rate      : " + String(SAMPLES_PER_SECOND) + " samples per second");
  Serial.println("Attenuation      : " + String(ATTENUATION));
  Serial.println("Samples Per Cycle: " + String(SAMPLES_PER_CYCLE) + " samples per cycle");
  uint32_t clock_speed = esp_clk_cpu_freq() / 1000000;  //MHz  
  Serial.println("Clock_Speed      : " + String(clock_speed) + " MHz");

  printHeapInfo();

  Serial.println("=======================================================");
  Serial.println();

  debug("Finished printSettings()");
}

void checkInputs(){
  debug("Start checkInputs()");
  if (sizeof(frequencies) != sizeof(amplitudes) || sizeof(frequencies) != sizeof(phases)){
    throw std::runtime_error("CONFIGURATION ERROR: The size of the 3 input arrays must be equal (frequencies, amplitudes and phases)");
  }
  numberOfWaves = sizeof(frequencies);
  if (GENERATE_WAVES == DYNAMIC && numberOfWaves == 0){
    throw std::runtime_error("CONFIGURATION ERROR: DYNAMIC waveform generation specified but number of waveforms is zero");
  }
  debug("Finished checkInputs()");
}

void setup() {
  debug("Start setup()");
  try {

    Serial.begin(115200); 
    delay(500); //a short delay to allow ESP32 to finish Serial output setup

    printSettings();

    checkInputs();

    if (GENERATE_WAVES == STATIC){
      //statically generate all the waveform values
      currentNode = createCircularLinkedList(SAMPLES_PER_CYCLE);
      populateCircularLinkedList(currentNode);
    } else {
      //we'll dynamically generating the waveform
    }

    dac_output_enable(DAC_CHANNEL);

    setupCallbackTimer(); 

    debug("Finished setup()");

  } catch (const std::exception &exc) {
    Serial.println(exc.what());
  } 
}

/**
 * @brief Does nothing, since all the work is handled by the timer and onTimer()
 * 
 */
void loop(){
  //do nothing, since the timer and its callbacks to onTimer() handle ALL of the work 
  delay(60000);
}


//============================================================================
//============================================================================
//============================================================================
//============================================================================
/*
int onTimer2() {

    double result;  // combined sine wave

    for (time = 0; time <= 1.0; time += 0.01) {
        result = 0;
        for (int i = 0; i < n; i++) {
            result += amplitudes[i] * sin(2 * M_PI * frequencies[i] * time + phases[i]);
        }
        result *= decay;  // apply exponential decay
        printf("%.2f\n", result);
    }

    return 0;
}

  struct node *current = head;

  for (int i = 0; i < SAMPLES_PER_CYCLE; i++) {
    float angleInDegrees = ((float)i) * (360.0/((float)SAMPLES_PER_CYCLE));
    float angleInRadians = 2.0 * PI * angleInDegrees / 360.0;
    if (DEBUG){
      Serial.println("i : degrees : radians " + String(i) + " : " + String(angleInDegrees) + " : " + String(angleInRadians));
    }
    long value = ATTENUATION * (DAC_AMPLITUDE + DAC_AMPLITUDE * sin(angleInRadians));
    current->data = value;
    current = current->next;
  }
  if (DEBUG){
    printLinkedList();
  }
*/
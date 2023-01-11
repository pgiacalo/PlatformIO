#include <Arduino.h>
#include "driver/dac.h"
#include "driver/timer.h"
#include "clk.h"


//Configurable items: specify the output frequency, sample rate, attenuation and DAC Channel
#define FREQUENCY           100    // the desired frequency (Hz) of the output waveform
#define SAMPLES_PER_SECOND  10000  // (140000 max) ADC samples per second. Per Nyquist, set this at least 2 x FREQUENCY
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

float frequencies[] = {100.0};   // Hz, frequencies of the sine waves
float amplitudes[] = {1.0};       // amplitudes of the sine waves (range is from 0.0 to 1.0)
float phases[] = {0.0};      //{0.0, PI/4.0}     radians, phase angles of the sine waves in radians
float decay = 0.99;          // decay coefficient

//the following are set by the system at runtime
double now = 0;               // seconds. keeps track of the time, when dynamically generating the waveforms
long long sampleCount = 0;       // keeps track of time steps, when dynamically generating the waveforms
int numberOfWaves = 0;  // set automatically at runtime. 
int MICROSECONDS_PER_SAMPLE = 0;    //set automatically at runtime.

// Node structure for a circular linked list.
// This will hold the waveform values for one complete cycle.
struct node {
  int data;
  struct node *next;
};
struct node* currentNode;

template <typename T>     //TODO change the debug function in original
void debug(T value) {
  if (DEBUG){
    Serial.println(String(value));
  }
}

/* Helper function to get the count of elements in an array */
template<typename T, size_t N>
int countElements(T (&array)[N]) {
    return N;
}

void onTimer() {

  int waveform_value = 0;  //TODO change to int in original

  if (GENERATE_WAVES == STATIC){ //------STATIC GENERATION OF WAVEFORMS------

    // get the waveform value from the linked list (value of 0 to 255)
    waveform_value = currentNode->data;
    // output the voltage to the DAC_CHANNEL
    dac_output_voltage(DAC_CHANNEL, waveform_value);
    // advance to the value in the linked list
    currentNode = currentNode->next; 

  } else { //------DYNAMIC GENERATION OF WAVEFORMS------

    now = ((float)sampleCount) * ((float)MICROSECONDS_PER_SAMPLE); //seconds since start
    debug("now=" + String(now));

    for (int i=0; i<numberOfWaves; i++){
      debug("-----numberOfWaves=" + String(numberOfWaves) + ", wave number i=" + String(i));
      // Serial.println("i=" + String(i));
      float omega_t = TWO_PI * frequencies[i] * ((float)now);
      debug("omega_t=" + String(omega_t));
      float phi = phases[i];  //already in radians
      debug("phi=" + String(phi));
      // formula for a sine wave with frequency (ω) amplitude (A) and phase angle (φ)
      // f(t) = A sin(ωt + φ)      
      float sine_value = sin(omega_t + phi); // sine_value is a float between +/-1.0
      debug("raw sine_value=" + String(sine_value));

      // single_wave_value is a float between 0.0 and 1.0
      float single_wave_value = amplitudes[i] * sine_value;
      debug("single_wave_value=" + String(single_wave_value));

      waveform_value = waveform_value + (ATTENUATION * ( ((float)DAC_AMPLITUDE) + ( ((float)DAC_AMPLITUDE) * single_wave_value))); 

//      waveform_value = waveform_value + (ATTENUATION * (  (  single_wave_value))); 
      debug("waveform_value=" + String(waveform_value));

      //error check
      if (sizeof(waveform_value) > DAC_BIT_DEPTH){
        throw "ERROR: function onTimer(): waveform_value exceeeded the DAC_BIT_DEPTH: DAC_BIT_DEPTH=" + String(DAC_BIT_DEPTH) + ", waveform_value=" + String(waveform_value);
      }

    } //end for
    dac_output_voltage(DAC_CHANNEL, waveform_value);
    sampleCount++;
    debug("output waveform_value=" + String(waveform_value) + ", sampleCount=" + String(sampleCount));

  } //end else
}

void printSettings(){
  debug("Start printSettings()");

  Serial.println();
  Serial.println();
  Serial.println("=======================================================");  
  if (GENERATE_WAVES == STATIC){
    Serial.println("Generate            : STATIC waveform data");
  } else {
    Serial.println("Generate            : DYNAMIC waveform data");
  }
  Serial.println("Frequency           : " + String(FREQUENCY) + " Hz");
  Serial.println("Sample Rate         : " + String(SAMPLES_PER_SECOND) + " samples per second");
  Serial.println("Attenuation         : " + String(ATTENUATION));
  Serial.println("Samples Per Cycle   : " + String(SAMPLES_PER_CYCLE) + " samples per cycle");
  Serial.println("uSeconds Per Sample : " + String(MICROSECONDS_PER_SAMPLE) + " microseconds per sample");
  
  uint32_t clock_speed = esp_clk_cpu_freq() / 1000000;  //MHz  
  Serial.println("Clock_Speed         : " + String(clock_speed) + " MHz");

  Serial.println("=======================================================");
  Serial.println();

  debug("Finished printSettings()");
}

void setup() {
  debug("Start setup()");

  Serial.begin(115200); 
  delay(500); //a short delay to allow ESP32 to finish Serial output setup


  long MICROSECONDS_PER_SECOND = 1000000; //the timer has a resolution of 1 microsecond (nice!) 
  MICROSECONDS_PER_SAMPLE = MICROSECONDS_PER_SECOND / SAMPLES_PER_SECOND; //sets the global variable

  numberOfWaves = countElements(frequencies);

  printSettings();

  debug("Finished setup()");
}


int loopCount = 0;
void loop(){
  try {
    debug("----------------------------------------- " + String(loopCount));
    onTimer();
    delay(100);
    loopCount++;
  } catch (const std::exception &exc) {
    Serial.println(exc.what());
    exit(0);
  }   
}

#include <Arduino.h>

#define UNDEFINED_WAVE  0
#define SINE_WAVE       1
#define SQUARE_WAVE     2
#define TRIANGLE_WAVE   3

const float deg_to_rad = PI/180;

//ESP32 has two 8-bit DAC (digital to analog converter) channels, connected to GPIO25 (Channel 1) and GPIO26 (Channel 2)
//out_voltage = Vref * digi_val / 255
//Vref is internal (ESP32 does NOT support an external voltage reference)
int OUTPUT_PIN = 25;

int WAVE_TYPE = SINE_WAVE;

float angles[360] = {};

void setup() {
  Serial.begin(115200);
}

boolean firstLoop = true;

void loop() {
  //store the angles in an array 
  if (firstLoop == true) {
    for (int deg = 0; deg < 360; deg++){
      angles[deg] = deg * deg_to_rad;
      firstLoop = false;
    }
  }

  for (int deg = 0; deg < 360; deg++){
    float val = angles[deg];
//    printf("Angle in Radians = %f\n", val);

    if (WAVE_TYPE == SINE_WAVE){
      dacWrite(25, int(100 + 64 * (sin(angles[deg]))));
    } else if (WAVE_TYPE == SQUARE_WAVE){
      dacWrite(OUTPUT_PIN, int(128 + 80 * (sin(angles[deg])+sin(3*angles[deg])/3+sin(5*angles[deg])/5+sin(7*angles[deg])/7+sin(9*angles[deg])/9+sin(11*angles[deg])/11)));
    } else if (WAVE_TYPE == TRIANGLE_WAVE){
      dacWrite(OUTPUT_PIN, int(128 + 80 * (sin(angles[deg])+1/pow(3,2)*sin(3*angles[deg])+1/pow(5,2)*sin(5*angles[deg])+1/pow(7,2)*sin(7*angles[deg])+1/pow(9,2)*sin(9*angles[deg])))); 
    } else {
      //default to this...
      dacWrite(DAC1, 128);//255= 3.3V 128=1.65V
      delay(100);
    }
  }
}

// Square wave   = amplitude . sin(x) + sin(3.x) / 3 +  sin (5.x) / 5 + sin (7.x) / 7  + sin (9.x) / 9  + sin (11.x) / 11  Odd harmonics
// Triangle wave = amplitude . sin(x) - 1/3^2.sin(3.x) +  1/5^2.sin(5.x) - 1/7^2.sin (7.x) + 1/9^2.sin(9.x) - 1/11^2.sin (11.x) Odd harmonics
// dacWrite(25, int(128 + 80 * (sin(angles[deg])))); // GPIO Pin mode (OUTPUT) is set by the dacWrite function
// dacWrite(25, int(128 + 80 * (sin(angles[deg])+sin(3*angles[deg])/3+sin(5*angles[deg])/5+sin(7*angles[deg])/7+sin(9*angles[deg])/9+sin(11*angles[deg])/11))); // Square
// dacWrite(25, int(128 + 80 * (sin(angles[deg])+1/pow(3,2)*sin(3*angles[deg])+1/pow(5,2)*sin(5*angles[deg])+1/pow(7,2)*sin(7*angles[deg])+1/pow(9,2)*sin(9*angles[deg])))); // Triangle


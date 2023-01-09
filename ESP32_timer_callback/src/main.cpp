#include <Arduino.h>
#include <Time.h>
#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "esp_system.h"
#include "clk.h"
#include "driver/dac.h"
#include "driver/timer.h"

long SAMPLE_RATE = 10;
int TIMER_DIVIDER = 80; //do not change this value
long startTime;
hw_timer_t * timer = NULL;

void printClockSpeed(){
  //#include "clk.h"
  uint32_t clock_speed = esp_clk_cpu_freq();  
  Serial.println("clock_speed = " + String(clock_speed));
}

void onTimer(){
  Serial.println(String(millis() - startTime));
}

void setupCallbackTimer() {

  // set up timer 0 to generate a callback to onTimer() every 1 microsecond
  int timer_id = 0;
  boolean countUp = true;
  timer = timerBegin(timer_id, TIMER_DIVIDER, countUp);
  timerAttachInterrupt(timer, &onTimer, true);
  timerAlarmWrite(timer, 1000000 / SAMPLE_RATE, true);
}


void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200); delay(500); //a short delay is req'd to allow ESP32 to finish Serial output setup

  printClockSpeed();

  setupCallbackTimer();

  startTime = millis();

  timerAlarmEnable(timer);

}

void loop() {
  delay(60000);
}

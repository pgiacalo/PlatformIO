#include <Arduino.h>
#include <stdio.h>
#include "esp_system.h"
#include "esp_spi_flash.h"


void setup() {
  Serial.println("Start setup()");

  Serial.println("Finished setup()");
}

void loop(){
  Serial.println("loop()");
 delay(1000);
}
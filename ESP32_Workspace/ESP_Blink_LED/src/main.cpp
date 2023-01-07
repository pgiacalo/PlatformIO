/*********
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com/vs-code-platformio-ide-esp32-esp8266-arduino/
*********/

#include <Arduino.h>
#include <driver/gpio.h>  //an ESP32 library that defines GPIO_NUM_2 
#include "esp_system.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"

//#define LED 2
#define LED GPIO_NUM_26

void setup() {
  // put your setup code here, to run once:
  Serial.println("========setup() called==========");
  Serial.begin(115200);
  pinMode(LED, OUTPUT);

  Serial.println("\nData determined by the function esp_chip_info()");
  esp_chip_info_t info;
  esp_chip_info(&info);
  Serial.print("Chip Info Model: ");
  Serial.println(info.model, HEX);
  Serial.print("Chip Info Features: ");
  Serial.println(info.features);
  Serial.print("Chip Info Cores: ");
  Serial.println(info.cores, HEX);
  Serial.print("Chip Info Revision: ");
  Serial.println(info.revision, HEX);
  Serial.println();
}

/*
Data determined by the function esp_chip_info()
Chip Info Model: 1
Chip Info Features: 50
Chip Info Cores: 2
Chip Info Revision: 3

https://github.com/espressif/esp-idf/blob/8464186e67e34b417621df6b6f1f289a6c60b859/components/esp_hw_support/include/esp_chip_info.h
typedef enum {
    CHIP_ESP32  = 1, //!< ESP32
    CHIP_ESP32S2 = 2, //!< ESP32-S2
    CHIP_ESP32S3 = 9, //!< ESP32-S3
    CHIP_ESP32C3 = 5, //!< ESP32-C3
    CHIP_ESP32H4 = 6, //!< ESP32-H4
    CHIP_ESP32C2 = 12, //!< ESP32-C2
    CHIP_ESP32C6 = 13, //!< ESP32-C6
    CHIP_ESP32H2 = 16, //!< ESP32-H2
    CHIP_POSIX_LINUX = 999, //!< The code is running on POSIX/Linux simulator
} esp_chip_model_t;
*/
void loop() {
  // put your main code here, to run repeatedly:
  digitalWrite(LED, HIGH);
  Serial.println("LED is on");
  delay(500);
  digitalWrite(LED, LOW);
  Serial.println("LED is off");
  delay(500);
}
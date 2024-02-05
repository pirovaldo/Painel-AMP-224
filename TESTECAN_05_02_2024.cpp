#include <Arduino.h>
#include <stdlib.h>
#include <driver/adc.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "driver/can.h"
#include "EasyNextionLibrary.h"

EasyNex myNex(Serial2); // Display serial
#define LOOP_TIME 2000 // loop time
#define CAN_TX_PIN GPIO_NUM_17
#define CAN_RX_PIN GPIO_NUM_16
unsigned long timer;// positive time
uint16_t year = 0;
uint8_t month = 0;
uint8_t day = 0;
uint8_t hour = 0;
uint8_t minute = 0;
uint8_t sec = 0;
TaskHandle_t Task1 = NULL;
TaskHandle_t Task2 = NULL;


void setupCan(){
  can_general_config_t g_config = CAN_GENERAL_CONFIG_DEFAULT(CAN_TX_PIN, CAN_RX_PIN, CAN_MODE_NORMAL);
  can_timing_config_t t_config = CAN_TIMING_CONFIG_500KBITS();
  can_filter_config_t f_config;
  f_config.acceptance_code = (0x7FF << 21);
  f_config.acceptance_mask = ~(0x020 << 21);
  f_config.single_filter = true;

  
  if(can_driver_install(&g_config, &t_config, &f_config) == ESP_OK){
    // This installs the CAN driver
    Serial.print("CAN driver installed\n");
  }
  if (can_start() == ESP_OK) {
    // This starts the CAN driver
    Serial.print("CAN driver started\n");
  }
}

void Task1code (void * pvParameters)
{
  while(1) 
  {
    //Envia a mensagem
    can_message_t message2;
    message2.identifier = 0x010;         
    message2.data_length_code = 7;       
    message2.data[0] = (sec);
    message2.data[1] = (minute);
    message2.data[2] = (hour);
    message2.data[3] = (day);
    message2.data[4] = (month);
    message2.data[5] = (year);
    message2.data[6] = (year >> 8);


    can_transmit(&message2, pdMS_TO_TICKS(10));
    vTaskDelay(1 / portTICK_PERIOD_MS);   
}
}

void Task2code (void * pvParameters )
{
  while(1)
  {
    if((millis() - timer) > LOOP_TIME){
    year = myNex.readNumber("n2.val");
    Serial.println(year);
    month = myNex.readNumber("n1.val");
    Serial.println(month);
    day = myNex.readNumber("n0.val");
    Serial.println(day);
    hour = myNex.readNumber("n3.val");
    Serial.println(hour);
    minute = myNex.readNumber("n4.val");
    Serial.println(minute);
    sec = myNex.readNumber("n5.val");
    Serial.println(sec);
    

   }
    vTaskDelay(20 / portTICK_PERIOD_MS);
  }
}

void SetupTasks(){
  xTaskCreatePinnedToCore
  (
                    Task1code,                /* Task function. */
                    "Task1",                  /* name of task. */
                    10000,                    /* Stack size of task */
                    NULL,                     /* parameter of the task */
                    0,                        /* priority of the task */
                    &Task1,                   /* Task handle to keep track of created task */
                    tskNO_AFFINITY);          /* lets the RTOS decide the core*/   

  xTaskCreatePinnedToCore
  (
                    Task2code,                // Task function. 
                    "Task2",                  // name of task. 
                    10000,                    // Stack size of task 
                    NULL,                     // parameter of the task 
                    0,                        // priority of the task 
                    &Task2,                   // Task handle to keep track of created task 
                    tskNO_AFFINITY);          // lets the RTOS decide the core

}

void setup() {
  // put your setup code here, to run once:
  setupCan();
  Serial2.begin(115200, SERIAL_8N1, 25, 33); //Display serial begin 
  myNex.begin(115200);
  timer = millis();

  delay(500);
}


void loop() {
   vTaskDelay(100/ portTICK_PERIOD_MS);
}

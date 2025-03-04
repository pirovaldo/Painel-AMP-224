Ampera Racing - AMP-224 PAINEL
Analista Responsável - Guilherme Lettmann Penha
Head - Tomas Carrasco Ferrarezi 
Diretor - Marina Grisotti
Projetista - Lucas Paiva
*/

#include <Arduino.h>
#include <stdlib.h>
#include <driver/adc.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "driver/can.h"
#include "EasyNextionLibrary.h" //Library do Display

//Configuração chave seletora
#define SELECTOR_PIN_1 GPIO_NUM_23
#define SELECTOR_PIN_2 GPIO_NUM_22
#define SELECTOR_PIN_3 GPIO_NUM_19
#define SELECTOR_PIN_4 GPIO_NUM_21
#define SELECTOR_PIN_5 GPIO_NUM_18 // mudei os pinos
portMUX_TYPE selectorMux = portMUX_INITIALIZER_UNLOCKED;

//Configuracao pinos Falha Driverless
#define FALHA_DL_1 GPIO_NUM_5
#define FALHA_DL_2 GPIO_NUM_4


//Declaração das variáveis
uint16_t power = 0, botao = 0, SelectorPosition = 0, lastPosition = 0, accumulatorTemp, apps = 0, RTD, REGEN = 0, GPS, motorTemp, lowVoltage, StateofCharge, fault_bms, fault_inv, fault_ecu, inversorVoltage, RPM;
float speed = 0, highVoltage; // revisao das variaveis a serem mostradas
long double accumulatorCurrent;
int diferentialValue = 0; //valor do potenciometro do diferencial
int CurrentForm = 0; // variavel para controle da página atual
int period = 100; // tempo entre envio de dados para o display
unsigned long time_now = 0; // variavel para controle de envio de dados para o display
bool display_lock = false; // false: pode mudar a página, true: nao pode mudar a página

#define REGEN_PIN GPIO_NUM_15
portMUX_TYPE REGENbutton = portMUX_INITIALIZER_UNLOCKED;

#define CAN_TX_PIN GPIO_NUM_17
#define CAN_RX_PIN GPIO_NUM_16

EasyNex myNex(Serial2); //definir o Serial2 como serial do display

//Declaração das tarefas
TaskHandle_t Task1 = NULL;
TaskHandle_t Task2 = NULL;
TaskHandle_t Task3 = NULL;


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

/*
  functions of each task
  TASK1 -> CITEX selector and regen button handling
  TASK2 -> update display
  TASK3 -> CAN BUS
*/

void Task1code( void * pvParameters ) //task do seletor
{
  while(1)
  {
    vTaskDelay(400 / portTICK_PERIOD_MS);
    //Mudança da chave seletora
    if (display_lock == false){ 
      if (SelectorPosition != CurrentForm && SelectorPosition == 0)
      {
        myNex.writeStr("page page0");
        CurrentForm = 0;
        //Serial.print("page0");
      }
      if (SelectorPosition != CurrentForm && SelectorPosition == 1)
      {
        myNex.writeStr("page page1");
        CurrentForm = 1;
        //Serial.print("page1");
      }
      if (SelectorPosition != CurrentForm && SelectorPosition == 2)
      {
        myNex.writeStr("page page2");
        CurrentForm = 2;
       // Serial.print("page2");
      }
      if (SelectorPosition != CurrentForm && SelectorPosition == 3)
      {
        myNex.writeStr("page page3");
        CurrentForm = 3;
        //Serial.print("page3");
      }
      if (SelectorPosition != CurrentForm && SelectorPosition == 4)
      {
        myNex.writeStr("page page4");
        CurrentForm = 4;
        //Serial.print("page4");
      }
      display_lock = true;
    }
  }
  //Acionamento do botão REGEN no volante
  if (digitalRead(REGEN_PIN)==HIGH) {
      REGEN = 1;
    }
  else REGEN = 0; 
  
  vTaskDelay(30 / portTICK_PERIOD_MS);
}

void Task2code( void * pvParameters )
{
  while(1)
  {
    //Atualização dos objetos do painel
    vTaskDelay(10 / portTICK_PERIOD_MS);
    //Serial.println("------------Entrou 2----------------");
    if (millis() >= time_now + period){ //update display variables every 100 miliseconds 
      time_now += period;
      switch (CurrentForm) // Varia com a página atual do display
      {
        case 1: //Testes
          myNex.writeNum("n0.val", highVoltage); //Tensão
          myNex.writeNum("n1.val", inversorVoltage); //Tensão Inversão
          myNex.writeNum("n2.val", accumulatorCurrent); //Corrente Acumulador
          myNex.writeNum("n3.val", StateofCharge); //SOC
          myNex.writeNum("n4.val", motorTemp); //temperatura Motores
          myNex.writeNum("n5.val", accumulatorTemp); //Temperatura Acumulador
          myNex.writeNum("n6.val", fault_bms); //Erro BMS
          myNex.writeNum("n7.val", fault_inv); //Erro Inversor
          myNex.writeNum("n8.val", fault_ecu); //Erro ECU
        break;
        case 2: //provas curtas
          myNex.writeNum("n0.val", speed); //Velocidade
          myNex.writeNum("n1.val", power); //Potência dos motores
        break;
        case 3: //Endurance
          myNex.writeNum("n0.val", speed); //Velocidade
          //myNex.writeNum("n1.val", 30); //Temp. freio
          myNex.writeNum("n2.val", StateofCharge); //Baterias (%)
        default:
        break; // tirei o do encoder
      }
    }
    vTaskDelay(20 / portTICK_PERIOD_MS);
  }
}

void Task3code (void * pvParameters)
{
  while(1) 
  {
  //Recebe a mensagem
  can_message_t message1;
  can_receive(&message1, pdMS_TO_TICKS(10));
    switch (message1.identifier)
    {    
      case 0x0A0:
        fault_inv = message1.data[0];
        inversorVoltage = message1.data[1];
        motorTemp = message1.data[2];
        apps = (message1.data[3]);
        speed = (message1.data[4]);
      break;
    case 0x672:
        fault_bms = message1.data[4];
      break;
    case 0x673:
        highVoltage = (message1.data[0] << (8) | message1.data[1]);
      break;
    case 0x674:
        accumulatorCurrent = (message1.data[0] << (8) | message1.data[1]);
        power = (accumulatorCurrent * highVoltage)/1000;
      break;
    case 0x676:
        StateofCharge = message1.data[0];
      break;
    case 0x677:
        accumulatorTemp = message1.data[0];
      break;
    case 0x0C0:
      fault_ecu = (message1.data[3] << (8) | message1.data[2]);
    default:
      break;}
    
    vTaskDelay(10 / portTICK_PERIOD_MS);

    //Envia a mensagem
    can_message_t message2;
    message2.identifier = 0x010;         // CAN message identifier
    message2.data_length_code = 1;       // CAN message data length - 1 byte
    message2.data[0] = (botao | SelectorPosition << 5); // mudei isso aqui
    can_transmit(&message2, pdMS_TO_TICKS(10));
    vTaskDelay(1 / portTICK_PERIOD_MS);   //pensar em um modo de nao mandar o rtd constantemente
}
}

//Interrupção responsável pela mudança da chave seletora
void IRAM_ATTR selector_change(){
  portENTER_CRITICAL_ISR(&selectorMux);
  //A chave seletora envia nível baixo na posição atual
    if ((digitalRead(SELECTOR_PIN_1))&&(digitalRead(SELECTOR_PIN_2))&&(digitalRead(SELECTOR_PIN_3))&&(digitalRead(SELECTOR_PIN_4))&&(SelectorPosition == 1)){
      SelectorPosition = 0;}
    if (digitalRead(SELECTOR_PIN_1) == 0){
      SelectorPosition = 1;}
    if (digitalRead(SELECTOR_PIN_2) == 0){
      SelectorPosition = 2;}
    if (digitalRead(SELECTOR_PIN_3) == 0){
      SelectorPosition = 3;}
    if (digitalRead(SELECTOR_PIN_4) == 0){
      SelectorPosition = 4;}
    display_lock = false; // lock the display from changing pages
  portEXIT_CRITICAL_ISR(&selectorMux);
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

  xTaskCreatePinnedToCore
  (
                    Task3code,                // Task function. 
                    "Task3",                  // name of task. 
                    10000,                    // Stack size of task 
                    NULL,                     // parameter of the task 
                    0,                        // priority of the task 
                    &Task3,                   // Task handle to keep track of created task 
                    tskNO_AFFINITY);          // lets the RTOS decide the core

}

void setup(){
  Serial.begin(115200);
  setupCan();

  Serial2.begin(115200, SERIAL_8N1, 25, 33); //Início do serial do display
  myNex.begin(115200);

  delay (500); // give the display time to finish starting up

  //declaração de pinos 
  pinMode (SELECTOR_PIN_1, INPUT_PULLUP); 
  pinMode (SELECTOR_PIN_2, INPUT_PULLUP);
  pinMode (SELECTOR_PIN_3, INPUT_PULLUP); 
  pinMode (SELECTOR_PIN_4, INPUT_PULLUP); 
  pinMode (SELECTOR_PIN_5, INPUT_PULLUP);
  pinMode (REGEN_PIN, INPUT_PULLDOWN);
  
  //Declaração das interrupções
  attachInterrupt(SELECTOR_PIN_1, selector_change, CHANGE);
  attachInterrupt(SELECTOR_PIN_2, selector_change, CHANGE);
  attachInterrupt(SELECTOR_PIN_3, selector_change, CHANGE);
  attachInterrupt(SELECTOR_PIN_4, selector_change, CHANGE);
  attachInterrupt(SELECTOR_PIN_5, selector_change, CHANGE);

  SetupTasks();
}

void loop() {
  vTaskDelay(100/ portTICK_PERIOD_MS);
}
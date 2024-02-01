#include <Arduino.h>
#include "EasyNextionLibrary.h"

// put function declarations here:
EasyNex myNex(Serial2); // Display serial
#define LOOP_TIME 2000 // loop time
unsigned long timer;// positive time
uint16_t year = 0;
uint8_t month = 0;
uint8_t day = 0;
uint8_t hour = 0;
uint8_t min = 0;
uint8_t sec = 0;

void setup() {
  // put your setup code here, to run once:
  Serial2.begin(115200, SERIAL_8N1, 25, 33); //Display serial begin 
  myNex.begin(115200);
  timer = millis(); 
}

void loop() {
  // put your main code here, to run repeatedly:
   if((millis() - timer) > LOOP_TIME){
    year = myNex.readNumber("n2.val");
    Serial.println(year);
    month = myNex.readNumber("n1.val");
    Serial.println(month);
    day = myNex.readNumber("n0.val");
    Serial.println(day);
    hour = myNex.readNumber("n3.val");
    Serial.println(hour);
    min = myNex.readNumber("n4.val");
    Serial.println(min);
    sec = myNex.readNumber("n5.val");
    Serial.println(sec);
    

   }
}

#include <Arduino.h>
#include <EasyNextionLibrary.h>

EasyNex myNex(Serial2); 
uint32_t number = 0;
uint32_t lastnumber = 0;
#define LOOP_TIME 2000
unsigned long timer ;

void setup(){
  myNex.begin(115200); 
  delay(500);        
  timer = millis();
}

void loop(){

  if((millis() - timer) > LOOP_TIME){
    number = myNex.readNumber("n0.val");
    lastnumber = number;   
    myNex.writeNum("n1.val", number);       
    timer = millis();
               
  }
}
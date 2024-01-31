#include <Arduino.h>
#include "driver/gpio.h"

// put function declarations here:
#define BUTTON GPIO_NUM_15
bool RTD = false;

void setup() {
  // put your setup code here, to run once:
   pinMode (BUTTON, INPUT_PULLDOWN);
   Serial.begin(115200);
}

void loop() {
  // put your main code here, to run repeatedly:
  digitalRead(BUTTON);
  if (digitalRead(BUTTON)==HIGH) {
      RTD = HIGH;
    Serial.println("RTD Apertado");
    }
  else { RTD = LOW;
        Serial.println("RTD Solto");
    }
  


}

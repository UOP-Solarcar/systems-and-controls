#include <Arduino.h>

const uint8_t firstPin = 2;
const uint8_t lastPin  = 12;

void setup() {
  Serial.begin(9600);

  for (uint8_t i = firstPin; i <= lastPin; i++) {
    pinMode(i, INPUT_PULLUP);  // Button between pin and GND
  }
}

void loop() {
  for (uint8_t i = firstPin; i <= lastPin; i++) {
    if (digitalRead(i) == LOW) {  // LOW = button pressed
      Serial.print("Pin ");
      Serial.print(i);
      Serial.println(" pressed");
      delay(200); // simple debounce + prevents spam
    }
  }
}


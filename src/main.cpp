#include <Arduino.h>

const uint8_t RELAY_PIN_1 = 2;
const uint8_t RELAY_PIN_2 = 3;
const uint8_t RELAY_PIN_3 = 4;
const uint8_t RELAY_PIN_4 = 5;
const uint8_t RELAY_PIN_5 = 6;
const uint8_t RELAY_PIN_6 = 7;
const uint8_t RELAY_PIN_7 = 8;
const uint8_t RELAY_PIN_8 = 9;

void setup() {
    pinMode(RELAY_PIN_1, OUTPUT);
    pinMode(RELAY_PIN_2, OUTPUT);
    pinMode(RELAY_PIN_3, OUTPUT);
    pinMode(RELAY_PIN_4, OUTPUT);
    pinMode(RELAY_PIN_5, OUTPUT);
    pinMode(RELAY_PIN_6, OUTPUT);
    pinMode(RELAY_PIN_7, OUTPUT);
    pinMode(RELAY_PIN_8, OUTPUT);

    // Initialize relay pins as off
    digitalWrite(RELAY_PIN_1, HIGH);
    digitalWrite(RELAY_PIN_2, HIGH);
    digitalWrite(RELAY_PIN_3, HIGH);
    digitalWrite(RELAY_PIN_4, HIGH);
    digitalWrite(RELAY_PIN_5, HIGH);
    digitalWrite(RELAY_PIN_6, HIGH);
    digitalWrite(RELAY_PIN_7, HIGH);
    digitalWrite(RELAY_PIN_8, HIGH);

}

void loop() {
    // Main code goes here I guess

}
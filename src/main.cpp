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
    Serial.begin(9600);
    
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
    // Cycle through relays
    Serial.println("Turning on relay 1");
    digitalWrite(RELAY_PIN_1, LOW);
    delay(1000);
    Serial.println("Turning off relay 1");
    digitalWrite(RELAY_PIN_1, HIGH);

    Serial.println("Turning on relay 2");
    digitalWrite(RELAY_PIN_2, LOW);
    delay(1000);
    Serial.println("Turning off relay 2");
    digitalWrite(RELAY_PIN_2, HIGH);

    Serial.println("Turning on relay 3");
    digitalWrite(RELAY_PIN_3, LOW);
    delay(1000);
    Serial.println("Turning off relay 3");
    digitalWrite(RELAY_PIN_3, HIGH);

    Serial.println("Turning on relay 4");
    digitalWrite(RELAY_PIN_4, LOW);
    delay(1000);
    Serial.println("Turning off relay 4");
    digitalWrite(RELAY_PIN_4, HIGH);

    Serial.println("Turning on relay 5");
    digitalWrite(RELAY_PIN_5, LOW);
    delay(1000);
    Serial.println("Turning off relay 5");
    digitalWrite(RELAY_PIN_5, HIGH);

    Serial.println("Turning on relay 6");
    digitalWrite(RELAY_PIN_6, LOW);
    delay(1000);
    Serial.println("Turning off relay 6");
    digitalWrite(RELAY_PIN_6, HIGH);

    Serial.println("Turning on relay 7");
    digitalWrite(RELAY_PIN_7, LOW);
    delay(1000);
    Serial.println("Turning off relay 7");
    digitalWrite(RELAY_PIN_7, HIGH);

    Serial.println("Turning on relay 8");
    digitalWrite(RELAY_PIN_8, LOW);
    delay(1000);
    Serial.println("Turning off relay 8");
    digitalWrite(RELAY_PIN_8, HIGH);

}

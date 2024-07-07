#include <Arduino.h>

const uint8_t RIGHT_PIN = 2;
const uint8_t LEFT_PIN = 3;
const uint8_t HEADLIGHT_PIN = 4;
const uint8_t RELAY_PIN_4 = 5;
const uint8_t RELAY_PIN_5 = 6;
const uint8_t RELAY_PIN_6 = 7;
const uint8_t RELAY_PIN_7 = 8;
const uint8_t RELAY_PIN_8 = 9;

enum SignalState {
  HAZARD,
  TURNRIGHTBRAKE,
  TURNLEFTBRAKE,
  TURNLEFT,
  TURNRIGHT,
  DEF
};

enum HeadlightState {
  ON,
  OFF
};

SignalState state = TURNLEFTBRAKE;
HeadlightState Hlstate = ON;

void reset(){
    // Initialize relay pins as off
    digitalWrite(RIGHT_PIN, HIGH);
    digitalWrite(LEFT_PIN, HIGH);
    digitalWrite(HEADLIGHT_PIN, HIGH);
    digitalWrite(RELAY_PIN_4, HIGH);
    digitalWrite(RELAY_PIN_5, HIGH);
    digitalWrite(RELAY_PIN_6, HIGH);
    digitalWrite(RELAY_PIN_7, HIGH);
    digitalWrite(RELAY_PIN_8, HIGH);
}
void setup() {
    Serial.begin(9600);

    pinMode(RIGHT_PIN, OUTPUT);
    pinMode(LEFT_PIN, OUTPUT);
    pinMode(HEADLIGHT_PIN, OUTPUT);
    pinMode(RELAY_PIN_4, OUTPUT);
    pinMode(RELAY_PIN_5, OUTPUT);
    pinMode(RELAY_PIN_6, OUTPUT);
    pinMode(RELAY_PIN_7, OUTPUT);
    pinMode(RELAY_PIN_8, OUTPUT);

    reset();
}

void flash(uint8_t pin, int wait = 750){
    digitalWrite(pin, LOW);
    delay(wait);
    digitalWrite(pin, HIGH);
    delay(wait);
}

void flash2(uint8_t pin0, uint8_t pin1, int wait = 750){
    digitalWrite(pin0, LOW);
    digitalWrite(pin1, LOW);
    delay(wait);
    digitalWrite(pin0, HIGH);
    digitalWrite(pin1, HIGH);
    delay(wait);
}
void loop() {
    switch (state) {
      case HAZARD:
        flash2(RIGHT_PIN, LEFT_PIN);
        break;
      case TURNRIGHTBRAKE:
        digitalWrite(LEFT_PIN, LOW);
        flash(RIGHT_PIN);
        break;
      case TURNLEFTBRAKE:
        digitalWrite(RIGHT_PIN, LOW);
        flash(LEFT_PIN);
        break;
      case TURNLEFT:
        digitalWrite(LEFT_PIN, LOW);
        break;
      case TURNRIGHT:
        digitalWrite(RIGHT_PIN, LOW);
        break;
      case DEF:
        reset();
        break;
    }
    switch (Hlstate){
      case ON:
        digitalWrite(HEADLIGHT_PIN, LOW);
        break;
      case OFF:
        digitalWrite(HEADLIGHT_PIN, HIGH);
        break;
    }
}

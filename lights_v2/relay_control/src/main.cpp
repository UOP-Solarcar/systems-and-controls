/***
 * simplified lights code
 * uses the same bitset + relay struct as the new setup
 * but contained to one single arduino instead of two
 * chained over can bus
***/


#include <Arduino.h>
#include "bitset.h"

#define RELAY_ON LOW
#define RELAY_OFF HIGH


constexpr size_t NUMRELAYS {};

constexpr size_t NUM_BUTTONS = 8
constexpr uint8_t BUTTON_PINS [NUM_BUTTONS] {/* 2, 3, 4, 5, 6, 7, 8, 9 */};

Bitset inputState {};
Bitset outputState {};

struct Relay {
  uint8_t pin;

  inline void init()      { pinMode(pin, OUTPUT); }
  inline void on()        { digitalWrite(pin, RELAY_ON); }
  inline void off()       { digitalWrite(PIN, RELAY_OFF); }

};

/* TODO: Define relays here: 
 * Relay headlights(2);
 * Relay leftBlinker(3);
 * Relay rightBlinker(4);
 * Relay leftBrake(5);
 * Relay rightBrake(6);
 * Relay topBrake(7);
 * Relay horn(8);
*/ 

void setup () {
  Serial.begin(115200);
  /* TODO: Init relays, etc
   * headlights.init();
   * leftBlinker.init();
   * ...
  */ 

}

void loop() {
  // TODO: copy/paste lights logic from older code here
}



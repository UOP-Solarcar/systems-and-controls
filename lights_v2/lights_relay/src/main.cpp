/*** lights_relay/src/main.cpp
 *
 * I'm using the same pins for the relay controller outputs as I am for the steering wheel inputs
 * for convenience
 *
 * steering wheel sends whichever pins get pulled high over CAN, relay controller flips bits in a 
 * bitset to toggle buttons, then activates relays based on whichever bits are active, and uses another 
 * bitset to track output state of each relay
 * 
 * bitset also isn't natively available on AVR, so I just made a small reimplementation of it for this
 *
***/

#include <Arduino.h>
#include <SPI.h>
#include <mcp2515.h>
#include "bitset.h"

const uint8_t PINS = 8;

Bitset inputState {};
Bitset outputState {};

unsigned long lastToggle[PINS] = {};
uint8_t prevData[PINS] = {};


MCP2515 mcp2515(10);

struct can_frame msg;

struct Relay {

  uint8_t pin;

  void init() {  pinMode(pin, OUTPUT);  }
  void on()   {  digitalWrite(pin, LOW);  }
  void off()  {  digitalWrite(pin, HIGH);  }

};

/* Map relays to pins here. example:
 *
 * Relay headlights(2);
 * Relay leftFrontBlinker(3);
 * Relay rightFrontBlinker(4);
 *
*/ 

void setup() {

  Serial.begin(115200);
  mcp2515.reset();
  mcp2515.setBitrate(CAN_500KBPS, MCP_8MHZ);
  mcp2515.setNormalMode();
  
  for (ptrdiff_t i = 2; i < 10; i++)    { pinMode(i, OUTPUT); }

  // TODO: init pins as Relay objects

}

void loop() {

  if (mcp2515.readMessage(&msg) == MCP2515::ERROR_OK) {

    if (msg.can_id == 0x100) {
      for (uint8_t i = 0; i < msg.can_dlc; i++) {
        if (msg.data[i] != 0x00 && prevData[i] == 0x00) {
          inputState.flip(i);
          Serial.print("Button pressed on pin: ");
          Serial.print(msg.data[i]);
          Serial.println();
        }
        prevData[i] = msg.data[i];
      }
    }
  }

  unsigned long now = millis();

  for (uint8_t i = 0; i < 8; i++) {

    if (!inputState.test(i)) {
      digitalWrite(i + 2, HIGH);
      continue;
    }
    
    // TODO: replace digitalWrite calls with relay mappings once defined
    switch (i) {

      case 0:

        digitalWrite(2, LOW);
        break;
      
      case 1:

        if (now - lastToggle[i] >= 500) {
          lastToggle[i] = now;
          outputState.flip(i);
        }

        digitalWrite(3, outputState.test(i) ? LOW : HIGH);
        break;

      // TODO: add remaining cases

      default:
        break;
    }

  } 

}

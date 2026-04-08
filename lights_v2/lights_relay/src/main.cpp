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
const bool FRAME_DEBUG = false;

Bitset inputState {};
Bitset outputState {};

unsigned long lastToggle[PINS] = {};
uint8_t prevData[PINS] = {};


MCP2515 mcp2515(10);

struct can_frame msg;

struct Relay {

  uint8_t pin;

  Relay(uint8_t pin) : pin(pin) {}

  inline void init() {  pinMode(pin, OUTPUT);  }
  inline void on()   {  digitalWrite(pin, LOW);  }
  inline void off()  {  digitalWrite(pin, HIGH);  }

};

/* Map relays to pins here. example:
 *
 * Relay headlights(2);
 * Relay leftFrontBlinker(3);
 * Relay rightFrontBlinker(4);
 *
*/ 

Relay headlights(2);
Relay leftFrontBlinker(3);
Relay rightFrontBlinker(4);
Relay backRightBlinker(5);
Relay backLeftBlinker(6);
Relay topBrakeLight(7);
Relay horn(8);

void setup() {

  Serial.begin(115200);
  mcp2515.reset();
  mcp2515.setBitrate(CAN_500KBPS, MCP_8MHZ);
  mcp2515.setConfigMode();
  mcp2515.setFilterMask(MCP2515::MASK0, false, 0x000);
  mcp2515.setFilterMask(MCP2515::MASK1, false, 0x000);
  mcp2515.setFilter(MCP2515::RXF0, false, 0x000);
  mcp2515.setFilter(MCP2515::RXF1, false, 0x000);
  mcp2515.setFilter(MCP2515::RXF2, false, 0x000);
  mcp2515.setFilter(MCP2515::RXF3, false, 0x000);
  mcp2515.setFilter(MCP2515::RXF4, false, 0x000);
  mcp2515.setFilter(MCP2515::RXF5, false, 0x000);
  mcp2515.setNormalMode();
  
  headlights.init();
  leftFrontBlinker.init();
  rightFrontBlinker.init();
  backRightBlinker.init();
  backLeftBlinker.init();
  topBrakeLight.init();
  horn.init();


}

void loop() {
  
  if (mcp2515.readMessage(&msg) == MCP2515::ERROR_OK) {

    if (FRAME_DEBUG){
    
      // Dump all frames
      Serial.print("ID: 0x");
      Serial.print(msg.can_id, HEX);
      Serial.print(" DLC: ");
      Serial.print(msg.can_dlc);
      Serial.print(" Data:");
      for (uint8_t i = 0; i < msg.can_dlc; i++) {
        Serial.print(" 0x");
        Serial.print(msg.data[i], HEX);
      }
      Serial.println();
  
    }

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

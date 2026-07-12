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
#include "button.h"
#include "input_decoder.h"

const uint8_t PINS = 8;
const bool FRAME_DEBUG = false;

Bitset inputState {};
uint8_t prevData[PINS] = {};

unsigned long flasherT0 = 0;
const unsigned long interval = 500;


MCP2515 mcp2515(10);

struct can_frame msg;

struct Relay {

  uint8_t pin;
  bool closed = false;

  Relay(uint8_t pin) : pin(pin) {}

  inline void init()      { pinMode(pin, OUTPUT); digitalWrite(pin, HIGH); }
  inline void close()     { digitalWrite(pin, LOW);  closed = true; }
  inline void open()      { digitalWrite(pin, HIGH); closed = false; }
  inline void toggle()    { closed ? open() : close(); }
  inline bool isClosed()  { return closed; }

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

Button RightSignal(0, true);
Button HeadlightsBtn(1, true);
Button BrakeSignal(2, false);
Button HornBtn(3, false);
Button HazardBtn(6, true);
Button LeftSignal(7, true);

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
    
    // Bitmask for CAN frames
    if (msg.can_id == 0x100) {
      uint8_t pressedMask = updateInputState(inputState, prevData, msg.data, msg.can_dlc);
      for (uint8_t i = 0; i < msg.can_dlc; i++) {
        if (pressedMask & (1U << i)) {
          Serial.print("Button pressed on pin: ");
          Serial.print(msg.data[i]);
          Serial.println();
        }
      }
    }
  }

  // Map input state bits to named signals
  // bit 0 = pin 2 = right turn, bit 1 = pin 3 = headlights, bit 2 = pin 4 = brake
  // bit 3 = pin 5 = horn,       bit 6 = pin 8 = hazards,    bit 7 = pin 9 = left turn
  bool rightSignal   = RightSignal.update(inputState.test(0));
  bool headlightsBtn = HeadlightsBtn.update(inputState.test(1));
  bool brakeSignal   = BrakeSignal.update(inputState.test(2));
  bool hornBtn       = HornBtn.update(inputState.test(3));
  bool hazardBtn     = HazardBtn.update(inputState.test(6));
  bool leftSignal    = LeftSignal.update(inputState.test(7));

  // Hazards
  if (hazardBtn) {
    Serial.println("hazards");
    if (millis() - flasherT0 >= interval) {
      leftFrontBlinker.toggle();
      rightFrontBlinker.toggle();
      backLeftBlinker.toggle();
      backRightBlinker.toggle();
      flasherT0 = millis();
    }
  } else {
    if (leftFrontBlinker.isClosed())  leftFrontBlinker.open();
    if (rightFrontBlinker.isClosed()) rightFrontBlinker.open();
  }

  // Left turn signal
  if (leftSignal) {
    Serial.println("left signal");
    if (millis() - flasherT0 >= interval) {
      leftFrontBlinker.toggle();
      backLeftBlinker.toggle();
      flasherT0 = millis();
    }
  } else if (!hazardBtn) {
    if (leftFrontBlinker.isClosed()) leftFrontBlinker.open();
  }

  // Right turn signal
  if (rightSignal) {
    Serial.println("right signal");
    if (millis() - flasherT0 >= interval) {
      rightFrontBlinker.toggle();
      backRightBlinker.toggle();
      flasherT0 = millis();
    }
  } else if (!hazardBtn) {
    if (rightFrontBlinker.isClosed()) rightFrontBlinker.open();
  }

  // Headlights
  if (headlightsBtn) {
    Serial.println("headlights");
    headlights.close();
  } else {
    if (headlights.isClosed()) headlights.open();
  }

  // Brake
  if (brakeSignal) {
    Serial.println("brake");
    topBrakeLight.close();
    if (!leftSignal || !hazardBtn) {
      backLeftBlinker.close();
    }
    if (!rightSignal || !hazardBtn) {
      backRightBlinker.close();
    }
  } else {
    topBrakeLight.open();
    if (!leftSignal && !hazardBtn) {
      backLeftBlinker.open();
    }
    if (!rightSignal && !hazardBtn) {
      backRightBlinker.open();
    }
  }

  // Horn
  if (hornBtn) {
    Serial.println("horn");
    horn.close();
  } else {
    if (horn.isClosed()) horn.open();
  }

}

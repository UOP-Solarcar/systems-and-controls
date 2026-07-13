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
unsigned long hazardT0 = 0;
bool hazardPhase = false;
bool prevLeftSignal = false;
bool prevRightSignal = false;
bool prevHazard = false;
bool prevHeadlights = false;
bool prevBrake = false;
bool prevHorn = false;
const unsigned long interval = 500;

// Announce a signal on its rising edge only. Printing every loop while a button
// is held stalls the loop long enough to back up the CAN RX buffers, which makes
// presses register late.
inline void logOnRise(bool state, bool &prev, const char *label) {
  if (state && !prev) Serial.println(label);
  prev = state;
}


MCP2515 mcp2515(10);

struct can_frame msg;

struct Relay {

  uint8_t pin;
  bool closed = false;

  Relay(uint8_t pin) : pin(pin) {}

  // Active-high relay module: driving the pin HIGH energizes (closes) the relay.
  inline void init()      { pinMode(pin, OUTPUT); digitalWrite(pin, LOW); }
  inline void close()     { digitalWrite(pin, HIGH); closed = true; }
  inline void open()      { digitalWrite(pin, LOW);  closed = false; }
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

Relay horn(A0);              // horn
Relay headlights(A1);        // headlights
Relay backLeftBlinker(A2);   // left rear indicator
Relay backRightBlinker(A3);  // right rear indicator
Relay topBrakeLight(A4);     // brake light
// A6/A7 are analog-input-only on the Nano's ATmega328P (no GPIO output driver),
// so the front blinkers can't live there - moved to digital pins D2/D3.
Relay leftFrontBlinker(2);   // left turn signal
Relay rightFrontBlinker(3);  // right turn signal

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

  // Turn signals are mutually exclusive, like a physical turn stalk: engaging
  // one side cancels the other so both can never latch on at once. Both-on is a
  // state the driver has no way to clear (only hazards flash both sides), and it
  // also left the two flashers fighting over the shared timer - the starved side
  // stayed latched but invisible, so pressing one side appeared to toggle the
  // other and the signals could never be turned off.
  bool leftEngaged  = leftSignal  && !prevLeftSignal;
  bool rightEngaged = rightSignal && !prevRightSignal;
  if (leftEngaged) {
    RightSignal.on = false;
    rightSignal = false;
    rightEngaged = false;
  }
  if (rightEngaged) {
    LeftSignal.on = false;
    leftSignal = false;
  }
  prevLeftSignal  = leftSignal;
  prevRightSignal = rightSignal;

  // Announce each signal on its rising edge only (see logOnRise). The turn
  // signals already carry engage edges from the mutual-exclusion pass above.
  if (leftEngaged)  Serial.println("left signal");
  if (rightEngaged) Serial.println("right signal");
  logOnRise(hazardBtn, prevHazard, "hazards");
  logOnRise(headlightsBtn, prevHeadlights, "headlights");
  logOnRise(brakeSignal, prevBrake, "brake");
  logOnRise(hornBtn, prevHorn, "horn");

  // Hazards: flash all four indicators together. Drive every lamp to a single
  // shared phase off a dedicated timer (rather than toggle()ing each one), so a
  // brake tap or a latched turn signal can't leave one lamp inverted and put
  // the hazards out of sync. Takes priority over the turn signals below.
  if (hazardBtn) {
    if (millis() - hazardT0 >= interval) {
      hazardPhase = !hazardPhase;
      hazardT0 = millis();
    }
    if (hazardPhase) {
      leftFrontBlinker.close();
      rightFrontBlinker.close();
      backLeftBlinker.close();
      backRightBlinker.close();
    } else {
      leftFrontBlinker.open();
      rightFrontBlinker.open();
      backLeftBlinker.open();
      backRightBlinker.open();
    }
  } else {
    // Front-lamp cleanup is left to the turn-signal blocks below. Opening the
    // front blinkers here would run every loop and re-open a lamp that an active
    // turn signal just toggled closed, so the front could never stay lit.
    hazardPhase = false;
  }

  // Left turn signal (suppressed while hazards own the lamps). A freshly engaged
  // signal fires immediately so it lights on press instead of waiting out a
  // stale timer left over from the other side.
  if (leftSignal && !hazardBtn) {
    if (leftEngaged || millis() - flasherT0 >= interval) {
      leftFrontBlinker.toggle();
      backLeftBlinker.toggle();
      flasherT0 = millis();
    }
  } else if (!hazardBtn) {
    if (leftFrontBlinker.isClosed()) leftFrontBlinker.open();
  }

  // Right turn signal (suppressed while hazards own the lamps)
  if (rightSignal && !hazardBtn) {
    if (rightEngaged || millis() - flasherT0 >= interval) {
      rightFrontBlinker.toggle();
      backRightBlinker.toggle();
      flasherT0 = millis();
    }
  } else if (!hazardBtn) {
    if (rightFrontBlinker.isClosed()) rightFrontBlinker.open();
  }

  // Headlights
  if (headlightsBtn) {
    headlights.close();
  } else {
    if (headlights.isClosed()) headlights.open();
  }

  // Brake
  if (brakeSignal) {
    topBrakeLight.close();
    // Only steady-light a rear lamp for the brake on a side with no active
    // flasher; if that side's turn signal (or hazards) is running, leave the
    // lamp to the flasher above so the indicator keeps blinking while braking.
    if (!leftSignal && !hazardBtn) {
      backLeftBlinker.close();
    }
    if (!rightSignal && !hazardBtn) {
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
    horn.close();
  } else {
    if (horn.isClosed()) horn.open();
  }

}

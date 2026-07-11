/*** steering_wheel/src/main.cpp ***/ 

#include <Arduino.h>
#include <SPI.h>
#include <mcp2515.h>

MCP2515 mcp2515(10);

constexpr uint8_t PINS [] = { 2, 3, 4, 5, 6, 7, 8, 9 };
constexpr size_t NUMPINS = 8;
constexpr long unsigned int debounceTime = 20;

uint8_t inputState = 0;
uint8_t lastRawState = 0;
uint8_t stableState = 0;
uint8_t lastStableState = 0;

struct can_frame msg;

inline void readPinMask(const uint8_t pins [], const size_t size) {
  for (size_t i = 0; i < size; i++) {
    if (digitalRead(pins[i]) == LOW) {
      inputState |= (1U << i);
    }
  }
}

void updateInputs(const uint8_t pins [], const size_t size) {
  readPinMask(pins, size);
  static unsigned long lastChangeTime;
  
  // debounce the entire bitmask state
  if (inputState != lastRawState) {
    lastChangeTime = millis();
    lastRawState = inputState;
  }

  if (millis() - lastChangeTime > debounceTime) {
    stableState = inputState;
  }
}

void setup() {

  Serial.begin(115200);
  mcp2515.reset();
  mcp2515.setBitrate(CAN_500KBPS, MCP_8MHZ);
  mcp2515.setNormalMode();

  msg.can_id  = 0x100;
  msg.can_dlc = 8;

  for (size_t i = 0; i < NUMPINS; i++) {
    msg.data[i] = 0x00;
  }

  for (size_t i = 0; i < NUMPINS; i++) {
    pinMode(PINS[i], INPUT_PULLUP);
  }

}

void loop () {

  updateInputs(PINS, NUMPINS);

  msg.can_id = 0x100;
  msg.can_dlc = 8;

  uint8_t pressed = stableState & ~lastStableState;

  for (size_t i = 0; i < NUMPINS; i++) {
    msg.data[i] = 0x00;
  }

  for (size_t i = 0; i < NUMPINS; i++) {
    if (pressed & (1U << i)) {
      msg.data[i] = i+2;
    }
  }

  mcp2515.sendMessage(&msg);
  lastStableState = stableState;
}


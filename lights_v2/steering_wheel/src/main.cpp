/*** steering_wheel/src/main.cpp ***/ 

#include <Arduino.h>
#include <SPI.h>
#include <mcp2515.h>

MCP2515 mcp2515(10);

const uint8_t PINS [] = {2, 3, 4, 5, 6, 7, 8, 9};
const size_t NUMPINS = 8;

struct can_frame msg;

uint8_t readPinValue(const uint8_t* arr, size_t size) {

  for (size_t i = 0; i < size; i++)
    if (digitalRead(arr[i]) == LOW){
      return arr[i];
    }

  return 0;
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


void loop() {
  
  for (size_t i = 0; i < NUMPINS; i++) {
    msg.data[i] = 0x00;
  }

  uint8_t buttonPressed = readPinValue(PINS, NUMPINS);

  switch(buttonPressed) {

    case 2: msg.data[0] = 0x02; break;
    case 3: msg.data[1] = 0x03; break;
    case 4: msg.data[2] = 0x04; break;
    case 5: msg.data[3] = 0x05; break;
    case 6: msg.data[4] = 0x06; break;
    case 7: msg.data[5] = 0x07; break;
    case 8: msg.data[6] = 0x08; break;
    case 9: msg.data[7] = 0x09; break;

    default: break;
  }

  mcp2515.sendMessage(&msg);

}



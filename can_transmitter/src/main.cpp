#include <Arduino.h>
#include <SPI.h>     // Library for using SPI Communication
#include <mcp2515.h> // Library for using CAN Communication (https://github.com/autowp/arduino-mcp2515/)

MCP2515 mcp2515(10);

void setup() {
  SPI.begin(); // Begins SPI communication

  mcp2515.reset();
  mcp2515.setBitrate(CAN_500KBPS,
                     MCP_8MHZ); // Sets CAN at speed 500KBPS and Clock 8MHz
  mcp2515.setNormalMode();
}

uint8_t num = 0;
void loop() {
  can_frame can_msg;
  can_msg.can_id = 0x36; // CAN id as 0x36
  can_msg.can_dlc = 8;   // CAN data length as 8
  for (size_t i = 0; i < can_msg.can_dlc; i++) {
    can_msg.data[i] = num++;
  }

  mcp2515.sendMessage(&can_msg); // Sends the CAN message
  delay(100);
}

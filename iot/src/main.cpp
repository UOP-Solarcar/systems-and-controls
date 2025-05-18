#include "Arduino.h"
#include "SPI.h"
#include "mcp2515.h"

const uint8_t SPI_CS_PIN = 10;
MCP2515 mcp2515(SPI_CS_PIN);
const uint8_t MONITOR_SPEED = 115200;

void parse_frame() {
  // TODO: add parse frame code here 
}

void setup() {
  Serial.begin(MONITOR_SPEED);
  while (!Serial)
    ;
  Serial.println("Booted");
  SPI.begin();

  mcp2515.reset();
  mcp2515.setBitrate(CAN_500KBPS,
                     MCP_8MHZ); // Sets CAN at speed 500KBPS and Clock 8MHz
  mcp2515.setNormalMode();      // Sets CAN at normal mode
}

void loop() {
  can_frame frame{};
  if (mcp2515.readMessage(&frame) == MCP2515::ERROR_OK) {
    parse_frame(frame);
  }
}

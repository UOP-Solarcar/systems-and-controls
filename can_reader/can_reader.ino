#include <SPI.h>     //Library for using SPI Communication
#include <mcp2515.h> //Library for using CAN Communication (https://github.com/autowp/arduino-mcp2515/)

struct can_frame canMsg;

MCP2515 mcp2515(10); // SPI CS Pin 10

void setup() {
  Serial.begin(19200);
  SPI.begin();

  mcp2515.reset();
  mcp2515.setBitrate(CAN_500KBPS,
                     MCP_8MHZ); // Sets CAN at speed 500KBPS and Clock 8MHz
  mcp2515.setNormalMode();      // Sets CAN at normal mode
  Serial.println("Booted");
}

void loop() {
  if (mcp2515.readMessage(&canMsg) == MCP2515::ERROR_OK) {
    for (const auto &datum : canMsg.data) {
      Serial.print(datum);
    }
    Serial.println();

    delay(100); // delay for 1 second between each reading (this makes the
                // display less noisy)
  }
}

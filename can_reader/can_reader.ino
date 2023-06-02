#include <SPI.h>
#include <mcp2515.h> // (https://github.com/autowp/arduino-mcp2515/)

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
  can_frame can_msg;
  if (mcp2515.readMessage(&can_msg) == MCP2515::ERROR_OK) {
    Serial.print("[CAN] ");
    Serial.print("0x");
    Serial.print(can_msg.can_id, HEX);
    Serial.print(" [0x");
    Serial.print(can_msg.can_dlc, HEX);
    Serial.print("]:");
    for (size_t i = 0; i < can_msg.can_dlc; i++) {
      Serial.print(" 0x");
      Serial.print(can_msg.data[i], HEX);
    }
    Serial.println();
  }
}

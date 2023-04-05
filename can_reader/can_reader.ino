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
    {
      Serial.print("0x");
      char out[((32 / 8) * 2) + 1];
      sprintf(out, "%X", can_msg.can_id);
      Serial.print(out);
    }
    Serial.print(":");
    for (const auto &datum : can_msg.data) {
      Serial.print(" 0x");
      char out[((8 / 8) * 2) + 1];
      sprintf(out, "%X", datum);
      Serial.print(out);
    }
    Serial.println();

    delay(100);
  }
}

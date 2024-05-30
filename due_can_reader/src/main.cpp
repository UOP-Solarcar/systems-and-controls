#include <Arduino.h>
#include <due_can.h>

void setup() {
  Serial.begin(MONITOR_SPEED);
  Can0.begin(CAN_BPS_250K);
  Can1.begin(CAN_BPS_250K);
  for (int filter = 0; filter < 3; filter++) {
    Can0.setRXFilter(filter, 0, 0, true);
    Can1.setRXFilter(filter, 0, 0, true);
  }
  for (int filter = 3; filter < 7; filter++) {
    Can0.setRXFilter(filter, 0, 0, false);
    Can1.setRXFilter(filter, 0, 0, false);
  }
}

void print_frame(CAN_FRAME &frame) {
  Serial.print("ID: 0x");
  Serial.print(frame.id, HEX);
  Serial.print(" Len: ");
  Serial.print(frame.length);
  Serial.print(" Data:");
  for (int count = 0; count < frame.length; count++) {
    Serial.print(" 0x");
    Serial.print(frame.data.bytes[count], HEX);
  }
  Serial.println();
}

void loop() {
  if (Can0.available() > 0) {
    CAN_FRAME incoming;
    Can0.read(incoming);
    print_frame(incoming);
  }
  if (Can1.available() > 0) {
    CAN_FRAME incoming;
    Can1.read(incoming);
    print_frame(incoming);
  }
}

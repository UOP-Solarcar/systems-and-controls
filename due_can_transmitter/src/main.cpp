#include <Arduino.h>
#include <due_can.h>
void setup() { Can0.init(CAN_BPS_500K); }

uint8_t num = 0;
void loop() {
  CAN_FRAME frame;
  frame.id = 0x13;
  frame.length = 8;
  for (size_t i = 0; i < frame.length; i++) {
    frame.data.bytes[i] = num++;
  }

  Can0.sendFrame(frame);
  delay(100);
}

#include "Arduino.h"
#include "VescUart.h"
#include "mcp_2515.h"
#include "SPI.h"


const uint8_t BAUD_RATE = 115200;
const uint8_t CAN_CS = D8;
VescUart VESC;
MCP_CAN CAN(CAN_CS);


void setup() {

  Serial.begin(BAUD_RATE); // GPIO1/3
  VESC.setSerialPort(&Serial);
  
  // bring up mcp2515 at 500 kbit/s 
  while (CAN_OK != CAN.begin(MCP_ANY, 500000, MCP_8MHZ)) {
    delay(100);
  }
  CAN.setMode(MCP_NORMAL);
}


void loop() {
  static uint32_t tLast = 0;
  if (millis() - tLast >= 100) {   // 10 Hz
    tLast = millis();

    if (VESC.getVescValues()) {
      uint8_t buf[8];

      int16_t rpm   = VESC.data.rpm / 10;              // scale
      uint16_t volt = (uint16_t)(VESC.data.inpVoltage * 100.0);
      int16_t amps  = (int16_t)(VESC.data.avgInputCurrent * 10.0);
      int16_t temp  = (int16_t)(VESC.data.tempMosfet * 10.0);

      buf[0] = rpm >> 8;   buf[1] = rpm;
      buf[2] = volt >> 8;  buf[3] = volt;
      buf[4] = amps >> 8;  buf[5] = amps;
      buf[6] = temp >> 8;  buf[7] = temp;

      CAN.sendMsgBuf(CAN_ID, 0, 8, buf);               // standard frame
    }
  }

}



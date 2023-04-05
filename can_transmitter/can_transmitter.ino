#include <SPI.h>          //Library for using SPI Communication 
#include <mcp2515.h>      //Library for using CAN Communication (https://github.com/autowp/arduino-mcp2515/)
 

 
struct can_frame canMsg;
 
MCP2515 mcp2515(10);

 
 
void setup()
{
  Serial.begin(9600);
  SPI.begin();               //Begins SPI communication
 
  mcp2515.reset();
  mcp2515.setBitrate(CAN_500KBPS, MCP_8MHZ); //Sets CAN at speed 500KBPS and Clock 8MHz
  mcp2515.setNormalMode();
}
 
// uint8_t xd = 0;
void loop()
{

  canMsg.can_id  = 0x022;           //CAN id as 0x036
  canMsg.can_dlc = 8;               //CAN data length as 8
  canMsg.data[0] = 0x01;               //Update humidity value in [0]
  canMsg.data[1] = 0x02;               //Update temperature value in [1]
  canMsg.data[2] = 0x03;            //Rest all with 0
  canMsg.data[3] = 0x04;
  canMsg.data[4] = 0x05;
  canMsg.data[5] = 0x6B;
  canMsg.data[6] = 0x07;
  canMsg.data[7] = 0x00;
  
  mcp2515.sendMessage(&canMsg);     //Sends the CAN message
  delay(100);
}

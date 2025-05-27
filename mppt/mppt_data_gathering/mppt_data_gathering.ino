/*
This code is a draft and attempting to get data from the mppt's rs485 port.

ChatGPT Response:
Here’s an Arduino Nano–compatible sketch using SoftwareSerial for your RS-485 bus. It assumes:

MAX485 DE/RE tied together to D2
SoftwareSerial RX on D10, TX on D11
Nano’s Serial (USB) used for debug output
Two MPPTs at Modbus addresses 1 and 2, reading six registers from 0x3100
*/

#include <SoftwareSerial.h>
#include <ModbusMaster.h>

// RS-485 DE/RE control pin
#define MAX485_DE_RE 2

// SoftwareSerial pins for RS-485 (RX, TX)
#define RS485_RX_PIN 10
#define RS485_TX_PIN 11

// Create a software serial port for RS-485 comms
SoftwareSerial rs485Serial(RS485_RX_PIN, RS485_TX_PIN);

// Modbus nodes for each controller
ModbusMaster mppt1;
ModbusMaster mppt2;

// Pre-transmission callback: enable RS-485 driver
void preTransmission() {
  digitalWrite(MAX485_DE_RE, HIGH);
}

// Post-transmission callback: disable RS-485 driver
void postTransmission() {
  digitalWrite(MAX485_DE_RE, LOW);
}

void setup() {
  // DE/RE control
  pinMode(MAX485_DE_RE, OUTPUT);
  digitalWrite(MAX485_DE_RE, LOW);

  // Debug serial
  Serial.begin(115200);
  while (!Serial);

  // RS-485 bus
  rs485Serial.begin(9600);

  // Initialize Modbus instances
  mppt1.begin(1, rs485Serial);  // Address = 1
  mppt2.begin(2, rs485Serial);  // Address = 2

  // Attach callbacks to both nodes
  mppt1.preTransmission(preTransmission);
  mppt1.postTransmission(postTransmission);
  mppt2.preTransmission(preTransmission);
  mppt2.postTransmission(postTransmission);

  Serial.println("Arduino Nano RS-485 MPPT reader ready");
}

void loop() {
  uint8_t status;
  uint16_t regs[6];

  // Read 6 input registers starting at 0x3100 from MPPT #1
  status = mppt1.readInputRegisters(0x3100, 6);
  if (status == mppt1.ku8MBSuccess) {
    Serial.print("MPPT #1 data: ");
    for (uint8_t i = 0; i < 6; i++) {
      regs[i] = mppt1.getResponseBuffer(i);
      Serial.print(regs[i]);
      Serial.print(" ");
    }
    Serial.println();
  } else {
    Serial.print("MPPT #1 error: 0x");
    Serial.println(status, HEX);
  }

  // Read same registers from MPPT #2
  status = mppt2.readInputRegisters(0x3100, 6);
  if (status == mppt2.ku8MBSuccess) {
    Serial.print("MPPT #2 data: ");
    for (uint8_t i = 0; i < 6; i++) {
      regs[i] = mppt2.getResponseBuffer(i);
      Serial.print(regs[i]);
      Serial.print(" ");
    }
    Serial.println();
  } else {
    Serial.print("MPPT #2 error: 0x");
    Serial.println(status, HEX);
  }

  delay(1000);
}

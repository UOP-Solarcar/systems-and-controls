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

// ----- Pin definitions -----
#define MAX485_DE_RE 2       // Driver enable / receiver enable
#define RS485_RX_PIN 10      // SoftwareSerial RX (to module DI)
#define RS485_TX_PIN 11      // SoftwareSerial TX (to module RO)

// ----- Create SoftwareSerial for RS-485 -----
SoftwareSerial rs485Serial(RS485_RX_PIN, RS485_TX_PIN);

// ----- Modbus nodes for each MPPT -----
ModbusMaster mppt1;

// Called before Modbus transmission: enable RS-485 driver
void preTransmission() {
  digitalWrite(MAX485_DE_RE, HIGH);
}
// Called after Modbus transmission: disable RS-485 driver
void postTransmission() {
  digitalWrite(MAX485_DE_RE, LOW);
}

void setup() {
  // DE/RE control pin
  pinMode(MAX485_DE_RE, OUTPUT);
  digitalWrite(MAX485_DE_RE, LOW);

  // USB serial for debugging
  Serial.begin(115200);
  while (!Serial);

  // Start RS-485 on software serial
  rs485Serial.begin(9600);

  // Initialize Modbus instances:
  // .begin(address, serialPort)
  mppt1.begin(1, rs485Serial);  // MPPT #1 at Modbus address 1

  // Attach the DE/RE toggling callbacks
  mppt1.preTransmission(preTransmission);
  mppt1.postTransmission(postTransmission);

  Serial.println("Arduino Uno RS-485 MPPT reader ready");
}

void loop() {
  uint8_t status;
  uint16_t regs[6];

  // --- Read from MPPT #1 ---
  status = mppt1.readInputRegisters(0x3100, 6);
  if (status == mppt1.ku8MBSuccess) {
    Serial.print("MPPT #1: ");
    for (uint8_t i = 0; i < 6; i++) {
      regs[i] = mppt1.getResponseBuffer(i);
      Serial.print(regs[i]);
      Serial.print(" ");
    }
    Serial.println();
  } else {
    Serial.print("Error MPPT #1: 0x");
    Serial.println(status, HEX);
  }
  delay(1000);
}

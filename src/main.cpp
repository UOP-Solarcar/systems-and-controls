#include "bytetools.cpp"
#include "bytetools.hpp"
#include <Arduino.h>
#include <SPI.h>
#include <mcp2515.h> // (https://github.com/autowp/arduino-mcp2515/)

MCP2515 mcp2515(10); // SPI CS Pin 10

const uint8_t RELAYPIN = 7;
bool relay_disconnected;

void disconnect_relay() {
  if (!relay_disconnected) {
    relay_disconnected = true;
    digitalWrite(RELAYPIN, HIGH);
    Serial.print("Relay Disconnected");
  }
}

void connect_relay() {
  if (relay_disconnected) {
    relay_disconnected = false;
    digitalWrite(RELAYPIN, LOW);
    Serial.print("Relay Connected");
  }
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
  pinMode(RELAYPIN, OUTPUT);

  disconnect_relay();
}

void loop() {
  can_frame frame{};
  if (mcp2515.readMessage(&frame) == MCP2515::ERROR_OK) {
    switch (frame.can_id) {
    case 0x6B0: {
      uint16_t pack_current = bytetools::int_bswap(*(uint16_t *)&frame.data[0]),
               pack_inst_voltage =
                   bytetools::int_bswap(*(uint16_t *)&frame.data[2]);
      uint8_t pack_soc = frame.data[4];
      uint16_t relay_state = bytetools::int_bswap(*(uint16_t *)&frame.data[5]);
      uint8_t checksum = frame.data[7];
      if (pack_current > 1000) { // Pack Current is in deciamps
        disconnect_relay();
      } else {
        connect_relay();
      }
      Serial.print("Pack Current: ");
      Serial.print(pack_current);
      Serial.print(" Pack Inst. Voltage: ");
      Serial.print(pack_inst_voltage);
      Serial.print(" Pack SOC: ");
      Serial.print(pack_soc);
      Serial.print(" Relay State: ");
      Serial.print(relay_state);
      Serial.print(" Checksum: ");
      Serial.print(checksum);
    } break;
    case 0x6B1: {
      uint16_t pack_dcl = bytetools::int_bswap(*(uint16_t *)&frame.data[0]),
               pack_ccl = bytetools::int_bswap(*(uint16_t *)&frame.data[2]);
      uint8_t high_temp = frame.data[4], low_temp = frame.data[5],
              checksum = frame.data[7];
      if (high_temp > 60) { // Temp is in celsius
        disconnect_relay();
      } else {
        connect_relay();
      }
      Serial.print("Pack DCL: ");
      Serial.print(pack_dcl);
      Serial.print(" Pack CCL: ");
      Serial.print(pack_ccl);
      Serial.print(" High Temperature: ");
      Serial.print(high_temp);
      Serial.print(" Low Temperature: ");
      Serial.print(low_temp);
      Serial.print(" Checksum: ");
      Serial.print(checksum);
    } break;
    case 0x6B2: {
      uint16_t high_cell_voltage =
          bytetools::int_bswap(*(uint16_t *)&frame.data[0]);
      uint8_t high_cell_voltage_id = frame.data[2];
      uint16_t low_cell_voltage =
          bytetools::int_bswap(*(uint16_t *)&frame.data[3]);
      uint8_t low_cell_voltage_id = frame.data[5], checksum = frame.data[6];
      if ((low_cell_voltage < 26500) | (high_cell_voltage > 42000)) { // volts
        disconnect_relay();
      } else {
        connect_relay();
      }
      Serial.print("High Cell Voltage: ");
      Serial.print(high_cell_voltage);
      Serial.print(" High Cell Voltage ID: ");
      Serial.print(high_cell_voltage_id);
      Serial.print(" Low Cell Voltage: ");
      Serial.print(low_cell_voltage);
      Serial.print(" Low Cell Voltage ID: ");
      Serial.print(low_cell_voltage_id);
      Serial.print(" Checksum: ");
      Serial.print(checksum);
    } break;
    case 0x6B3: {
      uint8_t high_temp = frame.data[0], high_thermistor_id = frame.data[1],
              low_temp = frame.data[2], low_thermistor_id = frame.data[3],
              internal_temp = frame.data[4], checksum = frame.data[5];
      if (high_temp > 60) { // Temp is in celsius
        disconnect_relay();
      } else {
        connect_relay();
      }
      Serial.print("High Temperature: ");
      Serial.print(high_temp);
      Serial.print(" High Thermistor ID: ");
      Serial.print(high_thermistor_id);
      Serial.print(" Low Temperature: ");
      Serial.print(low_temp);
      Serial.print(" Low Thermistor ID: ");
      Serial.print(low_thermistor_id);
      Serial.print(" Internal Temperature: ");
      Serial.print(internal_temp);
      Serial.print(" Checksum: ");
      Serial.print(checksum);
    } break;
    default:
      break;
    }
  }
}

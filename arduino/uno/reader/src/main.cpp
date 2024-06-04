#include "bitcasting.h"
#include "bitcasting.cpp"
#include <Arduino.h>
#include <SPI.h>
#include <mcp2515.h> // (https://github.com/autowp/arduino-mcp2515/)

MCP2515 mcp2515(10); // SPI CS Pin 10

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
}

void print_frame(can_frame &frame, Print &print = Serial) {
  print.print("[CAN] 0x");
  print.print(frame.can_id, HEX);
  print.print(" [0x");
  print.print(frame.can_dlc, HEX);
  print.print("]:");
  for (size_t i = 0; i < frame.can_dlc; i++) {
    print.print(" 0x");
    print.print(frame.data[i], HEX);
  }
  print.println();
}

void parse_frame(can_frame &frame, Print &print = Serial) {
  switch (frame.can_id & 0xFFFFFF00) {
  // Motor Controller Statuses
  case 0x80000900: // status 1
  case 0x80000e00: // status 2
  case 0x80000f00: // status 3
  case 0x80001000: // status 4
  case 0x80001b00: // status 5
  case 0x80003a00: // status 6
    canid_t motor_controller_id = frame.can_id & 0x000000FF;
    print.print("Motor Controller ID: 0x");
    print.print(motor_controller_id, HEX);
    print.print(" Status #");
    switch (frame.can_id & 0xFFFFFF00) {
    // Motor Controller Statuses
    case 0x80000900: // status 1
    {
      print.print('1');
      int32_t rpm = bitcasting::int_bswap(*(uint32_t *)&frame.data[0]);
      int16_t current = bitcasting::int_bswap(*(uint16_t *)&frame.data[4]),
               duty_cycle = bitcasting::int_bswap(*(uint16_t *)&frame.data[6]);
      print.print(" RPM: ");
      print.print(rpm);
      print.print(" current: ");
      print.print(current);
      print.print(" duty cycle: ");
      print.print(duty_cycle);
    } break;
    case 0x80000e00: // status 2
    {
      print.print('2');
      int32_t ah_used = bitcasting::int_bswap(*(int32_t *)&frame.data[0]),
              ah_charged = bitcasting::int_bswap(*(int32_t *)&frame.data[4]);
      print.print(" Ah Used: ");
      print.print(ah_used);
      print.print(" Ah Charged: ");
      print.print(ah_charged);
    } break;
    case 0x80000f00: // status 3
    {
      print.print('3');
      int32_t wh_used = bitcasting::int_bswap(*(int32_t *)&frame.data[0]),
              wh_charged = bitcasting::int_bswap(*(int32_t *)&frame.data[4]);
      print.print(" Wh Used: ");
      print.print(wh_used);
      print.print(" Wh Charged: ");
      print.print(wh_charged);
    } break;
    case 0x80001000: // status 4
    {
      print.print('4');
      int16_t temp_fet = bitcasting::int_bswap(*(int16_t *)&frame.data[0]),
              temp_motor = bitcasting::int_bswap(*(int16_t *)&frame.data[2]),
              current_in = bitcasting::int_bswap(*(int16_t *)&frame.data[4]),
              pid_position_now =
                  bitcasting::int_bswap(*(int16_t *)&frame.data[6]);
      print.print(" Temp FET: ");
      print.print(temp_fet);
      print.print(" Temp Motor: ");
      print.print(temp_motor);
      print.print(" Current In: ");
      print.print(current_in);
      print.print(" PID-position Now: ");
      print.print(pid_position_now);
    } break;
    case 0x80001b00: // status 5
    {
      print.print('5');
      uint32_t tachometer = __builtin_bswap32(*(uint32_t *)&frame.data[0]);
      uint16_t voltage_in = __builtin_bswap16(*(uint16_t *)&frame.data[4]),
               reserved = __builtin_bswap16(*(uint16_t *)&frame.data[6]);
      print.print(" Tachometer: ");
      print.print(tachometer);
      print.print(" Voltage In: ");
      print.print(voltage_in);
      print.print(" Reserved: ");
      print.print(reserved);
    } break;
    case 0x80003a00: // status 6
    {
      print.print('6');
      int16_t adc1 = bitcasting::int_bswap(*(int16_t *)&frame.data[0]),
              adc2 = bitcasting::int_bswap(*(int16_t *)&frame.data[2]),
              adc3 = bitcasting::int_bswap(*(int16_t *)&frame.data[4]),
              ppm = bitcasting::int_bswap(*(int16_t *)&frame.data[6]);
      print.print(" ADC1: ");
      print.print(adc1);
      print.print(" ADC2: ");
      print.print(adc2);
      print.print(" ADC3: ");
      print.print(adc3);
      print.print(" PPM: ");
      print.print(ppm);
    } break;
    }
    print.println();
    break;
  }
}

void loop() {
  can_frame frame{};
  if (mcp2515.readMessage(&frame) == MCP2515::ERROR_OK) {
    parse_frame(frame);
  }
}

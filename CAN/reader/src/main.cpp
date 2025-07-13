#include "bytetools.cpp"
#include "bytetools.hpp"

#if defined(RPI4B)
#include <cstring>
#include <iostream>
#include <linux/can.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
#else
#include <Arduino.h>
#include <SPI.h>
#include <mcp2515.h> // (https://github.com/autowp/arduino-mcp2515/)
#endif

#if defined(RPI4B)
typedef std::ostream Out;
template <typename T> void print_n(T t, Out &out = std::cout) { out << t; }
void print_n(uint8_t n, Out &out = std::cout) { out << (uint16_t)n; }
void print_n(int8_t n, Out &out = std::cout) { out << (int16_t)n; }
template <typename T> void print_hex(T t, Out &out = std::cout) {
  out << std::hex << t << std::dec;
}
void println(Out &out = std::cout) { out << std::endl; }
#else
typedef Print Out;
template <typename T> void print_n(T t, Out &out = Serial) { out.print(t); }
template <typename T> void print_hex(T t, Out &out = Serial) {
  out.print(t, HEX);
}
void println(Out &out = Serial) { out.println(); }
#endif

void print_frame(can_frame &frame, Out &out =
#if defined(RPI4B)
                                       std::cout
#else
                                       Serial
#endif
) {
  print_n("[CAN] 0x", out);
  print_hex(frame.can_id, out);
  print_n(" [0x", out);
  print_hex(frame.can_dlc, out);
  print_n("]:", out);
  for (size_t i = 0; i < frame.can_dlc; i++) {
    print_n(" 0x", out);
    print_hex(frame.data[i], out);
  }
}

void parse_frame(can_frame &frame, Out &out =
#if defined(RPI4B)
                                       std::cout
#else
                                       Serial
#endif
) {
  switch (frame.can_id & 0xFFFFFF00) {
  // Motor Controller Statuses
  case 0x80000900: // status 1
  case 0x80000e00: // status 2
  case 0x80000f00: // status 3
  case 0x80001000: // status 4
  case 0x80001b00: // status 5
  case 0x80003a00: // status 6
  {
    canid_t motor_controller_id = frame.can_id & 0x000000FF;
    print_n("Motor Controller ID: 0x", out);
    print_hex(motor_controller_id, out);
    print_n(" Status #", out);
    switch (frame.can_id & 0xFFFFFF00) {
    // Motor Controller Statuses
    case 0x80000900: // status 1
    {
      print_n('1', out);
      int32_t rpm = bytetools::int_bswap(*(int32_t *)&frame.data[0]);
      int16_t current = bytetools::int_bswap(*(int16_t *)&frame.data[4]),
              duty_cycle = bytetools::int_bswap(*(int16_t *)&frame.data[6]);
      print_n(" RPM: ", out);
      print_n(rpm, out);
      print_n(" current: ", out);
      print_n(current, out);
      print_n(" duty cycle: ", out);
      print_n(duty_cycle, out);
    } break;
    case 0x80000e00: // status 2
    {
      print_n('2', out);
      int32_t ah_used = bytetools::int_bswap(*(int32_t *)&frame.data[0]),
              ah_charged = bytetools::int_bswap(*(int32_t *)&frame.data[4]);
      print_n(" Ah Used: ", out);
      print_n(ah_used, out);
      print_n(" Ah Charged: ", out);
      print_n(ah_charged, out);
    } break;
    case 0x80000f00: // status 3
    {
      print_n('3', out);
      int32_t wh_used = bytetools::int_bswap(*(int32_t *)&frame.data[0]),
              wh_charged = bytetools::int_bswap(*(int32_t *)&frame.data[4]);
      print_n(" Wh Used: ", out);
      print_n(wh_used, out);
      print_n(" Wh Charged: ", out);
      print_n(wh_charged, out);
    } break;
    case 0x80001000: // status 4
    {
      print_n('4', out);
      int16_t temp_fet = bytetools::int_bswap(*(int16_t *)&frame.data[0]),
              temp_motor = bytetools::int_bswap(*(int16_t *)&frame.data[2]),
              current_in = bytetools::int_bswap(*(int16_t *)&frame.data[4]),
              pid_position_now =
                  bytetools::int_bswap(*(int16_t *)&frame.data[6]);
      print_n(" Temp FET: ", out);
      print_n(temp_fet, out);
      print_n(" Temp Motor: ", out);
      print_n(temp_motor, out);
      print_n(" Current In: ", out);
      print_n(current_in, out);
      print_n(" PID-position Now: ", out);
      print_n(pid_position_now, out);
    } break;
    case 0x80001b00: // status 5
    {
      print_n('5', out);
      uint32_t tachometer = __builtin_bswap32(*(uint32_t *)&frame.data[0]);
      uint16_t voltage_in = __builtin_bswap16(*(uint16_t *)&frame.data[4]),
               reserved = __builtin_bswap16(*(uint16_t *)&frame.data[6]);
      print_n(" Tachometer: ", out);
      print_n(tachometer, out);
      print_n(" Voltage In: ", out);
      print_n(voltage_in, out);
      print_n(" Reserved: ", out);
      print_n(reserved, out);
    } break;
    case 0x80003a00: // status 6
    {
      print_n('6', out);
      int16_t adc1 = bytetools::int_bswap(*(int16_t *)&frame.data[0]),
              adc2 = bytetools::int_bswap(*(int16_t *)&frame.data[2]),
              adc3 = bytetools::int_bswap(*(int16_t *)&frame.data[4]),
              ppm = bytetools::int_bswap(*(int16_t *)&frame.data[6]);
      print_n(" ADC1: ", out);
      print_n(adc1, out);
      print_n(" ADC2: ", out);
      print_n(adc2, out);
      print_n(" ADC3: ", out);
      print_n(adc3, out);
      print_n(" PPM: ", out);
      print_n(ppm, out);
    } break;
    }
  } break;
  default: {
    switch (frame.can_id) {
    case 0x6B0: {
      int16_t pack_current = bytetools::int_bswap(*(int16_t *)&frame.data[0]);
      uint16_t pack_inst_voltage =
          bytetools::int_bswap(*(uint16_t *)&frame.data[2]);
      uint8_t pack_soc = frame.data[4];
      uint16_t relay_state = bytetools::int_bswap(*(uint16_t *)&frame.data[5]);
      uint8_t checksum = frame.data[7];
      print_n("Pack Current: ", out);
      print_n(pack_current, out);
      print_n(" Pack Inst. Voltage: ", out);
      print_n(pack_inst_voltage, out);
      print_n(" Pack SOC: ", out);
      print_n(pack_soc, out);
      print_n(" Relay State: ", out);
      print_n(relay_state, out);
      print_n(" Checksum: ", out);
      print_n(checksum, out);
    } break;
    case 0x6B1: {
      uint16_t pack_dcl = bytetools::int_bswap(*(uint16_t *)&frame.data[0]),
               pack_ccl = bytetools::int_bswap(*(uint16_t *)&frame.data[2]);
      uint8_t high_temp = frame.data[4], low_temp = frame.data[5],
              checksum = frame.data[7];
      print_n("Pack DCL: ", out);
      print_n(pack_dcl, out);
      print_n(" Pack CCL: ", out);
      print_n(pack_ccl, out);
      print_n(" High Temperature: ", out);
      print_n(high_temp, out);
      print_n(" Low Temperature: ", out);
      print_n(low_temp, out);
      print_n(" Checksum: ", out);
      print_n(checksum, out);
    } break;
    case 0x6B2: {
      uint16_t high_cell_voltage =
          bytetools::int_bswap(*(uint16_t *)&frame.data[0]);
      uint8_t high_cell_voltage_id = frame.data[2];
      uint16_t low_cell_voltage =
          bytetools::int_bswap(*(uint16_t *)&frame.data[3]);
      uint8_t low_cell_voltage_id = frame.data[5], checksum = frame.data[6];
      print_n("High Cell Voltage: ", out);
      print_n(high_cell_voltage, out);
      print_n(" High Cell Voltage ID: ", out);
      print_n(high_cell_voltage_id, out);
      print_n(" Low Cell Voltage: ", out);
      print_n(low_cell_voltage, out);
      print_n(" Low Cell Voltage ID: ", out);
      print_n(low_cell_voltage_id, out);
      print_n(" Checksum: ", out);
      print_n(checksum, out);
    } break;
    case 0x6B3: {
      uint8_t high_temp = frame.data[0], high_thermistor_id = frame.data[1],
              low_temp = frame.data[2], low_thermistor_id = frame.data[3],
              avg_temp = frame.data[4], internal_temp = frame.data[5],
              checksum = frame.data[6];
      print_n("High Temperature: ", out);
      print_n(high_temp, out);
      print_n(" High Thermistor ID: ", out);
      print_n(high_thermistor_id, out);
      print_n(" Low Temperature: ", out);
      print_n(low_temp, out);
      print_n(" Low Thermistor ID: ", out);
      print_n(low_thermistor_id, out);
      print_n(" Average Temperature: ", out);
      print_n(avg_temp, out);
      print_n(" Internal Temperature: ", out);
      print_n(internal_temp, out);
      print_n(" Checksum: ", out);
      print_n(checksum, out);
    } break;
    case 0x6B4: {
      uint8_t pack_health = frame.data[0];
      uint16_t adaptive_total_capacity =
                   bytetools::int_bswap(*(uint16_t *)&frame.data[3]),
               input_supply_voltage =
                   bytetools::int_bswap(*(uint16_t *)&frame.data[5]);
      uint8_t checksum = frame.data[7];
      print_n("Pack Health: ", out);
      print_n(pack_health, out);
      print_n(" Adaptive Total Capacity: ", out);
      print_n(adaptive_total_capacity, out);
      print_n(" Input Supply Voltage: ", out);
      print_n(input_supply_voltage, out);
      print_n(" Checksum: ", out);
      print_n(checksum, out);
    } break;
    case 0x36: {
      uint8_t cell_id = frame.data[0];
      uint16_t instant_voltage =
                   bytetools::int_bswap(*(uint16_t *)&frame.data[1]),
               internal_resistance =
                   bytetools::int_bswap(*(uint16_t *)&frame.data[3]),
               open_voltage = frame.data[5];
      uint8_t checksum = frame.data[7];
      print_n("Cell ID: ", out);
      print_n(cell_id, out);
      print_n(" Instant Voltage: ", out);
      print_n(instant_voltage, out);
      print_n(" Internal Resistance: ", out);
      print_n(internal_resistance, out);
      print_n(" Open Voltage: ", out);
      print_n(open_voltage, out);
      print_n(" Checksum: ", out);
      print_n(checksum, out);
    } break;
    default:
      print_frame(frame, out);
      break;
    }
  } break;
  }
  println(out);
}

#if defined(RPI4B)
class CanSocket {
public:
  explicit CanSocket(const std::string &interface) {
    if ((sock = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
      throw std::runtime_error("Error while opening socket");
    }

    ifreq ifr;
    std::strncpy(ifr.ifr_name, interface.c_str(), IFNAMSIZ - 1);
    if (ioctl(sock, SIOCGIFINDEX, &ifr) < 0) {
      throw std::runtime_error("Error in ioctl");
    }

    sockaddr_can addr;
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    if (bind(sock, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr)) <
        0) {
      throw std::runtime_error("Error in bind");
    }
  }

  ~CanSocket() { close(sock); }

  can_frame read_frame() {
    can_frame frame;
    auto nbytes = read(sock, &frame, sizeof(can_frame));
    if (nbytes < 0) {
      throw std::runtime_error("Error in read");
    } else if (nbytes < sizeof(can_frame)) {
      throw std::runtime_error("Incomplete CAN frame");
    }
    return frame;
  }

private:
  int sock;
};

int main() {
  CanSocket canSocket("can0");
  while (true) {
    can_frame frame = canSocket.read_frame();
    parse_frame(frame);
  }
  return 0;
}

#else
MCP2515 mcp2515(10); // SPI CS Pin 10

void setup() {
  Serial.begin(MONITOR_SPEED);
  while (!Serial)
    ;
  Serial.println("Booted");
  SPI.begin();

  mcp2515.reset();
  mcp2515.setBitrate(CAN_500KBPS,
                     MCP_8MHZ); // Sets CAN at speed 250KBPS and Clock 8MHz
  mcp2515.setNormalMode();      // Sets CAN at normal mode
}

void loop() {
  can_frame frame{};
  if (mcp2515.readMessage(&frame) == MCP2515::ERROR_OK) {
    parse_frame(frame);
  }
}
#endif

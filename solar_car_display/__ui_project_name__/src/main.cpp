#include <Arduino.h>
#include <string.h>

#include "bytetools.cpp"
#include "bytetools.hpp"

#include <Arduino.h>
#include <SPI.h>
#include <mcp2515.h> // (https://github.com/autowp/arduino-mcp2515/)
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <avr/wdt.h>

int total_kwh = 0; // Total energy in watt-hours
int SOC = 0;
int current_total = 0;
int voltage_total = 0;
int odometer = 0;
int previous_time_wh = millis();
int supplimental_soc = 0; // Additional SOC from supplimental battery

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display1(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Adafruit_SSD1306 display2(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const unsigned long DISPLAY_INTERVAL = 500;   // update OLED every 500 ms
unsigned long prevDisplayTime = 0;

typedef Print Out;

void parse_frame(can_frame &frame, Out &out = Serial) {
  switch (frame.can_id & 0xFFFFFF00) {
  // Motor Controller Statuses
  case 0x80000900: // status 1
  case 0x80000e00: // status 2
  case 0x80000f00: // status 3
  case 0x80001000: // status 4
  case 0x80001b00: // status 5
  case 0x80003a00: // status 6
  {
    //canid_t motor_controller_id = frame.can_id & 0x000000FF;
    //print_n("Motor Controller ID: 0x", out);
    //print_hex(motor_controller_id, out);
    //print_n(" Status #", out);
    switch (frame.can_id & 0xFFFFFF00) {
    // Motor Controller Statuses
    case 0x80000900: // status 1
    {
      //print_n('1', out);
      //int32_t rpm_l = bytetools::int_bswap(*(int32_t *)&frame.data[0]);
      //int16_t current = bytetools::int_bswap(*(int16_t *)&frame.data[4]),
      //        duty_cycle = bytetools::int_bswap(*(int16_t *)&frame.data[6]);
      //rpm = rpm_l;
    } break;
    case 0x80000e00: // status 2
    {
      //print_n('2', out);
      //int32_t ah_used = bytetools::int_bswap(*(int32_t *)&frame.data[0]),
      //        ah_charged = bytetools::int_bswap(*(int32_t *)&frame.data[4]);
    } break;
    case 0x80000f00: // status 3
    {
      //print_n('3', out);
      //int32_t wh_used = bytetools::int_bswap(*(int32_t *)&frame.data[0]),
              //wh_charged = bytetools::int_bswap(*(int32_t *)&frame.data[4]);
      //kWIn = wh_charged; // Calculate motor Wh
      //kWOut = wh_used; // Calculate motor Wh
    } break;
    case 0x80001000: // status 4
    {
      //print_n('4', out);
      //int16_t temp_fet = bytetools::int_bswap(*(int16_t *)&frame.data[0]),
      //        temp_motor = bytetools::int_bswap(*(int16_t *)&frame.data[2]),
      //        current_in = bytetools::int_bswap(*(int16_t *)&frame.data[4]),
      //        pid_position_now =
      //            bytetools::int_bswap(*(int16_t *)&frame.data[6]);
    } break;
    case 0x80001b00: // status 5
    {
      //print_n('5', out);
      //uint32_t tachometer = __builtin_bswap32(*(uint32_t *)&frame.data[0]);
      //uint16_t voltage_in = __builtin_bswap16(*(uint16_t *)&frame.data[4]),
      //         reserved = __builtin_bswap16(*(uint16_t *)&frame.data[6]);
    } break;
    case 0x80003a00: // status 6
    {
      //print_n('6', out);
      //int16_t adc1 = bytetools::int_bswap(*(int16_t *)&frame.data[0]),
      //        adc2 = bytetools::int_bswap(*(int16_t *)&frame.data[2]),
      //        adc3 = bytetools::int_bswap(*(int16_t *)&frame.data[4]),
      //        ppm = bytetools::int_bswap(*(int16_t *)&frame.data[6]);
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
      //uint16_t relay_state = bytetools::int_bswap(*(uint16_t *)&frame.data[5]);
      //uint8_t checksum = frame.data[7];
      voltage_total = pack_inst_voltage;
      current_total = pack_current;
      SOC = pack_soc;
    } break;
    case 0x6B1: {
      //uint16_t pack_dcl = bytetools::int_bswap(*(uint16_t *)&frame.data[0]),
      //         pack_ccl = bytetools::int_bswap(*(uint16_t *)&frame.data[2]);
      //uint8_t high_temp = frame.data[4], low_temp = frame.data[5],
      //        checksum = frame.data[7];
    } break;
    case 0x6B2: {
      //uint16_t high_cell_voltage =
      //    bytetools::int_bswap(*(uint16_t *)&frame.data[0]);
      //uint8_t high_cell_voltage_id = frame.data[2];
      //uint16_t low_cell_voltage =
      //    bytetools::int_bswap(*(uint16_t *)&frame.data[3]);
      //uint8_t low_cell_voltage_id = frame.data[5], checksum = frame.data[6];
    } break;
    case 0x6B3: {
      //uint8_t high_temp = frame.data[0], high_thermistor_id = frame.data[1],
      //        low_temp = frame.data[2], low_thermistor_id = frame.data[3],
      //        avg_temp = frame.data[4], internal_temp = frame.data[5],
      //        checksum = frame.data[6];
    } break;
    case 0x6B4: {
      //uint8_t pack_health = frame.data[0];
      //uint16_t adaptive_total_capacity =
      //             bytetools::int_bswap(*(uint16_t *)&frame.data[3]),
      //         input_supply_voltage =
      //             bytetools::int_bswap(*(uint16_t *)&frame.data[5]);
      //uint8_t checksum = frame.data[7];
    } break;
    case 0x36: {
      //uint8_t cell_id = frame.data[0];
      //uint16_t instant_voltage =
      //             bytetools::int_bswap(*(uint16_t *)&frame.data[1]),
      //         internal_resistance =
      //             bytetools::int_bswap(*(uint16_t *)&frame.data[3]),
      //         open_voltage = frame.data[5];
      //uint8_t checksum = frame.data[7];
    } break;
    default:
      break;
    }
  } break;
  }
}

MCP2515 mcp2515(10); // SPI CS Pin 10

void calculateKWh() {
    total_kwh = int(current_total * voltage_total * (millis() - previous_time_wh) / 3600000.0); // kWh = (A * V * t) / 3600000
    previous_time_wh = millis();
}

void calc_suplimental_soc() {
    // Calculate the supplemental SOC based on the current total and voltage
    // This is a placeholder calculation, adjust as needed
    supplimental_soc = (supplimental_soc + 1) % 100;
}

void display() {
    display1.clearDisplay();
    display2.clearDisplay();

    display1.setTextSize(4);
    display1.setTextColor(WHITE);
    display1.setCursor(0, 0);
    display1.print("Total kWh: ");
    display1.println(total_kwh);
    
    display2.setTextSize(4);
    display2.setTextColor(WHITE);
    display2.setCursor(0, 0);
    display2.print("SOC: ");
    display2.println(SOC);

    display2.setCursor(0, 20);
    display2.print("Suppl. SOC: ");
    display2.println(supplimental_soc);

    // Display the updates
    display1.display();
    display2.display();
}


void setup()
{
  //myGLCD.InitLCD();

  //myGLCD.clrScr();

  if(!display1.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    //Serial.println(F("SSD1306A allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  if(!display2.begin(SSD1306_SWITCHCAPVCC, 0x3D)) { 
    //Serial.println(F("SSD1306B allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  display1.display();
  display2.display();
  delay(1000);
  display1.clearDisplay();
  display1.drawPixel(10, 10, WHITE);
  display2.clearDisplay();
  display2.drawPixel(10, 10, WHITE);
  display1.display();
  display2.display();

  SPI.begin();

  mcp2515.reset();
  mcp2515.setBitrate(CAN_500KBPS,
                     MCP_8MHZ); // Sets CAN at speed 500KBPS and Clock 8MHz
  mcp2515.setNormalMode();      // Sets CAN at normal mode
}

void loop()
{
    /*myGLCD.setBackColor(0, 0, 0);

    myGLCD.setFont(BigFont);
    myGLCD.setColor(255, 255, 255);
    if (total_kwh < 0) {
        myGLCD.print(String("-"), 200, 20);
    }
    myGLCD.print(String("kW/h"), CENTER, 110);
    myGLCD.print(String("%SOC"), CENTER, 200);
    myGLCD.print(String("SUP"), CENTER, 290);
    myGLCD.setColor(0, 255, 0);
    myGLCD.print(String("-"), 400, 20);

    myGLCD.setFont(SevenSegNumFont);
    myGLCD.setColor(255, 255, 255);
    myGLCD.print(String(SOC), CENTER, 150);
    myGLCD.print(String(supplimental_soc), CENTER, 240);
    myGLCD.print(String(abs(total_kwh)), CENTER, 0);*/

    if (millis() - prevDisplayTime >= DISPLAY_INTERVAL) {
        prevDisplayTime = millis();
        display();
    }

    can_frame frame{};
    if (mcp2515.readMessage(&frame) == MCP2515::ERROR_OK) {
        parse_frame(frame);
    }
    calculateKWh();
    calc_suplimental_soc(); // Function to calculate the supplemental SOC
}
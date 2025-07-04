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
//Adafruit_SSD1306 display2(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const unsigned long DISPLAY_INTERVAL = 500;   // update OLED every 500 ms
unsigned long prevDisplayTime = 0;

MCP2515 mcp2515(10); // SPI CS Pin 10

inline int16_t  be16s(const uint8_t* p){ return  (int16_t)((p[0]<<8)|p[1]); }
inline uint16_t be16u(const uint8_t* p){ return (uint16_t)((p[0]<<8)|p[1]); }

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
    //display2.clearDisplay();

    display1.setTextSize(3);
    display1.setTextColor(WHITE);
    display1.setCursor(0, 0);
    display1.print("kWh:");
    display1.println(total_kwh);
    
    display1.setTextSize(3);
    display1.setTextColor(WHITE);
    display1.setCursor(0, 32);
    display1.print("SOC:");
    display1.println(SOC);

    //display2.setCursor(0, 20);
    //display2.print("Suppl. SOC: ");
    //display2.println(supplimental_soc);

    // Display the updates
    display1.display();
    //display2.display();
}


void setup()
{
  //myGLCD.InitLCD();

  //myGLCD.clrScr();

  if(!display1.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    //Serial.println(F("SSD1306A allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  //if(!display2.begin(SSD1306_SWITCHCAPVCC, 0x3D)) { 
    //Serial.println(F("SSD1306B allocation failed"));
  //  for(;;); // Don't proceed, loop forever
  //}

  display1.display();
  //display2.display();
  delay(1000);
  display1.clearDisplay();
  display1.drawPixel(10, 10, WHITE);
  //display2.clearDisplay();
  //display2.drawPixel(10, 10, WHITE);
  display1.display();
  //display2.display();

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

    can_frame f;
    while (mcp2515.readMessage(&f) == MCP2515::ERROR_OK) {
    switch (f.can_id) {
        case 0x6B0: current_total  = be16s(&f.data[0]) * 10;
                  voltage_total  = be16u(&f.data[2]) * 10;
                  SOC  = f.data[4];
                  break;
        default: break;
        }
    }
    calculateKWh();
    calc_suplimental_soc(); // Function to calculate the supplemental SOC
}
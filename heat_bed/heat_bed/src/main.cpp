// Пины реле
//All temps in Celsius
/*
Pinout:
A0: MuxInput
A1: Pot
A4: SDA for both OLED
A5: SCL for both OLED
2: Relay1
3: Relay2
4: Relay3
5: Relay4
6: Mux0
7: Mux1
8: Mux2
*/
#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <avr/wdt.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display1(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
//Adafruit_SSD1306 display2(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const int relay1 = 2;
const int relay2 = 3;
const int relay3 = 4;
const int relay4 = 5;
const int mux0 = 6;
const int mux1 = 7;
const int mux2 = 8;
const int mux3 = 9;
const int muxEN = 10;
//const int thermo1 = A0;
//const int thermo2 = A1;
//const int thermo3 = A2;
//const int thermo4 = A3;
const int muxSIG = A0;
const int potentiometer = A1;
int val1;
int val2;
int val3;
int val4;
int value;
int pot;
float temp;
int Vo;
float R1 = 10000; //Value of resitor in circuit
float logR2, R2, T;
float c1 =  9.44143417e-04, c2 = 1.34448989e-03, c3 = 1.88823919e-05;
int tempThreshold = 80;
const int potMin = 20;
const int potMax = 180;

const unsigned long READ_INTERVAL    = 200;   // read sensors every 200 ms
const unsigned long DISPLAY_INTERVAL = 500;   // update OLED every 500 ms
unsigned long prevReadTime    = 0;
unsigned long prevDisplayTime = 0;

const float R_pullup = 10000.0f;    // 10 kΩ fixed resistor
const float R0       = 100000.0f;   // 100 kΩ at 25 °C
const float T0       = 298.15f;     // 25 °C in Kelvin
const float B        = 4000.0f;     // Beta constant
float R_therm;
float invT;
float Tkelvin;

void setup() {
  //Serial.begin(115200);
  if(!display1.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("SSD1306A allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  //if(!display2.begin(SSD1306_SWITCHCAPVCC, 0x3D)) { 
    //Serial.println(F("SSD1306B allocation failed"));
    //for(;;); // Don't proceed, loop forever
  //}

  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  display1.display();
  //display2.display();
  delay(1000);
  display1.clearDisplay();
  display1.drawPixel(10, 10, WHITE);
  //display2.clearDisplay();
  //display2.drawPixel(10, 10, WHITE);
  display1.display();
  //display2.display();
  delay(2000);
  pinMode(relay1, OUTPUT);
  pinMode(relay2, OUTPUT);
  pinMode(relay3, OUTPUT);
  pinMode(relay4, OUTPUT);
  //pinMode(thermo1, INPUT);
  //pinMode(thermo2, INPUT);
  //pinMode(thermo3, INPUT);
  //pinMode(thermo4, INPUT);
  pinMode(mux0, OUTPUT);
  pinMode(mux1, OUTPUT);
  pinMode(mux2, OUTPUT);
  pinMode(mux3, OUTPUT);
  pinMode(muxEN, OUTPUT);
  pinMode(muxSIG, INPUT);
  pinMode(potentiometer, INPUT);

  digitalWrite(relay1, LOW);
  digitalWrite(relay2, LOW);
  digitalWrite(relay3, LOW);
  digitalWrite(relay4, LOW);
  digitalWrite(mux0, LOW);
  digitalWrite(mux1, LOW);
  digitalWrite(mux2, LOW);
  digitalWrite(mux3, LOW);
  wdt_enable(WDTO_2S); 
}

void loop() {
  unsigned long now = millis();
  wdt_reset();

  if (now - prevReadTime >= READ_INTERVAL) {
    prevReadTime = now;
    //int val1 = analogRead(thermo1);
    //int val2 = analogRead(thermo2);
    //int val3 = analogRead(thermo3);
    //int val4 = analogRead(thermo4);
    val2 = calcTemp(getMuxTemp(0));
    val1 = calcTemp(getMuxTemp(1));
    val3 = calcTemp(getMuxTemp(2));
    val4 = calcTemp(getMuxTemp(3));
    //Serial.println();
    tempThreshold = getPotTemp();
    // Включаем первое реле
    runHeatingBed(val1+1, val2+1, val3+1, val4+1);
  }
  if (now - prevDisplayTime >= DISPLAY_INTERVAL) {
    prevDisplayTime = now;
    //writeTemp(val1, val2, val3, val4);
    //displaySetTemp(tempThreshold);
    displayTemp(tempThreshold, val1, val2, val3, val4);
  }
}

int getMuxTemp(int pin){
  if (pin % 2 == 0){
    if (pin > 1){
      //2
      digitalWrite(mux0, LOW);
      digitalWrite(mux1, HIGH);
      digitalWrite(mux2, LOW);
      digitalWrite(mux3, LOW);
      digitalWrite(muxEN, LOW);
      delay(25);
      value = analogRead(muxSIG);
    }else{
      //0
      digitalWrite(mux0, LOW);
      digitalWrite(mux1, LOW);
      digitalWrite(mux2, LOW);
      digitalWrite(mux3, LOW);
      digitalWrite(muxEN, LOW);
      delay(25);
      value = analogRead(muxSIG);
    }
  } else {
    if (pin > 1){
      //3
      digitalWrite(mux0, HIGH);
      digitalWrite(mux1, HIGH);
      digitalWrite(mux2, LOW);
      digitalWrite(mux3, LOW);
      digitalWrite(muxEN, LOW);
      delay(25);
      value = analogRead(muxSIG);
    }else{
      //1
      digitalWrite(mux0, HIGH);
      digitalWrite(mux1, LOW);
      digitalWrite(mux2, LOW);
      digitalWrite(mux3, LOW);
      digitalWrite(muxEN, LOW);
      delay(25);
      value = analogRead(muxSIG);
    }
  }
  digitalWrite(mux0, LOW);
  digitalWrite(mux1, LOW);
  digitalWrite(mux2, LOW);
  digitalWrite(mux3, LOW);
  digitalWrite(muxEN, HIGH);
  //Serial.println(value);
  return value;
}

int getPotTemp() {
  pot = map(analogRead(potentiometer), 0, 1023, potMin, potMax);
  //Serial.println(pot);
  return pot;
}

void runHeatingBed(int val1, int val2, int val3, int val4){
  if (ifTempBelow(val1)) {
    digitalWrite(relay1, HIGH);
    delay(100); // Держим 100 мс
    digitalWrite(relay1, LOW);
    delay(5);  // Пауза 20 мс
  }
    
  if (ifTempBelow(val2)) {
    // Включаем второе реле
    digitalWrite(relay2, HIGH);
    delay(100); // Держим 100 мс
    digitalWrite(relay2, LOW);
    delay(5);  // Пауза 20 мс
  }

  if (ifTempBelow(val3)) {
    digitalWrite(relay3, HIGH);
    delay(100); // Держим 100 мс
    digitalWrite(relay3, LOW);
    delay(5);  // Пауза 20 мс
  }

  if (ifTempBelow(val4)) {
    digitalWrite(relay4, HIGH);
    delay(100); // Держим 100 мс
    digitalWrite(relay4, LOW);
    delay(5);  // Пауза 20 мс
  }
}


float calcTemp(int Vo) {
    // Avoid divide-by-zero
    if (Vo <= 0) return -273.15f;

    // Compute thermistor resistance from ADC (across pull-up resistor)
    R_therm = R_pullup * (1023.0f - Vo) / Vo;  // :contentReference[oaicite:7]{index=7}

    // Inverse temperature using Beta model
    invT = (1.0f / T0)
               + (1.0f / B) * log(R_therm / R0);      // :contentReference[oaicite:8]{index=8}

    Tkelvin = 1.0f / invT;
    return Tkelvin - 273.15f;  // °C
}

bool ifTempBelow(int value) {
  temp = (value);
  return temp < tempThreshold;
}

void writeTemp(int val1, int val2, int val3, int val4) {
  Serial.println(val1);
  Serial.println(val2);
  Serial.println(val3);
  Serial.println(val4);
  Serial.println();
}

void displayTemp(int temp, int val1, int val2, int val3, int val4) {
  display1.clearDisplay();

  display1.setTextSize(2);     
  display1.setTextColor(WHITE); 
  display1.cp437(true);
  display1.setCursor(0, 0);
  display1.print("Set:");
  display1.print(temp);
  display1.print("C");
  display1.display();
  display1.setCursor(0, SCREEN_HEIGHT/2);
  display1.print("1:");
  display1.print(val1);
  display1.setCursor(SCREEN_WIDTH/2, SCREEN_HEIGHT/2);
  display1.print("2:");
  display1.print(val2);
  display1.setCursor(0, 3*SCREEN_HEIGHT/4);
  display1.print("3:");
  display1.print(val3);
  display1.setCursor(SCREEN_WIDTH/2, 3*SCREEN_HEIGHT/4);
  display1.print("4:");
  display1.print(val4);
  display1.display();
  delay(20);
}

//void displaySetTemp(float temp){
  //display2.write("Desired Temp: ");
  //display2.write(temp);
  //display2.write(" C");
  //display2.display();
//}

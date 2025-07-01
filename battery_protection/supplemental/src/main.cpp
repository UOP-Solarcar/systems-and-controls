#include <Arduino.h>

const float R1 = 27000;
const float R2 = 10000;
const float VREF = 1.087;
const float DIV = R1 / (R1  + R2);
const float THRESHOLD = 11.0;
const byte LED_PIN = LED_BUILTIN;
const byte SENSE_PIN = A0;

float readPackVoltage() {
  uint16_t raw = analogRead(SENSE_PIN);
  float vOut = raw * VREF / 1023.0;
  return vOut / DIV;

}

void setup() {
  analogReference(INTERNAL);
  pinMode(LED_PIN, OUTPUT);
  Serial.begin(115200);

}

void loop() {
  float vPack = readPackVoltage();
  bool low = vPack < THRESHOLD;

  digitalWrite(LED_PIN, low);

}

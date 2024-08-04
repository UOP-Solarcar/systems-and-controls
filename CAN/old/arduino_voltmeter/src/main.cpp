#include <Arduino.h>

#ifndef MONITOR_SPEED
#define MONITOR_SPEED 115200
#endif

int ref_read = 1024;

const uint8_t ANALOG_IN_PIN = 0;

const double ref_v = 4.2;

void setup() {
  // Setup Serial Monitor
  Serial.begin(MONITOR_SPEED);
  Serial.println("DC Voltage Test");
  Serial.print("Supply ");
  Serial.print(ref_v);
  Serial.println("v for Callibration");
  for (size_t i = 0; i < 5; i++) {
    Serial.println(5 - i);
    delay(1000);
  };
  ref_read = analogRead(ANALOG_IN_PIN);
  Serial.print("Callibrated as ");
  Serial.println(ref_read);
}

void loop() {
  double v = analogRead(ANALOG_IN_PIN) / ((ref_read / ref_v));

  // Print results to Serial Monitor to 2 decimal places
  Serial.print("Input Voltage = ");
  Serial.println(v);

  // Short delay
  delay(250);
}

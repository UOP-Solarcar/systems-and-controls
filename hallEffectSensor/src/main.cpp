#include <Arduino.h>

const int Pin1 = A1;
const int Pin2 = A2;
const int Pin3 = A3;
int reading1;
int reading2;
int reading3;

void setup() {
  Serial.begin(9600);
  pinMode(Pin1, INPUT);
  pinMode(Pin2, INPUT);
  pinMode(Pin3, INPUT);
}

void loop() {
  reading1 = digitalRead(Pin1);
  reading2 = digitalRead(Pin2);
  reading3 = digitalRead(Pin3);
  Serial.print("Readings ");
  Serial.print(reading1);
  Serial.print(", ");
  Serial.print(reading2);
  Serial.print(", ");
  Serial.print(reading3);
  delay(20);
  Serial.print('\n');
}


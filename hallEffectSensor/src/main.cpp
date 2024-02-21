#include <Arduino.h>

const int Pin1 = A1;
const int Pin2 = A2;
const int Pin3 = A3;
const int Pin4 = A4;
int reading1;
int reading2;
int reading3;
int reading4;

void setup() {
  Serial.begin(115200);
  pinMode(Pin1, INPUT);
  pinMode(Pin2, INPUT);
  pinMode(Pin3, INPUT);
  pinMode(Pin4, INPUT);
}

void loop() {
  reading1 = analogRead(Pin1);
  reading2 = analogRead(Pin2);
  reading3 = analogRead(Pin3);
  reading4 = analogRead(Pin4);
  Serial.print("Readings ");
  Serial.print(reading1);
  Serial.print(", ");
  Serial.print(reading2);
  Serial.print(", ");
  Serial.print(reading3);
  Serial.print(", ");
  Serial.println(reading4);
  delay(20);
}


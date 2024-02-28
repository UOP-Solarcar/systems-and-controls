include <Arduino.h>

const int Pin1 = A1;
const int Pin2 = A2;
const int Pin3 = A3;
bool reading1;
bool reading2;
bool reading3;
int rotation;
unsigned int rpm;
long time;


void setup() {
  Serial.begin(115200);
  pinMode(Pin1, INPUT);
  pinMode(Pin2, INPUT);
  pinMode(Pin3, INPUT);
  rotation = 0;
  time = millis();
}

void loop() {
  if (reading1 != digitalRead(Pin1)) {
    reading1 = !reading1;
    rotation += 60;
  }
  if (reading2 != digitalRead(Pin2)) {
    reading2 = !reading2;
    rotation += 60;
  }
  if (reading3 != digitalRead(Pin3)) {
    reading3 = !reading3;
    rotation += 60;
  }

  while (rotation > 360){
    rotation -= 360;
    Serial.println("One rotation");
    rpm = (millis() - time) * 1000 * 60;
    time = millis();
    Serial.println(rpm);
  }
  
  delay(20);
}

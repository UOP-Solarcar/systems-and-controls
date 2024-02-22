#include <Arduino.h>

const int hallPin = A2;  // Replace with the actual pin number connected to the Hall Sensor

unsigned long lastTime = 0;
unsigned long currentTime = 0;
unsigned long timeDifference = 0;
float rpm = 0.0;

void setup() {
  pinMode(hallPin, INPUT);
  Serial.begin(115200);
}

void loop() {
  // Read the state of the Hall Sensor
  int hallState = digitalRead(hallPin);

  // Check for a rising edge (transition from LOW to HIGH)
  if (hallState == 1 && lastTime == 0) {
    lastTime = millis();  // Record the time of the rising edge
  } else if (hallState == 0 && lastTime != 0) {
    currentTime = millis();  // Record the time of the falling edge
    timeDifference = currentTime - lastTime;

    // Calculate RPM using the formula: RPM = (1 / (timeDifference / 1000)) * 60
    rpm = (1.0 / (timeDifference / 1000.0)) * 60.0;

    // Print the calculated RPM to the Serial Monitor
    Serial.print("RPM: ");
    Serial.println(rpm);
    Serial.println('\n');

    // Reset variables for the next calculation
    lastTime = 0;
    currentTime = 0;
    timeDifference = 0;
  }
}

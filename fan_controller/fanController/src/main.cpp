#include <Arduino.h>

const int pwmPin = 9; // OC1A output — must be pin 9 for Timer1

void setup() {
  pinMode(pwmPin, OUTPUT);
  Serial.begin(115200);

  TCCR1A = 0; TCCR1B = 0; TCCR1C = 0;

  // Configure Timer1 for 25kHz PWM (non-inverting, fast PWM mode)
  TCCR1A = 0;
  TCCR1B = 0;

  // Fast PWM, TOP = ICR1
  TCCR1A |= (1 << COM1A1); // Non-inverting on OC1A (pin 9)
  TCCR1A |= (1 << WGM11);
  TCCR1B |= (1 << WGM13) | (1 << WGM12);

  // No prescaler, 16MHz / 640 = 25kHz
  TCCR1B |= (1 << CS10);
  ICR1 = 639; // TOP value for 25kHz

  OCR1A = 0; // Start at 0% duty cycle
}

// Convert temps between 28 - 43 C to ramp up fan speed
int tempToFanSpeed(double temp) {
  return int((temp - 28) * 10);
}

// Set fan speed: 0–100 (%)
void setFanSpeed(double temp) {
  int percent = constrain(tempToFanSpeed(temp), 0, 100);
  OCR1A = (percent * 639UL) / 100;
}

void loop() {
  // Ramp up
  for (int temp = 0; temp <= 45; temp += 1) {
    setFanSpeed(temp);
    Serial.println(temp);
    delay(2000);
  }

  // Ramp down
  for (int temp = 45; temp >= 0; temp -= 1) {
    setFanSpeed(temp);
    Serial.println(temp);
    delay(2000);
  }
}
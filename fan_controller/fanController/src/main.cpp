/*
This is a outline for the fan speed controller.

Need to add how the controller will get the temperature of the battery.
*/

#include <Arduino.h>

const unsigned long INTERVAL = 5000;
int fanSpeed;
int temperature;
unsigned long now;
unsigned long prevReadTime = 0;
int fanCurve[10] = {33, 35, 37, 39, 41, 43, 45, 47, 49, 51};

const byte OC1A_PIN = 9;
const byte OC1B_PIN = 10;

const word PWM_FREQ_HZ = 25000; //Adjust this value to adjust the frequency
const word TCNT1_TOP = 16000000/(2*PWM_FREQ_HZ);

void setPwmDuty(byte duty) {
  OCR1A = (word) (duty*TCNT1_TOP)/100;
}

int getTemperature() {
  //somehow get temperature
  return 36;
}

void setup() {
  
  pinMode(OC1A_PIN, OUTPUT);

  // Clear Timer1 control and count registers
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0;

  // Set Timer1 configuration
  // COM1A(1:0) = 0b10   (Output A clear rising/set falling)
  // COM1B(1:0) = 0b00   (Output B normal operation)
  // WGM(13:10) = 0b1010 (Phase correct PWM)
  // ICNC1      = 0b0    (Input capture noise canceler disabled)
  // ICES1      = 0b0    (Input capture edge select disabled)
  // CS(12:10)  = 0b001  (Input clock select = clock/1)
  
  TCCR1A |= (1 << COM1A1) | (1 << WGM11);
  TCCR1B |= (1 << WGM13) | (1 << CS10);
  ICR1 = TCNT1_TOP;
}

void loop() {
  now = millis();
  if (now - prevReadTime >= INTERVAL){
    prevReadTime = now;
    int temp = getTemperature();
    fanSpeed = 0;
    for (int i = 0; i < 10; i++){
      if (temp > fanCurve[i]){
        fanSpeed = fanSpeed + 10;
      }
    }
    setPwmDuty(0);
    delay(5000);
    setPwmDuty(fanSpeed); //Change this value 0-100 to adjust duty cycle
  }
    
}

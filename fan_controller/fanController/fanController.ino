/*
This is a outline for the fan speed controller.

Need to add how the controller will get the temperature of the battery.
*/

#define FAN_PIN 3
#define FS_ADDR 0x01

const unsigned long INTERVAL = 1000;
int fanSpeed;
int temperature;
unsigned long now;
unsigned long prevReadTime = 0;
int fanCurve[8] = {35, 40, 45, 50, 55, 60, 65, 70};

void setup() {
  pinMode(FAN_PIN, OUTPUT);
}

void loop() {
  now = millis();
  if (now - prevReadTime >= INTERVAL){
    prevReadTime = now;
    int temp = getTemperature();
    fanSpeed = 0;
    for (int i = 0; i < 8; i++){
      if (temp > fanCurve[i]){
        fanSpeed = fanSpeed + 31;
      }
    }
    analogWrite(FAN_PIN, fanSpeed);
  }
}

int getTemperature() {
  //somehow get temperature
}
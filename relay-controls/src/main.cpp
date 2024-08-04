#include <Arduino.h>
#include <Keypad.h>

const uint8_t RIGHT_PIN = 2;
const uint8_t LEFT_PIN = 3;
const uint8_t HEADLIGHT_PIN = 4;

enum SignalState { HAZARD, TURNLEFT, TURNRIGHT, DEF };
enum HeadlightState { ON, OFF };

SignalState state = DEF;
HeadlightState Hlstate = OFF;

const byte ROWS = 4; // four rows
const byte COLS = 4; // four columns
// define the symbols on the buttons of the keypads
char hexaKeys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

uint8_t rowPins[ROWS] = {5, 6, 7, 8}; // connect to the row pinouts of the keypad
uint8_t colPins[COLS] = {9, 10, 11, 12}; // connect to the column pinouts of the keypad

Keypad customKeypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);

bool breaking = false;
unsigned long previousMillis = 0;
const long interval = 750; // interval for flashing

void reset() {
  state = DEF;
  Hlstate = OFF;
}

void setup() {
  Serial.begin(9600);
  pinMode(RIGHT_PIN, OUTPUT);
  pinMode(LEFT_PIN, OUTPUT);
  pinMode(HEADLIGHT_PIN, OUTPUT);
  reset();
}

void on(uint8_t pin) {
  digitalWrite(pin, LOW);
}

void on(uint8_t pin0, uint8_t pin1) {
  digitalWrite(pin0, LOW);
  digitalWrite(pin1, LOW);
}

void off(uint8_t pin) {
  digitalWrite(pin, HIGH);
}

void off(uint8_t pin0, uint8_t pin1) {
  digitalWrite(pin0, HIGH);
  digitalWrite(pin1, HIGH);
}

void loop() {
  char customKey = customKeypad.getKey();
  unsigned long currentMillis = millis();

  // if(customKey){
  //   Serial.println(customKey);
  // }

  switch (customKey) {
    case '1':
      state = HAZARD;
      break;
    case '2':
      state = TURNLEFT;
      break;
    case '3':
      state = TURNRIGHT;
      break;
    case '4':
      state = DEF;
      break;
  }

  switch (state) {
    case HAZARD:
      if (breaking) {
        on(LEFT_PIN, RIGHT_PIN);
      } else if (currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis;
        digitalWrite(LEFT_PIN, !digitalRead(LEFT_PIN));
        digitalWrite(RIGHT_PIN, !digitalRead(RIGHT_PIN));
      }
      break;
    case TURNRIGHT:
      digitalWrite(LEFT_PIN, !breaking);
      if (currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis;
        digitalWrite(RIGHT_PIN, !digitalRead(RIGHT_PIN));
      }
      break;
    case TURNLEFT:
      digitalWrite(RIGHT_PIN, !breaking);
      if (currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis;
        digitalWrite(LEFT_PIN, !digitalRead(LEFT_PIN));
      }
      break;
    case DEF:
      digitalWrite(LEFT_PIN, !breaking);
      digitalWrite(RIGHT_PIN, !breaking);
      break;
  }

  switch (Hlstate) {
    case ON:
      digitalWrite(HEADLIGHT_PIN, LOW);
      break;
    case OFF:
      digitalWrite(HEADLIGHT_PIN, HIGH);
      break;
  }
}

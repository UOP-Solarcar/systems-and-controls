#include <Arduino.h>
#include <Keypad.h>

const uint8_t RIGHT_PIN = 5;
const uint8_t LEFT_PIN = 6;
const uint8_t HEADLIGHT_PIN = 7;
// const uint8_t RELAY_PIN_4 = 5;
// const uint8_t RELAY_PIN_5 = 6;
// const uint8_t RELAY_PIN_6 = 7;
// const uint8_t RELAY_PIN_7 = 8;
// const uint8_t RELAY_PIN_8 = 9;

enum SignalState { HAZARD, TURNLEFT, TURNRIGHT, DEF };

enum HeadlightState { ON, OFF };

SignalState state = DEF;
HeadlightState Hlstate = OFF;

const byte ROWS = 4; // four rows
const byte COLS = 4; // four columns
// define the cymbols on the buttons of the keypads
char hexaKeys[ROWS][COLS] = {{'1', '2', '3', 'A'},
                             {'4', '5', '6', 'B'},
                             {'7', '8', '9', 'C'},
                             {'*', '0', '#', 'D'}};

uint8_t rowPins[ROWS] = {14, 15, 16,
                         17}; // connect to the row pinouts of the keypad
uint8_t colPins[COLS] = {18, 19, 3,
                         2}; // connect to the column pinouts of the keypad

Keypad customKeypad =
    Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);

bool breaking = false;

void reset() {
  state = DEF;
  Hlstate = OFF;
}

void setup() {
  Serial.begin(9600);
  pinMode(RIGHT_PIN, OUTPUT);
  pinMode(LEFT_PIN, OUTPUT);
  pinMode(HEADLIGHT_PIN, OUTPUT);
  // pinMode(RELAY_PIN_4, OUTPUT);
  // pinMode(RELAY_PIN_5, OUTPUT);
  // pinMode(RELAY_PIN_6, OUTPUT);
  // pinMode(RELAY_PIN_7, OUTPUT);
  // pinMode(RELAY_PIN_8, OUTPUT);
  reset();
}

void on(uint8_t pin) { digitalWrite(pin, LOW); }

void on(uint8_t pin0, uint8_t pin1) {
  digitalWrite(pin0, LOW);
  digitalWrite(pin1, LOW);
}

void off(uint8_t pin) { digitalWrite(pin, HIGH); }

void off(uint8_t pin0, uint8_t pin1) {
  digitalWrite(pin0, HIGH);
  digitalWrite(pin1, HIGH);
}

void flash(uint8_t pin, int wait = 750) {
  on(pin);
  delay(wait);
  off(pin);
  delay(wait);
}

void flash2(uint8_t pin0, uint8_t pin1, int wait = 750) {
  on(pin0, pin1);
  delay(wait);
  off(pin0, pin1);
  delay(wait);
}

void loop() {
  char customKey = customKeypad.getKey();

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
  }

  switch (state) {
  case HAZARD: {
    if (breaking) {
      on(LEFT_PIN, RIGHT_PIN);
    } else {
      flash2(RIGHT_PIN, LEFT_PIN);
    }
  } break;
  case TURNRIGHT: {
    digitalWrite(LEFT_PIN, !breaking);
    flash(RIGHT_PIN);
  } break;
  case TURNLEFT: {
    digitalWrite(RIGHT_PIN, !breaking);
    flash(LEFT_PIN);
  } break;
  case DEF: {
    digitalWrite(LEFT_PIN, !breaking);
    digitalWrite(RIGHT_PIN, !breaking);
  } break;
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

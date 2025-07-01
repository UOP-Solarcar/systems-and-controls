#include <Arduino.h>
#include <digitalWriteFast.h>   // already in your lib deps
#include <math.h>               // for log()
#include <mcp2515.h>

/* ─── Pin assignments ───────────────────────────── */
const uint8_t HALL_A = 2;   // INT0
const uint8_t HALL_B = 3;   // INT1
const uint8_t HALL_C = 5;   // PD5 / PCINT21
const uint8_t CAN_CS = 8;

const byte    TEMP_PIN  = A0;    // NTC divider node
/* ─── Motor-specific constants ───────────────────── */
const uint8_t polePairs = 7;     // <--- CHANGE to your motor’s value
/* Thermistor constants (10 k NTC 3950 β) */
const float SERIES_R  = 10000.0f;
const float NOMINAL_R = 10000.0f;
const float B_COEFF   = 3950.0f;
const float T0_K      = 298.15f;   // 25 °C in kelvin


MCP2515 mcp2515(8);

/* ─── Globals updated in ISRs ────────────────────── */
volatile uint8_t hallState = 0;   // latest 3-bit code
volatile long    elecTicks = 0;   // +1 / –1 per 60° sector

/* ─── Direction lookup table (Gray-cycle) ──────────
   Row = previous state, Col = new state, Val = +1,-1,0 */
const int8_t dirLUT[8][8] PROGMEM = {
/*to 0  1  2  3  4  5  6  7 */
/*0*/{0, 0, 0, 0, 0, 0, 0, 0},
/*1*/{0, 0,+1, 0,-1, 0, 0, 0},
/*2*/{0,-1, 0,+1, 0, 0, 0, 0},
/*3*/{0, 0,-1, 0, 0,+1, 0, 0},
/*4*/{0,+1, 0, 0, 0, 0,-1, 0},
/*5*/{0, 0, 0,-1, 0, 0,+1, 0},
/*6*/{0, 0,+1, 0,-1, 0, 0, 0},
/*7*/{0, 0, 0, 0, 0, 0, 0, 0}
};

/* ─── Forward declarations ───────────────────────── */
void hallISR();
float readMotorTempC();

/* ─── Setup ───────────────────────────────────────── */
void setup() {
  pinMode(HALL_A, INPUT_PULLUP);
  pinMode(HALL_B, INPUT_PULLUP);
  pinMode(HALL_C, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(HALL_A), hallISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(HALL_B), hallISR, CHANGE);

  /* Enable pin-change IRQ for PD5 = D5 = PCINT21 */
  PCICR  |= (1 << PCIE2);       // PORTD group
  PCMSK2 |= (1 << PCINT21);     // PD5 bit
  PCMSK3 |= (1 << PCINT22);     // PD6 bit for CAN interrupt

  analogReference(DEFAULT);     // 5 V analog reference 
  
  mcp2515.reset();

  mcp2515.setBitrate(CAN_500KBPS,
                     MCP_8MHZ); // Sets CAN at speed 500KBPS and Clock 8MHz
  mcp2515.setNormalMode();      // Sets CAN at normal mode
  Serial.begin(115200);
}

ISR(PCINT2_vect) { hallISR(); } // reuse same routine

/* ─── Hall interrupt service routine ─────────────── */
void hallISR() {
  uint8_t a = digitalReadFast(HALL_A);
  uint8_t b = digitalReadFast(HALL_B);
  uint8_t c = digitalReadFast(HALL_C);

  uint8_t newState = (c << 2) | (b << 1) | a;
  static uint8_t lastState = 0;

  if (newState == 0 || newState == 7) return;         // illegal

  int8_t dir = pgm_read_byte(&dirLUT[lastState][newState]);
  elecTicks += dir;
  lastState  = newState;
  hallState  = newState;
}

/* ─── Main loop (10 ms window) ───────────────────── */
void loop() {
  static uint32_t lastMicros = 0;

  if (micros() - lastMicros >= 10000UL) {             // 10 ms
    noInterrupts();
    long ticks = elecTicks;
    elecTicks  = 0;
    interrupts();

    /* 6 electrical sectors per e-rev */
    float mechRPS = (ticks / 6.0f) / polePairs / 0.01f; // 0.01 s window
    float RPM     = mechRPS * 60.0f;
    float tempC   = readMotorTempC();

    Serial.print("RPM: ");   Serial.print(RPM, 1);
    Serial.print("  Temp: ");Serial.print(tempC, 1);
    Serial.println(" °C");

    lastMicros = micros();
  }
}

/* ─── Thermistor helper ──────────────────────────── */
float readMotorTempC() {
  int   raw    = analogRead(TEMP_PIN);        // 0-1023
  float vRatio = raw / 1023.0f;
  if (vRatio <= 0.0f || vRatio >= 1.0f) return NAN; // open/short check

  float rTherm = SERIES_R * (vRatio / (1.0f - vRatio));
  float invT   = 1.0f / T0_K + (1.0f / B_COEFF) * log(rTherm / NOMINAL_R);
  return (1.0f / invT) - 273.15f;
}

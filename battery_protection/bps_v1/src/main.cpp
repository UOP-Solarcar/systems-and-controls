/********************************************************************
 *  Orion BMS run / trip controller + Noctua fan speed control
 *
 *  Pin map:
 *    D3  : E-stop input (INPUT_PULLUP, rising edge ISR)
 *    D4  : Precharge bypass relay (normally-closed; de-energized shorts the
 *          precharge resistor out, energized inserts the resistor)
 *    D5  : Main contactor relay
 *    D6  : Fault lamp (blinks while fault present)
 *    D9  : Fan PWM output (Timer1, 25 kHz — OC1A, do not use analogWrite)
 *    D10 : MCP2515 SPI CS
 *
 *  Fan behavior:
 *    - Driven by temp_hi from Orion CAN frame 0x6B3
 *    - Linear ramp: 0% at <=28 C, 100% at >=43 C
 *    - Forced to 100% on any fault, e-stop, or CAN watchdog event
 *    - Held at 0% until first valid CAN temp reading (temp_hi != 0xFF)
 *
 *  E-stop behavior:
 *    - Debounced: raw D3 level must hold steady for ESTOP_DEBOUNCE_MS before
 *      the state changes, so contact bounce cannot chatter the fault code / lamp
 *    - Latched: a confirmed press trips until power cycle; releasing the button
 *      does NOT clear it (matches the 2 s-hold and CAN-latched fault behavior)
 *
 *  CAN watchdog (3 states):
 *    NOMINAL  -- frames arriving within 2000 ms, normal operation
 *    TIMEOUT  -- no valid frame for >2000 ms; contactors open immediately,
 *                fan -> 100%, 30 s recovery window begins
 *    LATCHED  -- 30 s elapsed with no recovery; permanent fault,
 *                requires power cycle to clear
 *    Recovery -- any valid 0x6B0/6B2/6B3 frame within the 30 s window
 *                returns to NOMINAL and recloses contactors
 *
 *  CAN TX frames (BPS-originated, never overlap with Orion IDs):
 *    0x790  BPS Status heartbeat  -- 100 ms, always broadcasting
 *    0x791  BPS Fault detail      -- 500 ms while faulted, immediate on change
 *
 *  Fault codes (byte 2 of 0x790, byte 1 of 0x791):
 *    0x00 No fault
 *    0x01 Over-current
 *    0x02 Charge over-current
 *    0x03 Pack over-voltage
 *    0x04 Pack under-voltage
 *    0x05 Over-temperature
 *    0x06 Cell over-voltage
 *    0x07 Cell under-voltage
 *    0x08 E-stop
 *    0x09 CAN timeout
 *    0x0A CAN latched
 ********************************************************************/
#include <SPI.h>
#include <mcp2515.h>
#include "bps_logic.h"

/* ================================================================
 *  PIN MAP
 * ================================================================ */
#define RELAY_CLOSE_LEVEL   HIGH          // Change to LOW for active-LOW relay boards
constexpr uint8_t PRECHARGE_PIN    = 4;
constexpr uint8_t CONTACTOR_PINS[] = {5};
constexpr uint8_t PIN_FAULT_LAMP   = 6;
constexpr uint8_t FAN_PWM_PIN      = 9;  // OC1A -- Timer1, must be pin 9
constexpr uint8_t ESTOP_PIN        = 3;

constexpr uint8_t RELAY_OPEN_LEVEL =
        (RELAY_CLOSE_LEVEL == HIGH ? LOW : HIGH);

/* The precharge relay is a normally-closed bypass across the precharge
   resistor: de-energized (LOW) it sits closed and shorts the resistor out,
   energized (HIGH) it opens and forces current through the resistor. That's
   the opposite sense of the main (normally-open) contactor, so it needs its
   own close/open levels rather than sharing RELAY_CLOSE_LEVEL. */
constexpr uint8_t PRECHARGE_CLOSE_LEVEL = LOW;
constexpr uint8_t PRECHARGE_OPEN_LEVEL  = HIGH;

/* Precharge dwell: on startup the bypass relay is opened (LOW->HIGH) so the
   precharge resistor current-limits the initial charge into the load caps
   while the main contactor closes; after this dwell the bypass relay recloses
   (HIGH->LOW) to short the resistor out for full continuous current. */
constexpr unsigned long PRECHARGE_DELAY_MS = 5000UL;

/* ================================================================
 *  FAN CONSTANTS
 *  Timer1 at 16 MHz, no prescaler: ICR1 = 16e6/25000 - 1 = 639
 *  OCR1A range: 0 (0%) ... 639 (100%)
 * ================================================================ */
constexpr uint16_t TIMER1_TOP   = 639;
constexpr uint8_t  FAN_TEMP_MIN = 28;   // C -> 0% speed
constexpr uint8_t  FAN_TEMP_MAX = 43;   // C -> 100% speed

/* ================================================================
 *  CAN WATCHDOG
 *  Only frames 0x6B0, 0x6B2, 0x6B3 count as a heartbeat.
 *  Unknown/noise frames on the bus do NOT reset the timer.
 * ================================================================ */
constexpr unsigned long CAN_TIMEOUT_MS = 2000UL;   // no frame -> TIMEOUT
constexpr unsigned long CAN_LATCH_MS   = 30000UL;  // no recovery -> LATCHED

enum class CanState : uint8_t {
  NOMINAL,   // frames arriving normally
  TIMEOUT,   // dropout detected, recovery window open
  LATCHED    // recovery window expired, power cycle required
};

CanState      canState    = CanState::NOMINAL;
unsigned long tLastCanRx  = 0;   // millis() of last valid heartbeat frame
unsigned long tTimeoutAt  = 0;   // millis() when TIMEOUT was entered

/* ================================================================
 *  CAN TX FRAME IDs
 * ================================================================ */
constexpr uint32_t CAN_TX_STATUS_ID     = 0x790;  // heartbeat, 100 ms
constexpr uint32_t CAN_TX_FAULT_ID      = 0x791;  // fault detail, 500 ms while faulted

constexpr unsigned long CAN_TX_STATUS_INTERVAL_MS = 100UL;
constexpr unsigned long CAN_TX_FAULT_INTERVAL_MS  = 500UL;

/* Fault codes */
constexpr uint8_t FC_NONE           = 0x00;
constexpr uint8_t FC_OVER_CURRENT   = 0x01;
constexpr uint8_t FC_CHARGE_CURRENT = 0x02;
constexpr uint8_t FC_PACK_OV        = 0x03;
constexpr uint8_t FC_PACK_UV        = 0x04;
constexpr uint8_t FC_OVER_TEMP      = 0x05;
constexpr uint8_t FC_CELL_OV        = 0x06;
constexpr uint8_t FC_CELL_UV        = 0x07;
constexpr uint8_t FC_ESTOP          = 0x08;
constexpr uint8_t FC_CAN_TIMEOUT    = 0x09;
constexpr uint8_t FC_CAN_LATCHED    = 0x0A;

/* ================================================================
 *  CAN / MCP2515
 * ================================================================ */
MCP2515 mcp2515(10);

/* ================================================================
 *  LIVE DATA  — stored in BpsData struct from bps_logic.h.
 *  Fault evaluation is deferred until dataReady() returns true
 *  (i.e. at least one of each frame type 0x6B0/6B2/6B3 received).
 * ================================================================ */
BpsData bps;

/* ================================================================
 *  FAULT STATE
 * ================================================================ */
bool liveFault        = false;
FaultTimer faultTimer;           // tracks how long a fault condition has held (2 s latch)
bool contactorsClosed = false;
bool prechargeClosed  = false;
volatile bool estopEdge = false;

EstopDebounce estopDebounce;   // filters mechanical contact bounce on ESTOP_PIN
bool estopLatched = false;     // e-stop trip latch — cleared only by power cycle

void eStopISR() { estopEdge = true; }

/* ================================================================
 *  HELPER FUNCTIONS
 * ================================================================ */

void setContactors(bool closed) {
  contactorsClosed = closed;
  for (uint8_t p : CONTACTOR_PINS)
    digitalWrite(p, closed ? RELAY_CLOSE_LEVEL : RELAY_OPEN_LEVEL);
}

void setPrecharge(bool closed) {
  prechargeClosed = closed;
  digitalWrite(PRECHARGE_PIN, closed ? PRECHARGE_CLOSE_LEVEL : PRECHARGE_OPEN_LEVEL);
}

void lampOff() { digitalWrite(PIN_FAULT_LAMP, LOW); }

/* ================================================================
 *  RELAY DEBUG
 *  When enabled, digitalReads each relay pin every loop and prints a
 *  line whenever the observed level changes, so the physical pin
 *  driving each relay can be confirmed against the pin map above.
 * ================================================================ */
constexpr bool RELAY_DEBUG = true;

/* DEBUG ONLY: require the e-stop input to hold steady for a full second
   before it trips, instead of the normal ESTOP_DEBOUNCE_MS. Set false to
   restore production debounce timing. */
constexpr bool ESTOP_DEBUG_HOLD = true;
constexpr unsigned long ESTOP_DEBUG_HOLD_MS = 5000;

/* DEBUG ONLY: force estopHigh to false regardless of the physical input, so
   other faults/behavior can be exercised on the bench while the e-stop
   wiring is being fixed. Contactors will NOT open on a real e-stop press
   while this is true -- MUST be false before this board drives the car. */
constexpr bool ESTOP_DEBUG_DISABLE = true;

void debugPrintRelayChanges() {
  constexpr uint8_t NUM_CONTACTORS = sizeof(CONTACTOR_PINS) / sizeof(CONTACTOR_PINS[0]);
  static uint8_t lastPrecharge = 0xFF;
  static uint8_t lastContactor[NUM_CONTACTORS];
  static bool    initialized = false;

  if (!initialized) {
    for (uint8_t &lvl : lastContactor) lvl = 0xFF;
    initialized = true;
  }

  uint8_t level = digitalRead(PRECHARGE_PIN);
  if (level != lastPrecharge) {
    Serial.print(F("[RELAY_DEBUG] "));
    Serial.print(millis());
    Serial.print(F(" ms -- precharge relay -- pin "));
    Serial.print(PRECHARGE_PIN);
    Serial.print(F(" -> "));
    Serial.println(level == PRECHARGE_CLOSE_LEVEL ? F("CLOSED") : F("OPEN"));
    lastPrecharge = level;
  }

  for (uint8_t i = 0; i < NUM_CONTACTORS; i++) {
    uint8_t cLevel = digitalRead(CONTACTOR_PINS[i]);
    if (cLevel != lastContactor[i]) {
      Serial.print(F("[RELAY_DEBUG] "));
      Serial.print(millis());
      Serial.print(F(" ms -- contactor relay -- pin "));
      Serial.print(CONTACTOR_PINS[i]);
      Serial.print(F(" -> "));
      Serial.println(cLevel == RELAY_CLOSE_LEVEL ? F("CLOSED") : F("OPEN"));
      lastContactor[i] = cLevel;
    }
  }
}

/* Configure Timer1 for 25 kHz fast PWM on pin 9 (OC1A).
   Must be called once in setup(). Never call analogWrite(9,...) after this. */
void initFanPWM() {
  pinMode(FAN_PWM_PIN, OUTPUT);

  // Reset all Timer1 registers before configuring
  TCCR1A = 0;
  TCCR1B = 0;
  TCCR1C = 0;

  // Fast PWM, TOP = ICR1, non-inverting output on OC1A
  TCCR1A = (1 << COM1A1) | (1 << WGM11);
  TCCR1B = (1 << WGM13)  | (1 << WGM12) | (1 << CS10);  // no prescaler

  ICR1  = TIMER1_TOP;   // TOP -> 25 kHz
  OCR1A = 0;            // start at 0% duty
}

/* Set fan speed from 0-100%. Clamps automatically. */
void setFanSpeed(uint8_t percent) {
  if (percent > 100) percent = 100;
  OCR1A = ((uint32_t)percent * TIMER1_TOP) / 100;
}

/* Derive fan speed from temperature using the configured ramp.
   Returns 0 if temp_hi is still invalid (0xFF = not yet received). */
uint8_t fanSpeedFromTemp(uint8_t t) {
  if (t == 0xFF)         return 0;    // no CAN data yet
  if (t <= FAN_TEMP_MIN) return 0;
  if (t >= FAN_TEMP_MAX) return 100;
  return (uint8_t)((t - FAN_TEMP_MIN) * 100UL / (FAN_TEMP_MAX - FAN_TEMP_MIN));
}

/* Invalidate all live data -- called when CAN watchdog fires so stale
   readings cannot trigger threshold trips after connection is lost.
   Also resets the dataReady flags so fault evaluation re-defers until
   all three frame types are received again after recovery. */
void invalidateLiveData() {
  bps = BpsData();   // reset to default sentinel values and clears got_* flags
}

/* ================================================================
 *  CAN RX  — delegates parsing to processCAN() from bps_logic.h.
 *  Only known Orion heartbeat IDs reset the watchdog timer.
 * ================================================================ */
void readCAN() {
  can_frame f;
  memset(&f, 0, sizeof(f));
  while (mcp2515.readMessage(&f) == MCP2515::ERROR_OK) {
    bool heartbeat = (f.can_id == 0x6B0 ||
                      f.can_id == 0x6B2 ||
                      f.can_id == 0x6B3);
    processCAN(f, bps);
    if (heartbeat) tLastCanRx = millis();
    memset(&f, 0, sizeof(f));   // driver only writes the first can_dlc bytes
  }

  /* Clear RX-overflow flags so the chip keeps receiving */
  uint8_t eflg = mcp2515.getErrorFlags();
  if (eflg & (MCP2515::EFLG_RX0OVR | MCP2515::EFLG_RX1OVR))
    mcp2515.clearRXnOVR();

  /* Recover from bus-off */
  if (eflg & MCP2515::EFLG_TXBO)
    mcp2515.setNormalMode();
}

/* Evaluate CAN watchdog state machine. Call once per loop after readCAN(). */
void updateCanWatchdog() {
  // LATCHED is permanent -- nothing resets it except a power cycle
  if (canState == CanState::LATCHED) return;

  unsigned long now = millis();

  if (canState == CanState::NOMINAL) {
    if (now - tLastCanRx > CAN_TIMEOUT_MS) {
      canState   = CanState::TIMEOUT;
      tTimeoutAt = now;
      invalidateLiveData();
      setContactors(false);
      setFanSpeed(100);
      Serial.println(F("!!! CAN WATCHDOG -- timeout, contactors open, fan @ 100%"));
      Serial.println(F("    30 s recovery window started"));
    }

  } else if (canState == CanState::TIMEOUT) {
    // Did a valid frame arrive? -> recover
    if (tLastCanRx > tTimeoutAt) {
      canState = CanState::NOMINAL;
      setContactors(true);
      Serial.print(F("    CAN recovered after "));
      Serial.print((tLastCanRx - tTimeoutAt) / 1000.0f, 1);
      Serial.println(F(" s -- contactors reclosed"));
      return;
    }
    // Recovery window expired? -> latch
    if (now - tTimeoutAt > CAN_LATCH_MS) {
      canState = CanState::LATCHED;
      Serial.println(F("!!! CAN WATCHDOG -- 30 s elapsed, no recovery"));
      Serial.println(F("    LATCHED -- power cycle required to reset"));
    }
  }
}

/* ================================================================
 *  FAULT CODE HELPER
 *  Returns the highest-priority active fault code, or FC_NONE.
 *  Priority: CAN faults > E-stop > electrical > thermal
 *
 *  liveFault is a time-based latch (see evaluateFault() in bps_logic.h):
 *  once tripped it stays true even if the offending reading recovers back
 *  under threshold. Remember which threshold code actually caused the trip
 *  so it keeps being reported for as long as liveFault is latched, instead
 *  of silently falling back to FC_NONE the moment the reading recovers.
 * ================================================================ */
uint8_t activeFaultCode(bool estopHigh, bool canFault) {
  static uint8_t latchedFault = FC_NONE;

  if (canState == CanState::LATCHED)               return FC_CAN_LATCHED;
  if (canState == CanState::TIMEOUT)               return FC_CAN_TIMEOUT;
  if (estopHigh)                                   return FC_ESTOP;

  uint8_t code = FC_NONE;
  if (bps.cell_hi_ct >= CELL_V_HI_ct)             code = FC_CELL_OV;
  else if (bps.cell_lo_ct != 0xFFFF &&
      bps.cell_lo_ct <= CELL_V_LO_ct)             code = FC_CELL_UV;
  else if (bps.temp_hi != 0xFF &&
      bps.temp_hi   >= TRIP_T_HI_C)               code = FC_OVER_TEMP;
  else if (bps.cur_dA != INT16_MIN &&
      bps.cur_dA  > TRIP_I_HI_dA)                 code = FC_OVER_CURRENT;
  else if (bps.cur_dA != INT16_MIN &&
      bps.cur_dA  < TRIP_I_LO_dA)                 code = FC_CHARGE_CURRENT;
  else if (bps.pack_dV != 0xFFFF &&
      bps.pack_dV > TRIP_V_HI_dV)                 code = FC_PACK_OV;
  else if (bps.pack_dV != 0xFFFF &&
      bps.pack_dV < TRIP_V_LO_dV)                 code = FC_PACK_UV;

  if (code != FC_NONE) {
    latchedFault = code;
    return code;
  }

  if (liveFault) return latchedFault;   // still tripped -- report what caused it

  latchedFault = FC_NONE;
  return FC_NONE;
}

/* ================================================================
 *  CAN TX
 *
 *  0x790  BPS Status  (8 bytes, 100 ms heartbeat)
 *    [0] status flags
 *          bit 0 : liveFault
 *          bit 1 : e-stop active
 *          bit 2 : CAN timeout
 *          bit 3 : CAN latched
 *          bit 4 : contactors closed
 *          bit 5 : precharge closed
 *    [1] fan speed %  (0-100)
 *    [2] fault code   (FC_xxx)
 *    [3-7] reserved 0x00
 *
 *  0x791  BPS Fault Detail  (8 bytes, 500 ms while faulted, immediate on change)
 *    [0] status flags (same as 0x790[0])
 *    [1] fault code
 *    [2-3] fault value, high word (e.g. cell_hi_ct, cur_dA cast to uint16)
 *    [4-5] fault value, low word
 *    [6] CAN watchdog state  (0=NOMINAL 1=TIMEOUT 2=LATCHED)
 *    [7] recovery seconds remaining  (0xFF if not in TIMEOUT)
 * ================================================================ */
void sendCanStatus(bool estopHigh, bool canFault, uint8_t fanPct, uint8_t faultCode) {
  can_frame f;
  f.can_id  = CAN_TX_STATUS_ID;
  f.can_dlc = 8;
  memset(f.data, 0, 8);

  uint8_t flags = 0;
  if (liveFault)                          flags |= (1 << 0);
  if (estopHigh)                          flags |= (1 << 1);
  if (canState == CanState::TIMEOUT)      flags |= (1 << 2);
  if (canState == CanState::LATCHED)      flags |= (1 << 3);
  if (contactorsClosed)                   flags |= (1 << 4);
  if (prechargeClosed)                    flags |= (1 << 5);

  f.data[0] = flags;
  f.data[1] = fanPct;
  f.data[2] = faultCode;
  // [3-7] remain 0x00

  mcp2515.sendMessage(&f);
}

void sendCanFaultDetail(bool estopHigh, bool canFault, uint8_t faultCode) {
  can_frame f;
  f.can_id  = CAN_TX_FAULT_ID;
  f.can_dlc = 8;
  memset(f.data, 0, 8);

  uint8_t flags = 0;
  if (liveFault)                          flags |= (1 << 0);
  if (estopHigh)                          flags |= (1 << 1);
  if (canState == CanState::TIMEOUT)      flags |= (1 << 2);
  if (canState == CanState::LATCHED)      flags |= (1 << 3);
  if (contactorsClosed)                   flags |= (1 << 4);
  if (prechargeClosed)                    flags |= (1 << 5);

  // Encode the most relevant fault value for the active fault code
  uint32_t faultVal = 0;
  switch (faultCode) {
    case FC_CELL_OV:        faultVal = bps.cell_hi_ct;                      break;
    case FC_CELL_UV:        faultVal = bps.cell_lo_ct;                      break;
    case FC_OVER_TEMP:      faultVal = bps.temp_hi;                         break;
    case FC_OVER_CURRENT:   // fall through
    case FC_CHARGE_CURRENT: faultVal = (uint16_t)(int16_t)bps.cur_dA;       break;
    case FC_PACK_OV:        // fall through
    case FC_PACK_UV:        faultVal = bps.pack_dV;                         break;
    default:                faultVal = 0;                                   break;
  }

  uint8_t watchdogSecs = 0xFF;  // 0xFF = not in TIMEOUT
  if (canState == CanState::TIMEOUT) {
    unsigned long elapsed = millis() - tTimeoutAt;
    unsigned long remaining = (elapsed < CAN_LATCH_MS)
                              ? (CAN_LATCH_MS - elapsed) / 1000UL
                              : 0;
    watchdogSecs = (remaining > 254) ? 254 : (uint8_t)remaining;
  }

  f.data[0] = flags;
  f.data[1] = faultCode;
  f.data[2] = (faultVal >> 8) & 0xFF;   // high byte of fault value
  f.data[3] = (faultVal     ) & 0xFF;   // low byte of fault value
  f.data[4] = 0x00;                     // reserved (extend to 32-bit if needed)
  f.data[5] = 0x00;
  f.data[6] = (uint8_t)canState;
  f.data[7] = watchdogSecs;

  mcp2515.sendMessage(&f);
}

/* ================================================================
 *  SETUP
 * ================================================================ */
void setup() {
  Serial.begin(115200);
  while (!Serial) { ; }

  // Relay / lamp pins
  for (uint8_t p : CONTACTOR_PINS) pinMode(p, OUTPUT);
  pinMode(PRECHARGE_PIN, OUTPUT);
  pinMode(PIN_FAULT_LAMP, OUTPUT);

  // Fan PWM -- must happen before any SPI / CAN activity
  initFanPWM();

  // E-stop interrupt
  pinMode(ESTOP_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ESTOP_PIN), eStopISR, RISING);

  // MCP2515 CAN
  SPI.begin();
  mcp2515.reset();
  mcp2515.setBitrate(CAN_125KBPS, MCP_8MHZ);

  // Accept all CAN IDs on both RX buffers
  mcp2515.setFilterMask(MCP2515::MASK0, false, 0x000);
  mcp2515.setFilterMask(MCP2515::MASK1, false, 0x000);
  mcp2515.setFilter(MCP2515::RXF0, false, 0x000);
  mcp2515.setFilter(MCP2515::RXF1, false, 0x000);
  mcp2515.setFilter(MCP2515::RXF2, false, 0x000);
  mcp2515.setFilter(MCP2515::RXF3, false, 0x000);
  mcp2515.setFilter(MCP2515::RXF4, false, 0x000);
  mcp2515.setFilter(MCP2515::RXF5, false, 0x000);

  mcp2515.setNormalMode();

  Serial.println(F("\nRun/Trip + Fan controller -- ready"));
  if (ESTOP_DEBUG_DISABLE) {
    Serial.println(F("!!! ESTOP_DEBUG_DISABLE is true -- e-stop input IGNORED !!!"));
    Serial.println(F("!!! DO NOT DRIVE -- set ESTOP_DEBUG_DISABLE = false before use !!!"));
  }

  // Precharge / startup sequence:
  //   Open the precharge bypass relay so the precharge resistor current-limits
  //   the initial charge into the load caps, then close the main contactor.
  //   After the dwell, reclose the bypass relay to short the resistor out for
  //   full continuous current. Main contactor stays closed throughout.
  setPrecharge(false);   // bypass opens -- resistor engaged
  if (RELAY_DEBUG) debugPrintRelayChanges();
  setContactors(true);   // main contactors close
  if (RELAY_DEBUG) debugPrintRelayChanges();
  delay(PRECHARGE_DELAY_MS);
  setPrecharge(true);    // bypass closes -- resistor shorted, full current; contactors stay closed
  if (RELAY_DEBUG) debugPrintRelayChanges();

  // Drain any CAN frames that arrived during precharge
  readCAN();

  // Seed watchdog timer after precharge so the 2 s window starts from here,
  // not from before the blocking delay above
  tLastCanRx = millis();
}

/* ================================================================
 *  LOOP
 * ================================================================ */
static unsigned long tPrint      = 0;
static unsigned long tLastRx     = 0;
static unsigned long tTxStatus   = 0;
static unsigned long tTxFault    = 0;
static uint8_t       lastFaultCode = FC_NONE;

void loop() {

  /* ---- 1. Read CAN (includes overflow/bus-off recovery) ---- */
  readCAN();

  /* ---- 2. CAN watchdog ---- */
  updateCanWatchdog();
  bool canFault = false;
  //bool canFault = (canState != CanState::NOMINAL);

  /* ---- 3. Evaluate threshold faults via bps_logic.h ----
     evaluateFault() defers until dataReady() (all 3 frame types seen),
     then requires the condition to hold continuously for FAULT_HOLD_MS
     (2 s) before tripping. */
  // Debounce the raw pin so contact bounce can't chatter the fault code /
  // 0x791 fault frames / lamp on a single physical press or release, then latch:
  // a confirmed press trips until power cycle so releasing the button can't drop
  // the reported fault while the contactors stay open.
  bool estopNow  = debounceEstop(estopDebounce, digitalRead(ESTOP_PIN), millis(),
                                  ESTOP_DEBUG_HOLD ? ESTOP_DEBUG_HOLD_MS : ESTOP_DEBOUNCE_MS);
  bool estopHigh = latchEstop(estopLatched, estopNow);
  if (ESTOP_DEBUG_DISABLE) estopHigh = false;
  //liveFault = evaluateFault(bps, liveFault, faultTimer, millis());

  /* ---- 4. Actuate contactors ---- */
  if (liveFault || estopHigh || canFault) {
    setContactors(false);
  }

  if (RELAY_DEBUG) debugPrintRelayChanges();

  /* ---- 5. Fan control ---- */
  // Force 100% on any fault, e-stop, or CAN watchdog event
  uint8_t fanPct;
  if (liveFault || estopHigh || canFault) {
    fanPct = 100;
    setFanSpeed(100);
  } else {
    fanPct = fanSpeedFromTemp(bps.temp_hi);
    setFanSpeed(fanPct);
  }

  /* ---- 6. Fault lamp blink ---- */
  static unsigned long blinkT = 0;
  if (liveFault || estopHigh || canFault) {
    if (millis() - blinkT >= 500) {
      digitalWrite(PIN_FAULT_LAMP, !digitalRead(PIN_FAULT_LAMP));
      blinkT = millis();
    }
  } else {
    lampOff();
  }

  /* ---- 7. CAN TX ---- */
  unsigned long now = millis();
  uint8_t faultCode = activeFaultCode(estopHigh, canFault);

  // 0x790 status heartbeat -- always, every 100 ms
  if (now - tTxStatus >= CAN_TX_STATUS_INTERVAL_MS) {
    sendCanStatus(estopHigh, canFault, fanPct, faultCode);
    tTxStatus = now;
  }

  // 0x791 fault detail -- every 500 ms while any fault active,
  // plus immediately on fault code change (new fault or fault cleared)
  bool anyFault = liveFault || estopHigh || canFault;
  bool faultCodeChanged = (faultCode != lastFaultCode);

  if (faultCodeChanged) {
    sendCanFaultDetail(estopHigh, canFault, faultCode);
    tTxFault      = now;
    lastFaultCode = faultCode;
  } else if (anyFault && (now - tTxFault >= CAN_TX_FAULT_INTERVAL_MS)) {
    sendCanFaultDetail(estopHigh, canFault, faultCode);
    tTxFault = now;
  }

  /* ---- 8. Serial status print every 1000 ms ---- */
  if (now - tPrint >= 1000) {
    // Print valid data if available, dashes if not
    if (bps.cur_dA != INT16_MIN) {
      float I   = bps.cur_dA / 10.0f;
      float Vhi = cellVolts(bps.cell_hi_ct);
      float Vlo = cellVolts(bps.cell_lo_ct);
      float age = (now - tLastRx) / 1000.0f;
      tLastRx   = now;

      Serial.print(F("I "));         Serial.print(I, 1);
      Serial.print(F(" A | V-hi "));  Serial.print(Vhi, 4);
      Serial.print(F(" V (cell "));   Serial.print(bps.cell_hi_id);
      Serial.print(F(") | V-lo "));   Serial.print(Vlo, 4);
      Serial.print(F(" V (cell "));   Serial.print(bps.cell_lo_id);
      Serial.print(F(") | SOC "));    Serial.print(bps.soc_pct);
      Serial.print(F("% | T-hi "));   Serial.print(bps.temp_hi  != 0xFF ? bps.temp_hi  : 0);
      Serial.print(F(" C | T-avg ")); Serial.print(bps.temp_avg != 0xFF ? bps.temp_avg : 0);
      Serial.print(F(" C | Fan "));   Serial.print(fanPct);
      Serial.print(F("% | CAN:"));
      Serial.print(canState == CanState::NOMINAL ? F("OK") :
                   canState == CanState::TIMEOUT ? F("TIMEOUT") :
                                                   F("LATCHED"));
      Serial.print(F(" | "));         Serial.print(age, 1);
      Serial.println(F(" s"));
    } else {
      Serial.print(F("-- awaiting CAN data -- CAN:"));
      Serial.print(canState == CanState::NOMINAL ? F("OK") :
                   canState == CanState::TIMEOUT ? F("TIMEOUT") :
                                                   F("LATCHED"));
      Serial.print(F(" | Fan ")); Serial.print(fanPct);
      Serial.println(F("%"));
    }

    if (liveFault || estopHigh || canFault) {
      Serial.print(F("!!! TRIP -- contactors open, fan @ 100%  ("));
      switch (faultCode) {
        case FC_CAN_LATCHED:    Serial.print(F("CAN latched -- power cycle required")); break;
        case FC_CAN_TIMEOUT:    Serial.print(F("CAN timeout"));                         break;
        case FC_ESTOP:          Serial.print(F("E-stop"));                              break;
        case FC_CELL_OV:        Serial.print(F("cell over-volt "));
                                Serial.print(cellVolts(bps.cell_hi_ct), 4);
                                Serial.print(F(" V (cell "));
                                Serial.print(bps.cell_hi_id);
                                Serial.print(F(")"));                                   break;
        case FC_CELL_UV:        Serial.print(F("cell under-volt "));
                                Serial.print(cellVolts(bps.cell_lo_ct), 4);
                                Serial.print(F(" V (cell "));
                                Serial.print(bps.cell_lo_id);
                                Serial.print(F(")"));                                   break;
        case FC_OVER_TEMP:      Serial.print(F("over-temp "));
                                Serial.print(bps.temp_hi);
                                Serial.print(F(" C"));                                  break;
        case FC_OVER_CURRENT:   Serial.print(F("over-current "));
                                Serial.print(bps.cur_dA / 10.0f, 1);
                                Serial.print(F(" A"));                                  break;
        case FC_CHARGE_CURRENT: Serial.print(F("charge over-current "));
                                Serial.print(bps.cur_dA / 10.0f, 1);
                                Serial.print(F(" A"));                                  break;
        case FC_PACK_OV:        Serial.print(F("pack over-volt "));
                                Serial.print(bps.pack_dV / 10.0f, 1);
                                Serial.print(F(" V"));                                  break;
        case FC_PACK_UV:        Serial.print(F("pack under-volt "));
                                Serial.print(bps.pack_dV / 10.0f, 1);
                                Serial.print(F(" V"));                                  break;
        default:                Serial.print(F("unknown"));                             break;
      }
      Serial.println(F(")"));
    }

    tPrint = now;
  }
}

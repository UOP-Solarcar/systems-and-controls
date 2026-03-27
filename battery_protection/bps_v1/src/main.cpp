/********************************************************************
 *  Orion BMS run / trip controller
 *  D4 & D5 : main contactor relays
 *  D6      : fault lamp (blinks while fault present)
 ********************************************************************/
#include <SPI.h>
#include <mcp2515.h>
#include "bps_logic.h"

bool liveFault = false;
bool lastFault = false;

/* ---------- pin map ---------- */
#define RELAY_CLOSE_LEVEL  HIGH                 // change to LOW if active-LOW boards
constexpr uint8_t PRECHARGE_PIN = 4;
constexpr uint8_t CONTACTOR_PINS[] = {5};
constexpr uint8_t PIN_FAULT_LAMP   = 6;
constexpr uint8_t ESTOP_PIN        = 3;
constexpr bool    ESTOP_ACTIVE     = LOW;       // LOW = pressed (active-low with pullup)

constexpr uint8_t RELAY_OPEN_LEVEL =
        (RELAY_CLOSE_LEVEL == HIGH ? LOW : HIGH);

/* ---------- CAN ---------- */
MCP2515 mcp2515(10);

/* ---------- live data ---------- */
BpsData bps;

/* ---------- E-stop debounce ---------- */
constexpr unsigned long ESTOP_DEBOUNCE_MS = 50;
bool estopConfirmed = false;
unsigned long estopStableStart  = 0;
bool          estopLastRaw      = false;

/* ---------- helpers ---------- */
void setContactors(bool closed){
  for (uint8_t p : CONTACTOR_PINS)
      digitalWrite(p, closed ? RELAY_CLOSE_LEVEL : RELAY_OPEN_LEVEL);
}

void setPrecharge(bool closed){
  digitalWrite(PRECHARGE_PIN, closed ? RELAY_CLOSE_LEVEL : RELAY_OPEN_LEVEL);
}

void lampOff(){ digitalWrite(PIN_FAULT_LAMP, LOW); }

void setup(){
  Serial.begin(115200); while(!Serial){;}

  for (uint8_t p : CONTACTOR_PINS) pinMode(p, OUTPUT);
  pinMode(PRECHARGE_PIN, OUTPUT);
  pinMode(PIN_FAULT_LAMP, OUTPUT);
  setContactors(true);                     // close at boot
  lampOff();

  pinMode(ESTOP_PIN, INPUT_PULLUP);

  // Init MCP2515 after estop interrupt is attached to prevent
  // fault state from triggering before CAN frames are read
  SPI.begin();
  mcp2515.reset();
  mcp2515.setBitrate(CAN_500KBPS, MCP_8MHZ);

  // Accept all CAN IDs
  mcp2515.setFilterMask(MCP2515::MASK0, false, 0x000);
  mcp2515.setFilterMask(MCP2515::MASK1, false, 0x000);
  mcp2515.setFilter(MCP2515::RXF0, false, 0x000);
  mcp2515.setFilter(MCP2515::RXF1, false, 0x000);
  mcp2515.setFilter(MCP2515::RXF2, false, 0x000);
  mcp2515.setFilter(MCP2515::RXF3, false, 0x000);
  mcp2515.setFilter(MCP2515::RXF4, false, 0x000);
  mcp2515.setFilter(MCP2515::RXF5, false, 0x000);

  Serial.println(F("\nRun/Trip controller — stateless fault logic"));

  mcp2515.setNormalMode();
  setContactors(false);
  setPrecharge(false);
  delay(1000);
  setPrecharge(true);
  delay(5000);
  setPrecharge(false);
  can_frame f;
  while (mcp2515.readMessage(&f) == MCP2515::ERROR_OK) {
    processCAN(f, bps);
  }
  delay(500);
  setContactors(true);
}

void loop(){

  /* ---------- read CAN ---------- */
  can_frame f;
  while (mcp2515.readMessage(&f) == MCP2515::ERROR_OK) {
    processCAN(f, bps);
  }

  /* clear RX-overflow & bus-error flags so the chip keeps receiving */
  uint8_t eflg = mcp2515.getErrorFlags();
  if (eflg & (MCP2515::EFLG_RX0OVR | MCP2515::EFLG_RX1OVR))
    mcp2515.clearRXnOVR();
  if (eflg & MCP2515::EFLG_TXBO)
    mcp2515.setNormalMode();           // recover from bus-off

  /* ---------- debounce E-stop ---------- */
  bool rawEstop = (digitalRead(ESTOP_PIN) == ESTOP_ACTIVE);
  if (rawEstop != estopLastRaw) {
      estopStableStart = millis();
      estopLastRaw = rawEstop;
  } else if (millis() - estopStableStart >= ESTOP_DEBOUNCE_MS) {
      estopConfirmed = rawEstop;
  }
  bool estopPressed = estopConfirmed;

  /* ---------- evaluate live faults ---------- */
  liveFault = evaluateFault(bps, liveFault, lastFault);

  /* ---------- actuation ---------- */
  if(liveFault || estopPressed){
    setContactors(false);
  }

  static unsigned long blinkT = 0;
  if (liveFault || estopPressed) {
      if (millis() - blinkT >= 500) {
          digitalWrite(PIN_FAULT_LAMP, !digitalRead(PIN_FAULT_LAMP));
          blinkT = millis();
      }
  } else {
      lampOff();
  }

  /* ---------- status print (1 s) ---------- */
  static unsigned long tPrint = 0, tLastRx = 0;
  unsigned long now = millis();

  if (now - tPrint >= 1000 && bps.cur_dA != INT16_MIN) {
      float I   = bps.cur_dA / 10.0f;
      float Vhi = cellVolts(bps.cell_hi_ct);
      float Vlo = cellVolts(bps.cell_lo_ct);
      float age = (now - tLastRx) / 1000.0f;
      tLastRx   = now;

      Serial.print("I ");  Serial.print(I,1);
      Serial.print(" A | V-hi "); Serial.print(Vhi,4);
      Serial.print(" V | V-lo "); Serial.print(Vlo,4);
      Serial.print(" V | SOC ");  Serial.print(bps.soc_pct * 0.5f, 1);
      Serial.print("% | T-hi ");  Serial.print(bps.temp_hi);
      Serial.print(" °C | T-avg ");Serial.print(bps.temp_avg);
      Serial.print(" °C | ");      Serial.print(age,1);
      Serial.println(" s");

      if (liveFault || estopPressed) {
        Serial.print(F("!!! FAULT: "));
        if (estopPressed) { Serial.print("E-stop"); }
        else if (bps.cell_hi_ct >= CELL_V_HI_ct)   { Serial.print("cell over-volt ");  Serial.print(Vhi,4); Serial.print(" V"); }
        else if (bps.cell_lo_ct <= CELL_V_LO_ct)   { Serial.print("cell under-volt "); Serial.print(Vlo,4); Serial.print(" V"); }
        else if (bps.temp_hi >= TRIP_T_HI_C)       { Serial.print("over-temp ");       Serial.print(bps.temp_hi); Serial.print(" C"); }
        else if (bps.cur_dA > TRIP_I_HI_dA)        { Serial.print("over-current ");    Serial.print(I,1);    Serial.print(" A"); }
        else if (bps.cur_dA < TRIP_I_LO_dA)        { Serial.print("charge current ");  Serial.print(I,1);    Serial.print(" A"); }
        else if (bps.pack_dV > TRIP_V_HI_dV)       { Serial.print("pack over-volt ");  Serial.print(bps.pack_dV/10.0f,1); Serial.print(" V"); }
        else if (bps.pack_dV < TRIP_V_LO_dV)       { Serial.print("pack under-volt "); Serial.print(bps.pack_dV/10.0f,1); Serial.print(" V"); }
        Serial.println();
      }
      tPrint = now;
  }
}

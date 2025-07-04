/********************************************************************
 *  Orion BMS run / trip controller
 *  D4 & D5 : main contactor relays
 *  D6      : fault lamp (blinks while fault present)
 ********************************************************************/
#include <SPI.h>
#include <mcp2515.h>

bool liveFault = false;
bool lastFault = false;

/* ---------- pin map ---------- */
#define RELAY_CLOSE_LEVEL  HIGH                 // change to LOW if active-LOW boards
constexpr uint8_t PRECHARGE_PIN = 4;
constexpr uint8_t CONTACTOR_PINS[] = {5};
constexpr uint8_t PIN_FAULT_LAMP   = 6;
constexpr uint8_t ESTOP_PIN        = 3;


constexpr uint8_t RELAY_OPEN_LEVEL =
        (RELAY_CLOSE_LEVEL == HIGH ? LOW : HIGH);

/* ---------- trip thresholds ---------- */
const int16_t  TRIP_I_HI_dA  = 1000;      // +100 A  (0.1 A units)
const int16_t  TRIP_I_LO_dA  = -425;      //  −42.5 A
const uint16_t TRIP_V_HI_dV  =  950;      //  95.0 V (0.1 V units)
const uint16_t TRIP_V_LO_dV  =  780;      //  78.0 V
const uint8_t  TRIP_T_HI_C   =   45;      //  45 °C  (change back to 60 if that was a typo)
const uint16_t CELL_V_HI_ct  = 42000;     // 4.2000 V (0.0001 V/ct)
const uint16_t CELL_V_LO_ct  = 25000;     // 2.5000 V

/* ---------- CAN helpers ---------- */
MCP2515 mcp2515(10);
inline int16_t  be16s(const uint8_t* p){ return  (int16_t)((p[0]<<8)|p[1]); }
inline uint16_t be16u(const uint8_t* p){ return (uint16_t)((p[0]<<8)|p[1]); }
inline float cellVolts(uint16_t ct){ return ct * 0.0001f; }   // 100 µV / ct

/* ---------- live data ---------- */
volatile int16_t  cur_dA     = INT16_MIN;
volatile uint16_t pack_dV    = 0;
volatile uint8_t  soc_pct    = 0xFF;
volatile uint8_t  temp_hi    = 0xFF;
volatile uint8_t  temp_avg   = 0xFF;
volatile uint16_t cell_hi_ct = 0;
volatile uint16_t cell_lo_ct = 0xFFFF;
volatile bool     estopEdge  = false;      // set by ISR on rising edge

void eStopISR(){ estopEdge = true; }

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
  attachInterrupt(digitalPinToInterrupt(ESTOP_PIN), eStopISR, RISING);

  SPI.begin();
  mcp2515.reset();
  mcp2515.setBitrate(CAN_500KBPS, MCP_8MHZ);
  mcp2515.setNormalMode();

  Serial.println(F("\nRun/Trip controller — stateless fault logic"));
  setContactors(false);
  setPrecharge(false);
  delay(1000);
  setPrecharge(true);
  delay(5000);
  setPrecharge(false);
  can_frame f;
  while (mcp2515.readMessage(&f) == MCP2515::ERROR_OK) {
    switch (f.can_id) {
      case 0x6B0: cur_dA   = be16s(&f.data[0]);
                  pack_dV  = be16u(&f.data[2]);
                  soc_pct  = f.data[4];
                  break;
      case 0x6B2: cell_hi_ct = be16u(&f.data[0]);
                  cell_lo_ct = be16u(&f.data[3]);
                  break;
      case 0x6B3: temp_hi  = f.data[0];
                  temp_avg = f.data[4];
                  break;
      default: break;
    }
  }
  delay(500);
  setContactors(true);
}

void loop(){

  /* ---------- read CAN ---------- */
  can_frame f;
  while (mcp2515.readMessage(&f) == MCP2515::ERROR_OK) {
    switch (f.can_id) {
      case 0x6B0: cur_dA   = be16s(&f.data[0]);
                  pack_dV  = be16u(&f.data[2]);
                  soc_pct  = f.data[4];
                  break;
      case 0x6B2: cell_hi_ct = be16u(&f.data[0]);
                  cell_lo_ct = be16u(&f.data[3]);
                  break;
      case 0x6B3: temp_hi  = f.data[0];
                  temp_avg = f.data[4];
                  break;
      default: break;
    }
  }

  /* ---------- evaluate live faults ---------- */
  bool estopHigh = digitalRead(ESTOP_PIN);       // HIGH = pressed
  if (!liveFault){
    liveFault =
        (temp_hi >= TRIP_T_HI_C) ||
        (cur_dA   > TRIP_I_HI_dA) || (cur_dA < TRIP_I_LO_dA) ||
        (pack_dV  > TRIP_V_HI_dV) || (pack_dV < TRIP_V_LO_dV) ||
        (cell_hi_ct >= CELL_V_HI_ct) || (cell_lo_ct <= CELL_V_LO_ct);
    if (!liveFault && lastFault) {
      lastFault = false;
    }
    if (liveFault && lastFault){
      liveFault = true;
    }
    if (liveFault && !lastFault){
      lastFault = true;
      liveFault = false;
    }
  }

  /* ---------- actuation ---------- */
  if(liveFault || estopHigh){
    setContactors(false);
  }


  static unsigned long blinkT = 0;
  if (liveFault || estopHigh) {
      if (millis() - blinkT >= 500) {
          digitalWrite(PIN_FAULT_LAMP, !digitalRead(PIN_FAULT_LAMP));
          blinkT = millis();
      }
  } else {
      lampOff();
  }

  /* ---------- status print (200 ms) ---------- */
  static unsigned long tPrint = 0, tLastRx = 0;
  unsigned long now = millis();
  if (now - tPrint >= 200 && cur_dA != INT16_MIN) {
      float I   = cur_dA / 10.0f;
      float Vhi = cellVolts(cell_hi_ct);
      float Vlo = cellVolts(cell_lo_ct);
      float age = (now - tLastRx) / 1000.0f;
      tLastRx   = now;

      Serial.print("I ");  Serial.print(I,1);
      Serial.print(" A | V-hi "); Serial.print(Vhi,4);
      Serial.print(" V | V-lo "); Serial.print(Vlo,4);
      Serial.print(" V | SOC ");  Serial.print(soc_pct);
      Serial.print("% | T-hi ");   Serial.print(temp_hi);
      Serial.print(" °C | T-avg ");Serial.print(temp_avg);
      Serial.print(" °C | ");      Serial.print(age,1);
      Serial.println(" s");

      if (liveFault || estopHigh) {
        Serial.print(F("!!! DC THE CONTACTOR  ("));
        if (estopHigh) { Serial.print("E-stop"); }
        else if (cell_hi_ct >= CELL_V_HI_ct)   { Serial.print("cell over-volt ");  Serial.print(Vhi,4); Serial.print(" V"); }
        else if (cell_lo_ct <= CELL_V_LO_ct)   { Serial.print("cell under-volt "); Serial.print(Vlo,4); Serial.print(" V"); }
        else if (temp_hi >= TRIP_T_HI_C)       { Serial.print("over-temp ");       Serial.print(temp_hi); Serial.print(" °C"); }
        else if (cur_dA > TRIP_I_HI_dA)        { Serial.print("over-current ");    Serial.print(I,1);    Serial.print(" A"); }
        else if (cur_dA < TRIP_I_LO_dA)        { Serial.print("charge current ");  Serial.print(I,1);    Serial.print(" A"); }
        else if (pack_dV > TRIP_V_HI_dV)       { Serial.print("pack over-volt ");  Serial.print(pack_dV/10.0f,1); Serial.print(" V"); }
        else if (pack_dV < TRIP_V_LO_dV)       { Serial.print("pack under-volt "); Serial.print(pack_dV/10.0f,1); Serial.print(" V"); }
        Serial.println(")");
      }
      tPrint = now;
  }
}

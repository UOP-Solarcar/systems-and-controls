#pragma once
/********************************************************************
 *  BPS core logic — pure functions, no hardware dependencies.
 *  Included by both src/main.cpp (Arduino) and native unit tests.
 ********************************************************************/
#include <stdint.h>
#include <limits.h>

/* ---------- trip thresholds ---------- */
constexpr int16_t  TRIP_I_HI_dA  = 1000;      // +100 A  (0.1 A units)
constexpr int16_t  TRIP_I_LO_dA  = -425;      //  -42.5 A
constexpr uint16_t TRIP_V_HI_dV  =  950;      //  95.0 V (0.1 V units)
constexpr uint16_t TRIP_V_LO_dV  =  780;      //  78.0 V
constexpr uint8_t  TRIP_T_HI_C   =   45;      //  45 C
constexpr uint16_t CELL_V_HI_ct  = 42000;     // 4.2000 V (0.0001 V/ct)
constexpr uint16_t CELL_V_LO_ct  = 25000;     // 2.5000 V

/* ---------- byte helpers ---------- */
inline int16_t  be16s(const uint8_t* p){ return  (int16_t)((p[0]<<8)|p[1]); }
inline uint16_t be16u(const uint8_t* p){ return (uint16_t)((p[0]<<8)|p[1]); }
inline float cellVolts(uint16_t ct){ return ct * 0.0001f; }

/* ---------- telemetry struct ---------- */
struct BpsData {
    int16_t  cur_dA     = INT16_MIN;
    uint16_t pack_dV    = 0;
    uint8_t  soc_pct    = 0xFF;
    uint8_t  temp_hi    = 0xFF;
    uint8_t  temp_avg   = 0xFF;
    uint16_t cell_hi_ct = 0;
    uint16_t cell_lo_ct = 0xFFFF;
};

/* ---------- CAN frame stub (used when mcp2515 library is not included) ---------- */
#ifndef CAN_H_
struct can_frame {
    uint32_t can_id;
    uint8_t  can_dlc;
    uint8_t  data[8];
};
#endif

/* ---------- CAN processing ---------- */
inline void processCAN(can_frame &f, BpsData &d){
  switch (f.can_id) {
    case 0x6B0: d.cur_dA   = be16s(&f.data[0]);
                d.pack_dV  = be16u(&f.data[2]);
                d.soc_pct  = f.data[4];
                break;
    case 0x6B2: d.cell_hi_ct = be16u(&f.data[0]);
                d.cell_lo_ct = be16u(&f.data[3]);
                break;
    case 0x6B3: d.temp_hi  = f.data[0];
                d.temp_avg = f.data[4];
                break;
    default: break;
  }
}

/* ---------- fault evaluation ---------- */
inline bool checkFaultCondition(const BpsData &d){
    return (d.temp_hi >= TRIP_T_HI_C) ||
           (d.cur_dA   > TRIP_I_HI_dA) || (d.cur_dA < TRIP_I_LO_dA) ||
           (d.pack_dV  > TRIP_V_HI_dV) || (d.pack_dV < TRIP_V_LO_dV) ||
           (d.cell_hi_ct >= CELL_V_HI_ct) || (d.cell_lo_ct <= CELL_V_LO_ct);
}

/* Two-strike latch: first fault sets lastFault, second consecutive fault latches.
   Once latched (liveFault == true), stays latched until reset. */
inline bool evaluateFault(const BpsData &d, bool liveFault, bool &lastFault){
    if (liveFault) return true;  // already latched

    bool faultNow = checkFaultCondition(d);
    if (!faultNow && lastFault) {
        lastFault = false;
    }
    if (faultNow && lastFault) {
        return true;
    }
    if (faultNow && !lastFault) {
        lastFault = true;
        return false;
    }
    return false;
}

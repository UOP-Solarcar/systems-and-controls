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
    uint8_t  cell_hi_id = 0xFF;   // module/cell ID holding the highest voltage
    uint8_t  cell_lo_id = 0xFF;   // module/cell ID holding the lowest voltage

    /* Set once a 0x6B2 frame reports cell_hi/cell_lo past the OV/UV threshold;
       cleared the moment a frame reports back in-range. Used by processCAN()
       to require two consecutive over/under-threshold frames before trusting
       the reading -- see the 0x6B2 case below. */
    bool cell_hi_flagged = false;
    bool cell_lo_flagged = false;

    /* Track which CAN frame types have been received at least once.
       Fault evaluation is deferred until all required frames arrive. */
    bool got_6B0 = false;   // current, pack voltage, SOC
    bool got_6B2 = false;   // cell voltages
    bool got_6B3 = false;   // temperatures
    bool dataReady() const { return got_6B0 && got_6B2 && got_6B3; }
};

/* ---------- CAN frame stub (used when mcp2515 library is not included) ---------- */
#ifndef CAN_H_
struct can_frame {
    uint32_t can_id;
    uint8_t  can_dlc;
    uint8_t  data[8];
};
constexpr uint32_t CAN_RTR_FLAG = 0x40000000UL;
#endif

/* ---------- CAN processing ---------- */
inline void processCAN(can_frame &f, BpsData &d){
  /* The mcp2515 driver only overwrites the first can_dlc bytes of f.data on
     receive -- it never clears the rest of the buffer. Since readCAN() reuses
     one can_frame across every message in a poll, a short or RTR frame (no
     data on the wire) landing on one of our IDs would otherwise be parsed
     against stale bytes left over from whatever frame was read before it. */
  if (f.can_dlc != 8 || (f.can_id & CAN_RTR_FLAG)) return;

  switch (f.can_id) {
    case 0x6B0: d.cur_dA   = be16s(&f.data[0]);
                d.pack_dV  = be16u(&f.data[2]);
                d.soc_pct  = f.data[4];
                d.got_6B0  = true;
                break;
    case 0x6B2: {
      uint16_t hi_ct = be16u(&f.data[0]);
      uint8_t  hi_id = f.data[2];
      uint16_t lo_ct = be16u(&f.data[3]);
      uint8_t  lo_id = f.data[5];

      /* A single frame reporting a cell past the OV/UV threshold could be a
         corrupted CAN/SPI transfer rather than a real cell excursion --
         require the very next 0x6B2 frame to also be past threshold before
         committing it, so one bad frame can't look like a sustained trip.
         In-range readings always commit immediately; there's no safety cost
         to trusting those right away. */
      if (hi_ct >= CELL_V_HI_ct) {
        if (d.cell_hi_flagged) { d.cell_hi_ct = hi_ct; d.cell_hi_id = hi_id; }
        d.cell_hi_flagged = true;
      } else {
        d.cell_hi_ct      = hi_ct;
        d.cell_hi_id      = hi_id;
        d.cell_hi_flagged = false;
      }

      if (lo_ct <= CELL_V_LO_ct) {
        if (d.cell_lo_flagged) { d.cell_lo_ct = lo_ct; d.cell_lo_id = lo_id; }
        d.cell_lo_flagged = true;
      } else {
        d.cell_lo_ct      = lo_ct;
        d.cell_lo_id      = lo_id;
        d.cell_lo_flagged = false;
      }

      d.got_6B2 = true;
      break;
    }
    case 0x6B3: d.temp_hi  = f.data[0];
                d.temp_avg = f.data[4];
                d.got_6B3  = true;
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

/* Fault hold time: a threshold condition must hold continuously for this
   long before it latches, so a single noisy/transient CAN reading can't
   trip the contactors. */
constexpr unsigned long FAULT_HOLD_MS = 2000;

struct FaultTimer {
    bool          candidate = false;   // faultNow currently being timed
    unsigned long since     = 0;       // millis() when candidate first went true
};

/* Time-based latch: checkFaultCondition() must return true continuously for
   FAULT_HOLD_MS before evaluateFault() reports a trip. Any recovery below
   threshold resets the timer. Once latched (liveFault == true), stays
   latched until reset. */
inline bool evaluateFault(const BpsData &d, bool liveFault, FaultTimer &timer, unsigned long now){
    if (liveFault) return true;  // already latched

    /* Don't evaluate until we have received at least one of each CAN frame */
    if (!d.dataReady()) return false;

    bool faultNow = checkFaultCondition(d);
    if (!faultNow) {
        timer.candidate = false;
        return false;
    }
    if (!timer.candidate) {
        timer.candidate = true;
        timer.since     = now;
        return false;
    }
    return (now - timer.since) >= FAULT_HOLD_MS;
}

/* ---------- E-stop debounce ----------
   The e-stop input (D3, INPUT_PULLUP, active-HIGH when pressed) is a mechanical
   contact and bounces for a few ms on both press and release. loop() reads the
   pin with a bare digitalRead() every iteration, so during the bounce window the
   reported state flips HIGH<->LOW several times. Each flip re-derives the fault
   code (FC_ESTOP <-> FC_NONE) and fires an immediate 0x791 fault-detail frame,
   turning one physical press into a burst of spurious CAN traffic and lamp/fan
   chatter. Require the raw level to hold steady for ESTOP_DEBOUNCE_MS before the
   debounced state is allowed to change. */
constexpr unsigned long ESTOP_DEBOUNCE_MS = 20;   // contacts settle well under 20 ms

struct EstopDebounce {
    bool          stable    = false;   // debounced state reported to the caller
    bool          candidate = false;   // raw level currently being timed
    unsigned long since     = 0;       // millis() when `candidate` was first seen
};

/* Feed the raw pin level and the current millis() every loop. The returned
   (debounced) state only flips once `raw` has held its new value continuously
   for debounce_ms (defaults to ESTOP_DEBOUNCE_MS); bounce pulses shorter than
   that are rejected. Callers may pass a longer hold time (e.g. a debug build
   requiring a full second) without changing the production default. */
inline bool debounceEstop(EstopDebounce &s, bool raw, unsigned long now,
                           unsigned long debounce_ms = ESTOP_DEBOUNCE_MS){
    if (raw != s.candidate) {
        // Level changed — (re)start the settle timer on the new candidate.
        s.candidate = raw;
        s.since     = now;
    } else if (raw != s.stable && (now - s.since) >= debounce_ms) {
        // Candidate has held steady long enough — accept it.
        s.stable = raw;
    }
    return s.stable;
}

/* E-stop trip latch: like the time-based fault latch and the CAN LATCHED state,
   a confirmed (debounced) e-stop press is a safety trip that must persist until
   the board is power-cycled — releasing the button does NOT clear it, otherwise
   the reported fault would drop while the contactors stay open. Pass the
   debounced level in; the latch stays set once it has ever seen a press. */
inline bool latchEstop(bool &estopLatched, bool debouncedPressed){
    if (debouncedPressed) estopLatched = true;
    return estopLatched;
}

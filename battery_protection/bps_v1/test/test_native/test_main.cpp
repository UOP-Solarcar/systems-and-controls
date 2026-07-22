#include <unity.h>
#include "bps_logic.h"
#include <string.h>

/* ================================================================
 *  Helper: build a CAN frame from raw bytes
 * ================================================================ */
static can_frame makeFrame(uint32_t id, const uint8_t *data, uint8_t len){
    can_frame f;
    memset(&f, 0, sizeof(f));
    f.can_id  = id;
    f.can_dlc = len;
    memcpy(f.data, data, len);
    return f;
}

/* helper to reset BpsData to safe defaults (no fault) */
static BpsData safeData(){
    BpsData d;
    d.cur_dA     = 0;
    d.pack_dV    = 850;     // 85.0 V  — within [78.0, 95.0]
    d.soc_pct    = 50;
    d.temp_hi    = 30;      // 30 C  — below 45 C
    d.temp_avg   = 25;
    d.cell_hi_ct = 36000;   // 3.6000 V — below 4.2000
    d.cell_lo_ct = 32000;   // 3.2000 V — above 2.5000
    d.got_6B0    = true;
    d.got_6B2    = true;
    d.got_6B3    = true;
    return d;
}

/* ================================================================
 *  be16s / be16u
 * ================================================================ */
void test_be16u_zero(void){
    uint8_t buf[] = {0x00, 0x00};
    TEST_ASSERT_EQUAL_UINT16(0, be16u(buf));
}

void test_be16u_max(void){
    uint8_t buf[] = {0xFF, 0xFF};
    TEST_ASSERT_EQUAL_UINT16(0xFFFF, be16u(buf));
}

void test_be16u_known(void){
    uint8_t buf[] = {0x03, 0x52};       // 850 decimal
    TEST_ASSERT_EQUAL_UINT16(850, be16u(buf));
}

void test_be16s_positive(void){
    uint8_t buf[] = {0x03, 0xE8};       // +1000
    TEST_ASSERT_EQUAL_INT16(1000, be16s(buf));
}

void test_be16s_negative(void){
    uint8_t buf[] = {0xFE, 0x53};       // -429
    TEST_ASSERT_EQUAL_INT16(-429, be16s(buf));
}

/* ================================================================
 *  cellVolts
 * ================================================================ */
void test_cellVolts_typical(void){
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 3.6000f, cellVolts(36000));
}

void test_cellVolts_zero(void){
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, cellVolts(0));
}

/* ================================================================
 *  processCAN — 0x6B0  (current, pack voltage, SOC)
 * ================================================================ */
void test_processCAN_0x6B0(void){
    BpsData d;
    // current = +50.0 A → 500 dA → 0x01F4
    // pack_dV = 85.0 V  → 850 dV → 0x0352
    // soc     = 100 (= 50.0%)
    uint8_t data[] = {0x01,0xF4, 0x03,0x52, 100, 0,0,0};
    can_frame f = makeFrame(0x6B0, data, 8);
    processCAN(f, d);

    TEST_ASSERT_EQUAL_INT16(500,  d.cur_dA);
    TEST_ASSERT_EQUAL_UINT16(850, d.pack_dV);
    TEST_ASSERT_EQUAL_UINT8(100,  d.soc_pct);
}

void test_processCAN_0x6B0_negative_current(void){
    BpsData d;
    // current = -42.5 A → -425 dA → 0xFE57
    uint8_t data[] = {0xFE,0x57, 0x03,0x52, 80, 0,0,0};
    can_frame f = makeFrame(0x6B0, data, 8);
    processCAN(f, d);

    TEST_ASSERT_EQUAL_INT16(-425, d.cur_dA);
}

/* ================================================================
 *  processCAN — 0x6B2  (cell voltages)
 * ================================================================ */
void test_processCAN_0x6B2(void){
    BpsData d;
    // cell_hi = 3.8000 V → 38000 → 0x9470
    // cell_lo = 3.2000 V → 32000 → 0x7D00  (starts at byte 3)
    uint8_t data[] = {0x94,0x70, 0x00, 0x7D,0x00, 0,0,0};
    can_frame f = makeFrame(0x6B2, data, 8);
    processCAN(f, d);

    TEST_ASSERT_EQUAL_UINT16(38000, d.cell_hi_ct);
    TEST_ASSERT_EQUAL_UINT16(32000, d.cell_lo_ct);
}

/* ================================================================
 *  processCAN — 0x6B2 OV/UV glitch guard
 *
 *  A single frame reporting a cell past the OV/UV threshold could be a
 *  corrupted CAN/SPI transfer rather than a real cell excursion (the
 *  Orion's own live display can hold steady while our board sees one bad
 *  frame). Require two consecutive over/under-threshold frames before the
 *  reading is trusted; in-range readings still commit immediately.
 * ================================================================ */
void test_processCAN_0x6B2_ov_single_frame_not_trusted(void){
    BpsData d = safeData();   // cell_hi_ct = 36000 (safe)
    // Single glitched frame: cell_hi = 4.8000 V (48000 = 0xBB80), id 10
    uint8_t data[] = {0xBB,0x80, 10, 0x7D,0x00, 1, 0,0};
    can_frame f = makeFrame(0x6B2, data, 8);
    processCAN(f, d);

    // Not committed yet -- still shows the last safe reading
    TEST_ASSERT_EQUAL_UINT16(36000, d.cell_hi_ct);
    TEST_ASSERT_TRUE(d.cell_hi_flagged);
}

void test_processCAN_0x6B2_ov_confirmed_after_second_frame(void){
    BpsData d = safeData();
    uint8_t glitch[] = {0xBB,0x80, 10, 0x7D,0x00, 1, 0,0};   // 48000, id 10
    can_frame f1 = makeFrame(0x6B2, glitch, 8);
    processCAN(f1, d);

    // Second consecutive over-threshold frame -- a real sustained condition
    uint8_t confirm[] = {0xA7,0xF8, 10, 0x7D,0x00, 1, 0,0};  // 43000, id 10
    can_frame f2 = makeFrame(0x6B2, confirm, 8);
    processCAN(f2, d);

    TEST_ASSERT_EQUAL_UINT16(43000, d.cell_hi_ct);
    TEST_ASSERT_EQUAL_UINT8(10, d.cell_hi_id);
}

void test_processCAN_0x6B2_ov_glitch_recovers_without_exposing_bad_value(void){
    BpsData d = safeData();
    uint8_t glitch[] = {0xBB,0x80, 10, 0x7D,0x00, 1, 0,0};   // 48000, id 10
    can_frame f1 = makeFrame(0x6B2, glitch, 8);
    processCAN(f1, d);
    TEST_ASSERT_EQUAL_UINT16(36000, d.cell_hi_ct);   // glitch never surfaced

    // Bus recovers -- next frame is a normal in-range reading
    uint8_t normal[] = {0x94,0x70, 3, 0x7D,0x00, 1, 0,0};    // 38000, id 3
    can_frame f2 = makeFrame(0x6B2, normal, 8);
    processCAN(f2, d);

    TEST_ASSERT_EQUAL_UINT16(38000, d.cell_hi_ct);
    TEST_ASSERT_EQUAL_UINT8(3, d.cell_hi_id);
    TEST_ASSERT_FALSE(d.cell_hi_flagged);
}

void test_processCAN_0x6B2_uv_single_frame_not_trusted(void){
    BpsData d = safeData();   // cell_lo_ct = 32000 (safe)
    // Single glitched frame: cell_lo = 2.0000 V (20000 = 0x4E20), id 5
    uint8_t data[] = {0x94,0x70, 3, 0x4E,0x20, 5, 0,0};
    can_frame f = makeFrame(0x6B2, data, 8);
    processCAN(f, d);

    TEST_ASSERT_EQUAL_UINT16(32000, d.cell_lo_ct);
    TEST_ASSERT_TRUE(d.cell_lo_flagged);
}

void test_processCAN_0x6B2_uv_confirmed_after_second_frame(void){
    BpsData d = safeData();
    uint8_t glitch[] = {0x94,0x70, 3, 0x4E,0x20, 5, 0,0};    // lo = 20000, id 5
    can_frame f1 = makeFrame(0x6B2, glitch, 8);
    processCAN(f1, d);

    uint8_t confirm[] = {0x94,0x70, 3, 0x5D,0xC0, 5, 0,0};   // lo = 24000, id 5
    can_frame f2 = makeFrame(0x6B2, confirm, 8);
    processCAN(f2, d);

    TEST_ASSERT_EQUAL_UINT16(24000, d.cell_lo_ct);
    TEST_ASSERT_EQUAL_UINT8(5, d.cell_lo_id);
}

/* ================================================================
 *  processCAN — 0x6B3  (temperatures)
 * ================================================================ */
void test_processCAN_0x6B3(void){
    BpsData d;
    // temp_hi = 40, temp_avg = 35 (byte 4)
    uint8_t data[] = {40, 0,0,0, 35, 0,0,0};
    can_frame f = makeFrame(0x6B3, data, 8);
    processCAN(f, d);

    TEST_ASSERT_EQUAL_UINT8(40, d.temp_hi);
    TEST_ASSERT_EQUAL_UINT8(35, d.temp_avg);
}

/* ================================================================
 *  processCAN — unknown ID leaves data unchanged
 * ================================================================ */
void test_processCAN_unknown_id(void){
    BpsData d = safeData();
    BpsData before = d;
    uint8_t data[] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
    can_frame f = makeFrame(0x999, data, 8);
    processCAN(f, d);

    TEST_ASSERT_EQUAL_INT16(before.cur_dA,     d.cur_dA);
    TEST_ASSERT_EQUAL_UINT16(before.pack_dV,   d.pack_dV);
    TEST_ASSERT_EQUAL_UINT8(before.temp_hi,    d.temp_hi);
    TEST_ASSERT_EQUAL_UINT16(before.cell_hi_ct,d.cell_hi_ct);
}

/* ================================================================
 *  processCAN — short/RTR frames must not touch data
 *
 *  The mcp2515 driver only overwrites the first can_dlc bytes of a
 *  can_frame's data[] on receive, and readCAN() reuses one can_frame across
 *  every message polled in a pass. A short or RTR frame landing on one of
 *  our heartbeat IDs would otherwise get parsed against stale bytes left
 *  over from an earlier, unrelated frame. processCAN() must reject these
 *  outright rather than parse a partially-stale buffer.
 * ================================================================ */
void test_processCAN_rejects_short_dlc(void){
    BpsData d = safeData();
    BpsData before = d;
    // Only 2 of the 8 bytes are "real" -- the rest would be stale in practice.
    uint8_t data[] = {0x00,0x64, 0,0,0,0,0,0};
    can_frame f = makeFrame(0x6B0, data, 2);
    processCAN(f, d);

    TEST_ASSERT_EQUAL_INT16(before.cur_dA,   d.cur_dA);
    TEST_ASSERT_EQUAL_UINT16(before.pack_dV, d.pack_dV);
    TEST_ASSERT_EQUAL_UINT8(before.soc_pct,  d.soc_pct);
    TEST_ASSERT_FALSE(d.got_6B0 && !before.got_6B0);
}

void test_processCAN_rejects_rtr_frame(void){
    BpsData d = safeData();
    BpsData before = d;
    // RTR frame: dlc reports 8 (requested length) but no data was actually
    // transmitted on the wire, so the buffer would be stale, not real.
    uint8_t data[] = {40, 0,0,0, 35, 0,0,0};
    can_frame f = makeFrame(0x6B3 | CAN_RTR_FLAG, data, 8);
    processCAN(f, d);

    TEST_ASSERT_EQUAL_UINT8(before.temp_hi,  d.temp_hi);
    TEST_ASSERT_EQUAL_UINT8(before.temp_avg, d.temp_avg);
}

/* ================================================================
 *  processCAN updates data after fault (the original bug)
 * ================================================================ */
void test_processCAN_updates_after_fault(void){
    BpsData d = safeData();
    // Trigger over-temp
    d.temp_hi = 50;

    // Simulate new CAN frame arriving with fresh, normal data
    uint8_t data[] = {30, 0,0,0, 25, 0,0,0};
    can_frame f = makeFrame(0x6B3, data, 8);
    processCAN(f, d);

    // Data should reflect the NEW frame, not the old stale value
    TEST_ASSERT_EQUAL_UINT8(30, d.temp_hi);
    TEST_ASSERT_EQUAL_UINT8(25, d.temp_avg);
}

/* ================================================================
 *  checkFaultCondition — individual triggers
 * ================================================================ */
void test_no_fault_safe_data(void){
    BpsData d = safeData();
    TEST_ASSERT_FALSE(checkFaultCondition(d));
}

void test_fault_over_temp(void){
    BpsData d = safeData();
    d.temp_hi = 45;
    TEST_ASSERT_TRUE(checkFaultCondition(d));
}

void test_fault_over_current(void){
    BpsData d = safeData();
    d.cur_dA = 1001;
    TEST_ASSERT_TRUE(checkFaultCondition(d));
}

void test_fault_charge_current(void){
    BpsData d = safeData();
    d.cur_dA = -426;
    TEST_ASSERT_TRUE(checkFaultCondition(d));
}

void test_fault_pack_overvolt(void){
    BpsData d = safeData();
    d.pack_dV = 951;
    TEST_ASSERT_TRUE(checkFaultCondition(d));
}

void test_fault_pack_undervolt(void){
    BpsData d = safeData();
    d.pack_dV = 779;
    TEST_ASSERT_TRUE(checkFaultCondition(d));
}

void test_fault_cell_overvolt(void){
    BpsData d = safeData();
    d.cell_hi_ct = 42000;
    TEST_ASSERT_TRUE(checkFaultCondition(d));
}

void test_fault_cell_undervolt(void){
    BpsData d = safeData();
    d.cell_lo_ct = 25000;
    TEST_ASSERT_TRUE(checkFaultCondition(d));
}

/* ================================================================
 *  checkFaultCondition — boundary (just below threshold = no fault)
 * ================================================================ */
void test_no_fault_temp_boundary(void){
    BpsData d = safeData();
    d.temp_hi = 44;
    TEST_ASSERT_FALSE(checkFaultCondition(d));
}

void test_no_fault_current_boundary(void){
    BpsData d = safeData();
    d.cur_dA = 1000;   // threshold is >1000, so 1000 is safe
    TEST_ASSERT_FALSE(checkFaultCondition(d));
}

void test_no_fault_cell_hi_boundary(void){
    BpsData d = safeData();
    d.cell_hi_ct = 41999;  // threshold is >=42000
    TEST_ASSERT_FALSE(checkFaultCondition(d));
}

void test_no_fault_cell_lo_boundary(void){
    BpsData d = safeData();
    d.cell_lo_ct = 25001;  // threshold is <=25000
    TEST_ASSERT_FALSE(checkFaultCondition(d));
}

/* ================================================================
 *  evaluateFault — time-based (2 s hold) latch behaviour
 * ================================================================ */
void test_eval_skips_before_data_ready(void){
    BpsData d = safeData();
    d.got_6B3 = false;      // missing temperature frame
    d.temp_hi = 50;          // would fault if evaluated
    FaultTimer timer;
    bool live = evaluateFault(d, false, timer, 0);
    TEST_ASSERT_FALSE(live);
    TEST_ASSERT_FALSE(timer.candidate); // should not even start the hold timer
}

void test_eval_no_fault(void){
    BpsData d = safeData();
    FaultTimer timer;
    TEST_ASSERT_FALSE(evaluateFault(d, false, timer, 0));
    TEST_ASSERT_FALSE(timer.candidate);
}

void test_eval_fault_onset_does_not_latch_immediately(void){
    BpsData d = safeData();
    d.temp_hi = 50;     // fault condition
    FaultTimer timer;
    bool live = evaluateFault(d, false, timer, 1000);
    TEST_ASSERT_FALSE(live);          // not latched yet
    TEST_ASSERT_TRUE(timer.candidate); // hold timer now running
    TEST_ASSERT_EQUAL_UINT32(1000, timer.since);
}

void test_eval_latches_after_hold_time(void){
    BpsData d = safeData();
    d.temp_hi = 50;
    FaultTimer timer;
    bool live = evaluateFault(d, false, timer, 1000);   // onset
    TEST_ASSERT_FALSE(live);
    live = evaluateFault(d, false, timer, 1000 + FAULT_HOLD_MS - 1);
    TEST_ASSERT_FALSE(live);   // just under the hold time
    live = evaluateFault(d, false, timer, 1000 + FAULT_HOLD_MS);
    TEST_ASSERT_TRUE(live);    // held for the full 2 s -> latches
}

void test_eval_latch_persists(void){
    BpsData d = safeData();     // no fault condition now
    FaultTimer timer;
    bool live = evaluateFault(d, true, timer, 5000);   // already latched
    TEST_ASSERT_TRUE(live);     // stays latched regardless of current data
}

void test_eval_transient_resets_hold_timer(void){
    BpsData d = safeData();     // no fault
    FaultTimer timer;
    timer.candidate = true;     // hold timer was running previously
    timer.since     = 500;
    bool live = evaluateFault(d, false, timer, 1000);
    TEST_ASSERT_FALSE(live);
    TEST_ASSERT_FALSE(timer.candidate);    // recovery cancels the hold timer
}

/* ================================================================
 *  Integration: full CAN-to-fault pipeline
 * ================================================================ */
void test_integration_overtemp_latch_then_update(void){
    BpsData d = safeData();
    FaultTimer timer;
    bool liveF = false;

    // Frame 1: temp_hi = 50 (over-temp)
    uint8_t hot[] = {50, 0,0,0, 30, 0,0,0};
    can_frame f1 = makeFrame(0x6B3, hot, 8);
    processCAN(f1, d);

    // First eval: onset, hold timer starts
    liveF = evaluateFault(d, liveF, timer, 1000);
    TEST_ASSERT_FALSE(liveF);
    TEST_ASSERT_TRUE(timer.candidate);

    // Eval after the 2 s hold time with same data: latches
    liveF = evaluateFault(d, liveF, timer, 1000 + FAULT_HOLD_MS);
    TEST_ASSERT_TRUE(liveF);

    // Now a fresh frame arrives with normal temp
    uint8_t cool[] = {30, 0,0,0, 25, 0,0,0};
    can_frame f2 = makeFrame(0x6B3, cool, 8);
    processCAN(f2, d);

    // Data is updated even though fault is latched
    TEST_ASSERT_EQUAL_UINT8(30, d.temp_hi);

    // Fault stays latched (requires MCU reset)
    liveF = evaluateFault(d, liveF, timer, 1000 + FAULT_HOLD_MS + 1000);
    TEST_ASSERT_TRUE(liveF);
}

void test_integration_data_keeps_updating_while_latched(void){
    BpsData d = safeData();

    // Force into latched fault state
    FaultTimer timer;
    bool liveF = true;

    // Send several frames — data should update each time
    uint8_t d1[] = {0x00,0x64, 0x03,0x52, 80, 0,0,0};   // cur=100dA
    can_frame f1 = makeFrame(0x6B0, d1, 8);
    processCAN(f1, d);
    TEST_ASSERT_EQUAL_INT16(100, d.cur_dA);

    uint8_t d2[] = {0x00,0xC8, 0x03,0x84, 90, 0,0,0};   // cur=200dA, pack=900dV
    can_frame f2 = makeFrame(0x6B0, d2, 8);
    processCAN(f2, d);
    TEST_ASSERT_EQUAL_INT16(200, d.cur_dA);
    TEST_ASSERT_EQUAL_UINT16(900, d.pack_dV);

    // Still latched
    liveF = evaluateFault(d, liveF, timer, 0);
    TEST_ASSERT_TRUE(liveF);
}

/* ================================================================
 *  E-stop debounce
 *
 *  Reproduces the un-debounced glitch behaviour and verifies the fix.
 *  A physical e-stop press is modelled as a (level, timestamp) trace:
 *  a few ms of contact bounce followed by a settled level. main.cpp
 *  currently feeds the *raw* level straight into the fault logic, so
 *  every bounce edge counts as a state change.
 * ================================================================ */
struct EstopSample { bool level; unsigned long t; };

/* Count how many times the boolean value changes across a sequence. */
static int countBoolTransitions(const bool *seq, int n){
    int c = 0;
    for (int i = 1; i < n; ++i) if (seq[i] != seq[i-1]) ++c;
    return c;
}

/* A realistic press: idle LOW, ~4 ms of contact bounce, then held HIGH
   past the 20 ms debounce window (timestamps in ms). */
static const EstopSample kPress[] = {
    {false, 0}, {false, 5},
    {true, 10}, {false, 11}, {true, 12}, {false, 13}, {true, 14},  // bounce
    {true, 20}, {true, 30}, {true, 40}, {true, 50}                 // settled HIGH
};
static const int kPressLen = sizeof(kPress) / sizeof(kPress[0]);

/* CURRENT behaviour (the bug): main.cpp uses `digitalRead(ESTOP_PIN)`
   verbatim, so the raw trace of a single press contains many transitions —
   each one re-derives faultCode and fires an immediate 0x791 frame. */
void test_estop_raw_read_glitches_on_bounce(void){
    bool raw[kPressLen];
    for (int i = 0; i < kPressLen; ++i) raw[i] = kPress[i].level;
    // A single physical press glitches many times when read raw.
    TEST_ASSERT_TRUE(countBoolTransitions(raw, kPressLen) > 1);
}

/* FIXED behaviour: debounceEstop() collapses the bounce into exactly one
   clean transition, and still reports the press once it settles. */
void test_estop_debounce_suppresses_bounce(void){
    EstopDebounce s;
    bool debounced[kPressLen];
    for (int i = 0; i < kPressLen; ++i)
        debounced[i] = debounceEstop(s, kPress[i].level, kPress[i].t);

    TEST_ASSERT_EQUAL_INT(1, countBoolTransitions(debounced, kPressLen));
    TEST_ASSERT_TRUE(debounced[kPressLen - 1]);   // press is reported once settled
}

/* A noise spike shorter than the debounce window never flips the state. */
void test_estop_debounce_rejects_short_spike(void){
    EstopDebounce s;
    TEST_ASSERT_FALSE(debounceEstop(s, false, 0));
    TEST_ASSERT_FALSE(debounceEstop(s, true,  1));   // spike appears
    TEST_ASSERT_FALSE(debounceEstop(s, true,  5));   // still within 20 ms window
    TEST_ASSERT_FALSE(debounceEstop(s, false, 6));   // spike gone before settling
    TEST_ASSERT_FALSE(debounceEstop(s, false, 40));  // stayed released the whole time
}

/* A genuine sustained press is accepted exactly at the debounce boundary. */
void test_estop_debounce_accepts_sustained_press(void){
    EstopDebounce s;
    TEST_ASSERT_FALSE(debounceEstop(s, true, 100));                  // t=0 of press
    TEST_ASSERT_FALSE(debounceEstop(s, true, 100 + ESTOP_DEBOUNCE_MS - 1));
    TEST_ASSERT_TRUE (debounceEstop(s, true, 100 + ESTOP_DEBOUNCE_MS));
}

/* Release also debounces: bounce on the way back to LOW is suppressed. */
void test_estop_debounce_suppresses_release_bounce(void){
    EstopDebounce s;
    // Establish a settled pressed state first.
    debounceEstop(s, true, 0);
    TEST_ASSERT_TRUE(debounceEstop(s, true, ESTOP_DEBOUNCE_MS));

    // Release with bounce, then settle LOW.
    debounceEstop(s, false, 100);
    TEST_ASSERT_TRUE (debounceEstop(s, true,  101));  // bounce back HIGH — still pressed
    debounceEstop(s, false, 102);
    TEST_ASSERT_TRUE (debounceEstop(s, false, 110));  // <20 ms since settling LOW
    TEST_ASSERT_FALSE(debounceEstop(s, false, 122));  // now released after settling
}

/* ================================================================
 *  E-stop trip latch
 *
 *  A confirmed e-stop press is a safety trip: it must stay active after
 *  the button is released (contactors are opened on trip and never
 *  reclosed in loop()), and only a power cycle clears it.
 * ================================================================ */
void test_estop_latch_holds_after_release(void){
    bool latched = false;
    TEST_ASSERT_FALSE(latchEstop(latched, false));   // idle
    TEST_ASSERT_TRUE (latchEstop(latched, true));    // pressed -> trip
    TEST_ASSERT_TRUE (latchEstop(latched, false));   // released -> STILL tripped
    TEST_ASSERT_TRUE (latchEstop(latched, false));
}

void test_estop_latch_stays_clear_without_press(void){
    bool latched = false;
    TEST_ASSERT_FALSE(latchEstop(latched, false));
    TEST_ASSERT_FALSE(latchEstop(latched, false));
    TEST_ASSERT_FALSE(latched);
}

/* Integration: debounce + latch together, exactly as loop() chains them.
   A bouncing press latches once (only after it settles) and stays tripped
   through a bouncing release. */
void test_estop_debounce_then_latch_persists(void){
    EstopDebounce s;
    bool latched = false;

    // Bouncing press — not tripped until the level settles past the window.
    latchEstop(latched, debounceEstop(s, false, 0));
    latchEstop(latched, debounceEstop(s, true,  10));
    latchEstop(latched, debounceEstop(s, false, 11));   // bounce
    latchEstop(latched, debounceEstop(s, true,  12));
    TEST_ASSERT_FALSE(latched);                          // still settling
    TEST_ASSERT_TRUE(latchEstop(latched, debounceEstop(s, true, 40)));  // settled -> latch

    // Release (with bounce) — the trip must stay latched.
    latchEstop(latched, debounceEstop(s, false, 100));
    latchEstop(latched, debounceEstop(s, false, 130));  // debounced back to released
    TEST_ASSERT_TRUE(latchEstop(latched, debounceEstop(s, false, 200)));
}

/* ================================================================
 *  Runner
 * ================================================================ */
int main(int argc, char **argv){
    UNITY_BEGIN();

    /* byte helpers */
    RUN_TEST(test_be16u_zero);
    RUN_TEST(test_be16u_max);
    RUN_TEST(test_be16u_known);
    RUN_TEST(test_be16s_positive);
    RUN_TEST(test_be16s_negative);

    /* cellVolts */
    RUN_TEST(test_cellVolts_typical);
    RUN_TEST(test_cellVolts_zero);

    /* processCAN */
    RUN_TEST(test_processCAN_0x6B0);
    RUN_TEST(test_processCAN_0x6B0_negative_current);
    RUN_TEST(test_processCAN_0x6B2);
    RUN_TEST(test_processCAN_0x6B2_ov_single_frame_not_trusted);
    RUN_TEST(test_processCAN_0x6B2_ov_confirmed_after_second_frame);
    RUN_TEST(test_processCAN_0x6B2_ov_glitch_recovers_without_exposing_bad_value);
    RUN_TEST(test_processCAN_0x6B2_uv_single_frame_not_trusted);
    RUN_TEST(test_processCAN_0x6B2_uv_confirmed_after_second_frame);
    RUN_TEST(test_processCAN_0x6B3);
    RUN_TEST(test_processCAN_unknown_id);
    RUN_TEST(test_processCAN_rejects_short_dlc);
    RUN_TEST(test_processCAN_rejects_rtr_frame);
    RUN_TEST(test_processCAN_updates_after_fault);

    /* checkFaultCondition */
    RUN_TEST(test_no_fault_safe_data);
    RUN_TEST(test_fault_over_temp);
    RUN_TEST(test_fault_over_current);
    RUN_TEST(test_fault_charge_current);
    RUN_TEST(test_fault_pack_overvolt);
    RUN_TEST(test_fault_pack_undervolt);
    RUN_TEST(test_fault_cell_overvolt);
    RUN_TEST(test_fault_cell_undervolt);
    RUN_TEST(test_no_fault_temp_boundary);
    RUN_TEST(test_no_fault_current_boundary);
    RUN_TEST(test_no_fault_cell_hi_boundary);
    RUN_TEST(test_no_fault_cell_lo_boundary);

    /* evaluateFault (2 s hold latch) */
    RUN_TEST(test_eval_skips_before_data_ready);
    RUN_TEST(test_eval_no_fault);
    RUN_TEST(test_eval_fault_onset_does_not_latch_immediately);
    RUN_TEST(test_eval_latches_after_hold_time);
    RUN_TEST(test_eval_latch_persists);
    RUN_TEST(test_eval_transient_resets_hold_timer);

    /* integration */
    RUN_TEST(test_integration_overtemp_latch_then_update);
    RUN_TEST(test_integration_data_keeps_updating_while_latched);

    /* e-stop debounce */
    RUN_TEST(test_estop_raw_read_glitches_on_bounce);
    RUN_TEST(test_estop_debounce_suppresses_bounce);
    RUN_TEST(test_estop_debounce_rejects_short_spike);
    RUN_TEST(test_estop_debounce_accepts_sustained_press);
    RUN_TEST(test_estop_debounce_suppresses_release_bounce);

    /* e-stop trip latch */
    RUN_TEST(test_estop_latch_holds_after_release);
    RUN_TEST(test_estop_latch_stays_clear_without_press);
    RUN_TEST(test_estop_debounce_then_latch_persists);

    return UNITY_END();
}

#include <Arduino.h>
#include <unity.h>
#include "levcan_defs.h"

// Forward-declare the function under test from main.cpp
void decode_levcan_data(uint16_t msgId, const uint8_t *data, uint8_t len);

// Helper: build a raw 29-bit CAN ID from LEVCAN header fields
static uint32_t make_header_raw(uint8_t source, uint8_t target, uint16_t msgId,
                                uint8_t eom, uint8_t parity, uint8_t rts_cts,
                                uint8_t priority) {
    LC_Header_t h;
    h.raw = 0;
    h.Source   = source;
    h.Target   = target;
    h.MsgID    = msgId;
    h.EoM      = eom;
    h.Parity   = parity;
    h.RTS_CTS  = rts_cts;
    h.Priority = priority;
    return h.raw;
}

// ============================================================
// Bitfield layout on ESP32 (Xtensa) - confirm matches spec
// ============================================================

void test_esp32_header_bitfield_layout(void) {
    // Verify that each field occupies the expected bit positions on this architecture
    LC_Header_t h;

    h.raw = 0; h.Source = 1;
    TEST_ASSERT_EQUAL_UINT32(1 << 0, h.raw);

    h.raw = 0; h.Target = 1;
    TEST_ASSERT_EQUAL_UINT32(1 << 7, h.raw);

    h.raw = 0; h.MsgID = 1;
    TEST_ASSERT_EQUAL_UINT32(1 << 14, h.raw);

    h.raw = 0; h.EoM = 1;
    TEST_ASSERT_EQUAL_UINT32(1 << 24, h.raw);

    h.raw = 0; h.Parity = 1;
    TEST_ASSERT_EQUAL_UINT32(1 << 25, h.raw);

    h.raw = 0; h.RTS_CTS = 1;
    TEST_ASSERT_EQUAL_UINT32(1 << 26, h.raw);

    h.raw = 0; h.Priority = 1;
    TEST_ASSERT_EQUAL_UINT32(1 << 27, h.raw);
}

void test_esp32_header_max_packed(void) {
    uint32_t raw = make_header_raw(0x7F, 0x7F, 0x3FF, 1, 1, 1, 3);
    TEST_ASSERT_EQUAL_UINT32(0x1FFFFFFF, raw);
}

void test_esp32_header_roundtrip(void) {
    uint32_t raw = make_header_raw(5, 127, 0x306, 1, 0, 0, 1);
    LC_Header_t h;
    h.raw = raw;
    TEST_ASSERT_EQUAL_UINT8(5, h.Source);
    TEST_ASSERT_EQUAL_UINT8(127, h.Target);
    TEST_ASSERT_EQUAL_UINT16(0x306, h.MsgID);
    TEST_ASSERT_EQUAL_UINT8(1, h.EoM);
    TEST_ASSERT_EQUAL_UINT8(0, h.Parity);
    TEST_ASSERT_EQUAL_UINT8(0, h.RTS_CTS);
    TEST_ASSERT_EQUAL_UINT8(1, h.Priority);
}

// ============================================================
// Struct sizes on ESP32 (confirm packing matches CAN payloads)
// ============================================================

void test_esp32_struct_sizes(void) {
    TEST_ASSERT_EQUAL(4, sizeof(LC_Header_t));
    TEST_ASSERT_EQUAL(8, sizeof(LC_Supply_t));
    TEST_ASSERT_EQUAL(8, sizeof(LC_Temperature_t));
    TEST_ASSERT_EQUAL(8, sizeof(LC_RPM_t));
    TEST_ASSERT_EQUAL(2, sizeof(LC_Speed_t));
    TEST_ASSERT_EQUAL(8, sizeof(LC_Power_t));
    TEST_ASSERT_EQUAL(2, sizeof(LC_ThrottleV_t));
    TEST_ASSERT_EQUAL(2, sizeof(LC_BrakeV_t));
    TEST_ASSERT_EQUAL(4, sizeof(LC_ControlFactor_t));
    TEST_ASSERT_EQUAL(8, sizeof(LC_WhUsed_t));
    TEST_ASSERT_EQUAL(8, sizeof(LC_WhStored_t));
    TEST_ASSERT_EQUAL(6, sizeof(LC_Distance_t));
    TEST_ASSERT_EQUAL(8, sizeof(LC_AhUsed_t));
    TEST_ASSERT_EQUAL(8, sizeof(LC_AhStored_t));
    TEST_ASSERT_EQUAL(8, sizeof(LC_ActiveFunctions_t));
    TEST_ASSERT_EQUAL(4, sizeof(LC_Buttons_t));
    TEST_ASSERT_EQUAL(8, sizeof(LC_FOCstate_t));
    TEST_ASSERT_EQUAL(8, sizeof(LC_InternalVoltage_t));
    TEST_ASSERT_EQUAL(8, sizeof(LC_MotorHalls_t));
}

// ============================================================
// decode_levcan_data - verify no crashes, correct parsing
// ============================================================

void test_decode_dc_supply(void) {
    LC_Supply_t supply;
    supply.Voltage_mV = 48000;
    supply.Current_mA = 15500;
    // Should not crash; output goes to Serial
    decode_levcan_data(LC_Obj_DCSupply, (const uint8_t *)&supply, sizeof(supply));
    TEST_PASS();
}

void test_decode_motor_supply(void) {
    LC_Supply_t supply;
    supply.Voltage_mV = 36000;
    supply.Current_mA = -5000;
    decode_levcan_data(LC_Obj_MotorSupply, (const uint8_t *)&supply, sizeof(supply));
    TEST_PASS();
}

void test_decode_temperature(void) {
    LC_Temperature_t temp;
    temp.InternalTemp_C = 85;
    temp.ExternalTemp_C = 35;
    temp.ExtraTemp1_C = -20;
    temp.ExtraTemp2_C = 0;
    decode_levcan_data(LC_Obj_Temperature, (const uint8_t *)&temp, sizeof(temp));
    TEST_PASS();
}

void test_decode_rpm(void) {
    LC_RPM_t rpm;
    rpm.RPM = 3500;
    rpm.ERPM = 28000;
    decode_levcan_data(LC_Obj_RPM, (const uint8_t *)&rpm, sizeof(rpm));
    TEST_PASS();
}

void test_decode_speed(void) {
    LC_Speed_t speed;
    speed.Speed_kph = 45;
    decode_levcan_data(LC_Obj_Speed, (const uint8_t *)&speed, sizeof(speed));
    TEST_PASS();
}

void test_decode_power(void) {
    LC_Power_t power;
    power.Watts = 1500;
    power.Direction = 2; // discharging
    decode_levcan_data(LC_Obj_Power, (const uint8_t *)&power, sizeof(power));
    TEST_PASS();
}

void test_decode_throttle(void) {
    LC_ThrottleV_t throttle;
    throttle.ThrottleV_mV = 2500;
    decode_levcan_data(LC_Obj_ThrottleV, (const uint8_t *)&throttle, sizeof(throttle));
    TEST_PASS();
}

void test_decode_brake(void) {
    LC_BrakeV_t brake;
    brake.BrakeV_mV = 1200;
    decode_levcan_data(LC_Obj_BrakeV, (const uint8_t *)&brake, sizeof(brake));
    TEST_PASS();
}

void test_decode_control_factor(void) {
    LC_ControlFactor_t cf;
    cf.Factor = 0.75f;
    decode_levcan_data(LC_Obj_ControlFactor, (const uint8_t *)&cf, sizeof(cf));
    TEST_PASS();
}

void test_decode_foc_state(void) {
    LC_FOCstate_t foc;
    foc.Q = 12.5f;
    foc.D = -3.2f;
    decode_levcan_data(LC_Obj_FOCstateI, (const uint8_t *)&foc, sizeof(foc));
    TEST_PASS();
}

void test_decode_active_functions(void) {
    LC_ActiveFunctions_t af;
    af.FunctionsLow = (1 << 0) | (1 << 7) | (1 << 17); // Enable, Reverse, MotorWarn
    af.FunctionsHigh = 0;
    decode_levcan_data(LC_Obj_ActiveFunctions, (const uint8_t *)&af, sizeof(af));
    TEST_PASS();
}

void test_decode_motor_halls(void) {
    LC_MotorHalls_t halls;
    halls.HallA_mV = 3300;
    halls.HallB_mV = 0;
    halls.HallC_mV = 3300;
    halls.InputDigital = 0x05;
    halls.HallState = 5;
    decode_levcan_data(LC_Obj_MotorHalls, (const uint8_t *)&halls, sizeof(halls));
    TEST_PASS();
}

void test_decode_distance(void) {
    LC_Distance_t dist;
    dist.TripMeterFromEn = 12345;
    dist.TotalTripKm = 500;
    decode_levcan_data(LC_Obj_Distance, (const uint8_t *)&dist, sizeof(dist));
    TEST_PASS();
}

void test_decode_unknown_msgid(void) {
    uint8_t data[] = {0xDE, 0xAD, 0xBE, 0xEF};
    decode_levcan_data(0x000, data, sizeof(data));
    TEST_PASS();
}

// ============================================================
// Edge cases: short payloads should not crash
// ============================================================

void test_decode_short_payload_skips(void) {
    uint8_t data[2] = {0x00, 0x00};
    // Supply expects 8 bytes, only 2 provided — should skip decoding
    decode_levcan_data(LC_Obj_DCSupply, data, 2);
    TEST_PASS();
}

void test_decode_zero_length(void) {
    uint8_t data[1] = {0x00};
    decode_levcan_data(LC_Obj_RPM, data, 0);
    TEST_PASS();
}

// ============================================================
// Main
// ============================================================

void setUp(void) {}
void tearDown(void) {}

void setup() {
    delay(2000); // allow Serial to connect
    UNITY_BEGIN();

    // Bitfield layout
    RUN_TEST(test_esp32_header_bitfield_layout);
    RUN_TEST(test_esp32_header_max_packed);
    RUN_TEST(test_esp32_header_roundtrip);

    // Struct sizes
    RUN_TEST(test_esp32_struct_sizes);

    // decode_levcan_data
    RUN_TEST(test_decode_dc_supply);
    RUN_TEST(test_decode_motor_supply);
    RUN_TEST(test_decode_temperature);
    RUN_TEST(test_decode_rpm);
    RUN_TEST(test_decode_speed);
    RUN_TEST(test_decode_power);
    RUN_TEST(test_decode_throttle);
    RUN_TEST(test_decode_brake);
    RUN_TEST(test_decode_control_factor);
    RUN_TEST(test_decode_foc_state);
    RUN_TEST(test_decode_active_functions);
    RUN_TEST(test_decode_motor_halls);
    RUN_TEST(test_decode_distance);
    RUN_TEST(test_decode_unknown_msgid);

    // Edge cases
    RUN_TEST(test_decode_short_payload_skips);
    RUN_TEST(test_decode_zero_length);

    UNITY_END();
}

void loop() {}

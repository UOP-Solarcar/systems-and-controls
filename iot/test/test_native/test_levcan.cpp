#include <unity.h>
#include "levcan_defs.h"

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
// Header parsing tests
// ============================================================

void test_header_source_field(void) {
    LC_Header_t h;
    h.raw = 0;
    h.Source = 42;
    TEST_ASSERT_EQUAL_UINT8(42, h.Source);
    // Source occupies bits 0-6, so raw should equal 42
    TEST_ASSERT_EQUAL_UINT32(42, h.raw & 0x7F);
}

void test_header_target_field(void) {
    LC_Header_t h;
    h.raw = 0;
    h.Target = 127; // broadcast
    TEST_ASSERT_EQUAL_UINT8(127, h.Target);
    TEST_ASSERT_EQUAL_UINT32(127 << 7, h.raw & (0x7F << 7));
}

void test_header_msgid_field(void) {
    LC_Header_t h;
    h.raw = 0;
    h.MsgID = 0x301; // DCSupply
    TEST_ASSERT_EQUAL_UINT16(0x301, h.MsgID);
    TEST_ASSERT_EQUAL_UINT32((uint32_t)0x301 << 14, h.raw & (0x3FF << 14));
}

void test_header_priority_field(void) {
    LC_Header_t h;
    h.raw = 0;
    h.Priority = 3;
    TEST_ASSERT_EQUAL_UINT8(3, h.Priority);
    TEST_ASSERT_EQUAL_UINT32((uint32_t)3 << 27, h.raw & ((uint32_t)0x3 << 27));
}

void test_header_control_bits(void) {
    LC_Header_t h;
    h.raw = 0;
    h.EoM = 1;
    h.Parity = 1;
    h.RTS_CTS = 1;
    TEST_ASSERT_EQUAL_UINT8(1, h.EoM);
    TEST_ASSERT_EQUAL_UINT8(1, h.Parity);
    TEST_ASSERT_EQUAL_UINT8(1, h.RTS_CTS);
    TEST_ASSERT_EQUAL_UINT32((uint32_t)1 << 24, h.raw & ((uint32_t)1 << 24));
    TEST_ASSERT_EQUAL_UINT32((uint32_t)1 << 25, h.raw & ((uint32_t)1 << 25));
    TEST_ASSERT_EQUAL_UINT32((uint32_t)1 << 26, h.raw & ((uint32_t)1 << 26));
}

void test_header_roundtrip(void) {
    // Simulate a real frame: Source=5, Target=127(broadcast), MsgID=0x306(RPM),
    // EoM=1, Parity=0, RTS_CTS=0, Priority=1
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

void test_header_all_fields_packed(void) {
    // All fields at max values
    uint32_t raw = make_header_raw(0x7F, 0x7F, 0x3FF, 1, 1, 1, 3);
    // Should use exactly 29 bits: 7+7+10+1+1+1+2 = 29
    TEST_ASSERT_EQUAL_UINT32(0x1FFFFFFF, raw);
}

void test_header_zero(void) {
    LC_Header_t h;
    h.raw = 0;
    TEST_ASSERT_EQUAL_UINT8(0, h.Source);
    TEST_ASSERT_EQUAL_UINT8(0, h.Target);
    TEST_ASSERT_EQUAL_UINT16(0, h.MsgID);
    TEST_ASSERT_EQUAL_UINT8(0, h.EoM);
    TEST_ASSERT_EQUAL_UINT8(0, h.Parity);
    TEST_ASSERT_EQUAL_UINT8(0, h.RTS_CTS);
    TEST_ASSERT_EQUAL_UINT8(0, h.Priority);
}

// ============================================================
// Object name lookup tests
// ============================================================

void test_obj_name_known_objects(void) {
    TEST_ASSERT_EQUAL_STRING("DCSupply",    lc_obj_name(0x301));
    TEST_ASSERT_EQUAL_STRING("MotorSupply", lc_obj_name(0x302));
    TEST_ASSERT_EQUAL_STRING("Temperature", lc_obj_name(0x305));
    TEST_ASSERT_EQUAL_STRING("RPM",         lc_obj_name(0x306));
    TEST_ASSERT_EQUAL_STRING("Speed",       lc_obj_name(0x308));
    TEST_ASSERT_EQUAL_STRING("ThrottleV",   lc_obj_name(0x309));
    TEST_ASSERT_EQUAL_STRING("BrakeV",      lc_obj_name(0x30A));
    TEST_ASSERT_EQUAL_STRING("Power",       lc_obj_name(0x304));
    TEST_ASSERT_EQUAL_STRING("ActiveFunctions", lc_obj_name(0x317));
    TEST_ASSERT_EQUAL_STRING("FOCstateI",   lc_obj_name(0x320));
    TEST_ASSERT_EQUAL_STRING("BatterySupply", lc_obj_name(0x324));
}

void test_obj_name_boundary_objects(void) {
    TEST_ASSERT_EQUAL_STRING("State",           lc_obj_name(0x300)); // first
    TEST_ASSERT_EQUAL_STRING("PowerModeLimits", lc_obj_name(0x330)); // last
}

void test_obj_name_unknown_returns_null(void) {
    TEST_ASSERT_NULL(lc_obj_name(0x000));
    TEST_ASSERT_NULL(lc_obj_name(0x2FF)); // just below range
    TEST_ASSERT_NULL(lc_obj_name(0x331)); // just above last
    TEST_ASSERT_NULL(lc_obj_name(0x329)); // gap in enum
    TEST_ASSERT_NULL(lc_obj_name(0x32A)); // gap in enum
    TEST_ASSERT_NULL(lc_obj_name(0xFFFF));
}

// ============================================================
// Struct size tests (must match CAN payload expectations)
// ============================================================

void test_struct_sizes(void) {
    TEST_ASSERT_EQUAL(8, sizeof(LC_Supply_t));       // 2x int32
    TEST_ASSERT_EQUAL(8, sizeof(LC_Temperature_t));  // 4x int16
    TEST_ASSERT_EQUAL(8, sizeof(LC_RPM_t));          // 2x int32
    TEST_ASSERT_EQUAL(2, sizeof(LC_Speed_t));         // 1x int16
    TEST_ASSERT_EQUAL(8, sizeof(LC_Power_t));         // 2x int32
    TEST_ASSERT_EQUAL(2, sizeof(LC_ThrottleV_t));     // 1x int16
    TEST_ASSERT_EQUAL(2, sizeof(LC_BrakeV_t));        // 1x int16
    TEST_ASSERT_EQUAL(4, sizeof(LC_ControlFactor_t)); // 1x float
    TEST_ASSERT_EQUAL(8, sizeof(LC_WhUsed_t));        // 2x int32
    TEST_ASSERT_EQUAL(8, sizeof(LC_WhStored_t));      // 2x int32
    TEST_ASSERT_EQUAL(6, sizeof(LC_Distance_t));      // uint32 + uint16
    TEST_ASSERT_EQUAL(8, sizeof(LC_AhUsed_t));        // 2x int32
    TEST_ASSERT_EQUAL(8, sizeof(LC_AhStored_t));      // 2x int32
    TEST_ASSERT_EQUAL(8, sizeof(LC_ActiveFunctions_t)); // 2x uint32
    TEST_ASSERT_EQUAL(4, sizeof(LC_Buttons_t));       // 2x uint16
    TEST_ASSERT_EQUAL(8, sizeof(LC_FOCstate_t));      // 2x float
    TEST_ASSERT_EQUAL(8, sizeof(LC_InternalVoltage_t)); // 4x int16
    TEST_ASSERT_EQUAL(8, sizeof(LC_MotorHalls_t));    // 3x int16 + 2x uint8
}

// ============================================================
// Data deserialization tests
// ============================================================

void test_supply_deserialization(void) {
    // 48.000V = 48000 mV, 15.500A = 15500 mA
    LC_Supply_t supply;
    supply.Voltage_mV = 48000;
    supply.Current_mA = 15500;

    uint8_t buf[8];
    memcpy(buf, &supply, sizeof(supply));

    const LC_Supply_t *parsed = (const LC_Supply_t *)buf;
    TEST_ASSERT_EQUAL_INT32(48000, parsed->Voltage_mV);
    TEST_ASSERT_EQUAL_INT32(15500, parsed->Current_mA);
}

void test_temperature_deserialization(void) {
    LC_Temperature_t temp;
    temp.InternalTemp_C = 45;
    temp.ExternalTemp_C = 30;
    temp.ExtraTemp1_C = -10;
    temp.ExtraTemp2_C = 0;

    uint8_t buf[8];
    memcpy(buf, &temp, sizeof(temp));

    const LC_Temperature_t *parsed = (const LC_Temperature_t *)buf;
    TEST_ASSERT_EQUAL_INT16(45, parsed->InternalTemp_C);
    TEST_ASSERT_EQUAL_INT16(30, parsed->ExternalTemp_C);
    TEST_ASSERT_EQUAL_INT16(-10, parsed->ExtraTemp1_C);
    TEST_ASSERT_EQUAL_INT16(0, parsed->ExtraTemp2_C);
}

void test_rpm_deserialization(void) {
    LC_RPM_t rpm;
    rpm.RPM = 3500;
    rpm.ERPM = 28000; // 8 pole pairs

    uint8_t buf[8];
    memcpy(buf, &rpm, sizeof(rpm));

    const LC_RPM_t *parsed = (const LC_RPM_t *)buf;
    TEST_ASSERT_EQUAL_INT32(3500, parsed->RPM);
    TEST_ASSERT_EQUAL_INT32(28000, parsed->ERPM);
}

void test_rpm_negative(void) {
    LC_RPM_t rpm;
    rpm.RPM = -1200;
    rpm.ERPM = -9600;

    uint8_t buf[8];
    memcpy(buf, &rpm, sizeof(rpm));

    const LC_RPM_t *parsed = (const LC_RPM_t *)buf;
    TEST_ASSERT_EQUAL_INT32(-1200, parsed->RPM);
    TEST_ASSERT_EQUAL_INT32(-9600, parsed->ERPM);
}

void test_control_factor_deserialization(void) {
    LC_ControlFactor_t cf;
    cf.Factor = 0.75f; // 75% throttle

    uint8_t buf[4];
    memcpy(buf, &cf, sizeof(cf));

    const LC_ControlFactor_t *parsed = (const LC_ControlFactor_t *)buf;
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.75f, parsed->Factor);
}

void test_control_factor_negative(void) {
    LC_ControlFactor_t cf;
    cf.Factor = -0.5f; // 50% braking

    uint8_t buf[4];
    memcpy(buf, &cf, sizeof(cf));

    const LC_ControlFactor_t *parsed = (const LC_ControlFactor_t *)buf;
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -0.5f, parsed->Factor);
}

void test_distance_deserialization(void) {
    LC_Distance_t dist;
    dist.TripMeterFromEn = 12345; // meters
    dist.TotalTripKm = 500;

    uint8_t buf[6];
    memcpy(buf, &dist, sizeof(dist));

    const LC_Distance_t *parsed = (const LC_Distance_t *)buf;
    TEST_ASSERT_EQUAL_UINT32(12345, parsed->TripMeterFromEn);
    TEST_ASSERT_EQUAL_UINT16(500, parsed->TotalTripKm);
}

void test_motor_halls_deserialization(void) {
    LC_MotorHalls_t halls;
    halls.HallA_mV = 3300;
    halls.HallB_mV = 0;
    halls.HallC_mV = 3300;
    halls.InputDigital = 0x05; // bits 0 and 2
    halls.HallState = 5;

    uint8_t buf[8];
    memcpy(buf, &halls, sizeof(halls));

    const LC_MotorHalls_t *parsed = (const LC_MotorHalls_t *)buf;
    TEST_ASSERT_EQUAL_INT16(3300, parsed->HallA_mV);
    TEST_ASSERT_EQUAL_INT16(0, parsed->HallB_mV);
    TEST_ASSERT_EQUAL_INT16(3300, parsed->HallC_mV);
    TEST_ASSERT_EQUAL_UINT8(0x05, parsed->InputDigital);
    TEST_ASSERT_EQUAL_UINT8(5, parsed->HallState);
}

void test_foc_state_deserialization(void) {
    LC_FOCstate_t foc;
    foc.Q = 12.5f;
    foc.D = -3.2f;

    uint8_t buf[8];
    memcpy(buf, &foc, sizeof(foc));

    const LC_FOCstate_t *parsed = (const LC_FOCstate_t *)buf;
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 12.5f, parsed->Q);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, -3.2f, parsed->D);
}

// ============================================================
// Special address tests
// ============================================================

void test_special_addresses(void) {
    TEST_ASSERT_EQUAL_UINT8(127, LC_Broadcast_Address);
    TEST_ASSERT_EQUAL_UINT8(126, LC_Null_Address);
}

void test_header_broadcast_target(void) {
    LC_Header_t h;
    h.raw = make_header_raw(5, LC_Broadcast_Address, 0x305, 1, 0, 0, 0);
    TEST_ASSERT_EQUAL_UINT8(LC_Broadcast_Address, h.Target);
    TEST_ASSERT_EQUAL_UINT8(5, h.Source);
}

// ============================================================
// Enum value tests (verify against LEVCAN spec)
// ============================================================

void test_obj_enum_values(void) {
    TEST_ASSERT_EQUAL_UINT16(0x300, LC_Obj_State);
    TEST_ASSERT_EQUAL_UINT16(0x301, LC_Obj_DCSupply);
    TEST_ASSERT_EQUAL_UINT16(0x305, LC_Obj_Temperature);
    TEST_ASSERT_EQUAL_UINT16(0x306, LC_Obj_RPM);
    TEST_ASSERT_EQUAL_UINT16(0x308, LC_Obj_Speed);
    TEST_ASSERT_EQUAL_UINT16(0x309, LC_Obj_ThrottleV);
    TEST_ASSERT_EQUAL_UINT16(0x30A, LC_Obj_BrakeV);
    TEST_ASSERT_EQUAL_UINT16(0x30E, LC_Obj_Buttons);
    TEST_ASSERT_EQUAL_UINT16(0x317, LC_Obj_ActiveFunctions);
    TEST_ASSERT_EQUAL_UINT16(0x31F, LC_Obj_FOCstateV);
    TEST_ASSERT_EQUAL_UINT16(0x320, LC_Obj_FOCstateI);
    TEST_ASSERT_EQUAL_UINT16(0x324, LC_Obj_BatterySupply);
    TEST_ASSERT_EQUAL_UINT16(0x330, LC_Obj_PowerModeLimits);
}

// ============================================================
// Header union size test
// ============================================================

void test_header_union_size(void) {
    TEST_ASSERT_EQUAL(4, sizeof(LC_Header_t));
}

// ============================================================
// Main
// ============================================================

void setUp(void) {}
void tearDown(void) {}

int main(int argc, char **argv) {
    UNITY_BEGIN();

    // Header parsing
    RUN_TEST(test_header_source_field);
    RUN_TEST(test_header_target_field);
    RUN_TEST(test_header_msgid_field);
    RUN_TEST(test_header_priority_field);
    RUN_TEST(test_header_control_bits);
    RUN_TEST(test_header_roundtrip);
    RUN_TEST(test_header_all_fields_packed);
    RUN_TEST(test_header_zero);
    RUN_TEST(test_header_union_size);

    // Object name lookup
    RUN_TEST(test_obj_name_known_objects);
    RUN_TEST(test_obj_name_boundary_objects);
    RUN_TEST(test_obj_name_unknown_returns_null);

    // Struct sizes
    RUN_TEST(test_struct_sizes);

    // Data deserialization
    RUN_TEST(test_supply_deserialization);
    RUN_TEST(test_temperature_deserialization);
    RUN_TEST(test_rpm_deserialization);
    RUN_TEST(test_rpm_negative);
    RUN_TEST(test_control_factor_deserialization);
    RUN_TEST(test_control_factor_negative);
    RUN_TEST(test_distance_deserialization);
    RUN_TEST(test_motor_halls_deserialization);
    RUN_TEST(test_foc_state_deserialization);

    // Special addresses
    RUN_TEST(test_special_addresses);
    RUN_TEST(test_header_broadcast_target);

    // Enum values
    RUN_TEST(test_obj_enum_values);

    UNITY_END();
    return 0;
}

#ifndef LEVCAN_DEFS_H_
#define LEVCAN_DEFS_H_

#include <stdint.h>
#include <stddef.h>
#include <string.h>

// ----- LEVCAN Header (packed into 29-bit extended CAN ID) -----
// Bit layout: [Priority:2][RTS_CTS:1][Parity:1][EoM:1][MsgID:10][Target:7][Source:7]
typedef union {
    uint32_t raw;
    struct {
        uint32_t Source   : 7;   // bits 0-6
        uint32_t Target   : 7;   // bits 7-13
        uint32_t MsgID    : 10;  // bits 14-23
        uint32_t EoM      : 1;   // bit  24
        uint32_t Parity   : 1;   // bit  25
        uint32_t RTS_CTS  : 1;   // bit  26
        uint32_t Priority : 2;   // bits 27-28
    };
} LC_Header_t;

// ----- LEVCAN Object IDs (from levcan_objects.h) -----
enum LC_ObjID : uint16_t {
    LC_Obj_State             = 0x300,
    LC_Obj_DCSupply          = 0x301,
    LC_Obj_MotorSupply       = 0x302,
    LC_Obj_InternalVoltage   = 0x303,
    LC_Obj_Power             = 0x304,
    LC_Obj_Temperature       = 0x305,
    LC_Obj_RPM               = 0x306,
    LC_Obj_RadSec            = 0x307,
    LC_Obj_Speed             = 0x308,
    LC_Obj_ThrottleV         = 0x309,
    LC_Obj_BrakeV            = 0x30A,
    LC_Obj_ControlFactor     = 0x30B,
    LC_Obj_SpeedCommand      = 0x30C,
    LC_Obj_TorqueCommand     = 0x30D,
    LC_Obj_Buttons           = 0x30E,
    LC_Obj_WhUsed            = 0x30F,
    LC_Obj_WhStored          = 0x310,
    LC_Obj_Distance          = 0x311,
    LC_Obj_MotorHalls        = 0x312,
    LC_Obj_CellsV            = 0x313,
    LC_Obj_CellMinMax        = 0x314,
    LC_Obj_CellBalance       = 0x315,
    LC_Obj_UserActivity      = 0x316,
    LC_Obj_ActiveFunctions   = 0x317,
    LC_Obj_LightSensor       = 0x318,
    LC_Obj_AccelerometerRaw  = 0x319,
    LC_Obj_Accelerometer     = 0x31A,
    LC_Obj_ControlFactorInt  = 0x31B,
    LC_Obj_DCLimitIFactor    = 0x31C,
    LC_Obj_DCLimitIValue     = 0x31D,
    LC_Obj_DCLimitVValue     = 0x31E,
    LC_Obj_FOCstateV         = 0x31F,
    LC_Obj_FOCstateI         = 0x320,
    LC_Obj_FOCrequest        = 0x321,
    LC_Obj_AhUsed            = 0x322,
    LC_Obj_AhStored          = 0x323,
    LC_Obj_BatterySupply     = 0x324,
    LC_Obj_AuxSupply         = 0x325,
    LC_Obj_ClimateSupply     = 0x326,
    LC_Obj_ACSupply          = 0x327,
    LC_Obj_ACSupply3Ph       = 0x328,
    LC_Obj_SelectedPowerMode = 0x32B,
    LC_Obj_PowerModeIndex    = 0x32C,
    LC_Obj_BatteryCurrents   = 0x32D,
    LC_Obj_BatteryVoltages   = 0x32E,
    LC_Obj_ControlDirection  = 0x32F,
    LC_Obj_PowerModeLimits   = 0x330,
};

// LEVCAN special addresses
static const uint8_t LC_Broadcast_Address = 127;
static const uint8_t LC_Null_Address      = 126;

// ----- LEVCAN Data Structures (little-endian, packed) -----
#pragma pack(push, 1)

struct LC_Supply_t {
    int32_t Voltage_mV;
    int32_t Current_mA;
};

struct LC_Temperature_t {
    int16_t InternalTemp_C;
    int16_t ExternalTemp_C;
    int16_t ExtraTemp1_C;
    int16_t ExtraTemp2_C;
};

struct LC_RPM_t {
    int32_t RPM;
    int32_t ERPM;
};

struct LC_Speed_t {
    int16_t Speed_kph;
};

struct LC_Power_t {
    int32_t Watts;
    int32_t Direction; // 0=idle, 1=charging, 2=discharging
};

struct LC_ThrottleV_t {
    int16_t ThrottleV_mV;
};

struct LC_BrakeV_t {
    int16_t BrakeV_mV;
};

struct LC_ControlFactor_t {
    float Factor; // -1.0 to 1.0
};

struct LC_WhUsed_t {
    int32_t WhUsed;
    int32_t WhUsedFromEn;
};

struct LC_WhStored_t {
    int32_t WhStored;
    int32_t WhTotalStorage;
};

struct LC_Distance_t {
    uint32_t TripMeterFromEn;
    uint16_t TotalTripKm;
};

struct LC_AhUsed_t {
    int32_t mAhUsed;
    int32_t mAhUsedFromEn;
};

struct LC_AhStored_t {
    int32_t mAhStored;
    int32_t mAhTotalStorage;
};

struct LC_ActiveFunctions_t {
    uint32_t FunctionsLow;
    uint32_t FunctionsHigh;
};

struct LC_Buttons_t {
    uint16_t Buttons;
    uint16_t ExtraButtons;
};

struct LC_FOCstate_t {
    float Q;
    float D;
};

struct LC_InternalVoltage_t {
    int16_t Int12V_mV;
    int16_t Int5V_mV;
    int16_t Int3_3V_mV;
    int16_t IntREFV_mV;
};

struct LC_MotorHalls_t {
    int16_t HallA_mV;
    int16_t HallB_mV;
    int16_t HallC_mV;
    uint8_t InputDigital;
    uint8_t HallState;
};

#pragma pack(pop)

// ----- Object name lookup -----
inline const char* lc_obj_name(uint16_t msgId) {
    switch (msgId) {
        case LC_Obj_State:            return "State";
        case LC_Obj_DCSupply:         return "DCSupply";
        case LC_Obj_MotorSupply:      return "MotorSupply";
        case LC_Obj_InternalVoltage:  return "InternalVoltage";
        case LC_Obj_Power:            return "Power";
        case LC_Obj_Temperature:      return "Temperature";
        case LC_Obj_RPM:              return "RPM";
        case LC_Obj_RadSec:           return "RadSec";
        case LC_Obj_Speed:            return "Speed";
        case LC_Obj_ThrottleV:        return "ThrottleV";
        case LC_Obj_BrakeV:           return "BrakeV";
        case LC_Obj_ControlFactor:    return "ControlFactor";
        case LC_Obj_SpeedCommand:     return "SpeedCommand";
        case LC_Obj_TorqueCommand:    return "TorqueCommand";
        case LC_Obj_Buttons:          return "Buttons";
        case LC_Obj_WhUsed:           return "WhUsed";
        case LC_Obj_WhStored:         return "WhStored";
        case LC_Obj_Distance:         return "Distance";
        case LC_Obj_MotorHalls:       return "MotorHalls";
        case LC_Obj_CellsV:           return "CellsV";
        case LC_Obj_CellMinMax:       return "CellMinMax";
        case LC_Obj_CellBalance:      return "CellBalance";
        case LC_Obj_UserActivity:     return "UserActivity";
        case LC_Obj_ActiveFunctions:  return "ActiveFunctions";
        case LC_Obj_LightSensor:      return "LightSensor";
        case LC_Obj_AccelerometerRaw: return "AccelerometerRaw";
        case LC_Obj_Accelerometer:    return "Accelerometer";
        case LC_Obj_ControlFactorInt: return "ControlFactorInt";
        case LC_Obj_DCLimitIFactor:   return "DCLimitIFactor";
        case LC_Obj_DCLimitIValue:    return "DCLimitIValue";
        case LC_Obj_DCLimitVValue:    return "DCLimitVValue";
        case LC_Obj_FOCstateV:        return "FOCstateV";
        case LC_Obj_FOCstateI:        return "FOCstateI";
        case LC_Obj_FOCrequest:       return "FOCrequest";
        case LC_Obj_AhUsed:           return "AhUsed";
        case LC_Obj_AhStored:         return "AhStored";
        case LC_Obj_BatterySupply:    return "BatterySupply";
        case LC_Obj_AuxSupply:        return "AuxSupply";
        case LC_Obj_ClimateSupply:    return "ClimateSupply";
        case LC_Obj_ACSupply:         return "ACSupply";
        case LC_Obj_ACSupply3Ph:      return "ACSupply3Ph";
        case LC_Obj_SelectedPowerMode:return "SelectedPowerMode";
        case LC_Obj_PowerModeIndex:   return "PowerModeIndex";
        case LC_Obj_BatteryCurrents:  return "BatteryCurrents";
        case LC_Obj_BatteryVoltages:  return "BatteryVoltages";
        case LC_Obj_ControlDirection: return "ControlDirection";
        case LC_Obj_PowerModeLimits:  return "PowerModeLimits";
        default:                      return nullptr;
    }
}

#endif // LEVCAN_DEFS_H_

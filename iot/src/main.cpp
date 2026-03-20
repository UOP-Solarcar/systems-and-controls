#include "Arduino.h"
<<<<<<< HEAD
#include "SPI.h"
#include "mcp2515.h"

const uint8_t SPI_CS_PIN = 10;
MCP2515 mcp2515(SPI_CS_PIN);
const uint8_t MONITOR_SPEED = 115200;

void parse_frame() {
  // TODO: add parse frame code here 
}

void setup() {
  Serial.begin(MONITOR_SPEED);
  while (!Serial)
    ;
  Serial.println("Booted");
  SPI.begin();

  mcp2515.reset();
  mcp2515.setBitrate(CAN_500KBPS,
                     MCP_8MHZ); // Sets CAN at speed 500KBPS and Clock 8MHz
  mcp2515.setNormalMode();      // Sets CAN at normal mode
  Serial.begin(MONITOR_SPEED);
  while (!Serial)
    ;
  Serial.println("Booted");
  SPI.begin();

  mcp2515.reset();
  mcp2515.setBitrate(CAN_500KBPS,
                     MCP_8MHZ); // Sets CAN at speed 500KBPS and Clock 8MHz
  mcp2515.setNormalMode();      // Sets CAN at normal mode
}

void loop() {
  can_frame frame{};
  if (mcp2515.readMessage(&frame) == MCP2515::ERROR_OK) {
    parse_frame(frame);
  }
  can_frame frame{};
  if (mcp2515.readMessage(&frame) == MCP2515::ERROR_OK) {
    parse_frame(frame);
  }
}
=======
#include "driver/twai.h"
#include "levcan_defs.h"

// ----- GPIO Configuration -----
// Connect these to your CAN transceiver (e.g., SN65HVD230)
static const gpio_num_t CAN_TX_PIN = GPIO_NUM_5;
static const gpio_num_t CAN_RX_PIN = GPIO_NUM_4;

static const uint32_t MONITOR_SPEED = 115200;

// ----- Decode and print LEVCAN data payloads -----
void decode_levcan_data(uint16_t msgId, const uint8_t *data, uint8_t len) {
    switch (msgId) {
        case LC_Obj_DCSupply:
        case LC_Obj_MotorSupply:
        case LC_Obj_BatterySupply:
        case LC_Obj_AuxSupply:
        case LC_Obj_ClimateSupply:
        case LC_Obj_ACSupply: {
            if (len >= sizeof(LC_Supply_t)) {
                const LC_Supply_t *s = (const LC_Supply_t *)data;
                Serial.printf("  Voltage: %.3f V, Current: %.3f A\n",
                              s->Voltage_mV / 1000.0, s->Current_mA / 1000.0);
            }
            break;
        }
        case LC_Obj_Temperature: {
            if (len >= sizeof(LC_Temperature_t)) {
                const LC_Temperature_t *t = (const LC_Temperature_t *)data;
                Serial.printf("  Internal: %d C, External: %d C, Extra1: %d C, Extra2: %d C\n",
                              t->InternalTemp_C, t->ExternalTemp_C,
                              t->ExtraTemp1_C, t->ExtraTemp2_C);
            }
            break;
        }
        case LC_Obj_RPM: {
            if (len >= sizeof(LC_RPM_t)) {
                const LC_RPM_t *r = (const LC_RPM_t *)data;
                Serial.printf("  RPM: %d, ERPM: %d\n", r->RPM, r->ERPM);
            }
            break;
        }
        case LC_Obj_Speed: {
            if (len >= sizeof(LC_Speed_t)) {
                const LC_Speed_t *s = (const LC_Speed_t *)data;
                Serial.printf("  Speed: %d kph\n", s->Speed_kph);
            }
            break;
        }
        case LC_Obj_Power: {
            if (len >= sizeof(LC_Power_t)) {
                const LC_Power_t *p = (const LC_Power_t *)data;
                const char *dir = p->Direction == 0 ? "Idle" :
                                  p->Direction == 1 ? "Charging" : "Discharging";
                Serial.printf("  Power: %d W (%s)\n", p->Watts, dir);
            }
            break;
        }
        case LC_Obj_ThrottleV: {
            if (len >= sizeof(LC_ThrottleV_t)) {
                const LC_ThrottleV_t *t = (const LC_ThrottleV_t *)data;
                Serial.printf("  Throttle: %d mV\n", t->ThrottleV_mV);
            }
            break;
        }
        case LC_Obj_BrakeV: {
            if (len >= sizeof(LC_BrakeV_t)) {
                const LC_BrakeV_t *b = (const LC_BrakeV_t *)data;
                Serial.printf("  Brake: %d mV\n", b->BrakeV_mV);
            }
            break;
        }
        case LC_Obj_ControlFactor: {
            if (len >= sizeof(LC_ControlFactor_t)) {
                const LC_ControlFactor_t *c = (const LC_ControlFactor_t *)data;
                Serial.printf("  Control Factor: %.4f\n", c->Factor);
            }
            break;
        }
        case LC_Obj_WhUsed: {
            if (len >= sizeof(LC_WhUsed_t)) {
                const LC_WhUsed_t *w = (const LC_WhUsed_t *)data;
                Serial.printf("  Wh Used: %d, Wh From Enable: %d\n",
                              w->WhUsed, w->WhUsedFromEn);
            }
            break;
        }
        case LC_Obj_WhStored: {
            if (len >= sizeof(LC_WhStored_t)) {
                const LC_WhStored_t *w = (const LC_WhStored_t *)data;
                Serial.printf("  Wh Stored: %d / %d\n", w->WhStored, w->WhTotalStorage);
            }
            break;
        }
        case LC_Obj_Distance: {
            if (len >= sizeof(LC_Distance_t)) {
                const LC_Distance_t *d = (const LC_Distance_t *)data;
                Serial.printf("  Trip: %u m, Total: %u km\n",
                              d->TripMeterFromEn, d->TotalTripKm);
            }
            break;
        }
        case LC_Obj_ActiveFunctions: {
            if (len >= sizeof(LC_ActiveFunctions_t)) {
                const LC_ActiveFunctions_t *a = (const LC_ActiveFunctions_t *)data;
                Serial.printf("  Functions: 0x%08X %08X", a->FunctionsHigh, a->FunctionsLow);
                if (a->FunctionsLow & (1 << 0))  Serial.print(" [Enable]");
                if (a->FunctionsLow & (1 << 1))  Serial.print(" [Lock]");
                if (a->FunctionsLow & (1 << 7))  Serial.print(" [Reverse]");
                if (a->FunctionsLow & (1 << 8))  Serial.print(" [Cruise]");
                if (a->FunctionsLow & (1 << 17)) Serial.print(" [MotorWarn]");
                if (a->FunctionsLow & (1 << 18)) Serial.print(" [MotorFail]");
                if (a->FunctionsLow & (1 << 19)) Serial.print(" [CtrlWarn]");
                if (a->FunctionsLow & (1 << 20)) Serial.print(" [CtrlFail]");
                if (a->FunctionsLow & (1 << 21)) Serial.print(" [LowBatt]");
                Serial.println();
            }
            break;
        }
        case LC_Obj_Buttons: {
            if (len >= sizeof(LC_Buttons_t)) {
                const LC_Buttons_t *b = (const LC_Buttons_t *)data;
                Serial.printf("  Buttons: 0x%04X, Extra: 0x%04X\n",
                              b->Buttons, b->ExtraButtons);
            }
            break;
        }
        case LC_Obj_InternalVoltage: {
            if (len >= sizeof(LC_InternalVoltage_t)) {
                const LC_InternalVoltage_t *v = (const LC_InternalVoltage_t *)data;
                Serial.printf("  12V: %d mV, 5V: %d mV, 3.3V: %d mV, REF: %d mV\n",
                              v->Int12V_mV, v->Int5V_mV, v->Int3_3V_mV, v->IntREFV_mV);
            }
            break;
        }
        case LC_Obj_MotorHalls: {
            if (len >= sizeof(LC_MotorHalls_t)) {
                const LC_MotorHalls_t *h = (const LC_MotorHalls_t *)data;
                Serial.printf("  Halls A:%d B:%d C:%d mV, Digital:0x%02X, State:%d\n",
                              h->HallA_mV, h->HallB_mV, h->HallC_mV,
                              h->InputDigital, h->HallState);
            }
            break;
        }
        case LC_Obj_FOCstateV:
        case LC_Obj_FOCstateI:
        case LC_Obj_FOCrequest: {
            if (len >= sizeof(LC_FOCstate_t)) {
                const LC_FOCstate_t *f = (const LC_FOCstate_t *)data;
                Serial.printf("  Q: %.3f, D: %.3f\n", f->Q, f->D);
            }
            break;
        }
        case LC_Obj_AhUsed: {
            if (len >= sizeof(LC_AhUsed_t)) {
                const LC_AhUsed_t *a = (const LC_AhUsed_t *)data;
                Serial.printf("  mAh Used: %d, mAh From Enable: %d\n",
                              a->mAhUsed, a->mAhUsedFromEn);
            }
            break;
        }
        case LC_Obj_AhStored: {
            if (len >= sizeof(LC_AhStored_t)) {
                const LC_AhStored_t *a = (const LC_AhStored_t *)data;
                Serial.printf("  mAh Stored: %d / %d\n", a->mAhStored, a->mAhTotalStorage);
            }
            break;
        }
        case LC_Obj_RadSec: {
            if (len >= sizeof(float)) {
                float radSec;
                memcpy(&radSec, data, sizeof(float));
                Serial.printf("  Rad/s: %.3f\n", radSec);
            }
            break;
        }
        default: {
            Serial.print("  Data:");
            for (int i = 0; i < len; i++) {
                Serial.printf(" %02X", data[i]);
            }
            Serial.println();
            break;
        }
    }
}

#ifndef PIO_UNIT_TESTING
// ----- TWAI (CAN) Setup -----
void setup() {
    Serial.begin(MONITOR_SPEED);
    while (!Serial);
    Serial.println("LEVCAN Bus Reader - Nucular P24F");
    Serial.println("Initializing TWAI (CAN) at 1 Mbps...");

    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(CAN_TX_PIN, CAN_RX_PIN, TWAI_MODE_NORMAL);
    g_config.rx_queue_len = 32;
    g_config.tx_queue_len = 0;

    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_1MBITS();
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    esp_err_t err = twai_driver_install(&g_config, &t_config, &f_config);
    if (err != ESP_OK) {
        Serial.printf("TWAI install failed: 0x%X\n", err);
        while (1);
    }

    err = twai_start();
    if (err != ESP_OK) {
        Serial.printf("TWAI start failed: 0x%X\n", err);
        while (1);
    }

    Serial.println("TWAI started. Listening for LEVCAN frames...");
    Serial.println("-------------------------------------------");
}

void loop() {
    twai_message_t msg;

    esp_err_t err = twai_receive(&msg, pdMS_TO_TICKS(100));
    if (err != ESP_OK) {
        if (err != ESP_ERR_TIMEOUT) {
            Serial.printf("TWAI receive error: 0x%X\n", err);
            twai_status_info_t status;
            if (twai_get_status_info(&status) == ESP_OK) {
                if (status.state == TWAI_STATE_BUS_OFF) {
                    Serial.println("Bus-off detected, recovering...");
                    twai_initiate_recovery();
                    delay(100);
                    twai_start();
                }
            }
        }
        return;
    }

    if (!msg.extd) {
        return;
    }

    LC_Header_t header;
    header.raw = msg.identifier;

    uint16_t msgId  = header.MsgID;
    uint8_t  source = header.Source;
    uint8_t  target = header.Target;

    const char *name = lc_obj_name(msgId);

    if (name) {
        Serial.printf("[%s] Src:%d -> Dst:%d  P:%d EoM:%d RTS:%d  (%d bytes)\n",
                      name, source, target, header.Priority,
                      header.EoM, header.RTS_CTS, msg.data_length_code);
        decode_levcan_data(msgId, msg.data, msg.data_length_code);
    } else {
        Serial.printf("[MsgID:0x%03X] Src:%d -> Dst:%d  P:%d EoM:%d RTS:%d  (%d bytes)\n",
                      msgId, source, target, header.Priority,
                      header.EoM, header.RTS_CTS, msg.data_length_code);
        Serial.print("  Data:");
        for (int i = 0; i < msg.data_length_code; i++) {
            Serial.printf(" %02X", msg.data[i]);
        }
        Serial.println();
    }
}

#endif // PIO_UNIT_TESTING
>>>>>>> 2eca416a47b6b26f1ac17f3f4c3f240c479e6787

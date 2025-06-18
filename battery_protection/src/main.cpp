#include "Arduino.h"
#include "bytetools.cpp"
#include "bytetools.hpp"

#if defined(RPI4B)
#include <cstring>
#include <iostream>
#include <linux/can.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
#else
#include <Arduino.h>
#include <SPI.h>
#include <mcp2515.h> // (https://github.com/autowp/arduino-mcp2515/)
#endif

#if defined(RPI4B)
typedef std::ostream Out;
template <typename T> void print_n(T t, Out &out = std::cout) { out << t; }
void print_n(uint8_t n, Out &out = std::cout) { out << (uint16_t)n; }
void print_n(int8_t n, Out &out = std::cout) { out << (int16_t)n; }
template <typename T> void print_hex(T t, Out &out = std::cout) {
  out << std::hex << t << std::dec;
}
void println(Out &out = std::cout) { out << std::endl; }
#else
typedef Print Out;
template <typename T> void print_n(T t, Out &out = Serial) { out.print(t); }
template <typename T> void print_hex(T t, Out &out = Serial) {
  out.print(t, HEX);
}
void println(Out &out = Serial) { out.println(); }
#endif

class Relay {

public:

    enum Polarity : uint8_t { ACTIVE_HIGH, ACTIVE_LOW };

    Relay(uint8_t pin, Polarity pol = ACTIVE_HIGH)
        : _pin(pin), _pol(pol) {}

    void begin() {
        pinMode(_pin, OUTPUT);
        digitalWrite(_pin, _inactiveLevel());
    }

    void close()  { _write(true);  }
    void open()   { _write(false); }
    void toggle() { _write(!_closed); }

    bool isClosed() const { return _closed; }
    bool isOpen()   const { return !_closed; }
    unsigned long lastChange() const { return _tLast; }

private:

    const uint8_t  _pin;
    const Polarity _pol;

    bool           _closed  = false;
    unsigned long  _tLast   = 0;

    uint8_t _activeLevel()   const { return _pol == ACTIVE_LOW ? LOW : HIGH;  }
    uint8_t _inactiveLevel() const { return _pol == ACTIVE_LOW ? HIGH  : LOW; }

    void _write(bool wantClosed) {
        if (wantClosed == _closed) return;
        digitalWrite(_pin, wantClosed ? _activeLevel() : _inactiveLevel());
        _closed = wantClosed;
        _tLast  = millis();
    }

};

class Button {
public:
  enum Mode : uint8_t { PULLUP, PULLDOWN };
  enum Behavior : uint8_t { MOMENTARY, TOGGLE };

  Button(uint8_t  pin,
         Mode     mode      = PULLUP,
         Behavior behavior  = MOMENTARY,
         uint16_t debounce  = 20,
         bool     initLatch = false)
      : _pin(pin),
        _mode(mode),
        _behavior(behavior),
        _debounce(debounce),
        _latched(initLatch) {}

  void begin() {
    pinMode(_pin, _mode == PULLUP ? INPUT_PULLUP : INPUT);
    _raw         = _read();
    _stable      = _raw;
    _lastChange  = millis();
  }

  
  void update() {
    bool now = _read();

    if (now != _raw) {
      _raw        = now;
      _lastChange = millis();
    }

    if ((millis() - _lastChange) >= _debounce && now != _stable) {
      _stable = now;
      _fell   =  _pressed(_stable);
      _rose   = !_fell;

      if (_behavior == TOGGLE && _fell)
        _latched = !_latched;
    } else {
      _fell = _rose = false;
    }
  }

  // ---------- queries ----------
  bool fell()       const { return _fell; }
  bool rose()       const { return _rose; }
  bool isPressed()  const { return _pressed(_stable); }
  bool isReleased() const { return !_pressed(_stable); }


  bool latched()    const { return _latched; }
  operator bool()   const { return _latched; }

private:

  inline bool _read()   const { return digitalRead(_pin); }
  inline bool _pressed(bool level) const {
    return _mode == PULLUP ? !level : level;
  }

  uint8_t  _pin;
  Mode     _mode;
  Behavior _behavior;
  uint16_t _debounce;

  bool     _raw        = false;
  bool     _stable     = false;
  uint32_t _lastChange = 0;
  bool     _fell       = false;
  bool     _rose       = false;
  bool     _latched    = false;
};

/*
Cases that trip bps:
Estop
overvoltage and overcurrent 
100A
-20A Charging current
Any module over 4.2 volts
Any module under 2.5 volts
*/
bool parse_frame(can_frame &frame, Out &out =
#if defined(RPI4B)
                                       std::cout
#else
                                       Serial
#endif
) {
  switch (frame.can_id) {
    case 0x6B0: {
      int16_t pack_current = bytetools::int_bswap(*(int16_t *)&frame.data[0]);
      uint16_t pack_inst_voltage =
          bytetools::int_bswap(*(uint16_t *)&frame.data[2]);
      uint8_t pack_soc = frame.data[4];
      uint16_t relay_state = bytetools::int_bswap(*(uint16_t *)&frame.data[5]);
      uint8_t checksum = frame.data[7];

      if (pack_current < -20){
        Serial.println("Exceeding Maximum Charging Current");
        return true;
      }
      if (pack_current > 100){
        Serial.println("Exceeding Maximum Discharge Current");
        return true;
      }
    } return false;
    case 0x6B1: {
      uint16_t pack_dcl = bytetools::int_bswap(*(uint16_t *)&frame.data[0]),
               pack_ccl = bytetools::int_bswap(*(uint16_t *)&frame.data[2]);
      uint8_t high_temp = frame.data[4], low_temp = frame.data[5],
              checksum = frame.data[7];
    } return false;
    case 0x6B2: {
      uint16_t high_cell_voltage =
          bytetools::int_bswap(*(uint16_t *)&frame.data[0]);
      uint8_t high_cell_voltage_id = frame.data[2];
      uint16_t low_cell_voltage =
          bytetools::int_bswap(*(uint16_t *)&frame.data[3]);
      uint8_t low_cell_voltage_id = frame.data[5], checksum = frame.data[6];
      if (high_cell_voltage > 4.2){
        Serial.println("Cell Overvoltage");
        return true;
      }
      if (low_cell_voltage <= 2.5){
        Serial.println("Cell Undervoltage");
        return true;
      }
    } return false;
    case 0x6B3: {
      uint8_t high_temp = frame.data[0], high_thermistor_id = frame.data[1],
              low_temp = frame.data[2], low_thermistor_id = frame.data[3],
              avg_temp = frame.data[4], internal_temp = frame.data[5],
              checksum = frame.data[6];
    } return false;
    case 0x6B4: {
      uint8_t pack_health = frame.data[0];
      uint16_t adaptive_total_capacity =
                   bytetools::int_bswap(*(uint16_t *)&frame.data[3]),
               input_supply_voltage =
                   bytetools::int_bswap(*(uint16_t *)&frame.data[5]);
      uint8_t checksum = frame.data[7];
    } return false;
    case 0x36: {
      uint8_t cell_id = frame.data[0];
      uint16_t instant_voltage =
                   bytetools::int_bswap(*(uint16_t *)&frame.data[1]),
               internal_resistance =
                   bytetools::int_bswap(*(uint16_t *)&frame.data[3]),
               open_voltage = frame.data[5];
      uint8_t checksum = frame.data[7];
    } return false;
    default:
      return false;
  }
}

MCP2515 mcp2515(10);

Relay contactorPrecharge(4, Relay::ACTIVE_HIGH);
Relay contactorPositive(5, Relay::ACTIVE_HIGH);
Relay contactorNegative(6, Relay::ACTIVE_HIGH);
Relay faultIndicator(7, Relay::ACTIVE_HIGH);

constexpr uint8_t ESTOP_PIN = 3;
constexpr uint16_t ESTOP_RESET_DEBOUNCE = 1000;
static unsigned long estopLowSince = 0;

volatile bool estopTripped = false;
bool bpsFaultState = false;
bool onOffState {};


void eStopISR() {
  estopTripped = true;
}

enum StartupState { IDLE, PRECHARGING, WAITING, RUNNING } state = IDLE;
unsigned long stateT0 = 0;

const unsigned long INTERVAL = 5000;
int fanSpeed;
int temperature;
unsigned long now;
unsigned long prevReadTime = 0;
int fanCurve[10] = {33, 35, 37, 39, 41, 43, 45, 47, 49, 51};

const byte OC1A_PIN = 9;
const byte OC1B_PIN = 10;

const word PWM_FREQ_HZ = 25000; //Adjust this value to adjust the frequency
const word TCNT1_TOP = 16000000/(2*PWM_FREQ_HZ);

void setPwmDuty(byte duty) {
  OCR1A = (word) (duty*TCNT1_TOP)/100;
}

int getTemperature() {
  //somehow get temperature
  return 36;
}

void setup() {
  pinMode(OC1A_PIN, OUTPUT);

  // Clear Timer1 control and count registers
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0;

  // Set Timer1 configuration
  // COM1A(1:0) = 0b10   (Output A clear rising/set falling)
  // COM1B(1:0) = 0b00   (Output B normal operation)
  // WGM(13:10) = 0b1010 (Phase correct PWM)
  // ICNC1      = 0b0    (Input capture noise canceler disabled)
  // ICES1      = 0b0    (Input capture edge select disabled)
  // CS(12:10)  = 0b001  (Input clock select = clock/1)
  
  TCCR1A |= (1 << COM1A1) | (1 << WGM11);
  TCCR1B |= (1 << WGM13) | (1 << CS10);
  ICR1 = TCNT1_TOP;

  
  // initalize relays
  contactorPrecharge.begin();
  contactorPositive.begin();
  contactorNegative.begin();
  faultIndicator.begin();

  // attach estop isr
  pinMode(ESTOP_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ESTOP_PIN), eStopISR, RISING);
  

  Serial.begin(115200);
  while (!Serial);
  Serial.println("Booted");
  SPI.begin();

  mcp2515.reset();
  mcp2515.setBitrate(CAN_500KBPS,
                     MCP_8MHZ); // Sets CAN at speed 500KBPS and Clock 8MHz
  mcp2515.setNormalMode();      // Sets CAN at normal mode
}

void loop() {

  if (estopTripped) {
    contactorPositive.open();
    contactorNegative.open();
    bpsFaultState = true;
    estopTripped = false;
    Serial.println("E-Stop Tripped");

  }

  static unsigned long ledT0 = 0;
  
  can_frame frame{};
  if (mcp2515.readMessage(&frame) == MCP2515::ERROR_OK) {
    bpsFaultState = parse_frame(frame);
  }

  if (bpsFaultState) {


      if (millis() - ledT0 >= 500) {
          faultIndicator.toggle();
          ledT0 = millis();
      }

      if (digitalRead(ESTOP_PIN) == LOW) {
          if (estopLowSince == 0) {
              estopLowSince = millis();
          } else if (millis() - estopLowSince > ESTOP_RESET_DEBOUNCE) {

              bpsFaultState = false;
              estopLowSince = 0;
              faultIndicator.open();


              contactorPositive.open();
              contactorNegative.open();
              contactorPrecharge.open();

              state = IDLE;
              Serial.println(F("E-Stop reset – back to IDLE"));
          }
      } else {
          estopLowSince = 0;
      }
      return;
  } else {
      faultIndicator.open();
  }

  if (now - prevReadTime >= INTERVAL){
    prevReadTime = now;
    int temp = getTemperature();
    fanSpeed = 0;
    for (int i = 0; i < 10; i++){
      if (temp > fanCurve[i]){
        fanSpeed = fanSpeed + 10;
      }
    }
    setPwmDuty(0);
    delay(5000);
    setPwmDuty(fanSpeed); //Change this value 0-100 to adjust duty cycle
  }

  switch (state) {


    case IDLE:

      if (contactorPrecharge.isOpen() &&
          contactorNegative.isOpen() && 
          contactorPositive.isOpen() && 
          !bpsFaultState)
      {
      
        state = PRECHARGING;
      }

      break;

    case PRECHARGING:

      contactorPrecharge.close();
      Serial.println("Precharge Contactor Closed");

      contactorNegative.close();
      Serial.println("Negative Contactor Closed");

      stateT0 = millis();

      state = WAITING;

    case WAITING:

      if (millis() - stateT0 >= 3000) {
        contactorPositive.close();
        Serial.println("Positive Contactor Closed");

        contactorPrecharge.open();
        Serial.println("Precharge Contactor Opened");
        
        Serial.println("Ready");
        state = RUNNING;
      }

      break;

    case RUNNING:      

      break;
  }
}

#include "Arduino.h"

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

    uint8_t _activeLevel()   const { return _pol == ACTIVE_HIGH ? HIGH : LOW;  }
    uint8_t _inactiveLevel() const { return _pol == ACTIVE_HIGH ? LOW  : HIGH; }

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

Relay contactorPrecharge(4, Relay::ACTIVE_LOW);
Relay contactorPositive(5, Relay::ACTIVE_LOW);
Relay contactorNegative(6, Relay::ACTIVE_LOW);
Relay faultIndicator(7, Relay::ACTIVE_LOW);

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

void setup() {
  
  // initalize relays
  contactorPrecharge.begin();
  contactorPositive.begin();
  contactorNegative.begin();
  faultIndicator.begin();

  // attach estop isr
  pinMode(ESTOP_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ESTOP_PIN), eStopISR, RISING);
  

  Serial.begin(115200);

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

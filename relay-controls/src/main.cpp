#include <Arduino.h>

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

Relay headlights(A5, Relay::ACTIVE_LOW);
Relay leftTurn(A1, Relay::ACTIVE_LOW);
Relay rightTurn(A2, Relay::ACTIVE_LOW);
Relay brakesLights(A3, Relay::ACTIVE_LOW);
Relay motorController(A4, Relay::ACTIVE_LOW);
Relay direction(A6, Relay::ACTIVE_LOW);
Relay rightRear(11, Relay::ACTIVE_LOW);
Relay leftRear(12, Relay::ACTIVE_LOW);

Button leftSignal(5, Button::PULLUP, Button::TOGGLE, 10);
Button comms(4, Button::PULLUP, Button::TOGGLE, 10);
Button hazardBtn(3, Button::PULLUP, Button::TOGGLE, 10);
Button rightSignal(2, Button::PULLUP, Button::TOGGLE, 10);
Button directionToggle(6, Button::PULLUP, Button::TOGGLE, 10);
Button headlightsBtn(7, Button::PULLUP, Button::TOGGLE, 10);
//Button brakesLightsBtn(8, Button::PULLUP, Button::TOGGLE, 10);
Button motorToggle(8, Button::PULLUP, Button::TOGGLE, 10);
Button brakeSignal(9, Button::PULLUP, Button::MOMENTARY, 10);

void setup() {
  //Serial.begin(115200);

  hazardBtn.begin();
  leftSignal.begin();
  rightSignal.begin();
  headlightsBtn.begin();
  comms.begin();
  directionToggle.begin();
  motorToggle.begin();
  brakeSignal.begin();

  headlights.begin();
  leftTurn.begin();
  rightTurn.begin();
  brakesLights.begin();
  motorController.begin();
  direction.begin();
  rightRear.begin();
  leftRear.begin();
}


void loop() {

  hazardBtn.update();
  leftSignal.update();
  rightSignal.update();
  headlightsBtn.update();
  comms.update();
  directionToggle.update();
  motorToggle.update();
  brakeSignal.update();

  static unsigned long flasherT0 = 0;
  const unsigned long interval = 300;          // 300 ms ≈ 1.7 Hz

  if (motorToggle) {
    motorController.close();
  } else {
    if (motorController.isClosed()) motorController.open();
  }

  if (directionToggle) {
    direction.close();
  } else {
    if (direction.isClosed()) direction.open();
  }

  if (hazardBtn) {// hazards ON (latched)
    Serial.println("hazards");
    if (millis() - flasherT0 >= interval) {    // time to flip?
      leftTurn.toggle();
      rightTurn.toggle();
      leftRear.toggle();
      rightRear.toggle();
      flasherT0 = millis();                    // reset timer
    }
  } else {                                     // hazards OFF
    if (leftTurn.isClosed())  leftTurn.open(); // make sure both lamps off
    if (rightTurn.isClosed()) rightTurn.open();
  }

  if (leftSignal) {
    Serial.println("left signal");
    if (millis() - flasherT0 >= interval) {
      leftTurn.toggle();
    }
  } else {
    if (leftTurn.isClosed()) leftTurn.open();
  }

  if (rightSignal) {
    Serial.println("right signal");
    if (millis() - flasherT0 >= interval) {
      rightTurn.toggle();
    }
  } else {
    if (rightTurn.isClosed()) rightTurn.open();
  }

  if (headlightsBtn) {
    Serial.println("headlights");
    headlights.close();
  } else {
    if (headlights.isClosed()) headlights.open();
  }

  if (brakeSignal) {
    Serial.println("brake");
    brakesLights.close();
    if (!leftSignal){
      leftRear.open();
    } if (!rightSignal) {
      rightRear.open();
    }
  }
}


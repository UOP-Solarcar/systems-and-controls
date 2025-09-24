#include "Button.h"

Button::Button(uint8_t  pin,
               Mode     mode,
               Behavior behavior,
               uint16_t debounce,
               bool     initLatch)
    : _pin(pin),
      _mode(mode),
      _behavior(behavior),
      _debounce(debounce),
      _latched(initLatch) {}

void Button::begin() {
    pinMode(_pin, _mode == PULLUP ? INPUT_PULLUP : INPUT);
    _raw         = _read();
    _stable      = _raw;
    _lastChange  = millis();
}

void Button::update() {
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

bool Button::fell() const {
    return _fell;
}

bool Button::rose() const {
    return _rose;
}

bool Button::isPressed() const {
    return _pressed(_stable);
}

bool Button::isReleased() const {
    return !_pressed(_stable);
}

bool Button::latched() const {
    return _latched;
}

Button::operator bool() const {
    return _latched;
}

inline bool Button::_read() const {
    return digitalRead(_pin);
}

inline bool Button::_pressed(bool level) const {
    return _mode == PULLUP ? !level : level;
}

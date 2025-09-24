#include "Relay.h"

Relay::Relay(uint8_t pin, Polarity pol)
    : _pin(pin), _pol(pol) {}

void Relay::begin() {
    pinMode(_pin, OUTPUT);
    digitalWrite(_pin, _inactiveLevel());
}

void Relay::close() {
    _write(true);
}

void Relay::open() {
    _write(false);
}

void Relay::toggle() {
    _write(!_closed);
}

bool Relay::isClosed() const {
    return _closed;
}

bool Relay::isOpen() const {
    return !_closed;
}

unsigned long Relay::lastChange() const {
    return _tLast;
}

uint8_t Relay::_activeLevel() const {
    return _pol == ACTIVE_HIGH ? HIGH : LOW;
}

uint8_t Relay::_inactiveLevel() const {
    return _pol == ACTIVE_HIGH ? LOW : HIGH;
}

void Relay::_write(bool wantClosed) {
    if (wantClosed == _closed) return;
    digitalWrite(_pin, wantClosed ? _activeLevel() : _inactiveLevel());
    _closed = wantClosed;
    _tLast = millis();
}

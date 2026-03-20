#ifndef RELAY_H
#define RELAY_H

#include <Arduino.h>

class Relay {
public:
    enum Polarity : uint8_t { ACTIVE_HIGH, ACTIVE_LOW };

    Relay(uint8_t pin, Polarity pol = ACTIVE_HIGH);

    void begin();
    void close();
    void open();
    void toggle();

    bool isClosed() const;
    bool isOpen() const;
    unsigned long lastChange() const;

private:
    const uint8_t  _pin;
    const Polarity _pol;

    bool           _closed  = false;
    unsigned long  _tLast   = 0;

    uint8_t _activeLevel() const;
    uint8_t _inactiveLevel() const;
    void _write(bool wantClosed);
};

#endif // RELAY_H

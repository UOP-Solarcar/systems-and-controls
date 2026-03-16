#ifndef BUTTON_H
#define BUTTON_H

#include <Arduino.h>

class Button {
public:
    enum Mode : uint8_t { PULLUP, PULLDOWN };
    enum Behavior : uint8_t { MOMENTARY, TOGGLE };

    Button(uint8_t  pin,
           Mode     mode      = PULLUP,
           Behavior behavior  = MOMENTARY,
           uint16_t debounce  = 20,
           bool     initLatch = false);

    void begin();
    void update();

    // Queries
    bool fell() const;
    bool rose() const;
    bool isPressed() const;
    bool isReleased() const;
    bool latched() const;
    
    operator bool() const;

private:
    inline bool _read() const;
    inline bool _pressed(bool level) const;

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

#endif // BUTTON_H

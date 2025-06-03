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

//States
bool daylights = true;
bool headlights = false;
bool left = false;
bool right = false;
bool braking = false;
bool hazards = false;


//Pins are just filler numbers
Relay daylightRunning(4, Relay::ACTIVE_LOW);
Relay headlight(5, Relay::ACTIVE_LOW);
Relay leftTurn(6, Relay::ACTIVE_LOW);
Relay rightTurn(7, Relay::ACTIVE_LOW);
Relay brakeMiddle(8, Relay::ACTIVE_LOW);
Relay brakeLeft(9, Relay::ACTIVE_LOW);
Relay brakeRight(10, Relay::ACTIVE_LOW);


void controlLights(){
    if (daylights != daylightRunning.isOpen()){
        if (daylights){
            daylightRunning.open();
        }
        else {
            daylightRunning.close();
        }
    }
    if (headlights != headlight.isOpen()){
        if (headlights){
            headlight.open();
        }
        else {
            headlight.close();
        }
    }
    if (braking){
        if (braking != brakeMiddle.isOpen()){
            if (braking){
                brakeMiddle.open();
            }
            else {
                brakeMiddle.close();
            }
        }
        if (!left){
            brakeLeft.open();
        }
        if (!right){
            brakeRight.open();
        }
    }
    else {
        if (left || hazards){
            brakeLeft.toggle();
        }
        if (right || hazards){
            brakeRight.toggle();
        }
    }
}

void setup() {
    //setup
}

void loop() {
    //add stuff to input current state
    controlLights();
}


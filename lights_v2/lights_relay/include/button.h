/*** button.h
 *
 * Button abstraction: converts a raw "is this pin currently active" signal
 * into either a momentary state (on == pressed) or a toggled state (on
 * flips on each rising edge of pressed), depending on the toggle flag.
 *
***/

#pragma once
#include <stdint.h>

struct Button {
  uint8_t pin;
  bool toggle;
  bool on = false;
  bool last = false;

  Button(uint8_t pin, bool toggle) : pin(pin), toggle(toggle) {}

  inline bool update(bool pressed) {
    if (toggle && pressed && pressed != last) {
      on = !on;
    }
    else if (!toggle) {
      on = pressed;
    }
    last = pressed;
    return on;
  }
};

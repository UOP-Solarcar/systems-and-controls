/*** input_decoder.h
 *
 * Mirrors raw per-pin CAN data (whichever bytes are non-zero) into inputState,
 * so inputState always reflects whether each pin is *currently* pulled high.
 * Toggle vs. momentary interpretation of that raw signal is Button's job
 * (see button.h) - this layer must not also latch/toggle it, or a toggle
 * Button ends up double-toggled and needs two presses to change state.
 *
 * Returns a bitmask of pins that just saw a rising edge (0x00 -> non-zero),
 * i.e. a new button press, so callers can log/react to it.
 *
***/

#pragma once
#include <stdint.h>
#include "bitset.h"

inline uint8_t updateInputState(Bitset &state, uint8_t *prevData, const uint8_t *data, uint8_t len) {
  uint8_t pressedMask = 0;
  for (uint8_t i = 0; i < len; i++) {
    if (data[i] != 0x00) {
      state.set(i);
      if (prevData[i] == 0x00) pressedMask |= (1U << i);
    } else {
      state.reset(i);
    }
    prevData[i] = data[i];
  }
  return pressedMask;
}

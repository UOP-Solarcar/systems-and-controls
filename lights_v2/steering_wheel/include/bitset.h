/*** bitset.h 
 * Boilerplate code for bitset operations since it isn't included in AVR-GCC
***/ 

#pragma once
#include <stdint.h>

struct Bitset {
  uint8_t x;

  void set(int i)       { x |= (1U << i); }
  void reset(int i)     { x &= ~(1U << i); }
  void flip(int i)      { x ^= (1U << i); }
  bool test(int i)      { return (x >> i) & 1U; }
  bool any()            { return x != 0; }
  bool none()           { return x == 0; }
  bool all()            { return x == 0xFF; }

  uint8_t count() {
    uint8_t tmp = x;
    uint8_t ct = 0;
    while (tmp) {
      tmp &= (tmp - 1);
      ct++;
    }
    return ct;
  }

  Bitset(uint8_t val = 0) : x(val) {}

};

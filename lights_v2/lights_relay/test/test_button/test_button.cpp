#include <unity.h>
#include "bitset.h"
#include "input_decoder.h"
#include "button.h"

void setUp(void) {}
void tearDown(void) {}

// Drives one full CAN press-then-release cycle for `bit`, mirroring how loop()
// calls updateInputState() then Button::update() on every incoming CAN frame.
// Returns the relay state observed while the button is held down.
static bool simulatePress(Bitset &state, uint8_t *prevData, Button &button, uint8_t bit) {
  uint8_t down[8] = {0};
  down[bit] = 0x01;
  updateInputState(state, prevData, down, 8);
  bool onWhileHeld = button.update(state.test(bit));

  uint8_t up[8] = {0};
  updateInputState(state, prevData, up, 8);
  button.update(state.test(bit));

  return onWhileHeld;
}

// A toggle button should flip its relay on every single press: on, off, on, ...
void test_toggle_button_alternates_every_press(void) {
    Bitset state{0};
    uint8_t prevData[8] = {0};
    Button headlights(1, true);

    TEST_ASSERT_TRUE_MESSAGE(simulatePress(state, prevData, headlights, 1),
        "1st press: relay should turn ON");
    TEST_ASSERT_FALSE_MESSAGE(simulatePress(state, prevData, headlights, 1),
        "2nd press: relay should turn OFF");
    TEST_ASSERT_TRUE_MESSAGE(simulatePress(state, prevData, headlights, 1),
        "3rd press: relay should turn back ON");
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_toggle_button_alternates_every_press);
    return UNITY_END();
}

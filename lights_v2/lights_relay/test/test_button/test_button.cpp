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

// A momentary button (horn, brake) must stay closed for the *entire* time its
// pin is held, i.e. across every CAN frame the steering wheel sends while the
// button is down - not just the rising-edge frame. Regression guard for the
// steering wheel transmitting held state (level) instead of only the edge.
void test_momentary_button_held_across_frames(void) {
    Bitset state{0};
    uint8_t prevData[8] = {0};
    Button horn(3, false);

    // Steering wheel sends the pin number on every frame while it is held.
    uint8_t down[8] = {0};
    down[3] = 0x05;
    for (uint8_t frame = 0; frame < 5; frame++) {
        updateInputState(state, prevData, down, 8);
        TEST_ASSERT_TRUE_MESSAGE(horn.update(state.test(3)),
            "held frame: momentary relay should stay CLOSED");
    }

    // Releasing the button (zeroed frames) must open the relay.
    uint8_t up[8] = {0};
    updateInputState(state, prevData, up, 8);
    TEST_ASSERT_FALSE_MESSAGE(horn.update(state.test(3)),
        "release: momentary relay should OPEN");
}

// A toggle button must still register exactly one flip when the press spans
// many held frames (level-triggered), and must not double-toggle mid-hold.
void test_toggle_button_flips_once_per_held_press(void) {
    Bitset state{0};
    uint8_t prevData[8] = {0};
    Button leftSignal(7, true);

    uint8_t down[8] = {0};
    down[7] = 0x09;
    bool on = false;
    for (uint8_t frame = 0; frame < 5; frame++) {
        updateInputState(state, prevData, down, 8);
        on = leftSignal.update(state.test(7));
    }
    TEST_ASSERT_TRUE_MESSAGE(on,
        "held press should latch ON exactly once, not oscillate");

    uint8_t up[8] = {0};
    updateInputState(state, prevData, up, 8);
    on = leftSignal.update(state.test(7));
    TEST_ASSERT_TRUE_MESSAGE(on,
        "releasing a toggle button must not change the latched state");
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_toggle_button_alternates_every_press);
    RUN_TEST(test_momentary_button_held_across_frames);
    RUN_TEST(test_toggle_button_flips_once_per_held_press);
    return UNITY_END();
}

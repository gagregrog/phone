#include <unity.h>
#include "Arduino.h"
#include "hardware/HandsetMonitor.h"
#include "hardware/pins.h"

static HandsetMonitor* handset;
static bool lastState;
static int callCount;

static void advance(uint32_t ms) {
    _mock_millis += ms;
    handset->update();
}

void setUp(void) {
    _mock_millis = 0;
    _mock_pin_values[PIN_HANDSET] = HIGH; // on hook at rest (pullup)
    lastState = false;
    callCount = 0;
    handset = new HandsetMonitor(PIN_HANDSET);
    handset->begin();
    handset->setOnChange([](bool offHook) { lastState = offHook; callCount++; });
}

void tearDown(void) {
    delete handset;
}

// --- Tests ---

void test_initial_state_on_hook(void) {
    TEST_ASSERT_FALSE(handset->isOffHook());
}

void test_initial_state_off_hook(void) {
    delete handset;
    _mock_pin_values[PIN_HANDSET] = LOW;
    handset = new HandsetMonitor(PIN_HANDSET);
    handset->begin();
    handset->setOnChange([](bool offHook) { lastState = offHook; callCount++; });
    TEST_ASSERT_TRUE(handset->isOffHook());
}

void test_lift_handset_fires_callback(void) {
    _mock_pin_values[PIN_HANDSET] = LOW; // handset lifted
    advance(1);  // raw change detected
    advance(50); // debounce passes
    TEST_ASSERT_EQUAL(1, callCount);
    TEST_ASSERT_TRUE(lastState);
    TEST_ASSERT_TRUE(handset->isOffHook());
}

void test_replace_handset_fires_callback(void) {
    // Start off hook
    _mock_pin_values[PIN_HANDSET] = LOW;
    advance(1); advance(50);
    TEST_ASSERT_EQUAL(1, callCount);

    // Replace handset
    _mock_pin_values[PIN_HANDSET] = HIGH;
    advance(1);  // raw change detected
    advance(50); // debounce passes
    TEST_ASSERT_EQUAL(2, callCount);
    TEST_ASSERT_FALSE(lastState);
    TEST_ASSERT_FALSE(handset->isOffHook());
}

void test_debounce_glitch_ignored(void) {
    // Brief LOW glitch (less than 50ms) should not trigger callback
    _mock_pin_values[PIN_HANDSET] = LOW;
    advance(1);  // raw change detected, debounce timer starts

    _mock_pin_values[PIN_HANDSET] = HIGH; // bounces back before settling
    advance(30); // less than debounceMs — still unsettled
    advance(50); // settles as HIGH
    TEST_ASSERT_EQUAL(0, callCount);
    TEST_ASSERT_FALSE(handset->isOffHook());
}

void test_callback_not_fired_before_debounce(void) {
    _mock_pin_values[PIN_HANDSET] = LOW;
    advance(1);  // raw change detected
    advance(49); // 1ms short of debounce
    TEST_ASSERT_EQUAL(0, callCount);

    advance(1);  // now at exactly 50ms
    TEST_ASSERT_EQUAL(1, callCount);
}

void test_callback_fires_once_per_transition(void) {
    _mock_pin_values[PIN_HANDSET] = LOW;
    advance(1); advance(50);
    TEST_ASSERT_EQUAL(1, callCount);

    // Many more ticks — should not fire again
    for (int i = 0; i < 20; i++) advance(10);
    TEST_ASSERT_EQUAL(1, callCount);
}

void test_no_crash_without_callback(void) {
    delete handset;
    handset = new HandsetMonitor(PIN_HANDSET);
    handset->begin();
    // No setOnChange — should not crash on state change
    _mock_pin_values[PIN_HANDSET] = LOW;
    advance(1); advance(50);
    TEST_PASS();
}

int main(int argc, char** argv) {
    UNITY_BEGIN();

    RUN_TEST(test_initial_state_on_hook);
    RUN_TEST(test_initial_state_off_hook);
    RUN_TEST(test_lift_handset_fires_callback);
    RUN_TEST(test_replace_handset_fires_callback);
    RUN_TEST(test_debounce_glitch_ignored);
    RUN_TEST(test_callback_not_fired_before_debounce);
    RUN_TEST(test_callback_fires_once_per_transition);
    RUN_TEST(test_no_crash_without_callback);

    return UNITY_END();
}

#include <unity.h>
#include "Arduino.h"
#include "hardware/DialManager.h"
#include "hardware/DialReader.h"
#include "hardware/HandsetMonitor.h"
#include "hardware/pins.h"

static DialReader*     dial;
static HandsetMonitor* handset;
static DialManager*    mgr;

// --- Helpers ---

static void advance(uint32_t ms) {
    _mock_millis += ms;
    handset->update();
    dial->tick();
    mgr->tick();
}

// Lift the handset (active low: LOW = off hook), debounce is 50ms.
static void pickUp() {
    _mock_pin_values[PIN_HANDSET] = LOW;
    advance(1);  // raw change detected; _lastChangeTime set
    advance(50); // debounce settles
}

// Return handset to cradle.
static void hangUp() {
    _mock_pin_values[PIN_HANDSET] = HIGH;
    advance(1);  // raw change detected; _lastChangeTime set
    advance(50); // debounce settles
}

// Simulate a complete rotary dial of one digit.
// Mirrors real rotary-dial timing: ~10 pulses/sec, 150ms settle.
static void dialDigit(int digit) {
    int pulses = (digit == 0) ? 10 : digit;

    _mock_pin_values[PIN_DIAL_ACTIVE] = LOW;
    advance(1);
    advance(20);  // DIAL_DEBOUNCE_MS → _dialActive = true, DIALING

    for (int i = 0; i < pulses; i++) {
        _mock_pin_values[PIN_DIAL_PULSE] = LOW;
        advance(1);
        advance(5);   // PULSE_DEBOUNCE_MS
        _mock_pin_values[PIN_DIAL_PULSE] = HIGH;
        advance(1);
        advance(40);  // inter-pulse gap
    }

    _mock_pin_values[PIN_DIAL_ACTIVE] = HIGH;
    advance(1);
    advance(20);  // debounce → DONE

    advance(150); // SETTLE_MS → digit callback fires
}

// --- Fixtures ---

void setUp(void) {
    _mock_millis = 0;
    _mock_pin_values[PIN_DIAL_ACTIVE] = HIGH;
    _mock_pin_values[PIN_DIAL_PULSE]  = HIGH;
    _mock_pin_values[PIN_HANDSET]     = HIGH; // on-hook

    dial    = new DialReader();
    handset = new HandsetMonitor(PIN_HANDSET);
    mgr     = new DialManager(*dial, *handset);

    dial->begin();
    handset->begin();
    mgr->begin();
}

void tearDown(void) {
    delete mgr;
    delete handset;
    delete dial;
}

// --- Tests ---

void test_no_digits_when_on_hook(void) {
    int digitCount = 0;
    mgr->addOnDigit([&digitCount](int, const char*) { digitCount++; });

    // Handset is on-hook; dial a digit — callback must not fire
    dialDigit(3);

    TEST_ASSERT_EQUAL(0, digitCount);
    TEST_ASSERT_EQUAL_STRING("", mgr->number());
}

void test_digits_accumulate_off_hook(void) {
    pickUp();
    TEST_ASSERT_TRUE(mgr->isOffHook());

    dialDigit(4);
    dialDigit(2);
    dialDigit(1);

    TEST_ASSERT_EQUAL_STRING("421", mgr->number());
}

void test_clear_on_hang_up(void) {
    pickUp();
    dialDigit(5);
    TEST_ASSERT_EQUAL_STRING("5", mgr->number());

    int clearCount = 0;
    mgr->addOnClear([&clearCount]() { clearCount++; });

    hangUp();

    TEST_ASSERT_FALSE(mgr->isOffHook());
    TEST_ASSERT_EQUAL(1, clearCount);
    TEST_ASSERT_EQUAL_STRING("", mgr->number());
}

void test_fresh_start_on_pickup(void) {
    // Accumulate digits during first off-hook session
    pickUp();
    dialDigit(7);
    TEST_ASSERT_EQUAL_STRING("7", mgr->number());

    int clearCount = 0;
    mgr->addOnClear([&clearCount]() { clearCount++; });

    // Hang up: clear fires, number reset
    hangUp();
    TEST_ASSERT_EQUAL(1, clearCount);
    TEST_ASSERT_EQUAL_STRING("", mgr->number());

    // Pick up again: number reset silently, no second onClear
    pickUp();
    TEST_ASSERT_TRUE(mgr->isOffHook());
    TEST_ASSERT_EQUAL(1, clearCount); // onClear not fired again
    TEST_ASSERT_EQUAL_STRING("", mgr->number());
}

void test_dial_start_suppressed_on_hook(void) {
    int startCount = 0;
    mgr->addOnDialStart([&startCount]() { startCount++; });

    // Handset on-hook: start dial rotation
    _mock_pin_values[PIN_DIAL_ACTIVE] = LOW;
    advance(1);
    advance(20); // IDLE→DIALING in DialReader; DialManager blocks because on-hook

    TEST_ASSERT_EQUAL(0, startCount);
}

int main(int argc, char** argv) {
    UNITY_BEGIN();

    RUN_TEST(test_no_digits_when_on_hook);
    RUN_TEST(test_digits_accumulate_off_hook);
    RUN_TEST(test_clear_on_hang_up);
    RUN_TEST(test_fresh_start_on_pickup);
    RUN_TEST(test_dial_start_suppressed_on_hook);

    return UNITY_END();
}

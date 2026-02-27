#include <unity.h>
#include "Arduino.h"
#include "hardware/DialReader.h"
#include "hardware/pins.h"

static DialReader* dial;
static int lastDigit;
static int callCount;

// --- Helpers ---

static void advance(uint32_t ms) {
    _mock_millis += ms;
    dial->tick();
}

// Simulate a complete dial of one digit.
// Timing mirrors real rotary-dial behaviour: ~10 pulses/sec, 150ms settle.
static void dialDigit(int digit) {
    int pulses = (digit == 0) ? 10 : digit;

    // Off-normal contact closes: dialing starts
    _mock_pin_values[PIN_DIAL_ACTIVE] = LOW;
    advance(1);   // tick detects raw change; sets _dialDebounceAt
    advance(20);  // DIAL_DEBOUNCE_MS passes → _dialActive = true, state = DIALING

    for (int i = 0; i < pulses; i++) {
        // Pulse contact closes (LOW) for ~60ms
        _mock_pin_values[PIN_DIAL_PULSE] = LOW;
        advance(1);   // raw change detected
        advance(5);   // PULSE_DEBOUNCE_MS passes → edge counted
        // Pulse contact opens (HIGH) for ~40ms
        _mock_pin_values[PIN_DIAL_PULSE] = HIGH;
        advance(1);   // raw change detected
        advance(40);  // inter-pulse gap (well above PULSE_DEBOUNCE_MS)
    }

    // Off-normal contact opens: dial has returned to rest
    _mock_pin_values[PIN_DIAL_ACTIVE] = HIGH;
    advance(1);   // raw change detected
    advance(20);  // debounce passes → _dialActive = false, state = DONE

    // Settle period fires callback
    advance(150); // SETTLE_MS
}

// --- Fixtures ---

void setUp(void) {
    _mock_millis = 0;
    _mock_pin_values[PIN_DIAL_ACTIVE] = HIGH;
    _mock_pin_values[PIN_DIAL_PULSE]  = HIGH;
    lastDigit = -1;
    callCount = 0;
    dial = new DialReader();
    dial->begin();
    dial->setOnDigit([](int d) { lastDigit = d; callCount++; });
}

void tearDown(void) {
    delete dial;
}

// --- Tests ---

void test_no_callback_when_idle(void) {
    for (int i = 0; i < 100; i++) advance(10);
    TEST_ASSERT_EQUAL(0, callCount);
}

void test_dial_1(void) {
    dialDigit(1);
    TEST_ASSERT_EQUAL(1, callCount);
    TEST_ASSERT_EQUAL(1, lastDigit);
}

void test_dial_5(void) {
    dialDigit(5);
    TEST_ASSERT_EQUAL(1, callCount);
    TEST_ASSERT_EQUAL(5, lastDigit);
}

void test_dial_9(void) {
    dialDigit(9);
    TEST_ASSERT_EQUAL(1, callCount);
    TEST_ASSERT_EQUAL(9, lastDigit);
}

void test_dial_0_is_10_pulses(void) {
    dialDigit(0);
    TEST_ASSERT_EQUAL(1, callCount);
    TEST_ASSERT_EQUAL(0, lastDigit);
}

void test_callback_not_fired_before_settle(void) {
    // Bring the state machine to DONE without completing the settle period
    _mock_pin_values[PIN_DIAL_ACTIVE] = LOW;
    advance(1); advance(20);

    _mock_pin_values[PIN_DIAL_PULSE] = LOW;
    advance(1); advance(5);
    _mock_pin_values[PIN_DIAL_PULSE] = HIGH;
    advance(1); advance(40);

    _mock_pin_values[PIN_DIAL_ACTIVE] = HIGH;
    advance(1); advance(20); // state = DONE

    advance(100); // less than SETTLE_MS (150)
    TEST_ASSERT_EQUAL(0, callCount);

    advance(50); // now at 150ms total — callback fires
    TEST_ASSERT_EQUAL(1, callCount);
    TEST_ASSERT_EQUAL(1, lastDigit);
}

void test_debounce_ignores_glitch_on_dial_active(void) {
    // Brief LOW glitch (less than DIAL_DEBOUNCE_MS) should not start a dial
    _mock_pin_values[PIN_DIAL_ACTIVE] = LOW;
    advance(1);  // raw change; _dialDebounceAt set

    // Goes HIGH again before debounce settles
    _mock_pin_values[PIN_DIAL_ACTIVE] = HIGH;
    advance(5);  // raw change again; debounce restarted
    advance(20); // settles as HIGH → _dialActive stays false

    advance(200); // plenty of time — no callback expected
    TEST_ASSERT_EQUAL(0, callCount);
}

void test_multiple_sequential_digits(void) {
    dialDigit(3);
    TEST_ASSERT_EQUAL(1, callCount);
    TEST_ASSERT_EQUAL(3, lastDigit);

    dialDigit(7);
    TEST_ASSERT_EQUAL(2, callCount);
    TEST_ASSERT_EQUAL(7, lastDigit);
}

void test_no_crash_without_callback(void) {
    // Replace fixture's dial with one that has no callback set
    delete dial;
    dial = new DialReader();
    dial->begin();

    dialDigit(4);
    // Reaching here without crashing is the assertion
    TEST_PASS();
}

void test_dial_start_callback_fires(void) {
    int startCount = 0;
    dial->setOnDialStart([&startCount]() { startCount++; });

    TEST_ASSERT_EQUAL(0, startCount);

    // Dial rotation begins — should fire start callback exactly once
    _mock_pin_values[PIN_DIAL_ACTIVE] = LOW;
    advance(1);
    advance(20); // debounce passes → IDLE→DIALING, start callback fires
    TEST_ASSERT_EQUAL(1, startCount);

    // Completing the dial must not fire start callback again
    _mock_pin_values[PIN_DIAL_PULSE] = LOW;
    advance(1); advance(5);
    _mock_pin_values[PIN_DIAL_PULSE] = HIGH;
    advance(1); advance(40);
    _mock_pin_values[PIN_DIAL_ACTIVE] = HIGH;
    advance(1); advance(20);
    advance(150);
    TEST_ASSERT_EQUAL(1, startCount);
}

void test_is_dialing_reflects_state(void) {
    // Idle: not dialing
    TEST_ASSERT_FALSE(dial->isDialing());

    // Dial rotation starts
    _mock_pin_values[PIN_DIAL_ACTIVE] = LOW;
    advance(1); advance(20); // IDLE→DIALING
    TEST_ASSERT_TRUE(dial->isDialing());

    // Dial returns to rest (DIALING→DONE)
    _mock_pin_values[PIN_DIAL_ACTIVE] = HIGH;
    advance(1); advance(20);
    TEST_ASSERT_FALSE(dial->isDialing());

    // Settle completes
    advance(150);
    TEST_ASSERT_FALSE(dial->isDialing());
}

void test_start_callback_fires_per_digit(void) {
    int startCount = 0;
    dial->setOnDialStart([&startCount]() { startCount++; });

    dialDigit(3);
    TEST_ASSERT_EQUAL(1, startCount);

    dialDigit(7);
    TEST_ASSERT_EQUAL(2, startCount);
}

int main(int argc, char** argv) {
    UNITY_BEGIN();

    RUN_TEST(test_no_callback_when_idle);
    RUN_TEST(test_dial_1);
    RUN_TEST(test_dial_5);
    RUN_TEST(test_dial_9);
    RUN_TEST(test_dial_0_is_10_pulses);
    RUN_TEST(test_callback_not_fired_before_settle);
    RUN_TEST(test_debounce_ignores_glitch_on_dial_active);
    RUN_TEST(test_multiple_sequential_digits);
    RUN_TEST(test_no_crash_without_callback);
    RUN_TEST(test_dial_start_callback_fires);
    RUN_TEST(test_is_dialing_reflects_state);
    RUN_TEST(test_start_callback_fires_per_digit);

    return UNITY_END();
}

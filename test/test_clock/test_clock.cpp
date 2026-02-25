#include <unity.h>
#include <string.h>
#include "clock/ClockManager.h"

// ---------------------------------------------------------------------------
// Mock time
// ---------------------------------------------------------------------------
static struct tm _mock_time;
static bool      _mock_time_synced = false;

static bool mockGetTime(struct tm* t) {
    if (!_mock_time_synced) return false;
    *t = _mock_time;
    return true;
}

static void setMockTime(int yday, int hour, int min) {
    _mock_time_synced  = true;
    _mock_time.tm_yday = yday;
    _mock_time.tm_hour = hour;
    _mock_time.tm_min  = min;
    _mock_time.tm_sec  = 0;
}

// ---------------------------------------------------------------------------
// Test fixtures
// ---------------------------------------------------------------------------
static MotorDriver*   motor;
static Ringer*        ringer;
static ClockManager*  clk;

void setUp(void) {
    _mock_time_synced = false;
    memset(&_mock_time, 0, sizeof(_mock_time));

    motor  = new MotorDriver(0, 0, 0);
    ringer = new Ringer(*motor);
    clk    = new ClockManager(*ringer, mockGetTime);
    clk->setEnabled(true);  // most tests exercise firing; disable tests opt out explicitly
}

void tearDown(void) {
    delete clk;
    delete ringer;
    delete motor;
}

// ---------------------------------------------------------------------------
// Tests: enabled/disabled
// ---------------------------------------------------------------------------

void test_disabled_no_ring_at_top_of_hour(void) {
    clk->setEnabled(false);
    setMockTime(45, 1, 0);
    clk->tick();
    TEST_ASSERT_FALSE(ringer->isRinging());
}

void test_disabled_by_default(void) {
    // Construct a fresh instance to check the true constructor default
    MotorDriver m(0, 0, 0);
    Ringer r(m);
    ClockManager fresh(r, mockGetTime);
    TEST_ASSERT_FALSE(fresh.isEnabled());
}

void test_set_enabled(void) {
    clk->setEnabled(false);
    TEST_ASSERT_FALSE(clk->isEnabled());
    clk->setEnabled(true);
    TEST_ASSERT_TRUE(clk->isEnabled());
}

// ---------------------------------------------------------------------------
// Tests: top-of-hour firing
// ---------------------------------------------------------------------------

void test_no_ring_not_top_of_hour(void) {
    setMockTime(45, 1, 30);
    clk->tick();
    TEST_ASSERT_FALSE(ringer->isRinging());
}

void test_no_ring_one_minute_past(void) {
    setMockTime(45, 1, 1);
    clk->tick();
    TEST_ASSERT_FALSE(ringer->isRinging());
}

void test_rings_at_top_of_hour(void) {
    setMockTime(45, 1, 0);
    clk->tick();
    TEST_ASSERT_TRUE(ringer->isRinging());
}

void test_rings_at_midnight(void) {
    // hour=0 → 12 rings
    setMockTime(45, 0, 0);
    clk->tick();
    TEST_ASSERT_TRUE(ringer->isRinging());
}

void test_rings_at_noon(void) {
    // hour=12 → 12 rings
    setMockTime(45, 12, 0);
    clk->tick();
    TEST_ASSERT_TRUE(ringer->isRinging());
}

void test_rings_at_1pm(void) {
    // hour=13 → 1 ring
    setMockTime(45, 13, 0);
    clk->tick();
    TEST_ASSERT_TRUE(ringer->isRinging());
}

// ---------------------------------------------------------------------------
// Tests: no-op when not synced
// ---------------------------------------------------------------------------

void test_no_ring_when_not_synced(void) {
    _mock_time_synced = false;
    clk->tick();
    TEST_ASSERT_FALSE(ringer->isRinging());
}

// ---------------------------------------------------------------------------
// Tests: deduplication (same minute key)
// ---------------------------------------------------------------------------

void test_no_double_fire_same_minute(void) {
    setMockTime(45, 3, 0);
    clk->tick();
    TEST_ASSERT_TRUE(ringer->isRinging());
    ringer->ringStop();

    // Second tick at same minute: minuteKey blocks it
    clk->tick();
    TEST_ASSERT_FALSE(ringer->isRinging());
}

void test_fires_again_next_hour(void) {
    setMockTime(45, 3, 0);
    clk->tick();
    ringer->ringStop();

    setMockTime(45, 4, 0);
    clk->tick();
    TEST_ASSERT_TRUE(ringer->isRinging());
}

// ---------------------------------------------------------------------------
// Tests: re-enable after disable at top of hour does not fire
// ---------------------------------------------------------------------------

void test_reenable_after_disable_no_fire(void) {
    clk->setEnabled(false);
    setMockTime(45, 5, 0);
    clk->tick();  // minuteKey advances but disabled, no ring

    clk->setEnabled(true);
    clk->tick();  // same minute key → early return, no ring
    TEST_ASSERT_FALSE(ringer->isRinging());
}

// ---------------------------------------------------------------------------
// Tests: chime mode
// ---------------------------------------------------------------------------

void test_default_mode_is_n_chimes(void) {
    TEST_ASSERT_EQUAL(CHIME_N_CHIMES, clk->getChimeMode());
}

void test_set_chime_mode(void) {
    clk->setChimeMode(CHIME_SINGLE);
    TEST_ASSERT_EQUAL(CHIME_SINGLE, clk->getChimeMode());
    clk->setChimeMode(CHIME_N_CHIMES);
    TEST_ASSERT_EQUAL(CHIME_N_CHIMES, clk->getChimeMode());
}

void test_single_mode_rings_at_top_of_hour(void) {
    clk->setChimeMode(CHIME_SINGLE);
    setMockTime(45, 7, 0);
    clk->tick();
    TEST_ASSERT_TRUE(ringer->isRinging());
}

void test_single_mode_disabled_no_ring(void) {
    clk->setChimeMode(CHIME_SINGLE);
    clk->setEnabled(false);
    setMockTime(45, 7, 0);
    clk->tick();
    TEST_ASSERT_FALSE(ringer->isRinging());
}

// ---------------------------------------------------------------------------
// Tests: skip if already ringing
// ---------------------------------------------------------------------------

void test_no_fire_if_ringer_busy(void) {
    // Start an existing ring manually
    ringer->ring(PATTERN_US);
    TEST_ASSERT_TRUE(ringer->isRinging());

    // Now tick at top of hour — should not interfere
    setMockTime(45, 6, 0);
    clk->tick();

    // Ringer is still ringing (from PATTERN_US, not restarted by clock)
    TEST_ASSERT_TRUE(ringer->isRinging());

    // Stop the ringer and verify the clock did not restart it on the same tick
    ringer->ringStop();
    TEST_ASSERT_FALSE(ringer->isRinging());

    // A second tick at the same minute should not ring (minuteKey advanced)
    clk->tick();
    TEST_ASSERT_FALSE(ringer->isRinging());
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main(int argc, char** argv) {
    UNITY_BEGIN();

    RUN_TEST(test_disabled_by_default);
    RUN_TEST(test_set_enabled);
    RUN_TEST(test_disabled_no_ring_at_top_of_hour);
    RUN_TEST(test_no_ring_not_top_of_hour);
    RUN_TEST(test_no_ring_one_minute_past);
    RUN_TEST(test_rings_at_top_of_hour);
    RUN_TEST(test_rings_at_midnight);
    RUN_TEST(test_rings_at_noon);
    RUN_TEST(test_rings_at_1pm);
    RUN_TEST(test_no_ring_when_not_synced);
    RUN_TEST(test_no_double_fire_same_minute);
    RUN_TEST(test_fires_again_next_hour);
    RUN_TEST(test_reenable_after_disable_no_fire);
    RUN_TEST(test_default_mode_is_n_chimes);
    RUN_TEST(test_set_chime_mode);
    RUN_TEST(test_single_mode_rings_at_top_of_hour);
    RUN_TEST(test_single_mode_disabled_no_ring);
    RUN_TEST(test_no_fire_if_ringer_busy);

    return UNITY_END();
}

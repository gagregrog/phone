#include <unity.h>
#include "Timer.h"

// Declared in mock/Arduino.h, defined in mock_millis.cpp
extern unsigned long _mock_millis;

// Test fixtures
static MotorDriver motor(0, 0, 0);
static Ringer ringer(motor);
static Timer* timer;

void setUp(void) {
  _mock_millis = 0;
  ringer.ringStop();
  timer = new Timer(ringer);
}

void tearDown(void) {
  delete timer;
}

// --- Basic state ---

void test_initially_inactive(void) {
  TEST_ASSERT_FALSE(timer->isActive());
  TEST_ASSERT_EQUAL(0, timer->remainingMs());
  TEST_ASSERT_EQUAL(0, timer->totalMs());
  TEST_ASSERT_NULL(timer->patternName());
}

// --- Start ---

void test_start_activates(void) {
  timer->start(5000, PATTERN_CHIRP);
  TEST_ASSERT_TRUE(timer->isActive());
  TEST_ASSERT_EQUAL(5000, timer->totalMs());
  TEST_ASSERT_EQUAL_STRING("chirp", timer->patternName());
}

void test_remaining_decreases(void) {
  timer->start(5000, PATTERN_CHIRP);
  _mock_millis = 2000;
  TEST_ASSERT_EQUAL(3000, timer->remainingMs());
}

// --- Firing ---

void test_fires_when_elapsed(void) {
  timer->start(5000, PATTERN_CHIRP, 3);
  TEST_ASSERT_FALSE(ringer.isRinging());

  _mock_millis = 5000;
  timer->update();

  TEST_ASSERT_FALSE(timer->isActive());
  TEST_ASSERT_TRUE(ringer.isRinging());
}

void test_fires_when_past_elapsed(void) {
  timer->start(5000, PATTERN_CHIRP, 3);
  _mock_millis = 6000;  // overshoot
  timer->update();

  TEST_ASSERT_FALSE(timer->isActive());
  TEST_ASSERT_TRUE(ringer.isRinging());
}

void test_does_not_fire_before_elapsed(void) {
  timer->start(5000, PATTERN_CHIRP, 3);
  _mock_millis = 4999;
  timer->update();

  TEST_ASSERT_TRUE(timer->isActive());
  TEST_ASSERT_FALSE(ringer.isRinging());
}

// --- Cancel ---

void test_cancel_prevents_firing(void) {
  timer->start(5000, PATTERN_CHIRP, 3);
  timer->cancel();

  TEST_ASSERT_FALSE(timer->isActive());

  _mock_millis = 6000;
  timer->update();
  TEST_ASSERT_FALSE(ringer.isRinging());
}

void test_cancel_when_inactive_is_safe(void) {
  timer->cancel();  // should not crash
  TEST_ASSERT_FALSE(timer->isActive());
}

// --- Overwrite ---

void test_new_start_overwrites_old(void) {
  timer->start(5000, PATTERN_CHIRP);
  _mock_millis = 2000;

  timer->start(10000, PATTERN_US);
  TEST_ASSERT_EQUAL(10000, timer->totalMs());
  TEST_ASSERT_EQUAL_STRING("us", timer->patternName());

  // Old timer would have fired at 5000, but new one shouldn't
  _mock_millis = 7000;
  timer->update();
  TEST_ASSERT_TRUE(timer->isActive());
  TEST_ASSERT_FALSE(ringer.isRinging());

  // New timer fires at 2000 + 10000 = 12000
  _mock_millis = 12000;
  timer->update();
  TEST_ASSERT_FALSE(timer->isActive());
  TEST_ASSERT_TRUE(ringer.isRinging());
}

// --- State after firing ---

void test_state_after_firing(void) {
  timer->start(1000, PATTERN_CHIRP);
  _mock_millis = 1000;
  timer->update();

  TEST_ASSERT_FALSE(timer->isActive());
  TEST_ASSERT_EQUAL(0, timer->remainingMs());
  TEST_ASSERT_EQUAL(0, timer->totalMs());
  TEST_ASSERT_NULL(timer->patternName());
}

// --- Stops existing ring when firing ---

void test_fire_overrides_active_ring(void) {
  ringer.ring(PATTERN_US);
  TEST_ASSERT_TRUE(ringer.isRinging());

  timer->start(1000, PATTERN_CHIRP, 3);
  _mock_millis = 1000;
  timer->update();

  // Timer should have called ringStop then ring with chirp
  TEST_ASSERT_TRUE(ringer.isRinging());
}

// --- Update when inactive is a no-op ---

void test_update_when_inactive(void) {
  _mock_millis = 99999;
  timer->update();  // should not crash or start ringing
  TEST_ASSERT_FALSE(timer->isActive());
  TEST_ASSERT_FALSE(ringer.isRinging());
}

int main(int argc, char** argv) {
  UNITY_BEGIN();

  RUN_TEST(test_initially_inactive);
  RUN_TEST(test_start_activates);
  RUN_TEST(test_remaining_decreases);
  RUN_TEST(test_fires_when_elapsed);
  RUN_TEST(test_fires_when_past_elapsed);
  RUN_TEST(test_does_not_fire_before_elapsed);
  RUN_TEST(test_cancel_prevents_firing);
  RUN_TEST(test_cancel_when_inactive_is_safe);
  RUN_TEST(test_new_start_overwrites_old);
  RUN_TEST(test_state_after_firing);
  RUN_TEST(test_fire_overrides_active_ring);
  RUN_TEST(test_update_when_inactive);

  return UNITY_END();
}

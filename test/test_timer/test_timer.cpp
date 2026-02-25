#include <unity.h>
#include "timer/Timer.h"

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

// --- Initial state ---

void test_initially_empty(void) {
  TEST_ASSERT_FALSE(timer->hasActive());
  TEST_ASSERT_EQUAL(0, timer->count());
}

// --- Start ---

void test_start_returns_nonzero_id(void) {
  uint32_t id = timer->start(5000, PATTERN_CHIRP);
  TEST_ASSERT_NOT_EQUAL(0, id);
}

void test_start_activates(void) {
  timer->start(5000, PATTERN_CHIRP);
  TEST_ASSERT_TRUE(timer->hasActive());
  TEST_ASSERT_EQUAL(1, timer->count());
}

void test_start_info(void) {
  timer->start(5000, PATTERN_CHIRP);
  TimerInfo info = timer->infoAt(0);
  TEST_ASSERT_EQUAL(5000, info.remainingMs);
  TEST_ASSERT_EQUAL(5000, info.totalMs);
  TEST_ASSERT_EQUAL_STRING("chirp", info.patternName);
}

void test_remaining_decreases(void) {
  timer->start(5000, PATTERN_CHIRP);
  _mock_millis = 2000;
  TEST_ASSERT_EQUAL(3000, timer->infoAt(0).remainingMs);
}

// --- Multiple timers ---

void test_multiple_timers(void) {
  uint32_t id1 = timer->start(5000, PATTERN_CHIRP);
  uint32_t id2 = timer->start(10000, PATTERN_US);
  TEST_ASSERT_NOT_EQUAL(id1, id2);
  TEST_ASSERT_EQUAL(2, timer->count());
}

void test_ids_are_unique(void) {
  uint32_t id1 = timer->start(5000, PATTERN_CHIRP);
  uint32_t id2 = timer->start(5000, PATTERN_CHIRP);
  uint32_t id3 = timer->start(5000, PATTERN_CHIRP);
  TEST_ASSERT_NOT_EQUAL(id1, id2);
  TEST_ASSERT_NOT_EQUAL(id2, id3);
  TEST_ASSERT_NOT_EQUAL(id1, id3);
}

void test_ids_increment_across_expirations(void) {
  uint32_t id1 = timer->start(1000, PATTERN_CHIRP);
  _mock_millis = 1000;
  timer->update();  // id1 fires and is removed
  TEST_ASSERT_EQUAL(0, timer->count());

  uint32_t id2 = timer->start(1000, PATTERN_CHIRP);
  TEST_ASSERT_NOT_EQUAL(id1, id2);
  TEST_ASSERT_EQUAL(id1 + 1, id2);
}

// --- Firing ---

void test_fires_when_elapsed(void) {
  timer->start(5000, PATTERN_CHIRP, 3);
  TEST_ASSERT_FALSE(ringer.isRinging());

  _mock_millis = 5000;
  timer->update();

  TEST_ASSERT_FALSE(timer->hasActive());
  TEST_ASSERT_TRUE(ringer.isRinging());
}

void test_fires_when_past_elapsed(void) {
  timer->start(5000, PATTERN_CHIRP, 3);
  _mock_millis = 6000;
  timer->update();

  TEST_ASSERT_FALSE(timer->hasActive());
  TEST_ASSERT_TRUE(ringer.isRinging());
}

void test_does_not_fire_before_elapsed(void) {
  timer->start(5000, PATTERN_CHIRP, 3);
  _mock_millis = 4999;
  timer->update();

  TEST_ASSERT_TRUE(timer->hasActive());
  TEST_ASSERT_FALSE(ringer.isRinging());
}

void test_only_expired_timer_fires(void) {
  timer->start(5000, PATTERN_CHIRP);
  timer->start(10000, PATTERN_US);

  _mock_millis = 5000;
  timer->update();

  TEST_ASSERT_EQUAL(1, timer->count());  // second timer still active
  TEST_ASSERT_TRUE(ringer.isRinging());
}

// --- Cancel by ID ---

void test_cancel_specific(void) {
  uint32_t id1 = timer->start(5000, PATTERN_CHIRP);
  uint32_t id2 = timer->start(10000, PATTERN_US);

  TEST_ASSERT_TRUE(timer->cancel(id1));
  TEST_ASSERT_EQUAL(1, timer->count());

  // remaining timer is id2
  TEST_ASSERT_EQUAL(id2, timer->infoAt(0).id);
}

void test_cancel_unknown_returns_false(void) {
  timer->start(5000, PATTERN_CHIRP);
  TEST_ASSERT_FALSE(timer->cancel(999));
  TEST_ASSERT_EQUAL(1, timer->count());
}

void test_cancel_prevents_firing(void) {
  uint32_t id = timer->start(5000, PATTERN_CHIRP, 3);
  timer->cancel(id);

  _mock_millis = 6000;
  timer->update();
  TEST_ASSERT_FALSE(ringer.isRinging());
}

void test_cancel_when_empty_returns_false(void) {
  TEST_ASSERT_FALSE(timer->cancel(1));
}

// --- Cancel all ---

void test_cancel_all(void) {
  timer->start(5000, PATTERN_CHIRP);
  timer->start(10000, PATTERN_US);
  timer->cancelAll();

  TEST_ASSERT_FALSE(timer->hasActive());
  TEST_ASSERT_EQUAL(0, timer->count());
}

void test_cancel_all_prevents_firing(void) {
  timer->start(5000, PATTERN_CHIRP);
  timer->start(10000, PATTERN_US);
  timer->cancelAll();

  _mock_millis = 10000;
  timer->update();
  TEST_ASSERT_FALSE(ringer.isRinging());
}

void test_cancel_all_when_empty_is_safe(void) {
  timer->cancelAll();
  TEST_ASSERT_EQUAL(0, timer->count());
}

// --- State after firing ---

void test_state_after_firing(void) {
  timer->start(1000, PATTERN_CHIRP);
  _mock_millis = 1000;
  timer->update();

  TEST_ASSERT_FALSE(timer->hasActive());
  TEST_ASSERT_EQUAL(0, timer->count());
}

// --- Update when empty is a no-op ---

void test_update_when_empty(void) {
  _mock_millis = 99999;
  timer->update();
  TEST_ASSERT_FALSE(timer->hasActive());
  TEST_ASSERT_FALSE(ringer.isRinging());
}

int main(int argc, char** argv) {
  UNITY_BEGIN();

  RUN_TEST(test_initially_empty);
  RUN_TEST(test_start_returns_nonzero_id);
  RUN_TEST(test_start_activates);
  RUN_TEST(test_start_info);
  RUN_TEST(test_remaining_decreases);
  RUN_TEST(test_multiple_timers);
  RUN_TEST(test_ids_are_unique);
  RUN_TEST(test_ids_increment_across_expirations);
  RUN_TEST(test_fires_when_elapsed);
  RUN_TEST(test_fires_when_past_elapsed);
  RUN_TEST(test_does_not_fire_before_elapsed);
  RUN_TEST(test_only_expired_timer_fires);
  RUN_TEST(test_cancel_specific);
  RUN_TEST(test_cancel_unknown_returns_false);
  RUN_TEST(test_cancel_prevents_firing);
  RUN_TEST(test_cancel_when_empty_returns_false);
  RUN_TEST(test_cancel_all);
  RUN_TEST(test_cancel_all_prevents_firing);
  RUN_TEST(test_cancel_all_when_empty_is_safe);
  RUN_TEST(test_state_after_firing);
  RUN_TEST(test_update_when_empty);

  return UNITY_END();
}

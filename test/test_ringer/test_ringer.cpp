#include <unity.h>
#include "ringer/Ringer.h"

// Declared in mock/Arduino.h, defined in mock_millis.cpp
extern unsigned long _mock_millis;

// Test fixtures
static MotorDriver motor(0, 0, 0);
static Ringer* ringer;

void setUp(void) {
  _mock_millis = 0;
  ringer = new Ringer(motor);
}

void tearDown(void) {
  delete ringer;
}

// --- Basic state ---

void test_initially_not_ringing(void) {
  TEST_ASSERT_FALSE(ringer->isRinging());
}

void test_ring_starts_ringing(void) {
  ringer->ring(PATTERN_US);
  TEST_ASSERT_TRUE(ringer->isRinging());
}

void test_ring_stop(void) {
  ringer->ring(PATTERN_US);
  ringer->ringStop();
  TEST_ASSERT_FALSE(ringer->isRinging());
}

// --- Phase transitions (US pattern: 2000ms ON, 4000ms OFF) ---

void test_stays_in_first_phase(void) {
  ringer->ring(PATTERN_US);
  _mock_millis = 1999;
  ringer->update();
  TEST_ASSERT_TRUE(ringer->isRinging());
  TEST_ASSERT_TRUE(motor.isActive());
}

void test_transitions_to_off_phase(void) {
  ringer->ring(PATTERN_US);
  _mock_millis = 2000;
  ringer->update();
  TEST_ASSERT_TRUE(ringer->isRinging());
  TEST_ASSERT_FALSE(motor.isActive());
}

void test_completes_full_cycle(void) {
  ringer->ring(PATTERN_US);

  // Phase 0: ON for 2000ms
  _mock_millis = 2000;
  ringer->update();
  TEST_ASSERT_FALSE(motor.isActive());

  // Phase 1: OFF for 4000ms — advance to phase start + 4000
  _mock_millis = 2000 + 4000;
  ringer->update();
  // Back to phase 0: motor should be active again
  TEST_ASSERT_TRUE(motor.isActive());
  TEST_ASSERT_TRUE(ringer->isRinging());
}

// --- Cycle counting ---

void test_infinite_cycles(void) {
  ringer->ring(PATTERN_US, 0);  // 0 = infinite

  // Run through 3 full cycles
  unsigned long t = 0;
  for (int cycle = 0; cycle < 3; cycle++) {
    t += 2000;  // ON phase
    _mock_millis = t;
    ringer->update();
    t += 4000;  // OFF phase
    _mock_millis = t;
    ringer->update();
  }

  TEST_ASSERT_TRUE(ringer->isRinging());
}

void test_single_cycle_stops(void) {
  ringer->ring(PATTERN_US, 1);

  // Complete one cycle
  _mock_millis = 2000;
  ringer->update();  // ON -> OFF
  _mock_millis = 6000;
  ringer->update();  // OFF -> wraps, cycle decrements to 0 -> stops

  TEST_ASSERT_FALSE(ringer->isRinging());
}

void test_three_cycles(void) {
  ringer->ring(PATTERN_US, 3);
  unsigned long t = 0;

  // Cycles 1 and 2 should keep ringing
  for (int cycle = 0; cycle < 2; cycle++) {
    t += 2000;
    _mock_millis = t;
    ringer->update();
    t += 4000;
    _mock_millis = t;
    ringer->update();
  }
  TEST_ASSERT_TRUE(ringer->isRinging());

  // Cycle 3 should stop
  t += 2000;
  _mock_millis = t;
  ringer->update();
  t += 4000;
  _mock_millis = t;
  ringer->update();

  TEST_ASSERT_FALSE(ringer->isRinging());
}

// --- Multi-phase pattern (UK: 400 ON, 200 OFF, 400 ON, 2000 OFF) ---

void test_uk_double_ring_cycle(void) {
  ringer->ring(PATTERN_UK, 1);
  unsigned long t = 0;

  // Phase 0: ON 400ms
  t += 400;
  _mock_millis = t;
  ringer->update();
  TEST_ASSERT_FALSE(motor.isActive());  // now OFF

  // Phase 1: OFF 200ms
  t += 200;
  _mock_millis = t;
  ringer->update();
  TEST_ASSERT_TRUE(motor.isActive());   // now ON

  // Phase 2: ON 400ms
  t += 400;
  _mock_millis = t;
  ringer->update();
  TEST_ASSERT_FALSE(motor.isActive());  // now OFF

  // Phase 3: OFF 2000ms — completes cycle, should stop (1 cycle)
  t += 2000;
  _mock_millis = t;
  ringer->update();

  TEST_ASSERT_FALSE(ringer->isRinging());
}

// --- onStop callback ---

void test_on_stop_fires_when_cycles_complete(void) {
  int callCount = 0;
  ringer->setOnStop([&callCount]{ callCount++; });
  ringer->ring(PATTERN_US, 1);

  _mock_millis = 2000;
  ringer->update();  // ON -> OFF
  _mock_millis = 6000;
  ringer->update();  // cycle completes -> stop

  TEST_ASSERT_EQUAL(1, callCount);
}

void test_on_stop_not_fired_by_external_ring_stop(void) {
  int callCount = 0;
  ringer->setOnStop([&callCount]{ callCount++; });
  ringer->ring(PATTERN_US, 1);
  ringer->ringStop();

  TEST_ASSERT_EQUAL(0, callCount);
}

void test_on_stop_fires_once_for_three_cycles(void) {
  int callCount = 0;
  ringer->setOnStop([&callCount]{ callCount++; });
  ringer->ring(PATTERN_US, 3);
  unsigned long t = 0;

  for (int cycle = 0; cycle < 3; cycle++) {
    t += 2000;
    _mock_millis = t;
    ringer->update();
    t += 4000;
    _mock_millis = t;
    ringer->update();
  }

  TEST_ASSERT_EQUAL(1, callCount);
}

// --- Ring guard ---

void test_guard_suppresses_ring(void) {
  ringer->setRingGuard([]{ return false; });
  bool result = ringer->ring(PATTERN_US);
  TEST_ASSERT_FALSE(result);
  TEST_ASSERT_FALSE(ringer->isRinging());
  TEST_ASSERT_FALSE(motor.isActive());
}

void test_guard_allows_ring(void) {
  ringer->setRingGuard([]{ return true; });
  bool result = ringer->ring(PATTERN_US);
  TEST_ASSERT_TRUE(result);
  TEST_ASSERT_TRUE(ringer->isRinging());
}

void test_guard_null_allows_ring(void) {
  // Default (no guard set) should always allow
  bool result = ringer->ring(PATTERN_US);
  TEST_ASSERT_TRUE(result);
  TEST_ASSERT_TRUE(ringer->isRinging());
}

// --- onStart callback ---

void test_on_start_fires_when_ring_starts(void) {
  const char* firedPattern = nullptr;
  ringer->setOnStart([&firedPattern](const char* p){ firedPattern = p; });
  ringer->ring(PATTERN_US);
  TEST_ASSERT_NOT_NULL(firedPattern);
  TEST_ASSERT_EQUAL_STRING("us", firedPattern);
}

void test_on_start_not_fired_when_guard_suppresses(void) {
  bool fired = false;
  ringer->setRingGuard([]{ return false; });
  ringer->setOnStart([&fired](const char*){ fired = true; });
  ringer->ring(PATTERN_US);
  TEST_ASSERT_FALSE(fired);
}

void test_on_start_fires_with_correct_pattern_name(void) {
  const char* firedPattern = nullptr;
  ringer->setOnStart([&firedPattern](const char* p){ firedPattern = p; });
  ringer->ring(PATTERN_UK);
  TEST_ASSERT_EQUAL_STRING("uk", firedPattern);
}

// --- Ring can be restarted ---

void test_ring_restart_resets_state(void) {
  ringer->ring(PATTERN_US, 1);
  _mock_millis = 2000;
  ringer->update();  // transition to OFF phase

  // Restart with a new pattern
  ringer->ring(PATTERN_UK, 1);
  TEST_ASSERT_TRUE(ringer->isRinging());
  TEST_ASSERT_TRUE(motor.isActive());  // phase 0 is always ON
}

// --- Update when idle is safe ---

void test_update_when_idle(void) {
  _mock_millis = 99999;
  ringer->update();  // should not crash
  TEST_ASSERT_FALSE(ringer->isRinging());
}

int main(int argc, char** argv) {
  UNITY_BEGIN();

  RUN_TEST(test_initially_not_ringing);
  RUN_TEST(test_ring_starts_ringing);
  RUN_TEST(test_ring_stop);

  RUN_TEST(test_stays_in_first_phase);
  RUN_TEST(test_transitions_to_off_phase);
  RUN_TEST(test_completes_full_cycle);

  RUN_TEST(test_infinite_cycles);
  RUN_TEST(test_single_cycle_stops);
  RUN_TEST(test_three_cycles);

  RUN_TEST(test_uk_double_ring_cycle);

  RUN_TEST(test_on_stop_fires_when_cycles_complete);
  RUN_TEST(test_on_stop_not_fired_by_external_ring_stop);
  RUN_TEST(test_on_stop_fires_once_for_three_cycles);

  RUN_TEST(test_guard_suppresses_ring);
  RUN_TEST(test_guard_allows_ring);
  RUN_TEST(test_guard_null_allows_ring);

  RUN_TEST(test_on_start_fires_when_ring_starts);
  RUN_TEST(test_on_start_not_fired_when_guard_suppresses);
  RUN_TEST(test_on_start_fires_with_correct_pattern_name);

  RUN_TEST(test_ring_restart_resets_state);
  RUN_TEST(test_update_when_idle);

  return UNITY_END();
}

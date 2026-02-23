#include <unity.h>
#include "RingPattern.h"
#include <string.h>

// --- findPattern lookup ---

void test_find_us(void) {
  const RingPattern* p = findPattern("us");
  TEST_ASSERT_NOT_NULL(p);
  TEST_ASSERT_EQUAL_STRING("us", p->name);
}

void test_find_chirp(void) {
  const RingPattern* p = findPattern("chirp");
  TEST_ASSERT_NOT_NULL(p);
  TEST_ASSERT_EQUAL_STRING("chirp", p->name);
}

void test_find_case_insensitive(void) {
  TEST_ASSERT_NOT_NULL(findPattern("US"));
  TEST_ASSERT_NOT_NULL(findPattern("Uk"));
  TEST_ASSERT_NOT_NULL(findPattern("CHIRP"));
}

void test_find_unknown_returns_null(void) {
  TEST_ASSERT_NULL(findPattern("nope"));
  TEST_ASSERT_NULL(findPattern(""));
}

// --- Pattern data integrity ---

void test_pattern_count(void) {
  TEST_ASSERT_EQUAL(8, PATTERN_COUNT);
}

void test_all_patterns_have_names(void) {
  for (uint8_t i = 0; i < PATTERN_COUNT; i++) {
    TEST_ASSERT_NOT_NULL(ALL_PATTERNS[i]->name);
    TEST_ASSERT_TRUE(strlen(ALL_PATTERNS[i]->name) > 0);
  }
}

void test_all_patterns_have_even_phase_count(void) {
  // Phases alternate ON/OFF, so count must be even
  for (uint8_t i = 0; i < PATTERN_COUNT; i++) {
    TEST_ASSERT_TRUE_MESSAGE(
      ALL_PATTERNS[i]->phaseCount % 2 == 0,
      ALL_PATTERNS[i]->name
    );
  }
}

void test_all_patterns_have_nonzero_phases(void) {
  for (uint8_t i = 0; i < PATTERN_COUNT; i++) {
    for (uint8_t j = 0; j < ALL_PATTERNS[i]->phaseCount; j++) {
      TEST_ASSERT_TRUE_MESSAGE(
        ALL_PATTERNS[i]->phases[j] > 0,
        ALL_PATTERNS[i]->name
      );
    }
  }
}

void test_no_duplicate_names(void) {
  for (uint8_t i = 0; i < PATTERN_COUNT; i++) {
    for (uint8_t j = i + 1; j < PATTERN_COUNT; j++) {
      TEST_ASSERT_TRUE_MESSAGE(
        strcmp(ALL_PATTERNS[i]->name, ALL_PATTERNS[j]->name) != 0,
        "duplicate pattern name"
      );
    }
  }
}

void test_chirp_phases(void) {
  const RingPattern* p = findPattern("chirp");
  TEST_ASSERT_NOT_NULL(p);
  TEST_ASSERT_EQUAL(4, p->phaseCount);
  TEST_ASSERT_EQUAL(150, p->phases[0]);  // ON
  TEST_ASSERT_EQUAL(100, p->phases[1]);  // OFF
  TEST_ASSERT_EQUAL(150, p->phases[2]);  // ON
  TEST_ASSERT_EQUAL(600, p->phases[3]);  // OFF
}

int main(int argc, char** argv) {
  UNITY_BEGIN();

  RUN_TEST(test_find_us);
  RUN_TEST(test_find_chirp);
  RUN_TEST(test_find_case_insensitive);
  RUN_TEST(test_find_unknown_returns_null);

  RUN_TEST(test_pattern_count);
  RUN_TEST(test_all_patterns_have_names);
  RUN_TEST(test_all_patterns_have_even_phase_count);
  RUN_TEST(test_all_patterns_have_nonzero_phases);
  RUN_TEST(test_no_duplicate_names);
  RUN_TEST(test_chirp_phases);

  return UNITY_END();
}

#include <unity.h>
#include "DurationParser.h"

// --- Valid single-unit durations ---

void test_seconds(void) {
  TEST_ASSERT_EQUAL(1000UL, parseDuration("1s"));
  TEST_ASSERT_EQUAL(90000UL, parseDuration("90s"));
  TEST_ASSERT_EQUAL(3600000UL, parseDuration("3600s"));
}

void test_minutes(void) {
  TEST_ASSERT_EQUAL(60000UL, parseDuration("1m"));
  TEST_ASSERT_EQUAL(1200000UL, parseDuration("20m"));
  TEST_ASSERT_EQUAL(5400000UL, parseDuration("90m"));
}

void test_hours(void) {
  TEST_ASSERT_EQUAL(3600000UL, parseDuration("1h"));
  TEST_ASSERT_EQUAL(7200000UL, parseDuration("2h"));
  TEST_ASSERT_EQUAL(86400000UL, parseDuration("24h"));
}

// --- Valid combined durations ---

void test_hours_minutes(void) {
  TEST_ASSERT_EQUAL(5400000UL, parseDuration("1h30m"));
  TEST_ASSERT_EQUAL(86340000UL, parseDuration("23h59m"));
}

void test_hours_seconds(void) {
  TEST_ASSERT_EQUAL(3630000UL, parseDuration("1h30s"));
}

void test_minutes_seconds(void) {
  TEST_ASSERT_EQUAL(90000UL, parseDuration("1m30s"));
  TEST_ASSERT_EQUAL(119000UL, parseDuration("1m59s"));
}

void test_hours_minutes_seconds(void) {
  TEST_ASSERT_EQUAL(5430000UL, parseDuration("1h30m30s"));
  TEST_ASSERT_EQUAL(86399000UL, parseDuration("23h59m59s"));
}

// --- Sub-unit capping (>59 rejected when larger unit present) ---

void test_minutes_capped_with_hours(void) {
  TEST_ASSERT_EQUAL(0UL, parseDuration("1h60m"));
  TEST_ASSERT_EQUAL(0UL, parseDuration("1h90m"));
}

void test_seconds_capped_with_minutes(void) {
  TEST_ASSERT_EQUAL(0UL, parseDuration("1m60s"));
  TEST_ASSERT_EQUAL(0UL, parseDuration("1m90s"));
}

void test_seconds_capped_with_hours(void) {
  TEST_ASSERT_EQUAL(0UL, parseDuration("1h60s"));
}

// Large values are fine for standalone units
void test_large_standalone_units(void) {
  TEST_ASSERT_EQUAL(5400000UL, parseDuration("90m"));
  TEST_ASSERT_EQUAL(3600000UL, parseDuration("3600s"));
}

// --- Bare numbers rejected ---

void test_bare_number_rejected(void) {
  TEST_ASSERT_EQUAL(0UL, parseDuration("5"));
  TEST_ASSERT_EQUAL(0UL, parseDuration("120"));
}

// --- Invalid formats ---

void test_empty_string(void) {
  TEST_ASSERT_EQUAL(0UL, parseDuration(""));
}

void test_null_pointer(void) {
  TEST_ASSERT_EQUAL(0UL, parseDuration(nullptr));
}

void test_no_digits_before_unit(void) {
  TEST_ASSERT_EQUAL(0UL, parseDuration("m"));
  TEST_ASSERT_EQUAL(0UL, parseDuration("h"));
  TEST_ASSERT_EQUAL(0UL, parseDuration("s"));
}

void test_wrong_order(void) {
  TEST_ASSERT_EQUAL(0UL, parseDuration("30m1h"));
  TEST_ASSERT_EQUAL(0UL, parseDuration("30s1m"));
  TEST_ASSERT_EQUAL(0UL, parseDuration("30s1h"));
}

void test_duplicate_units(void) {
  TEST_ASSERT_EQUAL(0UL, parseDuration("1h2h"));
  TEST_ASSERT_EQUAL(0UL, parseDuration("1m2m"));
  TEST_ASSERT_EQUAL(0UL, parseDuration("1s2s"));
}

void test_invalid_characters(void) {
  TEST_ASSERT_EQUAL(0UL, parseDuration("10x"));
  TEST_ASSERT_EQUAL(0UL, parseDuration("abc"));
  TEST_ASSERT_EQUAL(0UL, parseDuration("1.5h"));
}

void test_trailing_digits(void) {
  TEST_ASSERT_EQUAL(0UL, parseDuration("1h30"));
  TEST_ASSERT_EQUAL(0UL, parseDuration("1m30"));
}

void test_zero_duration(void) {
  TEST_ASSERT_EQUAL(0UL, parseDuration("0s"));
  TEST_ASSERT_EQUAL(0UL, parseDuration("0m"));
  TEST_ASSERT_EQUAL(0UL, parseDuration("0h"));
}

// --- Range limits ---

void test_max_allowed(void) {
  // 24h = 86400s = 86400000ms — should be accepted
  TEST_ASSERT_EQUAL(86400000UL, parseDuration("24h"));
  TEST_ASSERT_EQUAL(86400000UL, parseDuration("1440m"));
  TEST_ASSERT_EQUAL(86400000UL, parseDuration("86400s"));
}

void test_over_max_rejected(void) {
  TEST_ASSERT_EQUAL(0UL, parseDuration("25h"));
  TEST_ASSERT_EQUAL(0UL, parseDuration("1441m"));
  TEST_ASSERT_EQUAL(0UL, parseDuration("86401s"));
}

void test_overflow_digit_string(void) {
  TEST_ASSERT_EQUAL(0UL, parseDuration("99999999999s"));
  TEST_ASSERT_EQUAL(0UL, parseDuration("4294967296s"));
}

// --- formatDuration ---

void test_format_zero(void) {
  char buf[10];
  formatDuration(0, buf, sizeof(buf));
  TEST_ASSERT_EQUAL_STRING("0s", buf);
}

void test_format_seconds_only(void) {
  char buf[10];
  formatDuration(45, buf, sizeof(buf));
  TEST_ASSERT_EQUAL_STRING("45s", buf);
}

void test_format_minutes_only(void) {
  char buf[10];
  formatDuration(300, buf, sizeof(buf));  // 5m
  TEST_ASSERT_EQUAL_STRING("5m", buf);
}

void test_format_minutes_and_seconds(void) {
  char buf[10];
  formatDuration(330, buf, sizeof(buf));  // 5m30s
  TEST_ASSERT_EQUAL_STRING("5m30s", buf);
}

void test_format_hours_only(void) {
  char buf[10];
  formatDuration(3600, buf, sizeof(buf));
  TEST_ASSERT_EQUAL_STRING("1h", buf);
}

void test_format_hours_and_minutes(void) {
  char buf[10];
  formatDuration(5700, buf, sizeof(buf));  // 1h35m
  TEST_ASSERT_EQUAL_STRING("1h35m", buf);
}

void test_format_hours_and_seconds(void) {
  char buf[10];
  formatDuration(3601, buf, sizeof(buf));  // 1h1s
  TEST_ASSERT_EQUAL_STRING("1h1s", buf);
}

void test_format_hours_minutes_seconds(void) {
  char buf[10];
  formatDuration(3661, buf, sizeof(buf));  // 1h1m1s
  TEST_ASSERT_EQUAL_STRING("1h1m1s", buf);
}

void test_format_max(void) {
  char buf[10];
  formatDuration(86400, buf, sizeof(buf));  // 24h
  TEST_ASSERT_EQUAL_STRING("24h", buf);
}

void test_format_exact_minute(void) {
  char buf[10];
  formatDuration(60, buf, sizeof(buf));
  TEST_ASSERT_EQUAL_STRING("1m", buf);
}

int main(int argc, char** argv) {
  UNITY_BEGIN();

  // Valid single units
  RUN_TEST(test_seconds);
  RUN_TEST(test_minutes);
  RUN_TEST(test_hours);

  // Valid combinations
  RUN_TEST(test_hours_minutes);
  RUN_TEST(test_hours_seconds);
  RUN_TEST(test_minutes_seconds);
  RUN_TEST(test_hours_minutes_seconds);

  // Sub-unit capping
  RUN_TEST(test_minutes_capped_with_hours);
  RUN_TEST(test_seconds_capped_with_minutes);
  RUN_TEST(test_seconds_capped_with_hours);
  RUN_TEST(test_large_standalone_units);

  // Bare numbers rejected
  RUN_TEST(test_bare_number_rejected);

  // Invalid formats
  RUN_TEST(test_empty_string);
  RUN_TEST(test_null_pointer);
  RUN_TEST(test_no_digits_before_unit);
  RUN_TEST(test_wrong_order);
  RUN_TEST(test_duplicate_units);
  RUN_TEST(test_invalid_characters);
  RUN_TEST(test_trailing_digits);
  RUN_TEST(test_zero_duration);

  // Range limits
  RUN_TEST(test_max_allowed);
  RUN_TEST(test_over_max_rejected);
  RUN_TEST(test_overflow_digit_string);

  // formatDuration
  RUN_TEST(test_format_zero);
  RUN_TEST(test_format_seconds_only);
  RUN_TEST(test_format_minutes_only);
  RUN_TEST(test_format_minutes_and_seconds);
  RUN_TEST(test_format_hours_only);
  RUN_TEST(test_format_hours_and_minutes);
  RUN_TEST(test_format_hours_and_seconds);
  RUN_TEST(test_format_hours_minutes_seconds);
  RUN_TEST(test_format_max);
  RUN_TEST(test_format_exact_minute);

  return UNITY_END();
}

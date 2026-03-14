#include <unity.h>
#include "system/Logger.h"
#include <string>

extern std::string _mock_serial_output;

static std::string _cb_level;
static std::string _cb_category;
static std::string _cb_msg;
static int _cb_count;

void setUp() {
  _mock_serial_output.clear();
  _cb_level.clear();
  _cb_category.clear();
  _cb_msg.clear();
  _cb_count = 0;
  logger.setOnLog(nullptr);
}

void tearDown() {}

// --- Severity prefixes ---

void test_info_prefix(void) {
  logger.info("hello");
  TEST_ASSERT_EQUAL_STRING("[INFO] hello\n", _mock_serial_output.c_str());
}

void test_warn_prefix(void) {
  logger.warn("oops");
  TEST_ASSERT_EQUAL_STRING("[WARN] oops\n", _mock_serial_output.c_str());
}

void test_error_prefix(void) {
  logger.error("fail");
  TEST_ASSERT_EQUAL_STRING("[ERROR] fail\n", _mock_serial_output.c_str());
}

// --- Category prefixes ---

void test_hardware_prefix(void) {
  logger.hardware("ring motor on");
  TEST_ASSERT_EQUAL_STRING("[HARDWARE] ring motor on\n", _mock_serial_output.c_str());
}

void test_scheduler_prefix(void) {
  logger.scheduler("timer expired");
  TEST_ASSERT_EQUAL_STRING("[SCHEDULER] timer expired\n", _mock_serial_output.c_str());
}

void test_phone_prefix(void) {
  logger.phone("call answered");
  TEST_ASSERT_EQUAL_STRING("[PHONE] call answered\n", _mock_serial_output.c_str());
}

void test_api_prefix(void) {
  logger.api("GET /ring/status");
  TEST_ASSERT_EQUAL_STRING("[API] GET /ring/status\n", _mock_serial_output.c_str());
}

// --- Formatted variants ---

void test_infof_formats(void) {
  logger.infof("val=%d", 42);
  TEST_ASSERT_EQUAL_STRING("[INFO] val=42\n", _mock_serial_output.c_str());
}

void test_warnf_formats(void) {
  logger.warnf("x=%s y=%d", "hello", 7);
  TEST_ASSERT_EQUAL_STRING("[WARN] x=hello y=7\n", _mock_serial_output.c_str());
}

void test_errorf_formats(void) {
  logger.errorf("code=%d", 404);
  TEST_ASSERT_EQUAL_STRING("[ERROR] code=404\n", _mock_serial_output.c_str());
}

void test_hardwaref_formats(void) {
  logger.hardwaref("Handset: %s", "lifted");
  TEST_ASSERT_EQUAL_STRING("[HARDWARE] Handset: lifted\n", _mock_serial_output.c_str());
}

void test_phonef_formats(void) {
  logger.phonef("Ring rejected [%s]: %s", "OFF_HOOK", "us");
  TEST_ASSERT_EQUAL_STRING("[PHONE] Ring rejected [OFF_HOOK]: us\n", _mock_serial_output.c_str());
}

// --- onLog callback ---

void test_callback_fires_on_info(void) {
  logger.setOnLog([](const char* level, const char* category, const char* msg) {
    _cb_level = level; _cb_category = category; _cb_msg = msg; _cb_count++;
  });
  logger.info("hello");
  TEST_ASSERT_EQUAL(1, _cb_count);
  TEST_ASSERT_EQUAL_STRING("info", _cb_level.c_str());
  TEST_ASSERT_EQUAL_STRING("", _cb_category.c_str());
  TEST_ASSERT_EQUAL_STRING("hello", _cb_msg.c_str());
}

void test_callback_fires_on_warn(void) {
  logger.setOnLog([](const char* level, const char* category, const char* msg) {
    _cb_level = level; _cb_category = category; _cb_count++;
  });
  logger.warn("careful");
  TEST_ASSERT_EQUAL(1, _cb_count);
  TEST_ASSERT_EQUAL_STRING("warn", _cb_level.c_str());
  TEST_ASSERT_EQUAL_STRING("", _cb_category.c_str());
}

void test_callback_fires_on_error(void) {
  logger.setOnLog([](const char* level, const char* category, const char* msg) {
    _cb_level = level; _cb_category = category; _cb_count++;
  });
  logger.error("bad");
  TEST_ASSERT_EQUAL(1, _cb_count);
  TEST_ASSERT_EQUAL_STRING("error", _cb_level.c_str());
  TEST_ASSERT_EQUAL_STRING("", _cb_category.c_str());
}

void test_callback_fires_on_infof(void) {
  logger.setOnLog([](const char* level, const char* category, const char* msg) {
    _cb_level = level; _cb_category = category; _cb_msg = msg; _cb_count++;
  });
  logger.infof("val=%d", 42);
  TEST_ASSERT_EQUAL(1, _cb_count);
  TEST_ASSERT_EQUAL_STRING("info", _cb_level.c_str());
  TEST_ASSERT_EQUAL_STRING("", _cb_category.c_str());
  TEST_ASSERT_EQUAL_STRING("val=42", _cb_msg.c_str());
}

void test_callback_fires_with_category(void) {
  logger.setOnLog([](const char* level, const char* category, const char* msg) {
    _cb_level = level; _cb_category = category; _cb_msg = msg; _cb_count++;
  });
  logger.hardware("Ring started: us");
  TEST_ASSERT_EQUAL(1, _cb_count);
  TEST_ASSERT_EQUAL_STRING("info", _cb_level.c_str());
  TEST_ASSERT_EQUAL_STRING("HARDWARE", _cb_category.c_str());
  TEST_ASSERT_EQUAL_STRING("Ring started: us", _cb_msg.c_str());
}

void test_callback_category_correct_for_each_type(void) {
  std::string cats[4];
  int i = 0;
  logger.setOnLog([&cats, &i](const char*, const char* cat, const char*) {
    if (i < 4) cats[i++] = cat;
  });
  logger.hardware("a");
  logger.api("b");
  logger.phone("c");
  logger.scheduler("d");
  TEST_ASSERT_EQUAL_STRING("HARDWARE",  cats[0].c_str());
  TEST_ASSERT_EQUAL_STRING("API",       cats[1].c_str());
  TEST_ASSERT_EQUAL_STRING("PHONE",     cats[2].c_str());
  TEST_ASSERT_EQUAL_STRING("SCHEDULER", cats[3].c_str());
}

void test_callback_not_called_when_unset(void) {
  logger.info("no callback set");
  TEST_ASSERT_EQUAL(0, _cb_count);
}

int main(int argc, char** argv) {
  UNITY_BEGIN();

  RUN_TEST(test_info_prefix);
  RUN_TEST(test_warn_prefix);
  RUN_TEST(test_error_prefix);
  RUN_TEST(test_hardware_prefix);
  RUN_TEST(test_scheduler_prefix);
  RUN_TEST(test_phone_prefix);
  RUN_TEST(test_api_prefix);

  RUN_TEST(test_infof_formats);
  RUN_TEST(test_warnf_formats);
  RUN_TEST(test_errorf_formats);
  RUN_TEST(test_hardwaref_formats);
  RUN_TEST(test_phonef_formats);

  RUN_TEST(test_callback_fires_on_info);
  RUN_TEST(test_callback_fires_on_warn);
  RUN_TEST(test_callback_fires_on_error);
  RUN_TEST(test_callback_fires_on_infof);
  RUN_TEST(test_callback_fires_with_category);
  RUN_TEST(test_callback_category_correct_for_each_type);
  RUN_TEST(test_callback_not_called_when_unset);

  return UNITY_END();
}

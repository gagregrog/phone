#include <unity.h>
#include "system/Logger.h"
#include <string>

extern std::string _mock_serial_output;
extern std::string _mock_telnet_output;
extern int _mock_telnet_available;

static std::string _cb_level;
static std::string _cb_msg;
static int _cb_count;

void setUp() {
  _mock_serial_output.clear();
  _mock_telnet_output.clear();
  _mock_telnet_available = 0;
  _cb_level.clear();
  _cb_msg.clear();
  _cb_count = 0;
  logger.setOnLog(nullptr);
}

void tearDown() {}

// --- Prefixes ---

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

// --- Dual output ---

void test_outputs_to_both_serial_and_telnet(void) {
  logger.info("broadcast");
  TEST_ASSERT_EQUAL_STRING("[INFO] broadcast\n", _mock_serial_output.c_str());
  TEST_ASSERT_EQUAL_STRING("[INFO] broadcast\n", _mock_telnet_output.c_str());
}

// --- onLog callback ---

void test_callback_fires_on_info(void) {
  logger.setOnLog([](const char* level, const char* msg) {
    _cb_level = level; _cb_msg = msg; _cb_count++;
  });
  logger.info("hello");
  TEST_ASSERT_EQUAL(1, _cb_count);
  TEST_ASSERT_EQUAL_STRING("info", _cb_level.c_str());
  TEST_ASSERT_EQUAL_STRING("hello", _cb_msg.c_str());
}

void test_callback_fires_on_warn(void) {
  logger.setOnLog([](const char* level, const char* msg) {
    _cb_level = level; _cb_count++;
  });
  logger.warn("careful");
  TEST_ASSERT_EQUAL(1, _cb_count);
  TEST_ASSERT_EQUAL_STRING("warn", _cb_level.c_str());
}

void test_callback_fires_on_error(void) {
  logger.setOnLog([](const char* level, const char* msg) {
    _cb_level = level; _cb_count++;
  });
  logger.error("bad");
  TEST_ASSERT_EQUAL(1, _cb_count);
  TEST_ASSERT_EQUAL_STRING("error", _cb_level.c_str());
}

void test_callback_fires_on_infof(void) {
  logger.setOnLog([](const char* level, const char* msg) {
    _cb_level = level; _cb_msg = msg; _cb_count++;
  });
  logger.infof("val=%d", 42);
  TEST_ASSERT_EQUAL(1, _cb_count);
  TEST_ASSERT_EQUAL_STRING("info", _cb_level.c_str());
  TEST_ASSERT_EQUAL_STRING("val=42", _cb_msg.c_str());
}

void test_callback_not_called_when_unset(void) {
  logger.info("no callback set");
  TEST_ASSERT_EQUAL(0, _cb_count);
}

// --- Input draining ---

void test_handle_drains_telnet_input(void) {
  _mock_telnet_available = 5;
  logger.handle();
  TEST_ASSERT_EQUAL(0, _mock_telnet_available);
}

int main(int argc, char** argv) {
  UNITY_BEGIN();

  RUN_TEST(test_info_prefix);
  RUN_TEST(test_warn_prefix);
  RUN_TEST(test_error_prefix);
  RUN_TEST(test_infof_formats);
  RUN_TEST(test_warnf_formats);
  RUN_TEST(test_errorf_formats);
  RUN_TEST(test_outputs_to_both_serial_and_telnet);
  RUN_TEST(test_handle_drains_telnet_input);
  RUN_TEST(test_callback_fires_on_info);
  RUN_TEST(test_callback_fires_on_warn);
  RUN_TEST(test_callback_fires_on_error);
  RUN_TEST(test_callback_fires_on_infof);
  RUN_TEST(test_callback_not_called_when_unset);

  return UNITY_END();
}

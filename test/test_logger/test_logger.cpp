#include <unity.h>
#include "system/Logger.h"
#include <string>

extern std::string _mock_serial_output;
extern std::string _mock_telnet_output;
extern int _mock_telnet_available;

void setUp() {
  _mock_serial_output.clear();
  _mock_telnet_output.clear();
  _mock_telnet_available = 0;
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

  return UNITY_END();
}

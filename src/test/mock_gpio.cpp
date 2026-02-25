// Only compiled in the native test environment (via build_src_filter).
// Provides the _mock_pin_values storage referenced by test/mock/Arduino.h.
// Initialized to HIGH to reflect the default pull-up state.
#include <stdint.h>
#include <string.h>

uint8_t _mock_pin_values[40];

struct MockGpioInit {
    MockGpioInit() { memset(_mock_pin_values, 1, sizeof(_mock_pin_values)); }
} _mock_gpio_init;

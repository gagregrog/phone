// Minimal Arduino mock for native unit tests.
// Provides millis() backed by a controllable global, plus basic types/constants.
#pragma once
#include <stdint.h>
#include <stdbool.h>

// Controllable fake clock — tests set this directly
extern unsigned long _mock_millis;
inline unsigned long millis() { return _mock_millis; }

// GPIO constants (unused but needed for compilation)
#define INPUT   0
#define OUTPUT  1
#define INPUT_PULLUP 2
#define HIGH    1
#define LOW     0

// GPIO stubs (no-ops)
inline void pinMode(uint8_t, uint8_t) {}
inline void digitalWrite(uint8_t, uint8_t) {}
inline int  digitalRead(uint8_t) { return LOW; }

// Only compiled in the native test environment (via build_src_filter).
// Provides no-op stubs so that Ringer and tests link without ESP32 timer APIs.
#include "hardware/MotorDriver.h"

MotorDriver::MotorDriver(uint8_t, uint8_t, uint8_t, unsigned long)
    : _in1(0), _in2(0), _ena(0), _halfPeriod(0),
      _active(false), _phase(false) {}

void MotorDriver::begin() {}
void MotorDriver::activate()   { _active = true; }
void MotorDriver::deactivate() { _active = false; }
bool MotorDriver::isActive() const { return _active; }

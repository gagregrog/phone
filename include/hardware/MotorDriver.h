#pragma once
#include <Arduino.h>

class MotorDriver {
public:
  MotorDriver(uint8_t in1Pin, uint8_t in2Pin, uint8_t enaPin,
              unsigned long halfPeriodMs = 25);

  void begin();
  void activate();   // Enable motor oscillation
  void deactivate(); // Stop motor and disable driver
  void update();     // Call every loop iteration to drive oscillation
  bool isActive() const;

private:
  uint8_t _in1;
  uint8_t _in2;
  uint8_t _ena;
  unsigned long _halfPeriod;
  bool _active;
  bool _phase;             // false = direction 1, true = direction 2
  unsigned long _lastToggle;
};

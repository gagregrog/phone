#pragma once
#include <Arduino.h>

class MotorDriver {
public:
  MotorDriver(uint8_t in1Pin, uint8_t in2Pin, uint8_t enaPin,
              unsigned long halfPeriodMs = 25);

  void begin();
  void activate();   // Run one oscillation cycle (two half-periods)
  void deactivate(); // Stop motor and disable driver

private:
  uint8_t _in1;
  uint8_t _in2;
  uint8_t _ena;
  unsigned long _halfPeriod;
};

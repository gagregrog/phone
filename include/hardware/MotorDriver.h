#pragma once
#include <Arduino.h>

class MotorDriver {
public:
  MotorDriver(uint8_t in1Pin, uint8_t in2Pin, uint8_t enaPin,
              unsigned long halfPeriodMs = 25);

  void begin();
  void activate();   // Enable motor oscillation
  void deactivate(); // Stop motor and disable driver
  bool isActive() const;

private:
  uint8_t _in1;
  uint8_t _in2;
  uint8_t _ena;
  unsigned long _halfPeriod;
  volatile bool _active;
  volatile bool _phase;

#ifdef ESP32
  hw_timer_t* _timer;
  static MotorDriver* _instance;
  static void IRAM_ATTR onTimer();
#endif
};

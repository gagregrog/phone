#pragma once
#include <Arduino.h>
#include "MotorDriver.h"
#include "RingPattern.h"

class Ringer {
public:
  explicit Ringer(MotorDriver& motor);

  void ring(const RingPattern& pattern);  // Start ringing with a pattern
  void ringStop();                        // Stop all ringing immediately
  void update();                          // Call every loop iteration
  bool isRinging() const;

private:
  MotorDriver& _motor;

  enum State { IDLE, PATTERN };
  State _state;
  unsigned long _phaseStart;

  const RingPattern* _pattern;
  uint8_t _phaseIndex;
};

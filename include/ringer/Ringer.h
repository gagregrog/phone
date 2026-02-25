#pragma once
#include <Arduino.h>
#include "hardware/MotorDriver.h"
#include "ringer/RingPattern.h"

class Ringer {
public:
  explicit Ringer(MotorDriver& motor);

  void ring(const RingPattern& pattern, uint16_t cycles = 0);  // Start ringing (0 = infinite)
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
  uint16_t _cyclesRemaining;  // 0 = infinite
};

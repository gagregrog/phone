#pragma once
#include <Arduino.h>
#include "MotorDriver.h"

class Ringer {
public:
  explicit Ringer(MotorDriver& motor);

  void ringStart();    // Start continuous ringing
  void ringStop();     // Stop all ringing immediately
  void ringPattern();  // Start standard US cadence (2s on, 4s off)
  void update();       // Call every loop iteration to drive the ringer
  bool isRinging() const;

private:
  MotorDriver& _motor;

  enum State { IDLE, CONTINUOUS, PATTERN_ON, PATTERN_OFF };
  State _state;
  unsigned long _phaseStart;

  static const unsigned long PATTERN_ON_MS  = 2000;
  static const unsigned long PATTERN_OFF_MS = 4000;
};

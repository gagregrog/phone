#pragma once
#include <Arduino.h>
#include <functional>
#include "hardware/MotorDriver.h"
#include "ringer/RingPattern.h"

class Ringer {
public:
  explicit Ringer(MotorDriver& motor);

  bool ring(const RingPattern& pattern, uint16_t cycles = 0, bool force = false);  // Returns false if suppressed by guard
  void ringStop();                        // Stop all ringing immediately
  void update();                          // Call every loop iteration
  bool isRinging() const;
  void setOnStop(std::function<void()> cb);                    // Called when fixed-cycle ring completes naturally
  void setOnStart(std::function<void(const char*)> cb);       // Called when ring actually starts
  void setOnBlocked(std::function<void(const char*)> cb);     // Called when guard suppresses a ring
  void setRingGuard(std::function<bool()> guard);             // Returns false to suppress ring

private:
  MotorDriver& _motor;

  enum State { IDLE, PATTERN };
  State _state;
  unsigned long _phaseStart;

  const RingPattern* _pattern;
  uint8_t _phaseIndex;
  uint16_t _cyclesRemaining;  // 0 = infinite
  std::function<void()>            _onStop;
  std::function<void(const char*)> _onStart;
  std::function<void(const char*)> _onBlocked;
  std::function<bool()>            _guard;
};

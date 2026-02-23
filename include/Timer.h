#pragma once
#include <Arduino.h>
#include "Ringer.h"
#include "RingPattern.h"

class Timer {
public:
  explicit Timer(Ringer& ringer);

  void start(unsigned long durationMs, const RingPattern& pattern, uint16_t fireCycles = 3);
  void cancel();
  void update();

  bool isActive() const;
  unsigned long remainingMs() const;
  unsigned long totalMs() const;
  const char* patternName() const;

private:
  Ringer& _ringer;

  bool _active;
  unsigned long _startMs;
  unsigned long _durationMs;
  const RingPattern* _pattern;
  uint16_t _fireCycles;
};

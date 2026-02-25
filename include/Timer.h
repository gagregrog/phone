#pragma once
#include <Arduino.h>
#include <functional>
#include <vector>
#include "Ringer.h"
#include "RingPattern.h"

struct TimerInfo {
  uint32_t id;
  unsigned long remainingMs;
  unsigned long totalMs;
  const char* patternName;
};

class Timer {
public:
  explicit Timer(Ringer& ringer);

  uint32_t start(unsigned long durationMs, const RingPattern& pattern, uint16_t fireCycles = 1);
  bool cancel(uint32_t id);
  void cancelAll();
  void update();

  void setOnFire(std::function<void(uint32_t id, const char* pattern)> cb);

  bool hasActive() const;
  size_t count() const;
  TimerInfo infoAt(size_t index) const;

private:
  struct Entry {
    uint32_t id;
    unsigned long startMs;
    unsigned long durationMs;
    const RingPattern* pattern;
    uint16_t fireCycles;
  };

  Ringer& _ringer;
  std::vector<Entry> _entries;
  uint32_t _nextId;
  std::function<void(uint32_t, const char*)> _onFire;
};

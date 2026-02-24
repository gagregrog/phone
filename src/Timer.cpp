#include "Timer.h"

Timer::Timer(Ringer& ringer)
  : _ringer(ringer), _nextId(1) {}

uint32_t Timer::start(unsigned long durationMs, const RingPattern& pattern, uint16_t fireCycles) {
  Entry e;
  e.id = _nextId++;
  e.startMs = millis();
  e.durationMs = durationMs;
  e.pattern = &pattern;
  e.fireCycles = fireCycles;
  _entries.push_back(e);
  return e.id;
}

bool Timer::cancel(uint32_t id) {
  for (size_t i = 0; i < _entries.size(); ++i) {
    if (_entries[i].id == id) {
      _entries.erase(_entries.begin() + i);
      return true;
    }
  }
  return false;
}

void Timer::cancelAll() {
  _entries.clear();
}

void Timer::update() {
  unsigned long now = millis();
  size_t i = 0;
  while (i < _entries.size()) {
    Entry& e = _entries[i];
    if (now - e.startMs >= e.durationMs) {
      _ringer.ringStop();
      _ringer.ring(*e.pattern, e.fireCycles);
      _entries.erase(_entries.begin() + i);
    } else {
      ++i;
    }
  }
}

bool Timer::hasActive() const {
  return !_entries.empty();
}

size_t Timer::count() const {
  return _entries.size();
}

TimerInfo Timer::infoAt(size_t index) const {
  const Entry& e = _entries[index];
  unsigned long elapsed = millis() - e.startMs;
  unsigned long remaining = (elapsed >= e.durationMs) ? 0 : (e.durationMs - elapsed);
  TimerInfo info;
  info.id = e.id;
  info.remainingMs = remaining;
  info.totalMs = e.durationMs;
  info.patternName = e.pattern ? e.pattern->name : nullptr;
  return info;
}

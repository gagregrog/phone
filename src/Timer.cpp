#include "Timer.h"

Timer::Timer(Ringer& ringer)
  : _ringer(ringer),
    _active(false),
    _startMs(0),
    _durationMs(0),
    _pattern(nullptr),
    _fireCycles(3) {}

void Timer::start(unsigned long durationMs, const RingPattern& pattern, uint16_t fireCycles) {
  _durationMs = durationMs;
  _pattern = &pattern;
  _fireCycles = fireCycles;
  _startMs = millis();
  _active = true;
}

void Timer::cancel() {
  _active = false;
}

void Timer::update() {
  if (!_active) return;

  unsigned long elapsed = millis() - _startMs;
  if (elapsed >= _durationMs) {
    _active = false;
    _ringer.ringStop();
    _ringer.ring(*_pattern, _fireCycles);
  }
}

bool Timer::isActive() const {
  return _active;
}

unsigned long Timer::remainingMs() const {
  if (!_active) return 0;
  unsigned long elapsed = millis() - _startMs;
  if (elapsed >= _durationMs) return 0;
  return _durationMs - elapsed;
}

unsigned long Timer::totalMs() const {
  return _active ? _durationMs : 0;
}

const char* Timer::patternName() const {
  return (_active && _pattern) ? _pattern->name : nullptr;
}

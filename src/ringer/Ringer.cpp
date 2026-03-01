#include "ringer/Ringer.h"
#include "system/Logger.h"

Ringer::Ringer(MotorDriver& motor)
    : _motor(motor), _state(IDLE), _phaseStart(0),
      _pattern(nullptr), _phaseIndex(0), _cyclesRemaining(0),
      _onStop(nullptr), _onStart(nullptr), _onBlocked(nullptr), _guard(nullptr) {}

bool Ringer::ring(const RingPattern& pattern, uint16_t cycles) {
  if (_guard && !_guard()) {
    logger.phonef("Ring suppressed: %s", pattern.name);
    if (_onBlocked) _onBlocked(pattern.name);
    return false;
  }
  _pattern = &pattern;
  _phaseIndex = 0;
  _phaseStart = millis();
  _cyclesRemaining = cycles;
  _state = PATTERN;
  _motor.activate();  // Phase 0 is always ON
  if (_onStart) _onStart(pattern.name);
  return true;
}

void Ringer::ringStop() {
  _state = IDLE;
  _motor.deactivate();
}

bool Ringer::isRinging() const {
  return _state != IDLE;
}

void Ringer::setOnStop(std::function<void()> cb) {
  _onStop = std::move(cb);
}

void Ringer::setOnStart(std::function<void(const char*)> cb) {
  _onStart = std::move(cb);
}

void Ringer::setOnBlocked(std::function<void(const char*)> cb) {
  _onBlocked = std::move(cb);
}

void Ringer::setRingGuard(std::function<bool()> guard) {
  _guard = std::move(guard);
}

void Ringer::update() {
  if (_state == PATTERN) {
    if (millis() - _phaseStart >= _pattern->phases[_phaseIndex]) {
      _phaseIndex++;
      if (_phaseIndex >= _pattern->phaseCount) {
        _phaseIndex = 0;
        if (_cyclesRemaining > 0) {
          _cyclesRemaining--;
          if (_cyclesRemaining == 0) {
            ringStop();
            if (_onStop) _onStop();
            return;
          }
        }
      }

      _phaseStart = millis();

      // Even indices are ON, odd indices are OFF
      if (_phaseIndex % 2 == 0) {
        _motor.activate();
      } else {
        _motor.deactivate();
      }
    }
  }

  _motor.update();
}

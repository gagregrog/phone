#include "Ringer.h"

Ringer::Ringer(MotorDriver& motor)
    : _motor(motor), _state(IDLE), _phaseStart(0),
      _pattern(nullptr), _phaseIndex(0), _cyclesRemaining(0) {}

void Ringer::ring(const RingPattern& pattern, uint16_t cycles) {
  _pattern = &pattern;
  _phaseIndex = 0;
  _phaseStart = millis();
  _cyclesRemaining = cycles;
  _state = PATTERN;
  _motor.activate();  // Phase 0 is always ON
}

void Ringer::ringStop() {
  _state = IDLE;
  _motor.deactivate();
}

bool Ringer::isRinging() const {
  return _state != IDLE;
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

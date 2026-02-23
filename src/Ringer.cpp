#include "Ringer.h"

Ringer::Ringer(MotorDriver& motor)
    : _motor(motor), _state(IDLE), _phaseStart(0) {}

void Ringer::ringStart() {
  _state = CONTINUOUS;
}

void Ringer::ringStop() {
  _state = IDLE;
  _motor.deactivate();
}

void Ringer::ringPattern() {
  _state = PATTERN_ON;
  _phaseStart = millis();
}

bool Ringer::isRinging() const {
  return _state != IDLE;
}

void Ringer::update() {
  switch (_state) {
    case IDLE:
      break;

    case CONTINUOUS:
      _motor.activate();
      break;

    case PATTERN_ON:
      if (millis() - _phaseStart >= PATTERN_ON_MS) {
        _motor.deactivate();
        _state = PATTERN_OFF;
        _phaseStart = millis();
      } else {
        _motor.activate();
      }
      break;

    case PATTERN_OFF:
      if (millis() - _phaseStart >= PATTERN_OFF_MS) {
        _state = PATTERN_ON;
        _phaseStart = millis();
      }
      break;
  }
}

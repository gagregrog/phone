#include "Ringer.h"

Ringer::Ringer(MotorDriver& motor)
    : _motor(motor), _state(IDLE), _phaseStart(0) {}

void Ringer::ringStart() {
  _state = CONTINUOUS;
  _motor.activate();
}

void Ringer::ringStop() {
  _state = IDLE;
  _motor.deactivate();
}

void Ringer::ringPattern() {
  _state = PATTERN_ON;
  _phaseStart = millis();
  _motor.activate();
}

bool Ringer::isRinging() const {
  return _state != IDLE;
}

void Ringer::update() {
  switch (_state) {
    case IDLE:
      break;

    case CONTINUOUS:
      break;

    case PATTERN_ON:
      if (millis() - _phaseStart >= PATTERN_ON_MS) {
        _motor.deactivate();
        _state = PATTERN_OFF;
        _phaseStart = millis();
      }
      break;

    case PATTERN_OFF:
      if (millis() - _phaseStart >= PATTERN_OFF_MS) {
        _motor.activate();
        _state = PATTERN_ON;
        _phaseStart = millis();
      }
      break;
  }

  _motor.update();
}

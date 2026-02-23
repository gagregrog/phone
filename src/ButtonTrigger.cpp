#include "ButtonTrigger.h"

ButtonTrigger::ButtonTrigger(uint8_t pin, bool activeLow,
                             unsigned long debounceMs)
    : _pin(pin), _activeLow(activeLow), _debounceMs(debounceMs),
      _lastChangeTime(0), _lastRaw(false), _stable(false) {}

void ButtonTrigger::begin() {
  pinMode(_pin, _activeLow ? INPUT_PULLUP : INPUT);
  _stable = digitalRead(_pin) == (_activeLow ? LOW : HIGH);
  _lastRaw = _stable;
}

bool ButtonTrigger::isPressed() {
  bool raw = digitalRead(_pin) == (_activeLow ? LOW : HIGH);

  if (raw != _lastRaw) {
    _lastChangeTime = millis();
    _lastRaw = raw;
  }

  if ((millis() - _lastChangeTime) >= _debounceMs) {
    _stable = _lastRaw;
  }

  return _stable;
}

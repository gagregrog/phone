#include "ButtonTrigger.h"

ButtonTrigger::ButtonTrigger(uint8_t pin, bool activeLow)
    : _pin(pin), _activeLow(activeLow) {}

void ButtonTrigger::begin() {
  pinMode(_pin, _activeLow ? INPUT_PULLUP : INPUT);
}

bool ButtonTrigger::isPressed() const {
  return digitalRead(_pin) == (_activeLow ? LOW : HIGH);
}

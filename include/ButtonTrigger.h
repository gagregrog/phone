#pragma once
#include <Arduino.h>

class ButtonTrigger {
public:
  explicit ButtonTrigger(uint8_t pin, bool activeLow = true);

  void begin();
  bool isPressed() const;

private:
  uint8_t _pin;
  bool _activeLow;
};

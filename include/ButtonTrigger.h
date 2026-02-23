#pragma once
#include <Arduino.h>

class ButtonTrigger {
public:
  explicit ButtonTrigger(uint8_t pin, bool activeLow = true,
                         unsigned long debounceMs = 50);

  void begin();
  void update();          // Call every loop iteration
  bool isPressed() const; // Current debounced state
  bool wasPressed();      // True once per press (rising edge)

private:
  uint8_t _pin;
  bool _activeLow;
  unsigned long _debounceMs;
  unsigned long _lastChangeTime;
  bool _lastRaw;
  bool _stable;
  bool _prevStable;
};

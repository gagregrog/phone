#include "MotorDriver.h"

MotorDriver::MotorDriver(uint8_t in1Pin, uint8_t in2Pin, uint8_t enaPin,
                         unsigned long halfPeriodMs)
    : _in1(in1Pin), _in2(in2Pin), _ena(enaPin), _halfPeriod(halfPeriodMs),
      _active(false), _phase(false), _lastToggle(0) {}

void MotorDriver::begin() {
  pinMode(_in1, OUTPUT);
  pinMode(_in2, OUTPUT);
  pinMode(_ena, OUTPUT);
  digitalWrite(_ena, LOW);
}

void MotorDriver::activate() {
  if (!_active) {
    _active = true;
    _phase = false;
    _lastToggle = millis();
    digitalWrite(_ena, HIGH);
    digitalWrite(_in1, HIGH);
    digitalWrite(_in2, LOW);
  }
}

void MotorDriver::deactivate() {
  _active = false;
  digitalWrite(_in1, LOW);
  digitalWrite(_in2, LOW);
  digitalWrite(_ena, LOW);
}

void MotorDriver::update() {
  if (!_active) return;

  if (millis() - _lastToggle >= _halfPeriod) {
    _phase = !_phase;
    _lastToggle = millis();

    if (_phase) {
      digitalWrite(_in1, LOW);
      digitalWrite(_in2, HIGH);
    } else {
      digitalWrite(_in1, HIGH);
      digitalWrite(_in2, LOW);
    }
  }
}

bool MotorDriver::isActive() const {
  return _active;
}

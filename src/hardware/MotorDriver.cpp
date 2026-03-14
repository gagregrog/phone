#include "hardware/MotorDriver.h"

MotorDriver* MotorDriver::_instance = nullptr;

void IRAM_ATTR MotorDriver::onTimer() {
  MotorDriver* self = _instance;
  if (!self || !self->_active) return;

  self->_phase = !self->_phase;
  if (self->_phase) {
    digitalWrite(self->_in1, LOW);
    digitalWrite(self->_in2, HIGH);
  } else {
    digitalWrite(self->_in1, HIGH);
    digitalWrite(self->_in2, LOW);
  }
}

MotorDriver::MotorDriver(uint8_t in1Pin, uint8_t in2Pin, uint8_t enaPin,
                         unsigned long halfPeriodMs)
    : _in1(in1Pin), _in2(in2Pin), _ena(enaPin), _halfPeriod(halfPeriodMs),
      _active(false), _phase(false), _timer(nullptr) {
  _instance = this;
}

void MotorDriver::begin() {
  pinMode(_in1, OUTPUT);
  pinMode(_in2, OUTPUT);
  pinMode(_ena, OUTPUT);
  digitalWrite(_ena, LOW);

  // 80 MHz APB clock / 80 prescaler = 1 MHz (1 us per tick)
  _timer = timerBegin(0, 80, true);
  timerAttachInterrupt(_timer, &MotorDriver::onTimer, true);
  timerAlarmWrite(_timer, _halfPeriod * 1000, true);
}

void MotorDriver::activate() {
  if (!_active) {
    _active = true;
    _phase = false;
    digitalWrite(_ena, HIGH);
    digitalWrite(_in1, HIGH);
    digitalWrite(_in2, LOW);
    timerAlarmEnable(_timer);
  }
}

void MotorDriver::deactivate() {
  timerAlarmDisable(_timer);
  _active = false;
  digitalWrite(_in1, LOW);
  digitalWrite(_in2, LOW);
  digitalWrite(_ena, LOW);
}

bool MotorDriver::isActive() const {
  return _active;
}

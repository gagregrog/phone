#include "MotorDriver.h"

MotorDriver::MotorDriver(uint8_t in1Pin, uint8_t in2Pin, uint8_t enaPin,
                         unsigned long halfPeriodMs)
    : _in1(in1Pin), _in2(in2Pin), _ena(enaPin), _halfPeriod(halfPeriodMs) {}

void MotorDriver::begin() {
  pinMode(_in1, OUTPUT);
  pinMode(_in2, OUTPUT);
  pinMode(_ena, OUTPUT);
  digitalWrite(_ena, LOW);
}

void MotorDriver::activate() {
  digitalWrite(_ena, HIGH);

  // Direction 1
  digitalWrite(_in1, HIGH);
  digitalWrite(_in2, LOW);
  delay(_halfPeriod);

  // Direction 2
  digitalWrite(_in1, LOW);
  digitalWrite(_in2, HIGH);
  delay(_halfPeriod);
}

void MotorDriver::deactivate() {
  digitalWrite(_in1, LOW);
  digitalWrite(_in2, LOW);
  digitalWrite(_ena, LOW);
}

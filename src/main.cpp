#include <Arduino.h>
#include "pins.h"
#include "MotorDriver.h"
#include "ButtonTrigger.h"

MotorDriver motor(PIN_MOTOR_IN1, PIN_MOTOR_IN2, PIN_MOTOR_ENA);
ButtonTrigger button(PIN_TRIGGER);

void setup() {
  Serial.begin(115200);
  motor.begin();
  button.begin();
}

void loop() {
  bool triggered = button.isPressed();
  Serial.println(triggered);

  if (triggered) {
    motor.activate();
  } else {
    motor.deactivate();
    delay(10);
  }
}

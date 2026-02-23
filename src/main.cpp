#include <Arduino.h>
#include "pins.h"
#include "MotorDriver.h"
#include "ButtonTrigger.h"
#include "Ringer.h"
#include "WifiSetup.h"
#include "RingerAPI.h"

MotorDriver motor(PIN_MOTOR_IN1, PIN_MOTOR_IN2, PIN_MOTOR_ENA);
ButtonTrigger button(PIN_TRIGGER);
Ringer ringer(motor);

void setup() {
  Serial.begin(115200);
  motor.begin();
  button.begin();

  wifiSetupBegin("PhoneSetup");
  ringerAPIBegin(ringer);
}

void loop() {
  bool triggered = button.isPressed();

  if (triggered && !ringer.isRinging()) {
    ringer.ringPattern();
  } else if (!triggered && ringer.isRinging()) {
    ringer.ringStop();
  }

  ringer.update();
}

#include <Arduino.h>
#include "pins.h"
#include "MotorDriver.h"
#include "ButtonTrigger.h"
#include "Ringer.h"
#include "RingPattern.h"
#include "WifiSetup.h"
#include "RingerAPI.h"
#include "Timer.h"

MotorDriver motor(PIN_MOTOR_IN1, PIN_MOTOR_IN2, PIN_MOTOR_ENA);
ButtonTrigger button(PIN_TRIGGER);
Ringer ringer(motor);
Timer timer(ringer);

void setup() {
  Serial.begin(115200);
  motor.begin();
  button.begin();

  wifiSetupBegin("PhoneSetup");
  ringerAPIBegin(ringer, timer);
}

void loop() {
  button.update();

  if (button.wasPressed()) {
    if (ringer.isRinging()) {
      ringer.ringStop();
    } else {
      ringer.ring(PATTERN_US);
    }
  }

  timer.update();
  ringer.update();
}

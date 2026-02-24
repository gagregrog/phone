#include <Arduino.h>
#include <ArduinoOTA.h>
#include "pins.h"
#include "Logger.h"
#include "MotorDriver.h"
#include "ButtonTrigger.h"
#include "Ringer.h"
#include "RingPattern.h"
#include "WifiSetup.h"
#include "API.h"
#include "DeviceAPI.h"
#include "RingerAPI.h"
#include "TimerAPI.h"
#include "Timer.h"
#include "NVSAlarmStore.h"
#include "AlarmManager.h"
#include "AlarmClock.h"
#include "AlarmAPI.h"

#ifndef TZ_STRING
#define TZ_STRING "UTC0"
#endif

MotorDriver motor(PIN_MOTOR_IN1, PIN_MOTOR_IN2, PIN_MOTOR_ENA);
ButtonTrigger button(PIN_TRIGGER);
Ringer ringer(motor);
Timer timer(ringer);
NVSAlarmStore alarmStore;
AlarmManager  alarmMgr(ringer, alarmStore, alarmClockGetLocalTime);

void setup() {
    Serial.begin(115200);
    motor.begin();
    button.begin();

    wifiSetupBegin("PhoneSetup");
    logger.begin();

    alarmClockBegin(TZ_STRING);
    alarmMgr.init();
    apiInit();
    deviceAPIBegin();
    ringerAPIBegin(ringer);
    timerAPIBegin(timer);
    alarmAPIBegin(alarmMgr);
    apiStart();
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

    logger.handle();
    ArduinoOTA.handle();
    timer.update();
    alarmMgr.tick();
    ringer.update();
}

#include <Arduino.h>
#include <ArduinoOTA.h>
#include "hardware/pins.h"
#include "system/Logger.h"
#include "hardware/MotorDriver.h"
#include "hardware/ButtonTrigger.h"
#include "hardware/DialReader.h"
#include "ringer/Ringer.h"
#include "ringer/RingPattern.h"
#include "system/WifiSetup.h"
#include "system/LogEvents.h"
#include "web/API.h"
#include "web/DeviceAPI.h"
#include "ringer/RingerAPI.h"
#include "timer/TimerAPI.h"
#include "timer/Timer.h"
#include "alarm/NVSAlarmStore.h"
#include "alarm/AlarmManager.h"
#include "clock/Clock.h"
#include "alarm/AlarmAPI.h"
#include "clock/ClockManager.h"
#include "clock/ClockAPI.h"
#include "timer/TimerEvents.h"
#include "alarm/AlarmEvents.h"
#include "clock/ClockEvents.h"
#include "web/WebUI.h"
#include "web/Events.h"
#include "web/WebSocketAPI.h"

#ifndef TZ_STRING
#define TZ_STRING "UTC0"
#endif

MotorDriver motor(PIN_MOTOR_IN1, PIN_MOTOR_IN2, PIN_MOTOR_ENA);
ButtonTrigger button(PIN_TRIGGER);
DialReader dialReader;
Ringer ringer(motor);
Timer timer(ringer);
NVSAlarmStore alarmStore;
AlarmManager  alarmMgr(ringer, alarmStore, clockGetLocalTime);
ClockManager  clockMgr(ringer, clockGetLocalTime);

void setup() {
    Serial.begin(115200);
    motor.begin();
    button.begin();
    dialReader.begin();
    dialReader.setOnDigit([](int d) { logger.infof("Dialed: %d", d); });

    wifiSetupBegin("PhoneSetup");
    logger.begin();

    clockBegin(TZ_STRING);
    alarmMgr.init();
    timerEventsBegin(timer);
    alarmEventsBegin(alarmMgr);
    clockEventsBegin(clockMgr);
    logEventsBegin();

    apiInit();
    deviceAPIBegin();
    ringerAPIBegin(ringer);
    timerAPIBegin(timer);
    alarmAPIBegin(alarmMgr);
    clockAPIBegin(clockMgr);
    webSocketAPIBegin();
    webUIBegin();
    apiStart();
}

void loop() {
    button.update();

    if (button.wasPressed()) {
        if (ringer.isRinging()) {
            ringer.ringStop();
            eventsPublish("ring/stopped", "{}");
        } else {
            ringer.ring(PATTERN_US);
            eventsPublish("ring/started", "{\"pattern\":\"us\"}");
        }
    }

    dialReader.tick();
    logger.handle();
    ArduinoOTA.handle();
    webSocketLoop();
    timer.update();
    alarmMgr.tick();
    clockMgr.tick();
    ringer.update();
}

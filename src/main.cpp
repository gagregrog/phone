#include <Arduino.h>
#include <ArduinoOTA.h>
#include "hardware/pins.h"
#include "system/Logger.h"
#include "hardware/MotorDriver.h"
#include "hardware/ButtonTrigger.h"
#include "hardware/DialReader.h"
#include "hardware/HandsetMonitor.h"
#include "hardware/HandsetEvents.h"
#include "hardware/HandsetAPI.h"
#include "hardware/DialManager.h"
#include "hardware/DialManagerEvents.h"
#include "hardware/DialManagerAPI.h"
#include "ringer/Ringer.h"
#include "ringer/RingPattern.h"
#include "phone/PhoneController.h"
#include "system/WifiSetup.h"
#include "system/LogEvents.h"
#include "web/API.h"
#include "web/DeviceAPI.h"
#include "ringer/RingerAPI.h"
#include "phone/PhoneAPI.h"
#include "timer/TimerAPI.h"
#include "timer/Timer.h"
#include "alarm/NVSAlarmStore.h"
#include "alarm/AlarmManager.h"
#include "clock/Clock.h"
#include "alarm/AlarmAPI.h"
#include "clock/ClockManager.h"
#include "clock/ClockAPI.h"
#include "phonebook/NVSPhoneBookStore.h"
#include "phonebook/PhoneBookManager.h"
#include "phonebook/PhoneBookCaller.h"
#include "phonebook/PhoneBookAPI.h"
#include "timer/TimerEvents.h"
#include "alarm/AlarmEvents.h"
#include "clock/ClockEvents.h"
#include "web/WebUI.h"
#include "web/Events.h"
#include "ringer/RingerEvents.h"
#include "web/WebSocketAPI.h"
#include "hardware/MicReader.h"
#include "hardware/MicEvents.h"

#ifndef TZ_STRING
#define TZ_STRING "UTC0"
#endif

MotorDriver motor(PIN_MOTOR_IN1, PIN_MOTOR_IN2, PIN_MOTOR_ENA);
ButtonTrigger button(PIN_TRIGGER);
DialReader dialReader;
HandsetMonitor handset(PIN_HANDSET);
Ringer ringer(motor);
Timer timer(ringer);
NVSAlarmStore alarmStore;
AlarmManager  alarmMgr(ringer, alarmStore, clockGetLocalTime);
ClockManager  clockMgr(ringer, clockGetLocalTime);
DialManager   dialMgr(dialReader, handset);
PhoneController phoneCtrl(ringer, handset, dialMgr);
NVSPhoneBookStore phoneBookStore;
PhoneBookManager  phoneBookMgr(phoneBookStore);
MicReader micReader(ADC1_CHANNEL_6, 44100, 1024);

void setup() {
    Serial.begin(115200);
    motor.begin();
    button.begin();
    handset.begin();
    dialReader.begin();
    dialMgr.begin();

    wifiSetupBegin("PhoneSetup");

    clockBegin(TZ_STRING);
    alarmMgr.init();
    phoneBookMgr.init();

    // Guard internal ring callers (AlarmManager, ClockManager, Timer) from
    // ringing when the handset is off-hook.
    ringer.setRingGuard([&]() -> bool { return !handset.isOffHook(); });

    ringerEventsBegin(ringer);
    handsetEventsBegin(handset);
    dialManagerEventsBegin(dialMgr);
    phoneCtrl.begin();   // subscribes to ring/started, ring/stopped; adds handset+dial callbacks
    phoneBookCallerBegin(phoneBookMgr, phoneCtrl);
    timerEventsBegin(timer);
    alarmEventsBegin(alarmMgr);
    clockEventsBegin(clockMgr);
    logEventsBegin();
    micReader.begin();

    apiInit();
    deviceAPIBegin();
    ringerAPIBegin(phoneCtrl);
    phoneAPIBegin(phoneCtrl);
    timerAPIBegin(timer);
    alarmAPIBegin(alarmMgr);
    clockAPIBegin(clockMgr);
    handsetAPIBegin(handset);
    dialManagerAPIBegin(dialMgr);
    phoneBookAPIBegin(phoneBookMgr);
    webSocketAPIBegin();
    micEventsBegin(micReader);
    micReader.startTask();
    webUIBegin();
    apiStart();
}

void loop() {
    button.update();
    handset.update();

    if (button.wasPressed()) {
        if (phoneCtrl.isRinging()) {
            phoneCtrl.ringStop();
        } else {
            phoneCtrl.ring(PATTERN_US);
        }
    }

    dialReader.tick();
    dialMgr.tick();
    ArduinoOTA.handle();
    webSocketLoop();
    phoneCtrl.tick(millis());
    timer.update();
    alarmMgr.tick();
    clockMgr.tick();
    ringer.update();
}

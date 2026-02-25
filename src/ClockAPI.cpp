#include "ClockAPI.h"
#include "ClockJSON.h"
#include "API.h"
#include "Events.h"
#include "Logger.h"
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <Preferences.h>

static ClockManager* _clockMgr = nullptr;

static void sendJson(AsyncWebServerRequest* req, int code, JsonDocument& doc) {
    String body;
    serializeJson(doc, body);
    req->send(code, "application/json", body);
}

static ChimeMode chimeModeFromString(const char* s) {
    return (strcmp(s, "n_chimes") == 0) ? CHIME_N_CHIMES : CHIME_SINGLE;
}

void clockAPIBegin(ClockManager& mgr) {
    _clockMgr = &mgr;

    // Load persisted state (defaults: enabled=true, mode="n_chimes")
    Preferences prefs;
    prefs.begin("clock", true);
    bool enabled = prefs.getBool("enabled", false);
    String modeStr = prefs.getString("mode", "n_chimes");
    prefs.end();
    _clockMgr->setEnabled(enabled);
    _clockMgr->setChimeMode(chimeModeFromString(modeStr.c_str()));

    AsyncWebServer* server = apiGetServer();

    // GET /clock — return enabled state and chime mode
    server->on("/clock", HTTP_GET, [](AsyncWebServerRequest* req) {
        logger.info("GET /clock");
        JsonDocument doc;
        clockStateFillJson(doc.to<JsonObject>(), _clockMgr->isEnabled(), _clockMgr->getChimeMode());
        sendJson(req, 200, doc);
    });

    // POST /clock/toggle — flip enabled, persist, return new state
    server->on("/clock/toggle", HTTP_POST, [](AsyncWebServerRequest* req) {
        bool newEnabled = !_clockMgr->isEnabled();
        _clockMgr->setEnabled(newEnabled);

        Preferences prefs;
        prefs.begin("clock", false);
        prefs.putBool("enabled", newEnabled);
        prefs.end();

        logger.infof("POST /clock/toggle: enabled=%d", (int)newEnabled);

        JsonDocument doc;
        clockStateFillJson(doc.to<JsonObject>(), _clockMgr->isEnabled(), _clockMgr->getChimeMode());
        String body;
        serializeJson(doc, body);
        req->send(200, "application/json", body);
        eventsPublish("clock/updated", body.c_str());
    });

    // POST /clock/mode/toggle — flip chime mode, persist, return new state
    server->on("/clock/mode/toggle", HTTP_POST, [](AsyncWebServerRequest* req) {
        ChimeMode newMode = (_clockMgr->getChimeMode() == CHIME_N_CHIMES)
                          ? CHIME_SINGLE : CHIME_N_CHIMES;
        _clockMgr->setChimeMode(newMode);

        Preferences prefs;
        prefs.begin("clock", false);
        prefs.putString("mode", chimeModeToString(newMode));
        prefs.end();

        logger.infof("POST /clock/mode/toggle: mode=%s", chimeModeToString(newMode));

        JsonDocument doc;
        clockStateFillJson(doc.to<JsonObject>(), _clockMgr->isEnabled(), _clockMgr->getChimeMode());
        String body;
        serializeJson(doc, body);
        req->send(200, "application/json", body);
        eventsPublish("clock/updated", body.c_str());
    });
}

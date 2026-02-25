#include "ClockAPI.h"
#include "API.h"
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

static const char* chimeModeToString(ChimeMode mode) {
    return (mode == CHIME_N_CHIMES) ? "n_chimes" : "single";
}

static ChimeMode chimeModeFromString(const char* s) {
    return (strcmp(s, "n_chimes") == 0) ? CHIME_N_CHIMES : CHIME_SINGLE;
}

static void buildStateDoc(JsonDocument& doc) {
    doc["enabled"] = _clockMgr->isEnabled();
    doc["mode"]    = chimeModeToString(_clockMgr->getChimeMode());
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
        buildStateDoc(doc);
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
        buildStateDoc(doc);
        sendJson(req, 200, doc);
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
        buildStateDoc(doc);
        sendJson(req, 200, doc);
    });
}

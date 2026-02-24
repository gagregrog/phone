#include "AlarmAPI.h"
#include "API.h"
#include "AlarmClock.h"
#include "Logger.h"
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <stdlib.h>

static AlarmManager* _alarmMgr = nullptr;

static void sendJson(AsyncWebServerRequest* req, int code, JsonDocument& doc) {
    String body;
    serializeJson(doc, body);
    req->send(code, "application/json", body);
}

static void alarmToJson(JsonObject obj, const AlarmEntry& e) {
    char timeBuf[6];
    snprintf(timeBuf, sizeof(timeBuf), "%02u:%02u", (unsigned)e.hour, (unsigned)e.minute);
    obj["id"]           = e.id;
    obj["time"]         = timeBuf;
    obj["pattern"]      = e.patternName;
    obj["rings"]        = e.rings;
    obj["repeat"]       = e.repeat;
    obj["skipWeekends"] = e.skipWeekends;
    obj["enabled"]      = e.enabled;
}

void alarmAPIBegin(AlarmManager& mgr) {
    _alarmMgr = &mgr;
    AsyncWebServer* server = apiGetServer();

    // GET /alarm — list all alarms
    server->on("/alarm", HTTP_GET, [](AsyncWebServerRequest* req) {
        logger.info("GET /alarm");
        JsonDocument doc;
        JsonArray arr = doc.to<JsonArray>();
        for (const auto& e : _alarmMgr->getAll()) {
            alarmToJson(arr.add<JsonObject>(), e);
        }
        sendJson(req, 200, doc);
    });

    // DELETE /alarm — delete all alarms
    server->on("/alarm", HTTP_DELETE, [](AsyncWebServerRequest* req) {
        logger.info("DELETE /alarm");
        _alarmMgr->removeAll();
        JsonDocument doc;
        doc["status"] = "cleared";
        sendJson(req, 200, doc);
    });

    // POST /alarm — create alarm (body collected via the inline body handler below)
    server->on("/alarm", HTTP_POST,
        [](AsyncWebServerRequest* req) {
            if (!req->_tempObject) {
                req->send(400, "application/json", "{\"error\":\"missing body\"}");
                return;
            }

            JsonDocument doc;
            DeserializationError err = deserializeJson(doc, (const char*)req->_tempObject);
            free(req->_tempObject);
            req->_tempObject = nullptr;

            if (err != DeserializationError::Ok) {
                req->send(400, "application/json", "{\"error\":\"invalid JSON\"}");
                return;
            }

            uint8_t  hour         = doc["hour"]         | (uint8_t)255;
            uint8_t  minute       = doc["minute"]        | (uint8_t)255;
            const char* pattern   = doc["pattern"]       | "us";
            uint16_t rings        = doc["rings"]         | (uint16_t)0;
            bool     repeat       = doc["repeat"]        | false;
            bool     skipWeekends = doc["skipWeekends"]  | false;

            if (hour > 23 || minute > 59) {
                req->send(400, "application/json", "{\"error\":\"invalid hour or minute\"}");
                return;
            }

            if (!repeat) {
                struct tm now;
                if (!alarmClockGetLocalTime(&now)) {
                    req->send(503, "application/json", "{\"error\":\"time not synced\"}");
                    return;
                }
                if (!_alarmMgr->isTimeInFuture(hour, minute)) {
                    req->send(400, "application/json", "{\"error\":\"alarm time has already passed\"}");
                    return;
                }
            }

            uint32_t id = _alarmMgr->add(hour, minute, pattern, rings, repeat, skipWeekends);
            logger.infof("POST /alarm: created id=%u %02u:%02u repeat=%d", (unsigned)id,
                         (unsigned)hour, (unsigned)minute, (int)repeat);

            const std::vector<AlarmEntry>& all = _alarmMgr->getAll();
            JsonDocument resp;
            for (const auto& e : all) {
                if (e.id == id) {
                    alarmToJson(resp.to<JsonObject>(), e);
                    break;
                }
            }
            sendJson(req, 201, resp);
        },
        nullptr,
        // Body handler for POST /alarm
        [](AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t index, size_t total) {
            if (index == 0) {
                req->_tempObject = malloc(total + 1);
            }
            if (req->_tempObject) {
                memcpy((char*)req->_tempObject + index, data, len);
                if (index + len >= total) {
                    ((char*)req->_tempObject)[total] = '\0';
                }
            }
        });

    // PUT /alarm/* body accumulation via the global onRequestBody singleton.
    // POST /alarm uses server->on()'s inline body handler, but path-param routes
    // (PUT /alarm/{id}) are handled in onNotFound which fires after the body is
    // already complete — so we must buffer the body here.
    apiAddBodyHandler([](AsyncWebServerRequest* req, uint8_t* data, size_t len,
                         size_t index, size_t total) {
        if (req->method() == HTTP_PUT && req->url().startsWith("/alarm/")) {
            if (index == 0) {
                req->_tempObject = malloc(total + 1);
            }
            if (req->_tempObject) {
                memcpy((char*)req->_tempObject + index, data, len);
                if (index + len >= total) {
                    ((char*)req->_tempObject)[total] = '\0';
                }
            }
        }
    });

    // NotFoundHandler: PUT /alarm/{id} and DELETE /alarm/{id}
    apiAddNotFoundHandler([](AsyncWebServerRequest* request) -> bool {
        String url = request->url();
        if (!url.startsWith("/alarm/")) return false;

        int method = request->method();

        // PUT /alarm/<id>
        if (method == HTTP_PUT) {
            String idStr = url.substring(7);
            uint32_t id = (uint32_t)idStr.toInt();
            if (id == 0) {
                if (request->_tempObject) { free(request->_tempObject); request->_tempObject = nullptr; }
                request->send(400, "application/json", "{\"error\":\"invalid id\"}");
                return true;
            }

            const char* bodyStr = request->_tempObject ? (const char*)request->_tempObject : "{}";
            JsonDocument doc;
            DeserializationError err = deserializeJson(doc, bodyStr);
            if (request->_tempObject) { free(request->_tempObject); request->_tempObject = nullptr; }

            if (err != DeserializationError::Ok) {
                request->send(400, "application/json", "{\"error\":\"invalid JSON\"}");
                return true;
            }

            uint8_t  hour         = doc["hour"]         | (uint8_t)255;
            uint8_t  minute       = doc["minute"]        | (uint8_t)255;
            const char* pattern   = doc["pattern"]       | "us";
            uint16_t rings        = doc["rings"]         | (uint16_t)0;
            bool     repeat       = doc["repeat"]        | false;
            bool     skipWeekends = doc["skipWeekends"]  | false;

            if (hour > 23 || minute > 59) {
                request->send(400, "application/json", "{\"error\":\"invalid hour or minute\"}");
                return true;
            }

            JsonDocument resp;
            if (_alarmMgr->update(id, hour, minute, pattern, rings, repeat, skipWeekends)) {
                logger.infof("PUT /alarm/%u: updated", (unsigned)id);
                const std::vector<AlarmEntry>& all = _alarmMgr->getAll();
                for (const auto& e : all) {
                    if (e.id == id) {
                        char timeBuf[6];
                        snprintf(timeBuf, sizeof(timeBuf), "%02u:%02u", (unsigned)e.hour, (unsigned)e.minute);
                        resp["id"]           = e.id;
                        resp["time"]         = timeBuf;
                        resp["pattern"]      = e.patternName;
                        resp["rings"]        = e.rings;
                        resp["repeat"]       = e.repeat;
                        resp["skipWeekends"] = e.skipWeekends;
                        resp["enabled"]      = e.enabled;
                        break;
                    }
                }
                sendJson(request, 200, resp);
            } else {
                logger.infof("PUT /alarm/%u: not found", (unsigned)id);
                resp["error"] = "not found";
                sendJson(request, 404, resp);
            }
            return true;
        }

        // DELETE /alarm/<id>
        if (method == HTTP_DELETE) {
            String idStr = url.substring(7);
            uint32_t id = (uint32_t)idStr.toInt();
            if (id == 0) {
                request->send(400, "application/json", "{\"error\":\"invalid id\"}");
                return true;
            }
            JsonDocument doc;
            if (_alarmMgr->remove(id)) {
                logger.infof("DELETE /alarm/%u: deleted", (unsigned)id);
                doc["status"] = "deleted";
                doc["id"] = id;
                sendJson(request, 200, doc);
            } else {
                logger.infof("DELETE /alarm/%u: not found", (unsigned)id);
                doc["error"] = "not found";
                sendJson(request, 404, doc);
            }
            return true;
        }

        return false;
    });
}

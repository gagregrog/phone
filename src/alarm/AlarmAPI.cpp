#include "alarm/AlarmAPI.h"
#include "alarm/AlarmJSON.h"
#include "web/API.h"
#include "clock/Clock.h"
#include "web/Events.h"
#include "system/Logger.h"
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <stdlib.h>

static AlarmManager* _alarmMgr = nullptr;

static void sendJson(AsyncWebServerRequest* req, int code, JsonDocument& doc) {
    String body;
    serializeJson(doc, body);
    req->send(code, "application/json", body);
}

void alarmAPIBegin(AlarmManager& mgr) {
    _alarmMgr = &mgr;
    AsyncWebServer* server = apiGetServer();

    // GET /alarm — list all alarms
    server->on("/alarm", HTTP_GET, [](AsyncWebServerRequest* req) {
        logger.infof("[%s] GET /alarm", req->client()->remoteIP().toString().c_str());
        JsonDocument doc;
        JsonArray arr = doc.to<JsonArray>();
        for (const auto& e : _alarmMgr->getAll()) {
            alarmFillJson(arr.add<JsonObject>(), e);
        }
        sendJson(req, 200, doc);
    });

    // DELETE /alarm — delete all alarms
    server->on("/alarm", HTTP_DELETE, [](AsyncWebServerRequest* req) {
        logger.infof("[%s] DELETE /alarm", req->client()->remoteIP().toString().c_str());
        _alarmMgr->removeAll();
        JsonDocument doc;
        doc["status"] = "cleared";
        sendJson(req, 200, doc);
        eventsPublish("alarm/cleared", "{}");
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
                if (!clockGetLocalTime(&now)) {
                    req->send(503, "application/json", "{\"error\":\"time not synced\"}");
                    return;
                }
                if (!_alarmMgr->isTimeInFuture(hour, minute)) {
                    req->send(400, "application/json", "{\"error\":\"alarm time has already passed\"}");
                    return;
                }
            }

            uint32_t id = _alarmMgr->add(hour, minute, pattern, rings, repeat, skipWeekends);
            logger.infof("[%s] POST /alarm: created id=%u %02u:%02u repeat=%d",
                         req->client()->remoteIP().toString().c_str(),
                         (unsigned)id, (unsigned)hour, (unsigned)minute, (int)repeat);

            const std::vector<AlarmEntry>& all = _alarmMgr->getAll();
            JsonDocument resp;
            for (const auto& e : all) {
                if (e.id == id) {
                    alarmFillJson(resp.to<JsonObject>(), e);
                    break;
                }
            }
            String body;
            serializeJson(resp, body);
            req->send(201, "application/json", body);
            eventsPublish("alarm/created", body.c_str());
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
        String ip = request->client()->remoteIP().toString();

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
                logger.infof("[%s] PUT /alarm/%u: updated", ip.c_str(), (unsigned)id);
                const std::vector<AlarmEntry>& all = _alarmMgr->getAll();
                for (const auto& e : all) {
                    if (e.id == id) {
                        alarmFillJson(resp.to<JsonObject>(), e);
                        break;
                    }
                }
                String body;
                serializeJson(resp, body);
                request->send(200, "application/json", body);
                eventsPublish("alarm/updated", body.c_str());
            } else {
                logger.infof("[%s] PUT /alarm/%u: not found", ip.c_str(), (unsigned)id);
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
                logger.infof("[%s] DELETE /alarm/%u: deleted", ip.c_str(), (unsigned)id);
                doc["status"] = "deleted";
                doc["id"] = id;
                String body;
                serializeJson(doc, body);
                request->send(200, "application/json", body);
                eventsPublish("alarm/deleted", body.c_str());
            } else {
                logger.infof("[%s] DELETE /alarm/%u: not found", ip.c_str(), (unsigned)id);
                doc["error"] = "not found";
                sendJson(request, 404, doc);
            }
            return true;
        }

        return false;
    });
}

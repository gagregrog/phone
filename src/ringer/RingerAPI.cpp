#include "ringer/RingerAPI.h"
#include "ringer/RingerJSON.h"
#include "ringer/RingerEvents.h"
#include "web/API.h"
#include "system/Logger.h"
#include "ringer/RingPattern.h"
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

static void sendJson(AsyncWebServerRequest* request, int code, JsonDocument& doc) {
    String body;
    serializeJson(doc, body);
    request->send(code, "application/json", body);
}

void ringerAPIBegin(Ringer& ringer) {
    AsyncWebServer* server = apiGetServer();

    server->on("/ring/stop", HTTP_POST,
        [&ringer](AsyncWebServerRequest* request) {
            logger.infof("[%s] POST /ring/stop", request->client()->remoteIP().toString().c_str());
            ringer.ringStop();
            JsonDocument doc;
            ringStoppedFillJson(doc.to<JsonObject>());
            sendJson(request, 200, doc);
            publishRingStopped();
        });

    server->on("/ring/status", HTTP_GET,
        [&ringer](AsyncWebServerRequest* request) {
            logger.infof("[%s] GET /ring/status", request->client()->remoteIP().toString().c_str());
            JsonDocument doc;
            doc["ringing"] = ringer.isRinging();
            sendJson(request, 200, doc);
        });

    server->on("/ring/patterns", HTTP_GET,
        [](AsyncWebServerRequest* request) {
            logger.infof("[%s] GET /ring/patterns", request->client()->remoteIP().toString().c_str());
            JsonDocument doc;
            JsonArray arr = doc.to<JsonArray>();
            for (uint8_t i = 0; i < PATTERN_COUNT; i++) {
                arr.add(ALL_PATTERNS[i]->name);
            }
            sendJson(request, 200, doc);
        });

    // NotFoundHandler: POST /ring/{pattern}[/{count}]
    apiAddNotFoundHandler([&ringer](AsyncWebServerRequest* request) -> bool {
        String url = request->url();
        if (!url.startsWith("/ring/")) return false;

        String ip = request->client()->remoteIP().toString();
        if (request->method() != HTTP_POST) {
            logger.warnf("[%s] 404 %s", ip.c_str(), url.c_str());
            request->send(404, "text/plain", "Not Found");
            return true;
        }

        String rest = url.substring(6);
        String name;
        uint16_t cycles = 0;

        int slash = rest.indexOf('/');
        if (slash == -1) {
            name = rest;
        } else {
            name = rest.substring(0, slash);
            String countStr = rest.substring(slash + 1);
            long val = countStr.toInt();
            if (val <= 0 || val > 9) {
                logger.warnf("[%s] POST %s: invalid count", ip.c_str(), url.c_str());
                request->send(400, "application/json", "{\"error\":\"invalid count\"}");
                return true;
            }
            cycles = (uint16_t)val;
        }

        const RingPattern* p = findPattern(name.c_str());
        if (!p) {
            logger.warnf("[%s] POST %s: unknown pattern '%s'", ip.c_str(), url.c_str(), name.c_str());
            request->send(404, "application/json", "{\"error\":\"unknown pattern\"}");
            return true;
        }

        ringer.ring(*p, cycles);
        logger.infof("[%s] POST %s: ringing %s", ip.c_str(), url.c_str(), p->name);
        JsonDocument doc;
        ringStartedFillJson(doc.to<JsonObject>(), p->name);
        if (cycles > 0) doc["cycles"] = cycles;
        sendJson(request, 200, doc);
        publishRingStarted(p->name);
        return true;
    });
}

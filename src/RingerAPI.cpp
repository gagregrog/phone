#include "RingerAPI.h"
#include "API.h"
#include "Logger.h"
#include "RingPattern.h"
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
            logger.info("POST /ring/stop");
            ringer.ringStop();
            JsonDocument doc;
            doc["status"] = "stopped";
            sendJson(request, 200, doc);
        });

    server->on("/ring/status", HTTP_GET,
        [&ringer](AsyncWebServerRequest* request) {
            logger.info("GET /ring/status");
            JsonDocument doc;
            doc["ringing"] = ringer.isRinging();
            sendJson(request, 200, doc);
        });

    server->on("/ring/patterns", HTTP_GET,
        [](AsyncWebServerRequest* request) {
            logger.info("GET /ring/patterns");
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

        if (request->method() != HTTP_POST) {
            logger.warnf("404 %s", url.c_str());
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
                logger.warnf("POST %s: invalid count", url.c_str());
                request->send(400, "application/json", "{\"error\":\"invalid count\"}");
                return true;
            }
            cycles = (uint16_t)val;
        }

        const RingPattern* p = findPattern(name.c_str());
        if (!p) {
            logger.warnf("POST %s: unknown pattern '%s'", url.c_str(), name.c_str());
            request->send(404, "application/json", "{\"error\":\"unknown pattern\"}");
            return true;
        }

        ringer.ring(*p, cycles);
        logger.infof("POST %s: ringing %s", url.c_str(), p->name);
        JsonDocument doc;
        doc["status"] = p->name;
        if (cycles > 0) doc["cycles"] = cycles;
        sendJson(request, 200, doc);
        return true;
    });
}

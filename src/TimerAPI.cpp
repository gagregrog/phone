#include "TimerAPI.h"
#include "API.h"
#include "Logger.h"
#include "DurationParser.h"
#include "RingPattern.h"
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

static void sendJson(AsyncWebServerRequest* request, int code, JsonDocument& doc) {
    String body;
    serializeJson(doc, body);
    request->send(code, "application/json", body);
}

void timerAPIBegin(Timer& timer) {
    AsyncWebServer* server = apiGetServer();

    // GET /timer/status — list all active timers
    server->on("/timer/status", HTTP_GET,
        [&timer](AsyncWebServerRequest* request) {
            logger.info("GET /timer/status");
            char timeBuf[10];
            JsonDocument doc;
            JsonArray arr = doc.to<JsonArray>();
            for (size_t i = 0; i < timer.count(); ++i) {
                TimerInfo info = timer.infoAt(i);
                JsonObject t = arr.add<JsonObject>();
                t["id"] = info.id;
                formatDuration((info.remainingMs + 500) / 1000, timeBuf, sizeof(timeBuf));
                t["remaining"] = timeBuf;
                formatDuration(info.totalMs / 1000, timeBuf, sizeof(timeBuf));
                t["total"] = timeBuf;
                t["pattern"] = info.patternName;
            }
            sendJson(request, 200, doc);
        });

    // POST /timer/cancel — cancel all active timers
    server->on("/timer/cancel", HTTP_POST,
        [&timer](AsyncWebServerRequest* request) {
            size_t n = timer.count();
            timer.cancelAll();
            logger.infof("POST /timer/cancel: cleared %u timers", (unsigned)n);
            JsonDocument doc;
            doc["status"] = "cleared";
            doc["count"] = n;
            sendJson(request, 200, doc);
        });

    // NotFoundHandler: POST /timer/cancel/{id} and POST /timer/{duration}[/{pattern}]
    apiAddNotFoundHandler([&timer](AsyncWebServerRequest* request) -> bool {
        String url = request->url();
        if (!url.startsWith("/timer/")) return false;

        if (request->method() != HTTP_POST) {
            logger.warnf("404 %s", url.c_str());
            request->send(404, "text/plain", "Not Found");
            return true;
        }

        String rest = url.substring(7);

        // POST /timer/cancel/<id>
        if (rest.startsWith("cancel/")) {
            String idStr = rest.substring(7);
            uint32_t id = (uint32_t)idStr.toInt();
            if (id == 0) {
                request->send(400, "application/json", "{\"error\":\"invalid id\"}");
                return true;
            }
            JsonDocument doc;
            if (timer.cancel(id)) {
                logger.infof("POST /timer/cancel/%u: cancelled", (unsigned)id);
                doc["status"] = "cancelled";
                doc["id"] = id;
                sendJson(request, 200, doc);
            } else {
                logger.infof("POST /timer/cancel/%u: not found", (unsigned)id);
                doc["error"] = "not found";
                sendJson(request, 404, doc);
            }
            return true;
        }

        String durStr;
        String patName;

        int slash = rest.indexOf('/');
        if (slash == -1) {
            durStr = rest;
            patName = "chirp";
        } else {
            durStr = rest.substring(0, slash);
            patName = rest.substring(slash + 1);
        }

        unsigned long durationMs = parseDuration(durStr.c_str());
        if (durationMs == 0) {
            logger.warnf("POST %s: invalid duration", url.c_str());
            request->send(400, "application/json", "{\"error\":\"invalid duration\"}");
            return true;
        }

        const RingPattern* p = findPattern(patName.c_str());
        if (!p) {
            logger.warnf("POST %s: unknown pattern '%s'", url.c_str(), patName.c_str());
            request->send(404, "application/json", "{\"error\":\"unknown pattern\"}");
            return true;
        }

        uint32_t id = timer.start(durationMs, *p);
        char timeBuf[10];
        formatDuration(durationMs / 1000, timeBuf, sizeof(timeBuf));
        logger.infof("POST %s: timer started (id=%u, %s, %s)", url.c_str(), (unsigned)id, timeBuf, p->name);
        JsonDocument doc;
        doc["status"] = "started";
        doc["id"] = id;
        doc["duration"] = timeBuf;
        doc["pattern"] = p->name;
        sendJson(request, 200, doc);
        return true;
    });
}

#include "timer/TimerAPI.h"
#include "timer/TimerJSON.h"
#include "web/API.h"
#include "web/Events.h"
#include "system/Logger.h"
#include "timer/DurationParser.h"
#include "ringer/RingPattern.h"
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
            logger.apif("[%s] GET /timer/status", request->client()->remoteIP().toString().c_str());
            JsonDocument doc;
            JsonArray arr = doc.to<JsonArray>();
            for (size_t i = 0; i < timer.count(); ++i) {
                timerInfoFillJson(arr.add<JsonObject>(), timer.infoAt(i));
            }
            sendJson(request, 200, doc);
        });

    // POST /timer/cancel — cancel all active timers
    server->on("/timer/cancel", HTTP_POST,
        [&timer](AsyncWebServerRequest* request) {
            size_t n = timer.count();
            timer.cancelAll();
            logger.apif("[%s] POST /timer/cancel: cleared %u timers", request->client()->remoteIP().toString().c_str(), (unsigned)n);
            JsonDocument doc;
            doc["status"] = "cleared";
            doc["count"] = n;
            String body;
            serializeJson(doc, body);
            request->send(200, "application/json", body);
            eventsPublish("timer/cleared", body.c_str());
        });

    // NotFoundHandler: POST /timer/cancel/{id} and POST /timer/{duration}[/{pattern}]
    apiAddNotFoundHandler([&timer](AsyncWebServerRequest* request) -> bool {
        String url = request->url();
        if (!url.startsWith("/timer/")) return false;

        String ip = request->client()->remoteIP().toString();
        if (request->method() != HTTP_POST) {
            logger.apif("[%s] 404 %s", ip.c_str(), url.c_str());
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
                logger.apif("[%s] POST /timer/cancel/%u: cancelled", ip.c_str(), (unsigned)id);
                doc["status"] = "cancelled";
                doc["id"] = id;
                String body;
                serializeJson(doc, body);
                request->send(200, "application/json", body);
                eventsPublish("timer/cancelled", body.c_str());
            } else {
                logger.apif("[%s] POST /timer/cancel/%u: not found", ip.c_str(), (unsigned)id);
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
            logger.apif("[%s] POST %s: invalid duration", ip.c_str(), url.c_str());
            request->send(400, "application/json", "{\"error\":\"invalid duration\"}");
            return true;
        }

        const RingPattern* p = findPattern(patName.c_str());
        if (!p) {
            logger.apif("[%s] POST %s: unknown pattern '%s'", ip.c_str(), url.c_str(), patName.c_str());
            request->send(404, "application/json", "{\"error\":\"unknown pattern\"}");
            return true;
        }

        uint32_t id = timer.start(durationMs, *p);
        char timeBuf[10];
        formatDuration(durationMs / 1000, timeBuf, sizeof(timeBuf));
        logger.apif("[%s] POST %s: timer started (id=%u, %s, %s)", ip.c_str(), url.c_str(), (unsigned)id, timeBuf, p->name);
        JsonDocument doc;
        TimerInfo info{id, durationMs, durationMs, p->name};
        timerInfoFillJson(doc.to<JsonObject>(), info);
        String body;
        serializeJson(doc, body);
        request->send(200, "application/json", body);
        eventsPublish("timer/started", body.c_str());
        return true;
    });
}

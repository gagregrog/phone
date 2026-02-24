#include "RingerAPI.h"
#include "Logger.h"
#include "DurationParser.h"
#include "RingPattern.h"
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

static AsyncWebServer* server = nullptr;

static void sendJson(AsyncWebServerRequest* request, int code, JsonDocument& doc) {
  String body;
  serializeJson(doc, body);
  request->send(code, "application/json", body);
}

void ringerAPIBegin(Ringer& ringer, Timer& timer, uint16_t port) {
  server = new AsyncWebServer(port);

  server->on("/ring/stop", HTTP_POST,
    [&ringer](AsyncWebServerRequest* request) {
      logger.info("POST /ring/stop");
      ringer.ringStop();
      JsonDocument doc;
      doc["status"] = "stopped";
      sendJson(request, 200, doc);
    });

  server->on("/status", HTTP_GET,
    [&ringer, &timer](AsyncWebServerRequest* request) {
      logger.info("GET /status");
      char timeBuf[10];
      JsonDocument doc;
      doc["ringing"] = ringer.isRinging();
      JsonArray timers = doc["timers"].to<JsonArray>();
      for (size_t i = 0; i < timer.count(); ++i) {
        TimerInfo info = timer.infoAt(i);
        JsonObject t = timers.add<JsonObject>();
        t["id"] = info.id;
        formatDuration((info.remainingMs + 500) / 1000, timeBuf, sizeof(timeBuf));
        t["remaining"] = timeBuf;
        formatDuration(info.totalMs / 1000, timeBuf, sizeof(timeBuf));
        t["total"] = timeBuf;
        t["pattern"] = info.patternName;
      }
      sendJson(request, 200, doc);
    });

  // GET /ring/patterns — list all available pattern names
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

  server->on("/ip", HTTP_GET,
    [](AsyncWebServerRequest* request) {
      logger.info("GET /ip");
      request->send(200, "text/plain", WiFi.localIP().toString());
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

  // Catch-all for POST /ring/<pattern> and POST /timer/<duration>
  server->onNotFound([&ringer, &timer](AsyncWebServerRequest* request) {
    if (request->method() != HTTP_POST) {
      logger.warnf("405 %s", request->url().c_str());
      request->send(404, "text/plain", "Not Found");
      return;
    }

    String url = request->url();

    // POST /timer/<duration> or /timer/<duration>/<pattern> or /timer/cancel/<id>
    if (url.startsWith("/timer/")) {
      String rest = url.substring(7);

      // POST /timer/cancel/<id>
      if (rest.startsWith("cancel/")) {
        String idStr = rest.substring(7);
        uint32_t id = (uint32_t)idStr.toInt();
        if (id == 0) {
          request->send(400, "application/json", "{\"error\":\"invalid id\"}");
          return;
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
        return;
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
        return;
      }

      const RingPattern* p = findPattern(patName.c_str());
      if (!p) {
        logger.warnf("POST %s: unknown pattern '%s'", url.c_str(), patName.c_str());
        request->send(404, "application/json", "{\"error\":\"unknown pattern\"}");
        return;
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
      return;
    }

    // POST /ring/<pattern> or /ring/<pattern>/<count>
    if (url.startsWith("/ring/")) {
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
          return;
        }
        cycles = (uint16_t)val;
      }

      const RingPattern* p = findPattern(name.c_str());
      if (!p) {
        logger.warnf("POST %s: unknown pattern '%s'", url.c_str(), name.c_str());
        request->send(404, "application/json", "{\"error\":\"unknown pattern\"}");
        return;
      }

      ringer.ring(*p, cycles);
      logger.infof("POST %s: ringing %s", url.c_str(), p->name);
      JsonDocument doc;
      doc["status"] = p->name;
      if (cycles > 0) doc["cycles"] = cycles;
      sendJson(request, 200, doc);
      return;
    }

    logger.warnf("404 %s", request->url().c_str());
    request->send(404, "text/plain", "Not Found");
  });

  server->begin();
  logger.info("Ringer API listening");
}

#include "RingerAPI.h"
#include "DurationParser.h"
#include "RingPattern.h"
#include <WiFi.h>
#include <ESPAsyncWebServer.h>

static AsyncWebServer* server = nullptr;

void ringerAPIBegin(Ringer& ringer, Timer& timer, uint16_t port) {
  server = new AsyncWebServer(port);

  server->on("/ring/stop", HTTP_POST,
    [&ringer](AsyncWebServerRequest* request) {
      ringer.ringStop();
      request->send(200, "application/json", "{\"status\":\"stopped\"}");
    });

  server->on("/ring/status", HTTP_GET,
    [&ringer, &timer](AsyncWebServerRequest* request) {
      String body = "{\"ringing\":";
      body += ringer.isRinging() ? "true" : "false";
      body += ",\"timer\":{\"active\":";
      if (timer.isActive()) {
        body += "true,\"remainingSec\":";
        body += (timer.remainingMs() + 500) / 1000;
        body += ",\"totalSec\":";
        body += timer.totalMs() / 1000;
        body += ",\"pattern\":\"";
        body += timer.patternName();
        body += "\"";
      } else {
        body += "false";
      }
      body += "}}";
      request->send(200, "application/json", body);
    });

  // GET /ring/patterns — list all available pattern names
  server->on("/ring/patterns", HTTP_GET,
    [](AsyncWebServerRequest* request) {
      String body = "[";
      for (uint8_t i = 0; i < PATTERN_COUNT; i++) {
        if (i > 0) body += ",";
        body += "\"";
        body += ALL_PATTERNS[i]->name;
        body += "\"";
      }
      body += "]";
      request->send(200, "application/json", body);
    });

  server->on("/ip", HTTP_GET,
    [](AsyncWebServerRequest* request) {
      request->send(200, "text/plain", WiFi.localIP().toString());
    });

  // POST /timer/cancel
  server->on("/timer/cancel", HTTP_POST,
    [&timer](AsyncWebServerRequest* request) {
      if (timer.isActive()) {
        timer.cancel();
        request->send(200, "application/json", "{\"status\":\"cancelled\"}");
      } else {
        request->send(200, "application/json", "{\"status\":\"no_timer\"}");
      }
    });

  // Catch-all for POST /ring/<pattern> and POST /timer/<duration>
  server->onNotFound([&ringer, &timer](AsyncWebServerRequest* request) {
    if (request->method() != HTTP_POST) {
      request->send(404, "text/plain", "Not Found");
      return;
    }

    String url = request->url();

    // POST /timer/<duration> or /timer/<duration>/<pattern>
    if (url.startsWith("/timer/")) {
      String rest = url.substring(7);
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
        request->send(400, "application/json", "{\"error\":\"invalid duration\"}");
        return;
      }

      const RingPattern* p = findPattern(patName.c_str());
      if (!p) {
        request->send(404, "application/json", "{\"error\":\"unknown pattern\"}");
        return;
      }

      timer.start(durationMs, *p);
      String body = "{\"status\":\"started\",\"totalSec\":";
      body += durationMs / 1000;
      body += ",\"pattern\":\"";
      body += p->name;
      body += "\"}";
      request->send(200, "application/json", body);
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
          request->send(400, "application/json", "{\"error\":\"invalid count\"}");
          return;
        }
        cycles = (uint16_t)val;
      }

      const RingPattern* p = findPattern(name.c_str());
      if (!p) {
        request->send(404, "application/json", "{\"error\":\"unknown pattern\"}");
        return;
      }

      ringer.ring(*p, cycles);
      String body = "{\"status\":\"";
      body += p->name;
      if (cycles > 0) {
        body += "\",\"cycles\":";
        body += cycles;
      } else {
        body += "\"";
      }
      body += "}";
      request->send(200, "application/json", body);
      return;
    }

    request->send(404, "text/plain", "Not Found");
  });

  server->begin();
  Serial.println("Ringer API listening");
}

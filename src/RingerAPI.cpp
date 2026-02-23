#include "RingerAPI.h"
#include "RingPattern.h"
#include <WiFi.h>
#include <ESPAsyncWebServer.h>

static AsyncWebServer* server = nullptr;

void ringerAPIBegin(Ringer& ringer, uint16_t port) {
  server = new AsyncWebServer(port);

  server->on("/ring/stop", HTTP_POST,
    [&ringer](AsyncWebServerRequest* request) {
      ringer.ringStop();
      request->send(200, "application/json", "{\"status\":\"stopped\"}");
    });

  server->on("/ring/status", HTTP_GET,
    [&ringer](AsyncWebServerRequest* request) {
      const char* body = ringer.isRinging()
        ? "{\"ringing\":true}"
        : "{\"ringing\":false}";
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

  // POST /ring/<pattern> or /ring/<pattern>/<count>
  server->onNotFound([&ringer](AsyncWebServerRequest* request) {
    if (request->method() != HTTP_POST) {
      request->send(404, "text/plain", "Not Found");
      return;
    }

    String url = request->url();
    if (!url.startsWith("/ring/")) {
      request->send(404, "text/plain", "Not Found");
      return;
    }

    // Extract everything after "/ring/"
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

    // Look up the pattern by name
    const RingPattern* p = nullptr;
    for (uint8_t i = 0; i < PATTERN_COUNT; i++) {
      if (name == ALL_PATTERNS[i]->name) {
        p = ALL_PATTERNS[i];
        break;
      }
    }

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
  });

  server->begin();
  Serial.println("Ringer API listening");
}

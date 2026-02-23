#include "RingerAPI.h"
#include "RingPattern.h"
#include <WiFi.h>
#include <ESPAsyncWebServer.h>

static AsyncWebServer* server = nullptr;

void ringerAPIBegin(Ringer& ringer, uint16_t port) {
  server = new AsyncWebServer(port);

  // POST /ring/<pattern> — start a named ringing pattern
  for (uint8_t i = 0; i < PATTERN_COUNT; i++) {
    const RingPattern* p = ALL_PATTERNS[i];
    String path = "/ring/" + String(p->name);
    server->on(path.c_str(), HTTP_POST,
      [&ringer, p](AsyncWebServerRequest* request) {
        ringer.ring(*p);
        String body = "{\"status\":\"";
        body += p->name;
        body += "\"}";
        request->send(200, "application/json", body);
      });
  }

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

  server->begin();
  Serial.println("Ringer API listening");
}

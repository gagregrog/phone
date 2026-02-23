#include "RingerAPI.h"
#include <ESPAsyncWebServer.h>

static AsyncWebServer* server = nullptr;

void ringerAPIBegin(Ringer& ringer, uint16_t port) {
  server = new AsyncWebServer(port);

  server->on("/ring/start", HTTP_POST,
    [&ringer](AsyncWebServerRequest* request) {
      ringer.ringStart();
      request->send(200, "application/json", "{\"status\":\"ringing\"}");
    });

  server->on("/ring/stop", HTTP_POST,
    [&ringer](AsyncWebServerRequest* request) {
      ringer.ringStop();
      request->send(200, "application/json", "{\"status\":\"stopped\"}");
    });

  server->on("/ring/pattern", HTTP_POST,
    [&ringer](AsyncWebServerRequest* request) {
      ringer.ringPattern();
      request->send(200, "application/json", "{\"status\":\"pattern\"}");
    });

  server->on("/ring/status", HTTP_GET,
    [&ringer](AsyncWebServerRequest* request) {
      const char* body = ringer.isRinging()
        ? "{\"ringing\":true}"
        : "{\"ringing\":false}";
      request->send(200, "application/json", body);
    });

  server->begin();
  Serial.println("Ringer API listening");
}

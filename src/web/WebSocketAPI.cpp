#include "web/WebSocketAPI.h"
#include "web/API.h"
#include "web/Events.h"
#include "system/Logger.h"
#include <ESPAsyncWebServer.h>
#include <deque>
#include <map>
#include <string.h>

static AsyncWebSocket _ws("/ws");
static std::deque<String> _logBuffer;
static const size_t LOG_BUFFER_SIZE = 50;
static std::map<uint32_t, String> _clientIPs;

static void publishClientCount() {
    String payload = "{\"count\":";
    payload += _ws.count();
    payload += "}";
    eventsPublish("ws/clients", payload.c_str());
}

void webSocketAPIBegin() {
    _ws.onEvent([](AsyncWebSocket*, AsyncWebSocketClient* client,
                   AwsEventType type, void*, uint8_t*, size_t) {
        if (type == WS_EVT_CONNECT) {
            _clientIPs[client->id()] = client->remoteIP().toString();
            if (!_logBuffer.empty()) {
                String batch = "{\"topic\":\"log/history\",\"data\":[";
                for (size_t i = 0; i < _logBuffer.size(); i++) {
                    if (i > 0) batch += ',';
                    batch += _logBuffer[i];
                }
                batch += "]}";
                client->text(batch);
            }
            logger.infof("[%s] WS client %u connected", _clientIPs[client->id()].c_str(), client->id());
            publishClientCount();
        } else if (type == WS_EVT_DISCONNECT) {
            String ip = _clientIPs.count(client->id()) ? _clientIPs[client->id()] : "unknown";
            _clientIPs.erase(client->id());
            logger.infof("[%s] WS client %u disconnected", ip.c_str(), client->id());
            publishClientCount();
        }
    });

    eventsSubscribe([](const char* topic, const char* payload) {
        String msg;
        msg.reserve(strlen(topic) + strlen(payload) + 16);
        msg += "{\"topic\":\"";
        msg += topic;
        msg += "\",\"data\":";
        msg += payload;
        msg += "}";

        if (strcmp(topic, "log/message") == 0) {
            _logBuffer.push_back(String(payload));
            if (_logBuffer.size() > LOG_BUFFER_SIZE) {
                _logBuffer.pop_front();
            }
        }

        if (_ws.count() == 0) return;
        _ws.textAll(msg);
    });

    apiGetServer()->addHandler(&_ws);
}

void webSocketLoop() {
    _ws.cleanupClients();

    static unsigned long lastPingMs = 0;
    unsigned long now = millis();
    if (now - lastPingMs >= 10000) {
        lastPingMs = now;
        if (_ws.count() > 0) {
            _ws.textAll("{\"topic\":\"ping\",\"data\":{}}");
        }
    }
}

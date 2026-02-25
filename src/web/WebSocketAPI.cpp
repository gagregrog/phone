#include "web/WebSocketAPI.h"
#include "web/API.h"
#include "web/Events.h"
#include "system/Logger.h"
#include <ESPAsyncWebServer.h>
#include <deque>
#include <string.h>

static AsyncWebSocket _ws("/ws");
static std::deque<String> _logBuffer;
static const size_t LOG_BUFFER_SIZE = 100;

void webSocketAPIBegin() {
    _ws.onEvent([](AsyncWebSocket*, AsyncWebSocketClient* client,
                   AwsEventType type, void*, uint8_t*, size_t) {
        if (type == WS_EVT_CONNECT) {
            for (const auto& entry : _logBuffer) {
                client->text(entry);
            }
            logger.infof("WS client %u connected", client->id());
        } else if (type == WS_EVT_DISCONNECT) {
            logger.infof("WS client %u disconnected", client->id());
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
            _logBuffer.push_back(msg);
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
}

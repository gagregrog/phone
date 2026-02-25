#include "web/WebSocketAPI.h"
#include "web/API.h"
#include "web/Events.h"
#include "system/Logger.h"
#include <ESPAsyncWebServer.h>

static AsyncWebSocket _ws("/ws");

void webSocketAPIBegin() {
    _ws.onEvent([](AsyncWebSocket*, AsyncWebSocketClient* client,
                   AwsEventType type, void*, uint8_t*, size_t) {
        if (type == WS_EVT_CONNECT) {
            logger.infof("WS client %u connected", client->id());
        } else if (type == WS_EVT_DISCONNECT) {
            logger.infof("WS client %u disconnected", client->id());
        }
    });

    eventsSubscribe([](const char* topic, const char* payload) {
        if (_ws.count() == 0) return;
        // Build {"topic":"...","data":<payload>}
        String msg;
        msg.reserve(strlen(topic) + strlen(payload) + 16);
        msg += "{\"topic\":\"";
        msg += topic;
        msg += "\",\"data\":";
        msg += payload;
        msg += "}";
        _ws.textAll(msg);
    });

    apiGetServer()->addHandler(&_ws);
}

void webSocketLoop() {
    _ws.cleanupClients();
}

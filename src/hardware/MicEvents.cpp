#include "hardware/MicEvents.h"
#include "web/Events.h"
#include "web/WebSocketAPI.h"
#include <string.h>

void micEventsBegin(MicReader& mic) {
    // Gate sampling: enable only when WebSocket clients are connected
    eventsSubscribe([&mic](const char* topic, const char* payload) {
        if (strcmp(topic, "ws/clients") != 0) return;
        // payload is e.g. {"count":2}
        // Simple parse: find digits after "count":
        const char* p = strstr(payload, "\"count\":");
        if (!p) return;
        p += 8; // skip "count":
        while (*p == ' ') p++;
        int count = 0;
        while (*p >= '0' && *p <= '9') { count = count * 10 + (*p - '0'); p++; }
        mic.setEnabled(count > 0);
    });

    // On each DMA buffer: pack binary frame [0x01 | int16 samples...] and enqueue.
    // wsEnqueueBinary takes ownership of the buffer — do not delete[] after calling it.
    mic.setOnBuffer([](const int16_t* samples, size_t count) {
        size_t frameLen = 1 + count * sizeof(int16_t);
        uint8_t* frame = new uint8_t[frameLen];
        frame[0] = 0x01;  // audio message type
        memcpy(frame + 1, samples, count * sizeof(int16_t));
        wsEnqueueBinary(frame, frameLen);
    });
}

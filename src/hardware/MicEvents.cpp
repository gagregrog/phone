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

    // Batch two DMA buffers into one WS frame to halve frame rate (~21/sec instead of ~43).
    // wsEnqueueBinary takes ownership of the buffer — do not delete[] after calling it.
    static uint8_t* pending = nullptr;
    static size_t pendingBytes = 0;

    mic.setOnBuffer([](const int16_t* samples, size_t count) {
        size_t payloadBytes = count * sizeof(int16_t);

        if (!pending) {
            // First half: allocate frame for two buffers, copy first batch
            pending = new uint8_t[1 + payloadBytes * 2];
            pending[0] = 0x01;
            memcpy(pending + 1, samples, payloadBytes);
            pendingBytes = payloadBytes;
        } else {
            // Second half: append and send
            memcpy(pending + 1 + pendingBytes, samples, payloadBytes);
            wsEnqueueBinary(pending, 1 + pendingBytes + payloadBytes);
            pending = nullptr;
            pendingBytes = 0;
        }
    });
}

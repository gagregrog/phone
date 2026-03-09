#pragma once
#include <stddef.h>
#include <stdint.h>

void webSocketAPIBegin();
void webSocketLoop();
// Enqueue a binary frame to be broadcast on the next webSocketLoop() tick.
// Caller allocates with new uint8_t[]; this function takes ownership and will
// delete[] on drain or drop — do not free the buffer after calling this.
void wsEnqueueBinary(uint8_t* data, size_t len);

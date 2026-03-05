/*
 * Encoder.h - Stub for Teensy Encoder library (native tests).
 */
#pragma once
#include <cstdint>

class Encoder {
public:
    Encoder(uint8_t, uint8_t) {}
    long read() { return 0; }
    void write(long) {}
};

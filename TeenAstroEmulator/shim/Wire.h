/* Wire.h -- Stub I2C library (SHC emulator uses no real I2C) */
#pragma once
#include "arduino.h"

class TwoWire : public Stream {
public:
    void begin() {}
    void begin(uint8_t) {}
    void setClock(uint32_t) {}
    void beginTransmission(uint8_t) {}
    uint8_t endTransmission(bool = true) { return 0; }
    uint8_t requestFrom(uint8_t, uint8_t, bool = true) { return 0; }
    size_t write(uint8_t) override { return 1; }
    int available() override { return 0; }
    int read() override { return -1; }
    int peek() override { return -1; }
};

static TwoWire Wire;

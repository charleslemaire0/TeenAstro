/*
 * SPI.h - Stub for Arduino SPI (native tests).
 */
#pragma once
#include <cstdint>

#define SPI_MODE0 0
#define SPI_MODE1 1
#define SPI_MODE2 2
#define SPI_MODE3 3

struct SPISettings {
    SPISettings() {}
    SPISettings(uint32_t, uint8_t, uint8_t) {}
};

class SPIClass {
public:
    void begin() {}
    void end() {}
    void beginTransaction(SPISettings) {}
    void endTransaction() {}
    uint8_t transfer(uint8_t) { return 0; }
    uint16_t transfer16(uint16_t) { return 0; }
    void transfer(void*, size_t) {}
    void setBitOrder(uint8_t) {}
    void setDataMode(uint8_t) {}
    void setClockDivider(uint8_t) {}
    void usingInterrupt(uint8_t) {}
};

#ifndef SPI_INSTANCE_DEFINED
#define SPI_INSTANCE_DEFINED
static SPIClass SPI;
#endif

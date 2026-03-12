/*
 * esp_shim.h -- ESP8266/ESP32 API stubs for the SHC emulator.
 */
#pragma once
#include <cstdlib>

struct EspClass {
    void reset() { exit(0); }
    void restart() { exit(0); }
    uint32_t getFreeHeap() { return 65536; }
    uint32_t getChipId() { return 0x12345678; }
};

static EspClass ESP;

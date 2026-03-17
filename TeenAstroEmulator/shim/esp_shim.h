/*
 * esp_shim.h -- ESP8266/ESP32 API stubs for the SHC emulator.
 *
 * On real hardware ESP.reset()/restart() reboots the MCU and re-establishes
 * the serial link.  In the emulator we simulate this by resetting the
 * connection failure counter and attempting a TCP reconnect instead of
 * calling exit().  The forward-declared reconnect hook is defined in
 * shc_emu.cpp.
 */
#pragma once
#include <cstdlib>
#include <cstdio>

void emu_attempt_reconnect();

struct EspClass {
    void reset() {
        printf("[EMU] ESP.reset() intercepted -- attempting reconnect.\n");
        fflush(stdout);
        emu_attempt_reconnect();
    }
    void restart() {
        printf("[EMU] ESP.restart() intercepted -- attempting reconnect.\n");
        fflush(stdout);
        emu_attempt_reconnect();
    }
    uint32_t getFreeHeap() { return 65536; }
    uint32_t getChipId() { return 0x12345678; }
};

static EspClass ESP;

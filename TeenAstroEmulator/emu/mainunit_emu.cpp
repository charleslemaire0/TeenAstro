/*
 * mainunit_emu.cpp -- TeenAstro MainUnit PC emulator entry point.
 *
 * Runs the real MainUnit firmware on a desktop PC.
 * - TCP port 9997: USB serial (ASCOM / PC clients)
 * - TCP port 9998: SHC serial
 * - Timer thread fires ISR callbacks at real-time intervals
 * - micros()/millis() use wall-clock time via <chrono>
 *
 * Build:  pio run -d TeenAstroEmulator -e emu_mainunit
 * Run:    .pio\build\emu_mainunit\program.exe
 */

/* ------------------------------------------------------------------ */
/*  sim:: globals (must be defined before any shim header)             */
/* ------------------------------------------------------------------ */
namespace sim {
    unsigned long g_micros = 0;
    unsigned long g_millis = 0;
    bool          g_realtime = false;
}

/* ------------------------------------------------------------------ */
/*  Shim includes                                                      */
/* ------------------------------------------------------------------ */
#include "TcpStream.h"
#include "arduino.h"
#include "EEPROM.h"
#include "FocuserStub.h"
#include "TimeLib.h"
#include "Teensy3Clock.h"
#include "IntervalTimer.h"
#include "SPI.h"
#include "Encoder.h"
#include "TMC26XStepper.h"
#include "TMCStepper.h"
#include "TinyGPS++.h"

/* ------------------------------------------------------------------ */
/*  SDL2 (for cockpit window)                                          */
/* ------------------------------------------------------------------ */
#include <SDL.h>

/* ------------------------------------------------------------------ */
/*  Real firmware includes                                             */
/* ------------------------------------------------------------------ */
#include "Command.h"
#include "Application.h"
#include "XEEPROM.hpp"

/* ------------------------------------------------------------------ */
/*  Cockpit UI                                                         */
/* ------------------------------------------------------------------ */
#include "cockpit_sdl.h"

/* Globals defined in firmware .cpp files:
 *   Mount mount;                   (Mount.cpp)
 *   DateTimeTimers rtk;            (Command.cpp)
 *   timerLoop tlp;                 (Application.cpp)
 *   Application application;       (Application.cpp)
 *   CommandState commandState;     (Command.cpp)
 *   siteDefinition localSite;      (EEPROM.cpp)
 */

/* TCP serial streams that replace Serial (USB) and Serial1 (SHC) */
static TcpServerStream g_tcpUsb;
static TcpServerStream g_tcpShc;
static TcpServerStream g_tcpWifi;   /* Android / SkySafari LX200 */
static FocuserStub     g_focuserStub;

/* ------------------------------------------------------------------ */
/*  reboot() stub                                                      */
/* ------------------------------------------------------------------ */
void reboot() {
    printf("[EMU] reboot() called -- exiting.\n");
    fflush(stdout);
    exit(0);
}

/* ------------------------------------------------------------------ */
/*  EMU-specific setupCommandSerial (wired into Application.cpp via    */
/*  #ifdef EMU_MAINUNIT)                                               */
/* ------------------------------------------------------------------ */
/* Called when SHC sends :EW1# (open WiFi for Android). Only then do we listen on 9999. */
void emu_wifiStart(void) {
    if (!g_tcpWifi.isListening())
        g_tcpWifi.listen(9999, true);
    printf("[EMU] WiFi (port 9999) opened for Android/SkySafari.\n");
    fflush(stdout);
}
/* Called when SHC sends :EW0# (close WiFi). */
void emu_wifiStop(void) {
    g_tcpWifi.stop();
    printf("[EMU] WiFi (port 9999) closed.\n");
    fflush(stdout);
}

void emu_setupCommandSerial()
{
    g_tcpUsb.listen(9997);
    commandState.S_USB_.attach_Stream(&g_tcpUsb, COMMAND_SERIAL);

    g_tcpShc.listen(9998);
    commandState.S_SHC_.attach_Stream(&g_tcpShc, COMMAND_SERIAL1);

    /* WiFi server not started here; opened only when SHC sends :EW1# (WiFi on in menu) */
    commandState.S_WiFi_.attach_Stream(&g_tcpWifi, COMMAND_WIFI);
}

/* ------------------------------------------------------------------ */
/*  Main                                                                */
/* ------------------------------------------------------------------ */
int main(int, char**)
{
    printf("=== TeenAstro MainUnit Emulator ===\n");
    printf("USB serial:    TCP 127.0.0.1:9997\n");
    printf("SHC serial:    TCP 127.0.0.1:9998\n");
    printf("WiFi (Android): port 9999 opens when SHC menu has WiFi ON\n\n");
    fflush(stdout);

    sim::enableRealtime();

    /* Route Focus_Serial (Serial2) to the in-process focuser emulation */
    Serial2.setForward(&g_focuserStub);
    printf("[EMU] Focuser stub attached to Serial2 (Focus_Serial).\n");

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
        printf("[EMU] SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    application.setup();

    IntervalTimerThread::instance().start();
    printf("[EMU] Timer thread started.\n");

    if (!cockpit::cockpit_init()) {
        printf("[EMU] WARNING: cockpit window failed to create: %s\n", SDL_GetError());
    } else {
        printf("[EMU] Cockpit window opened.\n");
    }

    printf("[EMU] Entering main loop...\n\n");
    fflush(stdout);

    unsigned long lastEepromFlush = millis();

    while (true) {
        {
            std::lock_guard<std::mutex> lk(IntervalTimerThread::instance().mutex());
            application.loop();
        }
        cockpit::cockpit_update();

        /* Flush EEPROM every 5 s so settings survive TerminateProcess */
        if (millis() - lastEepromFlush > 5000) {
            EEPROM.commit();
            XEEPROM.commit();
            lastEepromFlush = millis();
        }

        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }

    cockpit::cockpit_quit();
    SDL_Quit();
    IntervalTimerThread::instance().stop();
    return 0;
}

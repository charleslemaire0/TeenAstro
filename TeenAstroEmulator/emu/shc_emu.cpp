/*
 * shc_emu.cpp -- TeenAstro SHC PC emulator entry point.
 *
 * Runs the real SHC firmware on a desktop PC.
 * - Connects to MainUnit emulator at TCP 127.0.0.1:9998
 * - Renders the 128x64 OLED display via SDL2 at 4x scale
 * - Maps keyboard keys to SHC buttons
 *
 * Build:  pio run -d TeenAstroEmulator -e emu_shc
 * Run:    .pio\build\emu_shc\program.exe
 */

/* ------------------------------------------------------------------ */
/*  sim:: globals                                                      */
/* ------------------------------------------------------------------ */
namespace sim {
    unsigned long g_micros = 0;
    unsigned long g_millis = 0;
    bool          g_realtime = false;
}

/* Board/platform defines are set via build_flags in platformio.ini:
 *   -DARDUINO=1 -DARDUINO_ESP8266_WEMOS_D1MINI -DARDUINO_ARCH_ESP8266
 *
 * Pin definitions for the emulator (values don't matter, Pad is SDL) */
#ifndef D0
#define D0 0
#define D1 1
#define D2 2
#define D3 3
#define D4 4
#define D5 5
#define D6 6
#define D7 7
#define D8 8
#endif
#ifndef A0
#define A0 0
#endif

/* ------------------------------------------------------------------ */
/*  Shim includes                                                      */
/* ------------------------------------------------------------------ */
#include "TcpStream.h"
#include "arduino.h"
#include "EEPROM.h"
#include "esp_shim.h"
#include "u8g2_sdl2.h"

/* ------------------------------------------------------------------ */
/*  SHC firmware includes                                              */
/* ------------------------------------------------------------------ */
#include "SmartConfig.h"
#include "SmartController.h"
#include <TeenAstroMountStatus.h>

/* ------------------------------------------------------------------ */
/*  TCP serial to MainUnit                                             */
/* ------------------------------------------------------------------ */
static TcpClientStream g_tcpToMainUnit;

/* ------------------------------------------------------------------ */
/*  Global instances (matching TeenAstroSHC.ino)                       */
/* ------------------------------------------------------------------ */
LX200Client*      g_lx200 = nullptr;
SmartHandController HdCrtlr;
TeenAstroMountStatus ta_MountStatus;

/* ------------------------------------------------------------------ */
/*  SDL2 display instance (populated by SmartController::setup)        */
/* ------------------------------------------------------------------ */
U8G2_EXT_SDL2* g_sdlDisplay = nullptr;

/* ------------------------------------------------------------------ */
/*  Blit helper called from delay() in arduino.h during setup/menus    */
/* ------------------------------------------------------------------ */
void _emu_shc_blit() {
    if (g_sdlDisplay) g_sdlDisplay->blitToSDL();
}

/* ------------------------------------------------------------------ */
/*  Main                                                                */
/* ------------------------------------------------------------------ */
int main(int, char**)
{
    printf("=== TeenAstro SHC Emulator ===\n");
    printf("Connecting to MainUnit at 127.0.0.1:9998...\n");
    fflush(stdout);

    sim::enableRealtime();

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
        printf("SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    /* Connect to MainUnit */
    int retries = 20;
    while (!g_tcpToMainUnit.connect("127.0.0.1", 9998)) {
        if (--retries <= 0) {
            printf("Failed to connect to MainUnit on port 9998.\n");
            printf("Start the MainUnit emulator first.\n");
            return 1;
        }
        printf("  Retrying connection...\n");
        SDL_Delay(500);
    }

    /* Create LX200 client over TCP */
    g_lx200 = new LX200Client(g_tcpToMainUnit, 100);

    ta_MountStatus.setClient(*g_lx200);
    TeenAstroWifi::setClient(*g_lx200);
    HdCrtlr.setClient(*g_lx200);

    const char SHCVersion[] = "1.6.0-emu";
    const int pin[7] = { 0, 0, 0, 0, 0, 0, 0 };
    const bool active[7] = { false, false, false, false, false, false, false };

    /* Override the display creation -- we intercept setup() */
    HdCrtlr.setup(SHCVersion, pin, active, 57600,
                   SmartHandController::OLED::OLED_SSD1306, 1);

    printf("[SHC-EMU] Setup complete. Entering main loop...\n");
    fflush(stdout);

    while (true) {
        HdCrtlr.update();
        if (g_sdlDisplay) g_sdlDisplay->blitToSDL();
        SDL_Delay(10);
    }

    SDL_Quit();
    return 0;
}

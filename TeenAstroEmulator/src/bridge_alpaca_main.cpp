/*
 * bridge_alpaca_main.cpp - native ASCOM Alpaca bridge for the
 * MainUnit emulator.
 *
 * Connects an LX200Client to mainunit_emu.exe over TCP/9997, then runs
 * the real TeenAstroAlpaca library on TCP/11111 (Alpaca API) + UDP/32227
 * (Alpaca discovery) using a winsock-backed ESP8266WebServer / WiFiUDP
 * shim.  This lets ASCOM ConformU drive the Alpaca interface end-to-end
 * with the emulator standing in for real TeenAstro hardware.
 *
 * Usage:
 *   1. Start mainunit_emu.exe (TCP 9997).
 *   2. Build:  pio run -d TeenAstroEmulator -e bridge_alpaca
 *   3. Run:    .pio\build\bridge_alpaca\program.exe
 *   4. From another shell:
 *        conformu conformance http://127.0.0.1:11111/api/v1/telescope/0
 */

// Force-include order: Arduino.h must come before everything else so the
// winsock + std headers are properly ordered.
#include "Arduino.h"
#include "ESP8266WiFi.h"
#include "ESP8266WebServer.h"
#include "WiFiUdp.h"

#include <cstdio>
#include <cstring>
#include <thread>
#include <chrono>

#include "LX200Client.h"
#include "TeenAstroMountStatus.h"
#include "TeenAstroAlpaca.h"
#include "Win32Transport.h"

// --- shim singletons referenced by TeenAstroAlpaca::uniqueId() ---
WiFiClass WiFi;

int main(int argc, char** argv)
{
  const char* mainunitHost = "127.0.0.1";
  int         mainunitPort = 9997;
  uint16_t    alpacaPort   = 11111;
  if (argc >= 2) mainunitHost = argv[1];
  if (argc >= 3) mainunitPort = std::atoi(argv[2]);
  if (argc >= 4) alpacaPort   = (uint16_t)std::atoi(argv[3]);

  std::printf("=== TeenAstro Alpaca bridge ===\n");
  std::printf("MainUnit (LX200): %s:%d\n", mainunitHost, mainunitPort);
  std::printf("Alpaca API:       http://0.0.0.0:%u/api/v1/telescope/0\n", (unsigned)alpacaPort);
  std::printf("Discovery:        UDP/%u\n", (unsigned)ALPACA_DISCOVERY_PORT);
  std::fflush(stdout);

  WinSockTransport transport;
  if (!transport.open(mainunitHost, mainunitPort))
  {
    std::printf("ERROR: cannot connect to MainUnit on %s:%d -- "
                "is mainunit_emu.exe running?\n", mainunitHost, mainunitPort);
    return 1;
  }
  transport.setTimeout(2000);
  std::printf("MainUnit link: connected.\n");
  std::fflush(stdout);

  LX200Client          client(transport, 200);
  TeenAstroMountStatus mountStatus;
  mountStatus.setClient(client);

  TeenAstroAlpaca alpaca;
  alpaca.setup(client, mountStatus, alpacaPort);
  std::printf("Alpaca server: started on port %u.\n", (unsigned)alpacaPort);
  std::fflush(stdout);

  // Refresh the bulk mount cache periodically so the very first GET on
  // every property has a fresh value (matches the firmware update() loop).
  unsigned long lastRefresh = 0;
  while (true)
  {
    alpaca.update();
    unsigned long now = (unsigned long)millis();
    if (now - lastRefresh > 250)
    {
      mountStatus.updateAllState();
      lastRefresh = now;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
  }
}

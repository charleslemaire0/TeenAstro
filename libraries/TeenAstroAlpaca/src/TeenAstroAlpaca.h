/*
 * TeenAstroAlpaca.h - ASCOM Alpaca server for TeenAstro
 *
 * Copyright (C) 2026 TeenAstro by Charles Lemaire
 * GNU General Public License v3
 *
 * Implements the ASCOM Alpaca REST/JSON interface as defined in
 *   https://ascom-standards.org/api/
 *
 * The library exposes a Telescope device (device number 0) backed by the
 * existing LX200Client + TeenAstroMountStatus, plus the Alpaca management
 * API and UDP discovery on the standard ports.
 *
 * It runs on its own ESP8266WebServer / WebServer instance bound to the
 * Alpaca port (default 11111) so it does not interfere with the existing
 * TeenAstroWifi configuration UI on port 80.
 *
 * Usage in the Arduino sketch:
 *   #include <TeenAstroAlpaca.h>
 *   TeenAstroAlpaca alpaca;
 *   ...
 *   void setup() {
 *     ...
 *     alpaca.setup(lx200, ta_MountStatus);  // after TeenAstroWifi.setup()
 *   }
 *   void loop() {
 *     ...
 *     alpaca.update();
 *   }
 */
#pragma once

#include <Arduino.h>

#if defined(ARDUINO_ARCH_ESP8266)
  #include <ESP8266WiFi.h>
  #include <ESP8266WebServer.h>
  #include <WiFiUdp.h>
#elif defined(ARDUINO_ARCH_ESP32)
  #include <WiFi.h>
  #include <WebServer.h>
  #include <WiFiUdp.h>
#else
  #error "TeenAstroAlpaca requires ESP8266 or ESP32"
#endif

#include <LX200Client.h>
#include <TeenAstroMountStatus.h>

#include "AlpacaResponse.h"
#include "AlpacaTelescope.h"

// Alpaca standard ports (https://ascom-standards.org/api/)
#ifndef ALPACA_DEFAULT_API_PORT
#define ALPACA_DEFAULT_API_PORT  11111
#endif
#ifndef ALPACA_DISCOVERY_PORT
#define ALPACA_DISCOVERY_PORT    32227
#endif

class TeenAstroAlpaca
{
public:
#if defined(ARDUINO_ARCH_ESP8266)
  using WebServerT = ESP8266WebServer;
#else
  using WebServerT = WebServer;
#endif

  TeenAstroAlpaca();

  /// Wire the LX200 client + mount status cache the telescope device will use.
  /// Starts the Alpaca HTTP server on `apiPort` and the UDP discovery
  /// responder on the standard discovery port.  Must be called after WiFi
  /// has been configured (it is OK to call before the link is up).
  void setup(LX200Client& client,
             TeenAstroMountStatus& mountStatus,
             uint16_t apiPort = ALPACA_DEFAULT_API_PORT);

  /// Stop the HTTP / UDP listeners (idempotent).
  void stop();

  /// Pump HTTP requests + UDP discovery.  Call from loop().
  void update();

  /// Currently bound API port (0 if not started).
  uint16_t apiPort() const { return m_apiPort; }

  /// Allocate a unique server transaction id for the next response.
  uint32_t nextServerTransactionId() { return ++m_serverTransactionId; }

  /// Globally unique device id used in the management /configureddevices
  /// response.  Derived from the WiFi MAC so it is stable across reboots
  /// for a given physical board.
  const char* uniqueId();

private:
  static void s_handleRoot();
  static void s_handleManagement();
  static void s_handleApi();
  static void s_handleNotFound();
  static void s_handleSetup();

  void handleManagement();
  void handleApi();
  void handleSetup();
  void serviceDiscovery();

  static TeenAstroAlpaca* s_self;

  WebServerT*           m_server = nullptr;
  WiFiUDP               m_discoveryUdp;
  AlpacaTelescope       m_telescope;
  uint16_t              m_apiPort = 0;
  uint32_t              m_serverTransactionId = 0;
  bool                  m_discoveryStarted = false;
  char                  m_uniqueId[33] = {0};
};

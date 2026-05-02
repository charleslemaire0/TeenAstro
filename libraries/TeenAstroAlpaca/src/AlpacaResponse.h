/*
 * AlpacaResponse.h - tiny JSON encoder + request decoder for ASCOM Alpaca
 *
 * Copyright (C) 2026 TeenAstro by Charles Lemaire
 * GNU General Public License v3
 *
 * Alpaca responses share a small, fixed envelope:
 *   { "Value": <T>,
 *     "ClientTransactionID": <uint32>,
 *     "ServerTransactionID": <uint32>,
 *     "ErrorNumber": <int32>,
 *     "ErrorMessage": <string> }
 *
 * To keep the footprint small on the ESP8266 (no ArduinoJson dependency),
 * we just build the JSON as a String.  The values we ever emit are simple
 * scalars (bool / int / double / string) or short arrays — well within
 * what String can handle without fragmentation issues.
 */
#pragma once

#include <Arduino.h>
#include <stdint.h>

#if defined(ARDUINO_ARCH_ESP8266)
  #include <ESP8266WebServer.h>
  using AlpacaWebServer = ESP8266WebServer;
#elif defined(ARDUINO_ARCH_ESP32)
  #include <WebServer.h>
  using AlpacaWebServer = WebServer;
#endif

// ASCOM standard error numbers (subset, see Alpaca spec)
enum AlpacaError : int32_t
{
  AE_OK                   = 0,
  AE_INVALID_VALUE        = 0x401,
  AE_VALUE_NOT_SET        = 0x402,
  AE_NOT_CONNECTED        = 0x407,
  AE_PARKED               = 0x408,
  AE_INVALID_OPERATION    = 0x40B,
  AE_ACTION_NOT_IMPL      = 0x40C,
  AE_NOT_IMPLEMENTED      = 0x400,
  AE_DRIVER               = 0x500
};

/// Decoded Alpaca request: holds the IDs the spec requires us to echo.
struct AlpacaRequest
{
  uint32_t  clientId            = 0;
  uint32_t  clientTransactionId = 0;
  bool      isPut               = false;
  /// Raw lowercase device path after stripping `/api/v1/`, e.g.
  /// `telescope/0/tracking`.
  String    devicePath;
  /// Lowercase property / method name, e.g. `tracking`.
  String    member;
  /// Lowercase device type, e.g. `telescope`.
  String    deviceType;
  /// Device number (typically 0).
  uint32_t  deviceNumber        = 0;

  /// Decode an incoming request.  `server.uri()` is the full path; common
  /// headers / form fields populate clientId + clientTransactionId.
  void decode(AlpacaWebServer& server);

  /// Lookup helper that handles both query-string (GET) and form-encoded
  /// body (PUT) — Arduino's WebServer exposes both via `server.arg()`.
  /// Names are case-insensitive per the Alpaca spec.
  static String arg(AlpacaWebServer& server, const char* name);
};

/// Simple JSON value formatter used inside the envelope.
/// All values produced are RFC8259 compliant.
namespace AlpacaJson
{
  String escape(const String& in);
  String boolStr(bool b);
  String intStr(long v);
  String uintStr(uint32_t v);
  String doubleStr(double v);
}

/// Build the standard Alpaca envelope and send it.  `valueJson` may be
/// empty (for void methods); otherwise it must be a JSON literal already
/// formatted (e.g. `"true"` / `"12.5"` / `"\"hello\""`).
void sendAlpacaResponse(AlpacaWebServer& server,
                        const AlpacaRequest& req,
                        uint32_t serverTxId,
                        AlpacaError err,
                        const String& errMessage,
                        const String& valueJson);

inline void sendAlpacaOk(AlpacaWebServer& server, const AlpacaRequest& req,
                         uint32_t serverTxId, const String& valueJson)
{
  sendAlpacaResponse(server, req, serverTxId, AE_OK, String(), valueJson);
}

inline void sendAlpacaVoid(AlpacaWebServer& server, const AlpacaRequest& req,
                           uint32_t serverTxId)
{
  sendAlpacaResponse(server, req, serverTxId, AE_OK, String(), String());
}

inline void sendAlpacaError(AlpacaWebServer& server, const AlpacaRequest& req,
                            uint32_t serverTxId, AlpacaError err,
                            const char* msg)
{
  sendAlpacaResponse(server, req, serverTxId, err, msg ? String(msg) : String(), String());
}

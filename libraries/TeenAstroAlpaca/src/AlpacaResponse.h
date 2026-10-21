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
  /// Path after `/api/v1/`, with the device type left in its original case.
  String    devicePath;
  /// Lowercase property / method name, e.g. `tracking`.
  String    member;
  /// Device type as written in the URL, e.g. `telescope`.
  String    deviceType;
  /// Device number (typically 0).
  uint32_t  deviceNumber        = 0;
  /// True when the path is `/api/v1/<type>/<digits>/<member>` with a
  /// numeric device number. Device type keeps its original case.
  bool      urlOk               = false;

  /// Decode an incoming request.  `server.uri()` is the full path; common
  /// headers / form fields populate clientId + clientTransactionId.
  void decode(AlpacaWebServer& server);

  /// This server's only device: type "telescope" (case-sensitive) number 0.
  bool isTelescope() const
  {
    return urlOk && deviceType == "telescope" && deviceNumber == 0;
  }

  /// Exact-case form/query lookup. PUT parameter names are case-sensitive.
  /// GET query names are matched case-insensitively by the readers.
  static String arg(AlpacaWebServer& server, const char* name);
};

/// Strict readers. A missing name, a wrong-case name, or a value that is
/// not the whole of a boolean / integer / number is HTTP 400. On success
/// the value is written and nothing is sent. `txId` is used only when
/// rejecting, so callers may pass a freshly allocated id.
bool alpacaArgBool(AlpacaWebServer& server, const AlpacaRequest& req,
                   uint32_t txId, const char* name, bool& out);
bool alpacaArgLong(AlpacaWebServer& server, const AlpacaRequest& req,
                   uint32_t txId, const char* name, long& out);
bool alpacaArgDouble(AlpacaWebServer& server, const AlpacaRequest& req,
                     uint32_t txId, const char* name, double& out);

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
                        const String& valueJson,
                        int httpStatus = 200);

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

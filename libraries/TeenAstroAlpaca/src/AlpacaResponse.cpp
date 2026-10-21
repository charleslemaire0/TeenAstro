/*
 * AlpacaResponse.cpp - JSON encoder + request decoder for ASCOM Alpaca
 *
 * Copyright (C) 2026 TeenAstro by Charles Lemaire
 * GNU General Public License v3
 */
#include "AlpacaResponse.h"

#include <cerrno>
#include <cstdlib>

namespace AlpacaJson
{

String escape(const String& in)
{
  String out;
  out.reserve(in.length() + 2);
  out += '"';
  for (size_t i = 0; i < in.length(); ++i)
  {
    char c = in[i];
    switch (c)
    {
      case '"':  out += "\\\""; break;
      case '\\': out += "\\\\"; break;
      case '\b': out += "\\b";  break;
      case '\f': out += "\\f";  break;
      case '\n': out += "\\n";  break;
      case '\r': out += "\\r";  break;
      case '\t': out += "\\t";  break;
      default:
        if ((uint8_t)c < 0x20)
        {
          char buf[8];
          snprintf(buf, sizeof(buf), "\\u%04x", c);
          out += buf;
        }
        else
        {
          out += c;
        }
        break;
    }
  }
  out += '"';
  return out;
}

String boolStr(bool b)         { return b ? F("true") : F("false"); }
String intStr(long v)          { return String(v); }
String uintStr(uint32_t v)     { return String(v); }
String doubleStr(double v)
{
  // Alpaca clients expect locale-independent dot decimal; Arduino's
  // String(double, prec) already uses '.' on AVR / ESP cores.
  // We use 8 digits which is more than enough for RA/Dec/Alt/Az precision.
  char buf[32];
  dtostrf(v, 0, 8, buf);
  return String(buf);
}

}  // namespace AlpacaJson


// -----------------------------------------------------------------------------
//  AlpacaRequest
// -----------------------------------------------------------------------------

String AlpacaRequest::arg(AlpacaWebServer& server, const char* name)
{
  return server.arg(name);
}

// ClientID / ClientTransactionID stay case-insensitive. Conform reports a
// round-trip of a differently cased id as information, not a failure.
static String argLoose(AlpacaWebServer& server, const char* name)
{
  String v = server.arg(name);
  if (v.length() > 0) return v;
  String lower(name);
  lower.toLowerCase();
  if (lower == name) return v;
  return server.arg(lower.c_str());
}

// A negative, blank, or non-numeric id is echoed as 0. String::toInt() of
// "-67890" wrapped to 4294899406, which Conform rejects.
static uint32_t parseClientUint(const String& raw)
{
  String t = raw;
  t.trim();
  if (t.length() == 0) return 0;
  for (unsigned i = 0; i < t.length(); ++i)
  {
    char c = t.charAt(i);
    if (c < '0' || c > '9') return 0;
  }
  errno = 0;
  char* end = nullptr;
  unsigned long n = strtoul(t.c_str(), &end, 10);
  if (end == t.c_str() || (end && *end != '\0') || errno == ERANGE) return 0;
  if (n > 0xFFFFFFFFul) return 0;
  return (uint32_t)n;
}

static bool allDigits(const String& s)
{
  if (s.length() == 0) return false;
  for (unsigned i = 0; i < s.length(); ++i)
  {
    char c = s.charAt(i);
    if (c < '0' || c > '9') return false;
  }
  return true;
}

static void rejectParameter(AlpacaWebServer& server, const AlpacaRequest& req,
                            uint32_t txId)
{
  sendAlpacaResponse(server, req, txId, AE_INVALID_VALUE,
                     "Invalid parameter", String(), 400);
}

static bool parseBool(const String& raw, bool& out)
{
  String t = raw;
  t.trim();
  t.toLowerCase();
  if (t == "true")  { out = true;  return true; }
  if (t == "false") { out = false; return true; }
  return false;
}

static bool parseLong(const String& raw, long& out)
{
  String t = raw;
  t.trim();
  if (t.length() == 0) return false;
  errno = 0;
  char* end = nullptr;
  long v = strtol(t.c_str(), &end, 10);
  if (end == t.c_str() || (end && *end != '\0') || errno == ERANGE) return false;
  out = v;
  return true;
}

static bool parseDouble(const String& raw, double& out)
{
  String t = raw;
  t.trim();
  if (t.length() == 0) return false;
  errno = 0;
  char* end = nullptr;
  double v = strtod(t.c_str(), &end);
  if (end == t.c_str() || (end && *end != '\0') || errno == ERANGE) return false;
  if (v != v) return false;
  out = v;
  return true;
}

// PUT form names are case-sensitive (Conform "Bad casing" expects HTTP 400).
// GET query names are not: Conform sends InvertCasing("Axis") == "aXIS" and
// expects HTTP 200.
static bool findArg(AlpacaWebServer& server, const AlpacaRequest& req,
                    const char* name, String& value)
{
  if (server.hasArg(name))
  {
    value = server.arg(name);
    return true;
  }
  if (req.isPut) return false;

  String want(name);
  want.toLowerCase();
  const int n = server.args();
  for (int i = 0; i < n; ++i)
  {
    String key = server.argName(i);
    key.toLowerCase();
    if (key == want)
    {
      value = server.arg(i);
      return true;
    }
  }
  return false;
}

bool alpacaArgBool(AlpacaWebServer& server, const AlpacaRequest& req,
                   uint32_t txId, const char* name, bool& out)
{
  String raw;
  if (!findArg(server, req, name, raw) || !parseBool(raw, out))
  {
    rejectParameter(server, req, txId);
    return false;
  }
  return true;
}

bool alpacaArgLong(AlpacaWebServer& server, const AlpacaRequest& req,
                   uint32_t txId, const char* name, long& out)
{
  String raw;
  if (!findArg(server, req, name, raw) || !parseLong(raw, out))
  {
    rejectParameter(server, req, txId);
    return false;
  }
  return true;
}

bool alpacaArgDouble(AlpacaWebServer& server, const AlpacaRequest& req,
                     uint32_t txId, const char* name, double& out)
{
  String raw;
  if (!findArg(server, req, name, raw) || !parseDouble(raw, out))
  {
    rejectParameter(server, req, txId);
    return false;
  }
  return true;
}

void AlpacaRequest::decode(AlpacaWebServer& server)
{
  isPut = (server.method() == HTTP_PUT);
  clientId = parseClientUint(argLoose(server, "ClientID"));
  clientTransactionId = parseClientUint(argLoose(server, "ClientTransactionID"));

  urlOk = false;
  member = "";
  deviceType = "";
  deviceNumber = 0;
  devicePath = "";

  // Path: /api/v1/<deviceType>/<deviceNumber>/<member>
  // Device type is compared case-sensitively, so TELESCOPE is not telescope.
  // The member is folded so Description still matches the dispatch table.
  String path = server.uri();
  int q = path.indexOf('?');
  if (q >= 0) path = path.substring(0, q);
  const char* prefix = "/api/v1/";
  int p = path.indexOf(prefix);
  if (p < 0) return;

  devicePath = path.substring(p + strlen(prefix));
  int s1 = devicePath.indexOf('/');
  if (s1 <= 0) return;
  deviceType = devicePath.substring(0, s1);

  int s2 = devicePath.indexOf('/', s1 + 1);
  String num = (s2 < 0) ? devicePath.substring(s1 + 1)
                        : devicePath.substring(s1 + 1, s2);
  num.trim();
  if (!allDigits(num)) return;
  errno = 0;
  char* end = nullptr;
  unsigned long n = strtoul(num.c_str(), &end, 10);
  if (end == num.c_str() || (end && *end != '\0') || errno == ERANGE) return;
  if (n > 0xFFFFFFFFul) return;
  deviceNumber = (uint32_t)n;

  if (s2 >= 0)
  {
    member = devicePath.substring(s2 + 1);
    member.toLowerCase();
  }
  urlOk = true;
}


// -----------------------------------------------------------------------------
//  Envelope writer
// -----------------------------------------------------------------------------

void sendAlpacaResponse(AlpacaWebServer& server,
                        const AlpacaRequest& req,
                        uint32_t serverTxId,
                        AlpacaError err,
                        const String& errMessage,
                        const String& valueJson,
                        int httpStatus)
{
  String body;
  body.reserve(160 + valueJson.length() + errMessage.length());
  body += '{';
  if (valueJson.length() > 0)
  {
    body += F("\"Value\":");
    body += valueJson;
    body += ',';
  }
  body += F("\"ClientTransactionID\":");
  body += AlpacaJson::uintStr(req.clientTransactionId);
  body += F(",\"ServerTransactionID\":");
  body += AlpacaJson::uintStr(serverTxId);
  body += F(",\"ErrorNumber\":");
  body += AlpacaJson::intStr((long)err);
  body += F(",\"ErrorMessage\":");
  body += AlpacaJson::escape(errMessage);
  body += '}';

  server.sendHeader(F("Cache-Control"), F("no-store"));
  server.send(httpStatus, F("application/json"), body);
}

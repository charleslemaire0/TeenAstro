/*
 * AlpacaResponse.cpp - JSON encoder + request decoder for ASCOM Alpaca
 *
 * Copyright (C) 2026 TeenAstro by Charles Lemaire
 * GNU General Public License v3
 */
#include "AlpacaResponse.h"

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
  // server.arg() is case-sensitive in the Arduino webserver; try both the
  // canonical PascalCase and a lowercase variant since Alpaca treats names
  // as case-insensitive.
  String v = server.arg(name);
  if (v.length() > 0) return v;

  String lower(name);
  lower.toLowerCase();
  return server.arg(lower.c_str());
}

void AlpacaRequest::decode(AlpacaWebServer& server)
{
#if defined(ARDUINO_ARCH_ESP8266)
  isPut = (server.method() == HTTP_PUT);
#else
  isPut = (server.method() == HTTP_PUT);
#endif

  String c = arg(server, "ClientID");
  clientId = (uint32_t)c.toInt();
  String t = arg(server, "ClientTransactionID");
  clientTransactionId = (uint32_t)t.toInt();

  // Path: /api/v1/<deviceType>/<deviceNumber>/<member>
  String path = server.uri();
  path.toLowerCase();
  const char* prefix = "/api/v1/";
  int p = path.indexOf(prefix);
  if (p >= 0)
    devicePath = path.substring(p + strlen(prefix));
  else
    devicePath = path;

  // Split devicePath into deviceType / deviceNumber / member
  int s1 = devicePath.indexOf('/');
  int s2 = (s1 >= 0) ? devicePath.indexOf('/', s1 + 1) : -1;
  if (s1 > 0)
  {
    deviceType = devicePath.substring(0, s1);
    if (s2 > s1)
    {
      deviceNumber = (uint32_t)devicePath.substring(s1 + 1, s2).toInt();
      member       = devicePath.substring(s2 + 1);
    }
    else
    {
      deviceNumber = (uint32_t)devicePath.substring(s1 + 1).toInt();
    }
  }
}


// -----------------------------------------------------------------------------
//  Envelope writer
// -----------------------------------------------------------------------------

void sendAlpacaResponse(AlpacaWebServer& server,
                        const AlpacaRequest& req,
                        uint32_t serverTxId,
                        AlpacaError err,
                        const String& errMessage,
                        const String& valueJson)
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
  server.send(200, F("application/json"), body);
}

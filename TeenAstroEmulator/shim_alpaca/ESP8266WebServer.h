/*
 * ESP8266WebServer.h - mock WebServer for the verify_alpaca test harness.
 *
 * Implements the API surface used by TeenAstroAlpaca + AlpacaTelescope
 * (uri / method / arg / send / sendHeader / on / onNotFound / handleClient
 * / setContentLength / sendContent / begin / stop), but does NOT actually
 * serve HTTP.  Test code injects `mock_request(...)` and reads
 * `last_code` / `last_body` afterwards.
 */
#pragma once

#include "Arduino.h"
#include <map>
#include <string>

#ifndef HTTP_GET
#define HTTP_GET   1
#define HTTP_POST  3
#define HTTP_PUT   4
#define HTTP_ANY   8
#endif

#ifndef CONTENT_LENGTH_UNKNOWN
#define CONTENT_LENGTH_UNKNOWN ((size_t)-1)
#endif

class ESP8266WebServer
{
public:
  ESP8266WebServer(int port = 80) : m_port(port) {}

  // ----- registration (no-ops in mock) -----
  void on(const char*, void (*)())              {}
  void on(const char*, int, void (*)())         {}
  void on(const __FlashStringHelper*, void (*)()) {}
  void onNotFound(void (*)())                   {}

  // ----- lifecycle (no-ops) -----
  void begin()                                  {}
  void stop()                                   {}
  void handleClient()                           {}

  // ----- response building -----
  void setContentLength(size_t)                 {}
  void sendHeader(const char*, const char*)     {}
  void sendHeader(const __FlashStringHelper*, const __FlashStringHelper*) {}
  void sendHeader(const __FlashStringHelper*, const String&) {}

  void send(int code, const char* type, const String& body)
  { last_code = code; last_type = type ? type : ""; last_body = body; sent = true; }

  void send(int code, const String& type, const String& body)
  { last_code = code; last_type = type.c_str(); last_body = body; sent = true; }

  void send(int code, const __FlashStringHelper* type, const String& body)
  { last_code = code; last_type = reinterpret_cast<const char*>(type); last_body = body; sent = true; }

  void sendContent(const String&)               {}
  void sendContent(const char*)                 {}

  // ----- request introspection -----
  String uri()                                  { return current_uri; }
  int    method() const                         { return current_method; }
  String arg(const char* name)
  {
    auto it = current_args.find(name ? name : "");
    return it == current_args.end() ? String("") : String(it->second.c_str());
  }
  String arg(const String& name)                { return arg(name.c_str()); }
  bool   hasArg(const char* name)
  { return current_args.find(name ? name : "") != current_args.end(); }

  // ----- test driver helpers -----
  void mock_request(const String& uri,
                    int method,
                    const std::map<std::string, std::string>& args)
  {
    current_uri    = uri;
    current_method = method;
    current_args   = args;
    last_code      = 0;
    last_type.clear();
    last_body      = String("");
    sent           = false;
  }

  // captured response
  int          last_code = 0;
  std::string  last_type;
  String       last_body;
  bool         sent = false;

private:
  int                                m_port;
  String                             current_uri;
  int                                current_method = HTTP_GET;
  std::map<std::string, std::string> current_args;
};

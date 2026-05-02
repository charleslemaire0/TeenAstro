/*
 * ESP8266WebServer.h - real (winsock-backed) HTTP/1.1 server shim used by
 * the `bridge_alpaca` test target.
 *
 * Implements just enough of the ESP8266WebServer surface that
 * TeenAstroAlpaca uses, so the same `libraries/TeenAstroAlpaca/src/*.cpp`
 * code that runs on the ESP8266 also runs natively on Windows for
 * conformance testing with ASCOM ConformU.
 *
 * Header-only for simplicity (the bridge is single-TU anyway).
 */
#pragma once

#include "Arduino.h"   // pulls in winsock2 + WString + min/max
#include <string>
#include <vector>
#include <map>
#include <cstdio>

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
  using Handler = void (*)();

  explicit ESP8266WebServer(int port = 80) : m_port(port) {}
  ~ESP8266WebServer() { stop(); }

  // ----- registration -----
  void on(const char* uri, Handler h)             { m_routes.push_back({uri ? uri : "", -1, h}); }
  void on(const char* uri, int method, Handler h) { m_routes.push_back({uri ? uri : "", method, h}); }
  void on(const __FlashStringHelper* uri, Handler h)
  { m_routes.push_back({uri ? reinterpret_cast<const char*>(uri) : "", -1, h}); }
  void onNotFound(Handler h)                      { m_notFound = h; }

  // ----- lifecycle -----
  bool begin()
  {
    static bool wsaInited = false;
    if (!wsaInited)
    {
      WSADATA wd; WSAStartup(MAKEWORD(2, 2), &wd);
      wsaInited = true;
    }
    m_listen = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_listen == INVALID_SOCKET) return false;
    int opt = 1;
    setsockopt(m_listen, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
    sockaddr_in addr; std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port        = htons((uint16_t)m_port);
    if (::bind(m_listen, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) { stop(); return false; }
    if (::listen(m_listen, 4) == SOCKET_ERROR)                            { stop(); return false; }
    u_long mode = 1; ioctlsocket(m_listen, FIONBIO, &mode);
    return true;
  }

  void stop()
  {
    if (m_listen != INVALID_SOCKET) { closesocket(m_listen); m_listen = INVALID_SOCKET; }
  }

  // ----- main pump -----
  void handleClient()
  {
    if (m_listen == INVALID_SOCKET) return;
    sockaddr_in ca; int len = sizeof(ca);
    SOCKET cs = accept(m_listen, (sockaddr*)&ca, &len);
    if (cs == INVALID_SOCKET) return;   // EWOULDBLOCK
    serveOne(cs);
    closesocket(cs);
  }

  // ----- request introspection (during a handler) -----
  String uri()    const { return String(m_curUri.c_str()); }
  int    method() const { return m_curMethod; }

  String arg(const char* name)
  {
    if (!name) return String("");
    auto it = m_curArgs.find(name);
    return it == m_curArgs.end() ? String("") : String(it->second.c_str());
  }
  String arg(const String& name) { return arg(name.c_str()); }
  bool   hasArg(const char* name)
  { return name && m_curArgs.find(name) != m_curArgs.end(); }

  // ----- response builders -----
  void send(int code, const char* type, const String& body)
  { sendImpl(code, type ? type : "text/plain", body.c_str(), body.length()); }
  void send(int code, const __FlashStringHelper* type, const String& body)
  { sendImpl(code, type ? reinterpret_cast<const char*>(type) : "text/plain", body.c_str(), body.length()); }
  void send(int code, const String& type, const String& body)
  { sendImpl(code, type.c_str(), body.c_str(), body.length()); }

  void sendHeader(const char* name, const char* value)
  { if (name && value) m_extraHeaders.push_back({name, value}); }
  void sendHeader(const __FlashStringHelper* name, const __FlashStringHelper* value)
  { if (name && value) m_extraHeaders.push_back({reinterpret_cast<const char*>(name),
                                                  reinterpret_cast<const char*>(value)}); }
  void sendHeader(const __FlashStringHelper* name, const String& value)
  { if (name) m_extraHeaders.push_back({reinterpret_cast<const char*>(name), value.c_str()}); }
  void sendHeader(const char* name, const String& value)
  { if (name) m_extraHeaders.push_back({name, value.c_str()}); }

  void setContentLength(size_t)            {}   // we always know our content length
  void sendContent(const String&)          {}
  void sendContent(const char*)            {}

private:
  struct Route { std::string uri; int method; Handler h; };

  // -------------------------------------------------------------------------
  //  Per-request state
  // -------------------------------------------------------------------------
  std::string                                          m_curUri;
  int                                                  m_curMethod = HTTP_GET;
  std::map<std::string, std::string>                   m_curArgs;
  SOCKET                                               m_curClient    = INVALID_SOCKET;
  bool                                                 m_responseSent = false;
  std::vector<std::pair<std::string,std::string>>      m_extraHeaders;

  int                                                  m_port;
  SOCKET                                               m_listen   = INVALID_SOCKET;
  std::vector<Route>                                   m_routes;
  Handler                                              m_notFound = nullptr;

  // -------------------------------------------------------------------------
  //  Recv one full HTTP request, parse, dispatch, ensure a response is sent.
  // -------------------------------------------------------------------------
  void serveOne(SOCKET cs)
  {
    DWORD timeoutMs = 5000;
    setsockopt(cs, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeoutMs, sizeof(timeoutMs));
    setsockopt(cs, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeoutMs, sizeof(timeoutMs));

    std::string raw;
    raw.reserve(2048);
    char buf[2048];
    size_t headerEnd = std::string::npos;

    // Read until end-of-headers.
    while (headerEnd == std::string::npos)
    {
      int r = recv(cs, buf, (int)sizeof(buf), 0);
      if (r <= 0) return;
      raw.append(buf, (size_t)r);
      headerEnd = raw.find("\r\n\r\n");
      if (raw.size() > 32768) return;  // protect against runaway clients
    }

    std::string headerBlock = raw.substr(0, headerEnd);
    std::string body        = raw.substr(headerEnd + 4);
    // Sentinel CRLF so the final header line is parsed by the find()-loop
    // below (otherwise the trailing `\r\n\r\n` is stripped and the last
    // header gets silently dropped).
    headerBlock += "\r\n";

    // Parse Content-Length to determine if we still need body bytes.
    size_t      contentLength = 0;
    std::string contentType;
    {
      size_t pos = 0;
      while (true)
      {
        size_t eol = headerBlock.find("\r\n", pos);
        if (eol == std::string::npos) break;
        std::string line = headerBlock.substr(pos, eol - pos);
        pos = eol + 2;
        size_t colon = line.find(':');
        if (colon == std::string::npos) continue;
        std::string name  = line.substr(0, colon);
        std::string value = line.substr(colon + 1);
        // trim
        while (!value.empty() && (value.front() == ' ' || value.front() == '\t')) value.erase(value.begin());
        while (!value.empty() && (value.back() == '\r'  || value.back()  == ' ')) value.pop_back();
        std::string lname = lowerStr(name);
        if      (lname == "content-length") contentLength = (size_t)std::strtoul(value.c_str(), nullptr, 10);
        else if (lname == "content-type")   contentType   = value;
      }
    }
    while (body.size() < contentLength)
    {
      int r = recv(cs, buf, (int)sizeof(buf), 0);
      if (r <= 0) break;
      body.append(buf, (size_t)r);
    }

    // Parse request line.
    size_t firstEol = headerBlock.find("\r\n");
    std::string requestLine = (firstEol == std::string::npos) ? headerBlock
                                                              : headerBlock.substr(0, firstEol);
    std::string methodStr, target, version;
    {
      size_t s1 = requestLine.find(' ');
      size_t s2 = (s1 == std::string::npos) ? std::string::npos : requestLine.find(' ', s1 + 1);
      if (s1 == std::string::npos || s2 == std::string::npos) { sendStatus(cs, 400, "Bad Request"); return; }
      methodStr = requestLine.substr(0, s1);
      target    = requestLine.substr(s1 + 1, s2 - s1 - 1);
      version   = requestLine.substr(s2 + 1);
    }

    // Split target into path + query.
    std::string path, query;
    size_t qmark = target.find('?');
    if (qmark == std::string::npos) { path = target; }
    else                            { path = target.substr(0, qmark); query = target.substr(qmark + 1); }

    // Decode args.
    std::map<std::string, std::string> args;
    parseUrlEncoded(query, args);
    if (lowerStr(contentType).find("application/x-www-form-urlencoded") != std::string::npos)
      parseUrlEncoded(body, args);

    // Set per-request state.
    m_curUri       = path;
    m_curMethod    = methodToInt(methodStr);
    m_curArgs      = args;
    m_curClient    = cs;
    m_responseSent = false;
    m_extraHeaders.clear();

#ifdef ALPACA_HTTP_DEBUG
    std::fprintf(stderr, "[HTTP] %s %s body=%zu ct=%s args={",
                 methodStr.c_str(), path.c_str(), body.size(), contentType.c_str());
    for (auto& kv : m_curArgs) std::fprintf(stderr, "%s=%s; ", kv.first.c_str(), kv.second.c_str());
    std::fprintf(stderr, "}\n"); std::fflush(stderr);
#endif

    // Dispatch: explicit route first (filter by method), then onNotFound.
    Handler dispatched = nullptr;
    for (auto& r : m_routes)
    {
      if (r.uri != path) continue;
      if (r.method >= 0 && r.method != m_curMethod) continue;
      dispatched = r.h;
      break;
    }
    if (!dispatched) dispatched = m_notFound;
    if (dispatched) dispatched();

    if (!m_responseSent) sendStatus(cs, 404, "Not Found");
    m_curClient = INVALID_SOCKET;
  }

  void sendImpl(int code, const char* type, const char* body, int bodyLen)
  {
    if (m_curClient == INVALID_SOCKET) return;
    const char* reason = (code == 200) ? "OK"
                       : (code == 400) ? "Bad Request"
                       : (code == 404) ? "Not Found"
                       : (code == 500) ? "Internal Server Error"
                                       : "Status";
    char hdr[1024];
    int hdrLen = std::snprintf(hdr, sizeof(hdr),
                               "HTTP/1.1 %d %s\r\n"
                               "Content-Type: %s\r\n"
                               "Content-Length: %d\r\n"
                               "Connection: close\r\n",
                               code, reason, type, bodyLen);
    for (auto& kv : m_extraHeaders)
    {
      int n = std::snprintf(hdr + hdrLen, sizeof(hdr) - (size_t)hdrLen,
                            "%s: %s\r\n", kv.first.c_str(), kv.second.c_str());
      if (n > 0) hdrLen += n;
    }
    if ((size_t)hdrLen + 2 < sizeof(hdr))
    {
      hdr[hdrLen++] = '\r'; hdr[hdrLen++] = '\n';
    }
    ::send(m_curClient, hdr, hdrLen, 0);
    if (body && bodyLen > 0) ::send(m_curClient, body, bodyLen, 0);
    m_responseSent = true;
  }

  static void sendStatus(SOCKET cs, int code, const char* reason)
  {
    char buf[256];
    int n = std::snprintf(buf, sizeof(buf),
                          "HTTP/1.1 %d %s\r\nContent-Length: 0\r\nConnection: close\r\n\r\n",
                          code, reason);
    ::send(cs, buf, n, 0);
  }

  // -------------------------------------------------------------------------
  //  Helpers
  // -------------------------------------------------------------------------
  static std::string lowerStr(std::string s)
  {
    for (auto& c : s) c = (char)std::tolower((unsigned char)c);
    return s;
  }

  static int methodToInt(const std::string& m)
  {
    if (m == "GET")  return HTTP_GET;
    if (m == "PUT")  return HTTP_PUT;
    if (m == "POST") return HTTP_POST;
    return HTTP_ANY;
  }

  static std::string urlDecode(const std::string& s)
  {
    std::string out;
    out.reserve(s.size());
    for (size_t i = 0; i < s.size(); ++i)
    {
      char c = s[i];
      if      (c == '+') out += ' ';
      else if (c == '%' && i + 2 < s.size())
      {
        auto hex = [](char x){
          if (x >= '0' && x <= '9') return x - '0';
          if (x >= 'a' && x <= 'f') return x - 'a' + 10;
          if (x >= 'A' && x <= 'F') return x - 'A' + 10;
          return 0;
        };
        out += (char)((hex(s[i+1]) << 4) | hex(s[i+2]));
        i += 2;
      }
      else out += c;
    }
    return out;
  }

  static void parseUrlEncoded(const std::string& s,
                              std::map<std::string, std::string>& out)
  {
    size_t i = 0;
    while (i < s.size())
    {
      size_t amp = s.find('&', i);
      if (amp == std::string::npos) amp = s.size();
      std::string pair = s.substr(i, amp - i);
      size_t eq = pair.find('=');
      if (eq == std::string::npos) { out[urlDecode(pair)] = ""; }
      else { out[urlDecode(pair.substr(0, eq))] = urlDecode(pair.substr(eq + 1)); }
      i = amp + 1;
    }
  }
};

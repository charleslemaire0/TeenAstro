#ifdef EMU_MAINUNIT

#ifdef _WIN32
#  ifndef _WIN32_WINNT
#    define _WIN32_WINNT 0x0601
#  endif
#  include <winsock2.h>
#  include <ws2tcpip.h>
#endif

#include "http_cmd_proxy.h"

#include <atomic>
#include <cstdio>
#include <cstring>
#include <string>
#include <thread>
#include <vector>

#ifdef _WIN32

namespace {

std::atomic<bool> g_proxy_stop{false};
std::thread       g_proxy_thread;

static void set_recv_timeout(SOCKET s, int ms)
{
  DWORD tv = (DWORD)ms;
  setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
}

static bool recv_line(SOCKET s, std::string& line)
{
  line.clear();
  char c;
  for (;;) {
    int r = recv(s, &c, 1, 0);
    if (r <= 0)
      return false;
    if (c == '\n') {
      while (!line.empty() && line.back() == '\r')
        line.pop_back();
      return true;
    }
    line.push_back(c);
    if (line.size() > 8192)
      return false;
  }
}

static std::string url_decode(const std::string& s)
{
  std::string out;
  out.reserve(s.size());
  auto hex = [](char c) -> int {
    if (c >= '0' && c <= '9')
      return c - '0';
    if (c >= 'a' && c <= 'f')
      return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
      return c - 'A' + 10;
    return -1;
  };
  for (size_t i = 0; i < s.size(); ++i) {
    if (s[i] == '%' && i + 2 < s.size()) {
      int h = hex(s[i + 1]), l = hex(s[i + 2]);
      if (h >= 0 && l >= 0) {
        out.push_back(char((h << 4) | l));
        i += 2;
        continue;
      }
    }
    out.push_back(s[i]);
  }
  return out;
}

static std::string extract_q_param(const std::string& uri)
{
  size_t qpos = uri.find('?');
  if (qpos == std::string::npos)
    return "";
  std::string qs = uri.substr(qpos + 1);
  size_t start = qs.find("q=");
  if (start == std::string::npos)
    return "";
  start += 2;
  size_t end = qs.find('&', start);
  std::string raw =
      (end == std::string::npos) ? qs.substr(start) : qs.substr(start, end - start);
  return url_decode(raw);
}

static SOCKET tcp_connect_loopback(int port)
{
  SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (sock == INVALID_SOCKET)
    return INVALID_SOCKET;
  sockaddr_in addr{};
  addr.sin_family      = AF_INET;
  addr.sin_port        = htons((uint16_t)port);
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  if (connect(sock, (sockaddr*)&addr, sizeof(addr)) != 0) {
    closesocket(sock);
    return INVALID_SOCKET;
  }
  return sock;
}

static bool recv_lx200_reply(SOCKET s, std::vector<char>& out)
{
  out.clear();
  char b;
  const size_t maxr = 262144;
  while (out.size() < maxr) {
    int r = recv(s, &b, 1, 0);
    if (r <= 0)
      return false;
    out.push_back(b);
    if (b == '#')
      break;
  }
  return true;
}

static void serve_one(SOCKET client, int lx200_port)
{
  set_recv_timeout(client, 30000);
  std::string req0;
  if (!recv_line(client, req0)) {
    closesocket(client);
    return;
  }
  for (;;) {
    std::string line;
    if (!recv_line(client, line)) {
      closesocket(client);
      return;
    }
    if (line.empty())
      break;
  }

  size_t sp = req0.find(' ');
  if (sp == std::string::npos) {
    closesocket(client);
    return;
  }
  size_t sp2 = req0.find(' ', sp + 1);
  if (sp2 == std::string::npos) {
    closesocket(client);
    return;
  }
  std::string uri = req0.substr(sp + 1, sp2 - sp - 1);

  std::string path = uri;
  {
    size_t qmark = path.find('?');
    if (qmark != std::string::npos)
      path = path.substr(0, qmark);
  }
  if (path == "/" || path.empty()) {
    static const char kIndex[] = R"HTML(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>TeenAstro PC emulator (HTTP)</title>
</head>
<body style="font-family:system-ui,Segoe UI,sans-serif;max-width:42rem;margin:2rem;line-height:1.55;color:#142;">
<h1 style="margin-top:0;font-size:1.35rem;">TeenAstro MainUnit PC emulator</h1>

<div role="note" style="border:2px solid #b8860b;background:#fffbeb;padding:1rem 1.2rem;margin:1.25rem 0;border-radius:8px;">
<strong style="display:block;margin-bottom:.35rem;">Full Wi-Fi configuration website &mdash; not implemented</strong>
<p style="margin:0;">On real TeenAstro hardware, the Smart Hand Controller or Wi-Fi module serves HTML pages (for example <code>/configuration_site.htm</code>, <code>/control.htm</code>) over HTTP on port&nbsp;80.</p>
<p style="margin:.75rem 0 0;"><strong>This emulator does not implement those pages.</strong> The SHC desktop build replaces TeenAstro Wi-Fi with stubs (no HTTP server, no ESP8266WebServer behaviour).</p>
<p style="margin:.75rem 0 0;">Use a physical SHC or Main Unit/server board to reach that UI in a browser.</p>
</div>

<h2 style="font-size:1.05rem;margin-top:1.75rem;">What <em>is</em> available here</h2>
<p>This port exposes only an ASCOM-style bridge: <code>GET /cmd?q=&hellip;</code> forwards LX200 commands to <code>TCP 127.0.0.1:9997</code> (USB serial channel on the emulator).</p>

<p style="margin-bottom:.35rem;"><strong>Try:</strong></p>
<ul>
<li><a href="/cmd?q=:GVP%23">Product name (<code>:GVP#</code>)</a></li>
<li><a href="/cmd?q=:GVN%23">Firmware name (<code>:GVN#</code>)</a></li>
<li><a href="/cmd?q=:GW%23">Site / status (<code>:GW#</code>)</a></li>
</ul>

<p style="margin-top:1.5rem;font-size:.9rem;color:#444;">REST tooling for Alpaca uses the separate <code>bridge_alpaca</code> build (typically port&nbsp;11111), not this page.</p>
</body>
</html>
)HTML";
    char hdr[256];
    int  hl = snprintf(hdr, sizeof(hdr),
                       "HTTP/1.0 200 OK\r\nContent-Type: text/html; charset=utf-8\r\nContent-Length: %zu\r\n"
                       "Connection: close\r\n\r\n",
                       sizeof(kIndex) - 1);
    if (hl > 0 && hl < (int)sizeof(hdr)) {
      send(client, hdr, hl, 0);
      send(client, kIndex, (int)(sizeof(kIndex) - 1), 0);
    }
    closesocket(client);
    return;
  }

  std::string cmd = extract_q_param(uri);
  if (cmd.empty()) {
    const char* bad =
        "HTTP/1.0 400 Bad Request\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
    send(client, bad, (int)strlen(bad), 0);
    closesocket(client);
    return;
  }

  SOCKET lx = tcp_connect_loopback(lx200_port);
  if (lx == INVALID_SOCKET) {
    const char* fail =
        "HTTP/1.0 503 Service Unavailable\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
    send(client, fail, (int)strlen(fail), 0);
    closesocket(client);
    return;
  }
  set_recv_timeout(lx, 30000);

  int sent = send(lx, cmd.data(), (int)cmd.size(), 0);
  if (sent != (int)cmd.size()) {
    closesocket(lx);
    const char* fail = "HTTP/1.0 502 Bad Gateway\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
    send(client, fail, (int)strlen(fail), 0);
    closesocket(client);
    return;
  }

  std::vector<char> body;
  if (!recv_lx200_reply(lx, body)) {
    closesocket(lx);
    const char* fail =
        "HTTP/1.0 504 Gateway Timeout\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
    send(client, fail, (int)strlen(fail), 0);
    closesocket(client);
    return;
  }
  closesocket(lx);

  char hdr[256];
  int hl = snprintf(hdr, sizeof(hdr),
                    "HTTP/1.0 200 OK\r\nContent-Length: %zu\r\nConnection: close\r\n\r\n",
                    body.size());
  if (hl > 0 && hl < (int)sizeof(hdr)) {
    send(client, hdr, hl, 0);
  }
  if (!body.empty())
    send(client, body.data(), (int)body.size(), 0);
  closesocket(client);
}

static void proxy_loop_fn(int listen_port, int lx200_port)
{
  SOCKET ls = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (ls == INVALID_SOCKET)
    return;
  int opt = 1;
  setsockopt(ls, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
  sockaddr_in addr{};
  addr.sin_family      = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  addr.sin_port        = htons((uint16_t)listen_port);
  if (bind(ls, (sockaddr*)&addr, sizeof(addr)) != 0) {
    fprintf(stderr, "[HttpCmdProxy] bind(127.0.0.1:%d) failed (WSA %d)\n", listen_port,
            WSAGetLastError());
    fflush(stderr);
    closesocket(ls);
    return;
  }
  if (listen(ls, 8) != 0) {
    closesocket(ls);
    return;
  }
  printf("[HttpCmdProxy] ASCOM HTTP  http://127.0.0.1:%d/cmd?q=  ->  LX200 TCP 127.0.0.1:%d\n",
         listen_port, lx200_port);
  fflush(stdout);

  while (!g_proxy_stop.load()) {
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(ls, &rfds);
    timeval tv{0, 200000};
    select(0, &rfds, nullptr, nullptr, &tv);
    if (!FD_ISSET(ls, &rfds))
      continue;
    SOCKET c = accept(ls, nullptr, nullptr);
    if (c == INVALID_SOCKET)
      continue;
    std::thread(serve_one, c, lx200_port).detach();
  }
  closesocket(ls);
}

} // namespace

void emu_http_cmd_proxy_start(int http_listen_port, int lx200_tcp_port)
{
  g_proxy_stop = false;
  if (g_proxy_thread.joinable())
    g_proxy_thread.join();
  g_proxy_thread = std::thread(proxy_loop_fn, http_listen_port, lx200_tcp_port);
}

#else /* !_WIN32 */

void emu_http_cmd_proxy_start(int, int) {}

#endif /* _WIN32 */

#endif /* EMU_MAINUNIT */

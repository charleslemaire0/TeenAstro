/*
 * WiFiUdp.h - real (winsock-backed) UDP shim for the bridge_alpaca env.
 *
 * Used by TeenAstroAlpaca::serviceDiscovery() to answer the Alpaca
 * `alpacadiscovery1` probe on UDP/32227.
 */
#pragma once

#include "Arduino.h"   // pulls in winsock2

class WiFiUDP
{
public:
  ~WiFiUDP() { stop(); }

  bool begin(uint16_t port)
  {
    static bool wsa = false;
    if (!wsa) { WSADATA wd; WSAStartup(MAKEWORD(2,2), &wd); wsa = true; }
    m_sock = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (m_sock == INVALID_SOCKET) return false;
    int opt = 1;
    setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
    setsockopt(m_sock, SOL_SOCKET, SO_BROADCAST, (const char*)&opt, sizeof(opt));
    sockaddr_in addr; std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port        = htons(port);
    if (::bind(m_sock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) { stop(); return false; }
    u_long mode = 1; ioctlsocket(m_sock, FIONBIO, &mode);
    m_port = port;
    return true;
  }

  void stop()
  {
    if (m_sock != INVALID_SOCKET) { closesocket(m_sock); m_sock = INVALID_SOCKET; }
  }

  int parsePacket()
  {
    if (m_sock == INVALID_SOCKET) return 0;
    int alen = sizeof(m_lastFrom);
    int r = ::recvfrom(m_sock, m_buf, (int)sizeof(m_buf), 0,
                       (sockaddr*)&m_lastFrom, &alen);
    if (r <= 0) { m_pending = 0; return 0; }
    m_pending = r;
    return r;
  }

  int read(char* dst, int max)
  {
    int n = (m_pending < max) ? m_pending : max;
    if (n > 0) std::memcpy(dst, m_buf, (size_t)n);
    m_pending = 0;
    return n;
  }

  IPAddress remoteIP()  { return IPAddress(m_lastFrom.sin_addr.s_addr); }
  uint16_t  remotePort(){ return ntohs(m_lastFrom.sin_port); }

  void beginPacket(IPAddress ip, uint16_t port)
  {
    std::memset(&m_to, 0, sizeof(m_to));
    m_to.sin_family = AF_INET;
    m_to.sin_addr.s_addr = (uint32_t)ip;
    m_to.sin_port        = htons(port);
    m_outLen = 0;
  }
  void write(const uint8_t* p, size_t n)
  {
    if (n + m_outLen > sizeof(m_outBuf)) n = sizeof(m_outBuf) - m_outLen;
    std::memcpy(m_outBuf + m_outLen, p, n);
    m_outLen += n;
  }
  void endPacket()
  {
    if (m_sock == INVALID_SOCKET || m_outLen == 0) return;
    ::sendto(m_sock, m_outBuf, (int)m_outLen, 0, (sockaddr*)&m_to, sizeof(m_to));
    m_outLen = 0;
  }

private:
  SOCKET       m_sock = INVALID_SOCKET;
  uint16_t     m_port = 0;
  sockaddr_in  m_lastFrom{};
  sockaddr_in  m_to{};
  char         m_buf[512]{};
  int          m_pending = 0;
  char         m_outBuf[512]{};
  size_t       m_outLen = 0;
};

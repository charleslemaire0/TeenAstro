/*
 * Win32Transport.cpp - Win32 Serial and TCP implementations
 *
 * Copyright (C) 2024 TeenAstro by Charles Lemaire
 * GNU General Public License v3
 */
#include "Win32Transport.h"
#include <cstring>

#pragma comment(lib, "ws2_32.lib")

// =============================================================================
//  Win32SerialTransport
// =============================================================================

Win32SerialTransport::Win32SerialTransport()
  : m_handle(INVALID_HANDLE_VALUE), m_timeoutMs(DEFAULT_TIMEOUT)
{
}

Win32SerialTransport::~Win32SerialTransport()
{
  close();
}

bool Win32SerialTransport::open(const char* port, int baud)
{
  if (m_handle != INVALID_HANDLE_VALUE)
    close();
  std::string path = "\\\\.\\";
  path += port;
  m_handle = CreateFileA(path.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL,
                        OPEN_EXISTING, 0, NULL);
  if (m_handle == INVALID_HANDLE_VALUE)
    return false;
  DCB dcb = { 0 };
  dcb.DCBlength = sizeof(dcb);
  if (!GetCommState(m_handle, &dcb))
  {
    CloseHandle(m_handle);
    m_handle = INVALID_HANDLE_VALUE;
    return false;
  }
  dcb.BaudRate = baud;
  dcb.ByteSize = 8;
  dcb.Parity = NOPARITY;
  dcb.StopBits = ONESTOPBIT;
  if (!SetCommState(m_handle, &dcb))
  {
    CloseHandle(m_handle);
    m_handle = INVALID_HANDLE_VALUE;
    return false;
  }
  COMMTIMEOUTS to = { 0 };
  to.ReadIntervalTimeout = 50;
  to.ReadTotalTimeoutMultiplier = 10;
  to.ReadTotalTimeoutConstant = (DWORD)m_timeoutMs;
  to.WriteTotalTimeoutMultiplier = 10;
  to.WriteTotalTimeoutConstant = (DWORD)m_timeoutMs;
  SetCommTimeouts(m_handle, &to);
  PurgeComm(m_handle, PURGE_RXCLEAR | PURGE_TXCLEAR);
  return true;
}

void Win32SerialTransport::close()
{
  if (m_handle != INVALID_HANDLE_VALUE)
  {
    CloseHandle(m_handle);
    m_handle = INVALID_HANDLE_VALUE;
  }
}

void Win32SerialTransport::flush()
{
  if (m_handle == INVALID_HANDLE_VALUE) return;
  PurgeComm(m_handle, PURGE_RXCLEAR | PURGE_TXCLEAR);
}

size_t Win32SerialTransport::write(const uint8_t* buf, size_t len)
{
  if (m_handle == INVALID_HANDLE_VALUE || !buf) return 0;
  DWORD written = 0;
  if (WriteFile(m_handle, buf, (DWORD)len, &written, NULL))
    return (size_t)written;
  return 0;
}

int Win32SerialTransport::available()
{
  if (m_handle == INVALID_HANDLE_VALUE) return 0;
  COMSTAT cs = { 0 };
  DWORD err = 0;
  if (ClearCommError(m_handle, &err, &cs))
    return (int)cs.cbInQue;
  return 0;
}

int Win32SerialTransport::read()
{
  if (m_handle == INVALID_HANDLE_VALUE) return -1;
  uint8_t b;
  DWORD readCount = 0;
  if (ReadFile(m_handle, &b, 1, &readCount, NULL) && readCount == 1)
    return (int)b;
  return -1;
}

void Win32SerialTransport::setTimeout(unsigned long ms)
{
  m_timeoutMs = ms;
  if (m_handle != INVALID_HANDLE_VALUE)
  {
    COMMTIMEOUTS to = { 0 };
    GetCommTimeouts(m_handle, &to);
    to.ReadTotalTimeoutConstant = (DWORD)ms;
    to.WriteTotalTimeoutConstant = (DWORD)ms;
    SetCommTimeouts(m_handle, &to);
  }
}

// =============================================================================
//  WinSockTransport
// =============================================================================

WinSockTransport::WinSockTransport()
  : m_socket(INVALID_SOCKET), m_timeoutMs(5000)
{
  WSADATA wsa = { 0 };
  WSAStartup(MAKEWORD(2, 2), &wsa);
}

WinSockTransport::~WinSockTransport()
{
  close();
  WSACleanup();
}

bool WinSockTransport::open(const char* ip, int port)
{
  if (m_socket != INVALID_SOCKET)
    close();
  m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (m_socket == INVALID_SOCKET)
    return false;
  sockaddr_in addr = { 0 };
  addr.sin_family = AF_INET;
  addr.sin_port = htons((u_short)port);
  addr.sin_addr.s_addr = inet_addr(ip);
  if (connect(m_socket, (sockaddr*)&addr, sizeof(addr)) != 0)
  {
    closesocket(m_socket);
    m_socket = INVALID_SOCKET;
    return false;
  }
  u_long nonblock = 0;
  ioctlsocket(m_socket, FIONBIO, &nonblock);
  DWORD to = (DWORD)m_timeoutMs;
  setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&to, sizeof(to));
  setsockopt(m_socket, SOL_SOCKET, SO_SNDTIMEO, (const char*)&to, sizeof(to));
  m_recvBuf.clear();
  return true;
}

void WinSockTransport::close()
{
  if (m_socket != INVALID_SOCKET)
  {
    closesocket(m_socket);
    m_socket = INVALID_SOCKET;
  }
  m_recvBuf.clear();
}

void WinSockTransport::flush()
{
  m_recvBuf.clear();
}

size_t WinSockTransport::write(const uint8_t* buf, size_t len)
{
  if (m_socket == INVALID_SOCKET || !buf) return 0;
  int sent = send(m_socket, (const char*)buf, (int)len, 0);
  return sent > 0 ? (size_t)sent : 0;
}

int WinSockTransport::available()
{
  if (m_socket == INVALID_SOCKET) return 0;
  u_long n = 0;
  if (ioctlsocket(m_socket, FIONREAD, &n) == 0)
    return (int)n;
  return 0;
}

int WinSockTransport::read()
{
  if (m_socket == INVALID_SOCKET) return -1;
  if (!m_recvBuf.empty())
  {
    int b = (unsigned char)m_recvBuf[0];
    m_recvBuf.erase(0, 1);
    return b;
  }
  char c;
  int r = recv(m_socket, &c, 1, 0);
  if (r == 1)
    return (unsigned char)c;
  return -1;
}

void WinSockTransport::setTimeout(unsigned long ms)
{
  m_timeoutMs = ms;
  if (m_socket != INVALID_SOCKET)
  {
    DWORD to = (DWORD)ms;
    setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&to, sizeof(to));
  }
}

/*
 * Win32Transport.h - Win32 Serial and TCP implementations of IX200Transport
 *
 * Copyright (C) 2024 TeenAstro by Charles Lemaire
 * GNU General Public License v3
 */
#pragma once

#include <winsock2.h>
#include <windows.h>
#include "IX200Transport.h"
#include <string>

/// Win32 Serial port implementation of IX200Transport
class Win32SerialTransport : public IX200Transport
{
public:
  Win32SerialTransport();
  ~Win32SerialTransport();

  bool open(const char* port, int baud = 57600);
  void close();
  bool isOpen() const { return m_handle != INVALID_HANDLE_VALUE; }

  void flush() override;
  size_t write(const uint8_t* buf, size_t len) override;
  int available() override;
  int read() override;
  void setTimeout(unsigned long ms) override;

private:
  HANDLE m_handle;
  unsigned long m_timeoutMs;
  static const DWORD DEFAULT_TIMEOUT = 5000;
};

/// Win32 TCP socket implementation of IX200Transport
class WinSockTransport : public IX200Transport
{
public:
  WinSockTransport();
  ~WinSockTransport();

  bool open(const char* ip, int port);
  void close();
  bool isOpen() const { return m_socket != INVALID_SOCKET; }

  void flush() override;
  size_t write(const uint8_t* buf, size_t len) override;
  int available() override;
  int read() override;
  void setTimeout(unsigned long ms) override;

private:
  SOCKET m_socket;
  unsigned long m_timeoutMs;
  std::string m_recvBuf;
};

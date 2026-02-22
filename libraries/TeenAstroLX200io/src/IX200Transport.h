/*
 * IX200Transport.h - Abstract transport for LX200 protocol
 *
 * Platform-agnostic interface for send/receive. Arduino uses StreamTransport
 * (wraps Stream); Windows ASCOM DLL uses Win32SerialTransport/WinSockTransport.
 *
 * Copyright (C) 2024 TeenAstro by Charles Lemaire
 * GNU General Public License v3
 */
#pragma once

#include <stddef.h>
#include <stdint.h>

/// Abstract transport for LX200 serial/TCP communication.
/// Implementations: StreamTransport (Arduino), Win32Transport (Windows DLL).
class IX200Transport
{
public:
  virtual ~IX200Transport() {}

  /// Flush output and discard any buffered input.
  virtual void flush() = 0;

  /// Write bytes. Returns number written.
  virtual size_t write(const uint8_t* buf, size_t len) = 0;

  /// Number of bytes available to read (non-blocking).
  virtual int available() = 0;

  /// Read one byte. Returns -1 on timeout or no data.
  virtual int read() = 0;

  /// Set timeout for blocking read (milliseconds).
  virtual void setTimeout(unsigned long ms) = 0;
};

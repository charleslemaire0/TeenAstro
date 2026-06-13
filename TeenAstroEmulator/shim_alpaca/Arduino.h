/*
 * Arduino.h - native shim for the verify_alpaca test harness.
 *
 * Provides just enough of the Arduino API that:
 *   - LX200Client (compiled with -DTEENASTRO_NATIVE_BUILD) gets millis()
 *   - TeenAstroAlpaca + AlpacaTelescope + AlpacaResponse get the Arduino
 *     `String`, `F()`, and `Stream` they expect.
 *
 * NOTE: This shim is NOT used by the firmware build — only by
 *       TeenAstroEmulator/src/verify_alpaca_main.cpp.
 */
#pragma once

#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <cmath>
// STL pulled in first so std::min / std::max templates parse before the
// Arduino-style `min` / `max` macros are introduced below.
// chrono/thread also need to be parsed BEFORE the macros since they
// declare templates (e.g. std::chrono::duration) that contain `<` tokens
// which would otherwise be misparsed by the preprocessor.
#include <algorithm>
#include <string>
#include <chrono>
#include <thread>

#ifdef _WIN32
  // Win32Transport.h includes winsock2.h; that header MUST be included
  // before windows.h or we get a warning + broken socket types.
  #ifndef WIN32_LEAN_AND_MEAN
  #define WIN32_LEAN_AND_MEAN
  #endif
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #include <windows.h>
#endif

typedef uint8_t  byte;
typedef uint16_t word;

#ifndef PROGMEM
#define PROGMEM
#endif

// ---------------------------------------------------------------------------
//  millis() / delay()
// ---------------------------------------------------------------------------

inline unsigned long millis()
{
#ifdef _WIN32
  return (unsigned long)GetTickCount();
#else
  using namespace std::chrono;
  static auto t0 = steady_clock::now();
  return (unsigned long)duration_cast<milliseconds>(steady_clock::now() - t0).count();
#endif
}

inline void delay(unsigned long ms)
{
#ifdef _WIN32
  Sleep((DWORD)ms);
#else
  std::this_thread::sleep_for(std::chrono::milliseconds(ms));
#endif
}

inline void yield() {}

// ---------------------------------------------------------------------------
//  __FlashStringHelper / F() / PGM stubs
// ---------------------------------------------------------------------------

class __FlashStringHelper;

#define F(x)    (reinterpret_cast<const __FlashStringHelper*>(x))
#define FPSTR(p)(reinterpret_cast<const __FlashStringHelper*>(p))

#define PSTR(x) (x)
#define pgm_read_byte(addr) (*(const uint8_t*)(addr))
#define strcpy_P  strcpy
#define strcmp_P  strcmp
#define strlen_P  strlen
#define memcpy_P  memcpy

// ---------------------------------------------------------------------------
//  Float to string (avr-libc helper)
// ---------------------------------------------------------------------------

inline char* dtostrf(double v, signed char width, unsigned char prec, char* out)
{
  char fmt[16];
  std::snprintf(fmt, sizeof(fmt), "%%%d.%df", (int)width, (int)prec);
  std::sprintf(out, fmt, v);
  return out;
}

// ---------------------------------------------------------------------------
//  Min / max
// ---------------------------------------------------------------------------

#ifndef max
#define max(a,b) (((a)>(b))?(a):(b))
#endif
#ifndef min
#define min(a,b) (((a)<(b))?(a):(b))
#endif

// ---------------------------------------------------------------------------
//  Print + Stream (LX200Client uses Stream when not in native build, but
//  some headers we pull in still reference Stream as a base class).
// ---------------------------------------------------------------------------

class Print
{
public:
  virtual ~Print() = default;
  virtual size_t write(uint8_t b) = 0;
  size_t write(const uint8_t* buf, size_t n) { for (size_t i=0;i<n;++i) write(buf[i]); return n; }
};

class Stream : public Print
{
public:
  virtual int  available() = 0;
  virtual int  read()      = 0;
  virtual int  peek()      = 0;
  virtual void flush()     {}
  void setTimeout(unsigned long ms) { m_timeout = ms; }
protected:
  unsigned long m_timeout = 1000;
};

// ---------------------------------------------------------------------------
//  WString  (full enough for AlpacaTelescope + AlpacaResponse)
// ---------------------------------------------------------------------------
#include "WString.h"

// ---------------------------------------------------------------------------
//  IPAddress (used by ESP8266WiFi.h shim)
// ---------------------------------------------------------------------------
class IPAddress
{
public:
  uint32_t v = 0;
  IPAddress() = default;
  IPAddress(uint32_t a) : v(a) {}
  IPAddress(uint8_t a, uint8_t b, uint8_t c, uint8_t d)
    : v(((uint32_t)a) | ((uint32_t)b<<8) | ((uint32_t)c<<16) | ((uint32_t)d<<24)) {}
  operator uint32_t() const { return v; }
};

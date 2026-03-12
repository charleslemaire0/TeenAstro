/*
 * Arduino.h - Minimal stub for TeenAstroAscomNative Windows DLL build.
 * Provides millis() and Stream so LX200Client/TeenAstroMountStatus compile.
 */
#pragma once

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <windows.h>

typedef uint8_t byte;

#ifndef max
#define max(a,b) (((a)>(b))?(a):(b))
#endif
#ifndef min
#define min(a,b) (((a)<(b))?(a):(b))
#endif

inline void delay(unsigned long ms) { (void)ms; }

inline unsigned long millis()
{
  return (unsigned long)GetTickCount();
}

class Stream {
public:
  virtual ~Stream() {}
  virtual int available() = 0;
  virtual int read() = 0;
  virtual int peek() = 0;
  virtual size_t write(uint8_t b) = 0;
  virtual void flush() {}
  void setTimeout(unsigned long ms) { (void)ms; }
};

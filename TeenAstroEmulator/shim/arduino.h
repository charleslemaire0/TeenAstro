/*
 * arduino.h / Arduino.h  -  Arduino compatibility shim for native tests.
 *
 * On Windows (NTFS is case-insensitive) this single file satisfies both
 * #include "arduino.h" and #include "Arduino.h".
 *
 * Provides enough of the Arduino/Teensy API so that the TeenAstro MainUnit
 * compiles on a desktop (native) platform for testing.
 */
#pragma once

#include <cstdio>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <chrono>
#include <thread>
#include <algorithm>
using std::isnan;

typedef uint8_t byte;
typedef unsigned short ushort;

#ifndef max
#define max(a,b) (((a)>(b))?(a):(b))
#endif
#ifndef min
#define min(a,b) (((a)<(b))?(a):(b))
#endif

/* ------------------------------------------------------------------ */
/*  Constants that Arduino.h normally provides                         */
/* ------------------------------------------------------------------ */
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#ifndef M_PI_2
#define M_PI_2 1.57079632679489661923
#endif
#ifndef M_PI_4
#define M_PI_4 0.78539816339744830962
#endif
#ifndef PI
#define PI M_PI
#endif
#ifndef HALF_PI
#define HALF_PI M_PI_2
#endif
#ifndef TWO_PI
#define TWO_PI (2.0 * M_PI)
#endif
#ifndef DEG_TO_RAD
#define DEG_TO_RAD (M_PI / 180.0)
#endif
#ifndef RAD_TO_DEG
#define RAD_TO_DEG (180.0 / M_PI)
#endif
#ifndef RAD_TO_HOUR
#define RAD_TO_HOUR (12.0 / M_PI)
#endif
#ifndef HOUR_TO_RAD
#define HOUR_TO_RAD (M_PI / 12.0)
#endif

/* ------------------------------------------------------------------ */
/*  GPIO constants                                                     */
/* ------------------------------------------------------------------ */
#ifndef HIGH
#define HIGH 1
#endif
#ifndef LOW
#define LOW 0
#endif
#ifndef INPUT
#define INPUT 0
#endif
#ifndef OUTPUT
#define OUTPUT 1
#endif
#ifndef INPUT_PULLUP
#define INPUT_PULLUP 2
#endif

/* ------------------------------------------------------------------ */
/*  AVR/ESP compatibility macros                                       */
/* ------------------------------------------------------------------ */
#ifndef PROGMEM
#define PROGMEM
#endif
#ifndef pgm_read_byte
#define pgm_read_byte(addr) (*(const uint8_t *)(addr))
#endif
#ifndef pgm_read_word
#define pgm_read_word(addr) (*(const uint16_t *)(addr))
#endif
#ifndef pgm_read_dword
#define pgm_read_dword(addr) (*(const uint32_t *)(addr))
#endif
#ifndef pgm_read_float
#define pgm_read_float(addr) (*(const float *)(addr))
#endif
#ifndef pgm_read_ptr
#define pgm_read_ptr(addr) (*(const void* const*)(addr))
#endif
#ifndef strcpy_P
#define strcpy_P strcpy
#endif
#ifndef strcmp_P
#define strcmp_P strcmp
#endif
#ifndef strlen_P
#define strlen_P strlen
#endif
#ifndef memcpy_P
#define memcpy_P memcpy
#endif
#ifndef F
#define F(x) (x)
#endif
#ifndef PSTR
#define PSTR(x) (x)
#endif

/* Undefine Windows CONST to avoid clash with ephemeris VSOP87.hpp */
#ifdef CONST
#undef CONST
#endif

/* ------------------------------------------------------------------ */
/*  Bit manipulation macros                                            */
/* ------------------------------------------------------------------ */
#ifndef bitRead
#define bitRead(value, bit) (((value) >> (bit)) & 0x01)
#endif
#ifndef bitWrite
#define bitWrite(value, bit, bitvalue) \
  ((bitvalue) ? ((value) |= (1UL << (bit))) : ((value) &= ~(1UL << (bit))))
#endif
#ifndef bitSet
#define bitSet(value, bit) ((value) |= (1UL << (bit)))
#endif
#ifndef bitClear
#define bitClear(value, bit) ((value) &= ~(1UL << (bit)))
#endif
#ifndef bit
#define bit(b) (1UL << (b))
#endif

/* ------------------------------------------------------------------ */
/*  Time -- two modes: simulated (tests) or real-time (emulator)       */
/* ------------------------------------------------------------------ */
namespace sim {
  extern unsigned long g_micros;
  extern unsigned long g_millis;
  extern bool          g_realtime;  // when true, use wall-clock time

  inline void enableRealtime() { g_realtime = true; }
}

inline unsigned long micros() {
    if (sim::g_realtime) {
        static auto epoch = std::chrono::steady_clock::now();
        auto now = std::chrono::steady_clock::now();
        return (unsigned long)std::chrono::duration_cast<std::chrono::microseconds>(now - epoch).count();
    }
    return sim::g_micros;
}
inline unsigned long millis() {
    if (sim::g_realtime) return micros() / 1000UL;
    return sim::g_millis;
}

/* ------------------------------------------------------------------ */
/*  Interrupt stubs (single-threaded emulation)                        */
/* ------------------------------------------------------------------ */
inline void cli() {}
inline void sei() {}
inline void interrupts() {}
inline void noInterrupts() {}

/* ------------------------------------------------------------------ */
/*  GPIO stubs                                                         */
/* ------------------------------------------------------------------ */
inline void pinMode(uint8_t, uint8_t) {}
inline void digitalWrite(uint8_t, uint8_t) {}
inline int  digitalRead(uint8_t) { return LOW; }
inline void digitalWriteFast(uint8_t, uint8_t) {}
inline void analogWrite(uint8_t, int) {}
inline int  analogRead(uint8_t) { return 300; }  // selects SSD1306 in SHC

/* ------------------------------------------------------------------ */
/*  Delay                                                              */
/* ------------------------------------------------------------------ */
#ifdef EMU_SHC
/*  Forward-declared in u8g2_sdl2.h -- we only need the pointer here. */
struct U8G2_EXT_SDL2;
extern U8G2_EXT_SDL2* g_sdlDisplay;
void _emu_shc_blit();   /* defined in shc_emu.cpp */

inline void delay(unsigned long ms) {
    if (ms == 0) return;
    unsigned long deadline = millis() + ms;
    while (millis() < deadline) {
        _emu_shc_blit();
        /* small granularity sleep to keep the window responsive */
        unsigned long sl = (ms < 16) ? ms : 16;
        std::this_thread::sleep_for(std::chrono::milliseconds(sl));
    }
}
#elif defined(EMU_MAINUNIT)
inline void delay(unsigned long ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}
#else
inline void delay(unsigned long) {}
#endif
inline void delayMicroseconds(unsigned int) {}

/* ------------------------------------------------------------------ */
/*  dtostrf (float-to-string, not in standard MinGW libc)              */
/* ------------------------------------------------------------------ */
inline char* dtostrf(double val, signed char width, unsigned char prec, char* buf) {
    char fmt[16];
    sprintf(fmt, "%%%d.%df", width, prec);
    sprintf(buf, fmt, val);
    return buf;
}

/* ------------------------------------------------------------------ */
/*  Arduino random()                                                    */
/* ------------------------------------------------------------------ */
inline long random(long max) { return rand() % max; }
inline long random(long min, long max) {
    if (min >= max) return min;
    return min + (rand() % (max - min));
}

/* ------------------------------------------------------------------ */
/*  Teensy NVIC / SCB stubs                                            */
/* ------------------------------------------------------------------ */
#ifndef SCB_SHPR3
static uint32_t SCB_SHPR3 = 0;
#endif
#define NVIC_SET_PRIORITY(irq, prio) ((void)0)
#define IRQ_PIT_CH0 0
#define IRQ_PIT_CH1 1
#define IRQ_PIT_CH2 2

/* ------------------------------------------------------------------ */
/*  Teensy board detection (none active for native)                    */
/* ------------------------------------------------------------------ */
#ifndef NATIVE_HAL_BUILD
#define NATIVE_HAL_BUILD
#endif

/* ------------------------------------------------------------------ */
/*  Stream base class (minimal Arduino-compatible interface)            */
/*  Inherits from Print (defined in Print.h) for U8G2 compatibility.   */
/* ------------------------------------------------------------------ */
#include "Print.h"

class Stream : public Print {
public:
    virtual ~Stream() {}

    virtual int available() = 0;
    virtual int read() = 0;
    virtual int peek() = 0;
    virtual size_t write(uint8_t b) override = 0;
    virtual void flush() {}

    void setTimeout(unsigned long ms) { m_timeout = ms; }
    unsigned long getTimeout() const { return m_timeout; }

protected:
    unsigned long m_timeout = 1000;
};

/* ------------------------------------------------------------------ */
/*  Stub Serial objects                                                */
/* ------------------------------------------------------------------ */
struct SerialStub : public Stream {
    void begin(unsigned long) {}
    void end() {}
    int available() override { return 0; }
    int read() override { return -1; }
    int peek() override { return -1; }
    size_t write(uint8_t) override { return 1; }
    void setRX(int) {}
    void setTX(int) {}
};

#ifndef SERIAL_STUBS_DEFINED
#define SERIAL_STUBS_DEFINED
static SerialStub Serial;
static SerialStub Serial1;
static SerialStub Serial2;
static SerialStub Serial3;
#endif

/* ------------------------------------------------------------------ */
/*  MISO / SPI pin constants                                           */
/* ------------------------------------------------------------------ */
#ifndef MISO
#define MISO 12
#endif
#ifndef MOSI
#define MOSI 11
#endif
#ifndef SCK
#define SCK 13
#endif

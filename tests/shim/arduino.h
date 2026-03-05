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
#include <algorithm>

typedef uint8_t byte;

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
/*  Simulated time (test harness can override)                         */
/* ------------------------------------------------------------------ */
namespace sim {
  extern unsigned long g_micros;
  extern unsigned long g_millis;
}

inline unsigned long micros() { return sim::g_micros; }
inline unsigned long millis() { return sim::g_millis; }

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

/* ------------------------------------------------------------------ */
/*  Delay stubs                                                        */
/* ------------------------------------------------------------------ */
inline void delay(unsigned long) {}
inline void delayMicroseconds(unsigned int) {}

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
/* ------------------------------------------------------------------ */
class Stream {
public:
    virtual ~Stream() {}

    virtual int available() = 0;
    virtual int read() = 0;
    virtual int peek() = 0;
    virtual size_t write(uint8_t b) = 0;
    virtual void flush() {}

    void setTimeout(unsigned long ms) { m_timeout = ms; }
    unsigned long getTimeout() const { return m_timeout; }

    size_t print(const char* s) {
        size_t n = 0;
        while (*s) { write((uint8_t)*s++); n++; }
        return n;
    }
    size_t print(int val) {
        char buf[16]; sprintf(buf, "%d", val); return print(buf);
    }
    size_t print(unsigned int val) {
        char buf[16]; sprintf(buf, "%u", val); return print(buf);
    }
    size_t print(long val) {
        char buf[24]; sprintf(buf, "%ld", val); return print(buf);
    }
    size_t print(unsigned long val) {
        char buf[24]; sprintf(buf, "%lu", val); return print(buf);
    }
    size_t print(double val) {
        char buf[24]; sprintf(buf, "%f", val); return print(buf);
    }
    size_t println(const char* s) {
        return print(s) + write('\n');
    }
    size_t println(int val) {
        return print(val) + write('\n');
    }
    size_t println() {
        return write('\n');
    }

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

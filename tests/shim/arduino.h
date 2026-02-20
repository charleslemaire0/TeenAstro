/*
 * arduino.h / Arduino.h  -  Minimal Arduino compatibility shim for native tests.
 *
 * On Windows (NTFS is case-insensitive) this single file satisfies both
 * #include "arduino.h" and #include "Arduino.h".
 *
 * Provides just enough of the Arduino API so that TeenAstro math
 * libraries compile on a desktop (native) platform.
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
#ifndef PI
#define PI M_PI
#endif
#ifndef DEG_TO_RAD
#define DEG_TO_RAD (M_PI / 180.0)
#endif
#ifndef RAD_TO_DEG
#define RAD_TO_DEG (180.0 / M_PI)
#endif

/* ------------------------------------------------------------------ */
/*  delay() stub (no-op for fast tests; link with real impl if needed) */
/* ------------------------------------------------------------------ */
inline void delay(unsigned long ms) { (void)ms; }

/* ------------------------------------------------------------------ */
/*  millis() stub using std::chrono                                    */
/* ------------------------------------------------------------------ */
inline unsigned long millis()
{
    static auto start = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    return (unsigned long)std::chrono::duration_cast<std::chrono::milliseconds>(
        now - start).count();
}

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
        char buf[16];
        sprintf(buf, "%d", val);
        return print(buf);
    }

    size_t print(unsigned int val) {
        char buf[16];
        sprintf(buf, "%u", val);
        return print(buf);
    }

    size_t print(long val) {
        char buf[24];
        sprintf(buf, "%ld", val);
        return print(buf);
    }

    size_t print(unsigned long val) {
        char buf[24];
        sprintf(buf, "%lu", val);
        return print(buf);
    }

    size_t print(double val) {
        char buf[24];
        sprintf(buf, "%f", val);
        return print(buf);
    }

    size_t println(const char* s) {
        size_t n = print(s);
        n += write('\n');
        return n;
    }

    size_t println(int val) {
        size_t n = print(val);
        n += write('\n');
        return n;
    }

protected:
    unsigned long m_timeout = 1000;
};

/* ------------------------------------------------------------------ */
/*  Stub Serial object (backward-compatible with existing tests)       */
/* ------------------------------------------------------------------ */
struct SerialStub : public Stream {
    void begin(unsigned long) {}
    int available() override { return 0; }
    int read() override { return -1; }
    int peek() override { return -1; }
    size_t write(uint8_t) override { return 1; }
};
static SerialStub Serial;

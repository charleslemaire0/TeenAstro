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
/*  Stub Serial object                                                 */
/*  (printV is gated by #ifdef DEBUG_COUT which we never define)       */
/* ------------------------------------------------------------------ */
struct SerialStub {
    void begin(unsigned long) {}
    void print(const char*) {}
    void println(const char*) {}
    void print(int) {}
    void println(int) {}
};
static SerialStub Serial;

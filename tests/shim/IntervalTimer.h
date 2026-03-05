/*
 * IntervalTimer.h - Stub for Teensy IntervalTimer (native tests).
 *
 * Stores callback and interval but does NOT auto-fire.
 * The test harness fires callbacks manually for deterministic control.
 */
#pragma once
#include <cstdint>

class IntervalTimer {
public:
    typedef void (*callback_t)();
    callback_t  cb_ = nullptr;
    float       interval_us_ = 0;
    int         prio_ = 128;

    bool begin(callback_t cb, float us) {
        cb_ = cb;
        interval_us_ = us;
        return true;
    }
    bool begin(callback_t cb, double us) { return begin(cb, (float)us); }
    void update(float us) { interval_us_ = us; }
    void update(double us) { interval_us_ = (float)us; }
    void end() { cb_ = nullptr; }
    void priority(int p) { prio_ = p; }

    void fire() { if (cb_) cb_(); }
};

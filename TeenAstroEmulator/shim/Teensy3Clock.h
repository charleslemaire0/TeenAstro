/*
 * Teensy3Clock.h - Stub for Teensy RTC (native tests).
 */
#pragma once
#include <cstdint>

class Teensy3ClockClass {
    unsigned long epoch_ = 1709600000UL;
public:
    unsigned long get() const { return epoch_; }
    void set(unsigned long t) { epoch_ = t; }
};

static Teensy3ClockClass Teensy3Clock;

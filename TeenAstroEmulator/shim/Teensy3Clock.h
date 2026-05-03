/*
 * Teensy3Clock.h - Stub for Teensy RTC (native tests).
 *
 * The hardware RTC advances with wall time; the previous stub used a fixed
 * epoch so GXAS UTC and sidereal math drifted relative to the host during
 * long ConformU runs.
 */
#pragma once
#include <chrono>
#include <cstdint>

class Teensy3ClockClass {
  unsigned long                         epoch_base_ = 1709600000UL;
  std::chrono::steady_clock::time_point wall_base_{};
  bool                                  have_wall_base_ = false;

public:
  unsigned long get() const
  {
    if (!have_wall_base_) return epoch_base_;
    auto dt = std::chrono::steady_clock::now() - wall_base_;
    auto sec = std::chrono::duration_cast<std::chrono::seconds>(dt).count();
    return epoch_base_ + (unsigned long)sec;
  }

  void set(unsigned long t)
  {
    epoch_base_     = t;
    wall_base_      = std::chrono::steady_clock::now();
    have_wall_base_ = true;
  }
};

static Teensy3ClockClass Teensy3Clock;

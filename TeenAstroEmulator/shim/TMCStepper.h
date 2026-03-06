/*
 * TMCStepper.h - Stub for TMCStepper library (native tests).
 * Provides the minimal API surface used by TeenAstroStepper's Driver class.
 */
#pragma once
#include <cstdint>

class TMC2130Stepper {
public:
    TMC2130Stepper(uint8_t) {}
    void reset() {}
    void push() {}
    void TPOWERDOWN(uint8_t) {}
    void tbl(uint8_t) {}
    void toff(uint8_t) {}
    void hstrt(uint8_t) {}
    void hend(uint8_t) {}
    void en_pwm_mode(bool) {}
    bool en_pwm_mode() { return false; }
    void pwm_autoscale(bool) {}
    void TPWMTHRS(uint32_t) {}
    void intpol(bool) {}
    void rms_current(uint16_t) {}
    void microsteps(uint16_t) {}
};

class TMC5160Stepper {
public:
    TMC5160Stepper(uint8_t) {}
    void reset() {}
    void push() {}
    void TPOWERDOWN(uint8_t) {}
    void tbl(uint8_t) {}
    void toff(uint8_t) {}
    void hstrt(uint8_t) {}
    void hend(uint8_t) {}
    void en_pwm_mode(bool) {}
    bool en_pwm_mode() { return false; }
    void pwm_autoscale(bool) {}
    void TPWMTHRS(uint32_t) {}
    void intpol(bool) {}
    void rms_current(uint16_t) {}
    void microsteps(uint16_t) {}
};

class TMC2660Stepper {
public:
    TMC2660Stepper(uint8_t, float) {}
    void push() {}
    void tbl(uint8_t) {}
    void toff(uint8_t) {}
    void hstrt(uint8_t) {}
    void hend(uint8_t) {}
    void intpol(bool) {}
    void rms_current(uint16_t) {}
    void microsteps(uint16_t) {}
};

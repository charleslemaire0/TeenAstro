/*
 * TMC26XStepper.h - Stub for TMC26X stepper driver (native tests).
 */
#pragma once
#include <cstdint>

#ifndef COOL_STEP_HALF_CS_LIMIT
#define COOL_STEP_HALF_CS_LIMIT 0
#endif
#ifndef COOL_STEP_QUARTER_CS_LIMIT
#define COOL_STEP_QUARTER_CS_LIMIT 1
#endif

class TMC26XStepper {
public:
    TMC26XStepper(int, int, int, int, int r = 150) { (void)r; }
    void start() {}
    void stop() {}
    void setMicrosteps(int) {}
    void setCurrent(unsigned int) {}
    void setStallGuardThreshold(char, char) {}
    void setCoolStepConfiguration(unsigned int, unsigned int, unsigned int, unsigned int, unsigned int) {}
    void setCoolStepEnabled(bool) {}
    bool isCoolStepEnabled() { return false; }
    void setEnabled(bool) {}
    unsigned int getCurrent() { return 0; }
    unsigned int getCurrentCurrent() { return 0; }
    int getStallGuardReading() { return 0; }
    int getCurrentStallGuardReading() { return 0; }
    bool isStallGuardOverThreshold() { return false; }
    unsigned int getStallGuardFilter() { return 0; }
    void setSpreadCycleChopper(int, int, int, int, int) {}
    void setRandomOffTime(int) {}
    unsigned long getDriverStatus() { return 0; }
    int getResolution() { return 256; }
    bool isCurrentScalingHalfed() { return false; }
    unsigned int getOverTemperature() { return 0; }
    bool isShortToGroundA() { return false; }
    bool isShortToGroundB() { return false; }
    bool isOpenLoadA() { return false; }
    bool isOpenLoadB() { return false; }
    bool isStandStill() { return false; }
    unsigned int getReadoutValue() { return 0; }
};

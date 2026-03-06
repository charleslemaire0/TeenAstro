/*
 * OneButton.h -- Minimal stub for the OneButton library (SHC emulator).
 * The SDL2 Pad replacement doesn't use OneButton; this stub just allows
 * TeenAstroPad.h to compile.
 */
#pragma once
#include "arduino.h"

extern "C" {
typedef void (*callbackFunction)(void);
typedef void (*parameterizedCallbackFunction)(void *);
}

class OneButton {
public:
    OneButton() {}
    explicit OneButton(const int, const bool = true, const bool = true) {}
    void attachClick(callbackFunction) {}
    void attachDoubleClick(callbackFunction) {}
    void attachLongPressStart(callbackFunction) {}
    void attachLongPressStop(callbackFunction) {}
    void attachDuringLongPress(callbackFunction) {}
    void setClickMs(int) {}
    void setDebounceMs(int) {}
    void setPressMs(int) {}
    void tick() {}
    void reset() {}
};

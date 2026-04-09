#pragma once

// PCB / hardware revision code (e.g. 240 = :GVB# board id). NOT firmware semver.
// When bumping a release, change FirmwareNumber (and SHC/app/ASCOM/etc.) only — never
// increment or "sync" this define to match patch digits; that was a past mistake.
#ifndef VERSION
#define VERSION 240
#endif

// firmware info, these are returned by the ":GV?#" commands
#define FirmwareDate    __DATE__
// Main unit only — bump from previous MainUnit semver (+1 step). Do not copy this
// string to WiFi server, Flutter, ASCOM, etc.; those have their own counters.
#define FirmwareNumber  "1.6.5"
#define FirmwareName    "TeenAstro"
#define FirmwareTime    "00:00:00"
// forces initialialization of a host of settings in XEEPROM.
#define initKey     152682  // unique identifier for the current initialization format, do not change

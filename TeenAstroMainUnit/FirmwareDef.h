#pragma once

#ifndef VERSION
#define VERSION 250
#endif

// firmware info, these are returned by the ":GV?#" commands
#define FirmwareDate    __DATE__
#define FirmwareNumber  "1.5.3"
#define FirmwareName    "TeenAstro"
#define FirmwareTime    "00:00:00"
// forces initialialization of a host of settings in XEEPROM.
#define initKey     152682  // unique identifier for the current initialization format, do not change

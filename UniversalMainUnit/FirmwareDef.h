#pragma once

#ifndef VERSION
#define VERSION 300
#endif

// firmware info, these are returned by the ":GV?#" commands
#define FirmwareDate    __DATE__
#define FirmwareNumber  "3.0.0"
#define FirmwareName    "TeenAstro"
#define FirmwareTime    __TIME__
// forces initialialization of a host of settings in XEEPROM. OnStep does this automatically, most likely, you will want to leave this alone
#define initKey     000000001                       // unique identifier for the current initialization format, do not change

#pragma once

#ifndef VERSION
#define VERSION 300
#endif

// firmware info, these are returned by the ":GV?#" commands
#define FirmwareDate    __DATE__
#define FirmwareNumber  "1.4.4"
#define FirmwareName    "TeenAstroUniversal"
#define FirmwareTime    __TIME__
// forces initialialization of a host of settings in XEEPROM. OnStep does this automatically, most likely, you will want to leave this alone
#define initKey     0x12345678            // unique identifier for the current initialization format

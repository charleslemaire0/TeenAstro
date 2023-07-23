#pragma once

#ifndef VERSION
#define VERSION 300
#endif

// firmware info, these are returned by the ":GV?#" commands
#define FirmwareDate    __DATE__
#define FirmwareNumber  "1.4"   // to keep compatibility with SHC
#define FirmwareName    "TeenAstro"
#define FirmwareSubName "UniversalMainUnit"
#define FirmwareTime    __TIME__
// forces initialization of a host of settings in XEEPROM. 
#define initKey     0x12345678            // unique identifier for the current initialization format

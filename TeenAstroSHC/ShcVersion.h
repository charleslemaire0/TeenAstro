#pragma once

// Identity of the hand-controller firmware. The web server uses this when it
// is compiled into the same image as the SHC display.
#ifndef Product
#define Product "Teenastro SHC"
#endif
#define SHCFirmwareDate          __DATE__
#define SHCFirmwareTime          __TIME__
#define SHCFirmwareVersionMajor  "1"
#define SHCFirmwareVersionMinor  "6"
#define SHCFirmwareVersionPatch  "18"

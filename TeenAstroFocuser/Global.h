// Global.h

#ifndef _GLOBAL_h
#define _GLOBAL_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif
#define PROJECT "TeenAstro Focuser"
#define Version "1.0.0"

//uncomment the version you are using
//#define VERSION220
#define VERSION220

#ifdef VERSION230
// RTC
#define BOARDINFO "2.3.0"
#define kCePin 4  // Chip Enable
#define kIoPin 3 // Input/Output
#define kSclkPin 2  // Serial Clock
//TMC2130
#define EnablePin 5
#define StepPin 7
#define DirPin 6
#define CSPin 10
#define FocuserRX 0
#define FocuserTX 1
#define LEDPin 23 
#endif // VERSION23

#ifdef VERSION220
// RTC
#define BOARDINFO "2.2.0"
#define kCePin 4  // Chip Enable
#define kIoPin 3 // Input/Output
#define kSclkPin 2  // Serial Clock
//TMC2130
#define EnablePin 7
#define StepPin 5
#define DirPin 6
#define CSPin 10
#define FocuserRX 26
#define FocuserTX 31
#define LEDPin 33 
#endif


#endif


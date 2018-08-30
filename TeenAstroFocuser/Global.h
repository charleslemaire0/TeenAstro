// Global.h

#ifndef _GLOBAL_h
#define _GLOBAL_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif
#define PROJECT "Simple Arduino Focuser"
#define Version 001
#define BOARDINFO "Teensy 3.2"
//uncomment the version you are using
//#define VERSION22
#define VERSION23

#ifdef VERSION23
#define FocuserRX 0
#define FocuserTX 1
#endif // VERSION23
#ifdef VERSION22
#define FocuserRX 26
#define FocuserTX 31
#endif

#endif


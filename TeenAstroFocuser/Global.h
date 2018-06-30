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


#endif


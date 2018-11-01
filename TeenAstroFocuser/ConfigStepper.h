// ConfigStepper.h
#include "Command.h"
#ifndef _CONFIGSTEPPER_h
#define _CONFIGSTEPPER_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif
#include "DS1302.h"

// give the motor control pins names:
#define StepPin 5
#define DirPin 6

// Button
//#define ButtonAPin 8
//#define ButtonBPin 7
// RTC
#define kCePin 4  // Chip Enable
#define kIoPin 3 // Input/Output
#define kSclkPin 2  // Serial Clock
#define micro 16
#define updaterate 1

int mdirOUTOld = 0;
int mdirINOld = 0;

unsigned int currSpeed;
unsigned long position;
bool halt = false;
double time_acc = 0;
DS1302 *rtc = NULL;
SerCom serCom0(Serial);
#ifdef VERSION22
SerCom serComSHC(Serial2);
#endif
#ifdef VERSION23
SerCom serComSHC(Serial1);
#endif
#endif


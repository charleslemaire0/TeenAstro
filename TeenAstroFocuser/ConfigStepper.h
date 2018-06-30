// ConfigStepper.h

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
#define ButtonAPin 8
#define ButtonBPin 7
// RTC
#define kCePin 4  // Chip Enable
#define kIoPin 3 // Input/Output
#define kSclkPin 2  // Serial Clock
#define micro 16
#define updaterate 100

int mdirAOld = 0;
int mdirBOld = 0;

int currSpeed;
int lastCurrSpeed;
unsigned int position;
bool halt = false;
unsigned int lastposition;
double time_acc = 0;
long lastEvent = millis() / updaterate;
long event = millis() / updaterate;
DS1302 *rtc = NULL;
void iniStepper();
#endif


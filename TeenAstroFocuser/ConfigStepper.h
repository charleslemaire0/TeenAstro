// ConfigStepper.h
#include "Command.h"
#ifndef _CONFIGSTEPPER_h
#define _CONFIGSTEPPER_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif
#include <AccelStepper.h>
#include <TMC2130Stepper.h>
#include "Command.h"

#include "DS1302.h"


TMC2130Stepper driver = TMC2130Stepper(EnablePin, DirPin, StepPin, CSPin);
AccelStepper stepper = AccelStepper(stepper.DRIVER, StepPin, DirPin);

long oldposition = 0;
int mdirOUTOld = 0;
int mdirINOld = 0;

bool halt = false;

DS1302 *rtc = NULL;
SerCom serCom0(Serial);
#ifdef VERSION22
SerCom serComSHC(Serial2);
#endif
#ifdef VERSION23

SerCom serComSHC(Serial1);
#endif
#endif


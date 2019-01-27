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
#include <TeenAstroStepper.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "Command.h"
#include "DS1302.h"

Motor teenAstroStepper;
AccelStepper stepper = AccelStepper(1, StepPin, DirPin);
// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
float lastTemp = -999;
OneWire oneWire(TempPin);
// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature tempSensors(&oneWire);
// arrays to hold device address
DeviceAddress insideThermometer;

long oldposition = 0;
int mdirOUTOld = 0;
int mdirINOld = 0;

bool halt = false;

DS1302 *rtc = NULL;
SerCom serCom0(Serial);
#ifdef VERSION220
SerCom serComSHC(Serial2);
#endif
#if defined(VERSION230) || defined(VERSION240)
SerCom serComSHC(Serial1);
#endif
#endif

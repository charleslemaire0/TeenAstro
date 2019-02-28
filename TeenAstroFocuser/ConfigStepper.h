// ConfigStepper.h
#include "Command.h"
#ifndef _CONFIGSTEPPER_h
#define _CONFIGSTEPPER_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "arduino.h"
#else
#include "WProgram.h"
#endif

#include <TeenAstroStepper.h>
#include <TeensyStep.h>


#include <OneWire.h>
#include <DallasTemperature.h>
#include "Command.h"
#include "DS1302.h"
#define AccFact 1000

Motor teenAstroStepper;
Stepper stepper(_StepPin, _DirPin);

StepControl controller;
RotateControl rotateController;
// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
float lastTemp = -99.9999;
int lastTempTick = millis();
OneWire oneWire(TempPin);
// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature tempSensors(&oneWire);
// arrays to hold device address
DeviceAddress insideThermometer;

long oldposition = 0;

DS1302 *rtc = NULL;
SerCom serCom0(Serial);
#ifdef VERSION220
SerCom serComSHC(Serial2);
#endif
#if defined(VERSION230) || defined(VERSION240)
SerCom serComSHC(Serial1);
#endif
#endif

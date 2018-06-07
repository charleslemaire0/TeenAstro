#pragma once


#include <TMC26XStepper.h>
TMC26XStepper *tmc26XStepper1;
TMC26XStepper *tmc26XStepper2;
#define Axis1StepPin    2                   // Pin 2 (Step)
#define Axis1DirPin     3                   // Pin 3 (Dir)
#define Axis1SGPin      4                   // Pin 4 (SG)
#define Axis1CSPin      5                   // Pin 5 (CS)

#define Axis2StepPin    6                   // Pin 6 (Step)
#define Axis2DirPin     7                   // Pin 7 (Dir)
#define Axis2SGPin      8                   // Pin 8 (SG)
#define Axis2CSPin      9                   // Pin 9 (CS)
// Pin 11 12 13 are used!! for SPI

#define PinFocusA 21
#define PinFocusB 22
// ST4 interface
//#define ST4RAw  24                          // Pin 24 ST4 RA- West
//#define ST4DEs  25                          // Pin 25 ST4 DE- South
//#define ST4DEn  26                          // Pin 26 ST4 DE+ North
//#define ST4RAe  27                          // Pin 27 ST4 RA+ East
#define LEDPin 33 


#pragma once
//uncomment the version you are using
//#define VERSION22
#define VERSION23


#include <TMC26XStepper.h>
TMC26XStepper *tmc26XStepper1;
TMC26XStepper *tmc26XStepper2;
#ifdef VERSION22
#define Axis1StepPin    2                   // Pin 2 (Step)
#define Axis1DirPin     3                   // Pin 3 (Dir)
#define Axis1SGPin      4                   // Pin 4 (SG)
#define Axis1CSPin      5                   // Pin 5 (CS)

#define Axis2StepPin    6                   // Pin 6 (Step)
#define Axis2DirPin     7                   // Pin 7 (Dir)
#define Axis2SGPin      8                   // Pin 8 (SG)
#define Axis2CSPin      9                   // Pin 9 (CS)
// Pin 11 12 13 are used!! for SPI
//Focuser Interface
#define FocuserRX       26
#define FocuserTX       31

// ST4 interface
//#define ST4RAw  24                          // Pin 24 ST4 RA- West
//#define ST4DEs  25                          // Pin 25 ST4 DE- South
//#define ST4DEn  26                          // Pin 26 ST4 DE+ North
//#define ST4RAe  27                          // Pin 27 ST4 RA+ East
#define LEDPin 33 
#endif
#ifdef VERSION23
#define Axis1StepPin    21                   // Pin 22 (Step)
#define Axis1DirPin     3                   // Pin 3 (Dir)
#define Axis1SGPin      22                   // Pin 22 (SG)
#define Axis1CSPin      2                   // Pin 5 (CS)
#define Axis1Enable     4                   // Pin 5 (enable)

#define Axis2StepPin    6                   // Pin 6 (Step)
#define Axis2DirPin     19                   // Pin 19 (Dir)
#define Axis2SGPin      5                   // Pin 5 (SG)
#define Axis2CSPin      20                   // Pin 20 (CS)
#define Axis2Enable     18                   // Pin 18 (enable)
// Pin 11 12 13 are used!! for SPI
//Focuser Interface
#define FocuserRX       9
#define FocuserTX       10

// ST4 interface
//#define ST4RAw  24                          // Pin 24 ST4 RA- West
//#define ST4DEs  25                          // Pin 25 ST4 DE- South
//#define ST4DEn  26                          // Pin 26 ST4 DE+ North
//#define ST4RAe  27                          // Pin 27 ST4 RA+ East
#define LEDPin 23 
#endif


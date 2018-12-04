#pragma once
//uncomment the version PCB you are using

//#define VERSION  200
//#define VERSION  220
#define VERSION  230
//#define VERSION  240


#if VERSION == 200
#define AxisDriver      1                   //only with TOS100

#define Axis1StepPin    2                   // Pin 2 (Step)
#define Axis1DirPin     3                   // Pin 3 (Dir)
#define Axis1SGPin      4                   // Pin 4 (SG)
#define Axis1CSPin      5                   // Pin 5 (CS)
#define Axis1EnablePin  -1                  //alway enable

#define Axis2StepPin    6                   // Pin 6 (Step)
#define Axis2DirPin     7                   // Pin 7 (Dir)
#define Axis2SGPin      8                   // Pin 8 (SG)
#define Axis2CSPin      9                   // Pin 9 (CS)
#define Axis2EnablePin  -1                  //alway enable

//Focuser Interface
#define FocuserRX       26
#define FocuserTX       31
#define Axis1Reverse   0
#define Axis2Reverse   0

#define LEDPin 33
#define MaxLED 255

#endif
#if VERSION == 220
#define AxisDriver      1                    //only with TOS100

#define Axis1StepPin    2                   // Pin 2 (Step)
#define Axis1DirPin     3                   // Pin 3 (Dir)
#define Axis1SGPin      4                   // Pin 4 (SG)
#define Axis1CSPin      5                   // Pin 5 (CS)
#define Axis1EnablePin  -1                  //alway enable

#define Axis2StepPin    6                   // Pin 6 (Step)
#define Axis2DirPin     7                   // Pin 7 (Dir)
#define Axis2SGPin      8                   // Pin 8 (SG)
#define Axis2CSPin      9                   // Pin 9 (CS)
#define Axis2EnablePin  -1                  //alway enable
// Pin 11 12 13 are used!! for SPI
#define Axis1Reverse   0
#define Axis2Reverse   0

//Focuser Interface
#define FocuserRX       26
#define FocuserTX       31

// ST4 interface
#define ST4RAe  14                      // ST4 REast
#define ST4DEs  15                      // ST4 South
#define ST4DEn  16                      // ST4 North
#define ST4RAw  17                      // ST4 West
#define LEDPin 33
#define MaxLED 255
#endif

#if VERSION == 230
#define AxisDriver      1                 //only with TOS100

#define Axis1StepPin    21                // Pin 22 (Step)
#define Axis1DirPin     3                 // Pin 3 (Dir)
#define Axis1SGPin      22                // Pin 22 (SG)
#define Axis1CSPin      2                 // Pin 5 (CS)
#define Axis1EnablePin  4                 // Pin 5 (enable)

#define Axis2StepPin    6                 // Pin 6 (Step)
#define Axis2DirPin     19                // Pin 19 (Dir)
#define Axis2SGPin      5                 // Pin 5 (SG)
#define Axis2CSPin      20                // Pin 20 (CS)
#define Axis2EnablePin  18                // Pin 18 (enable)
#define Axis1Reverse    0
#define Axis2Reverse    0
// Pin 11 12 13 are used!! for SPI
//Focuser Interface
#define FocuserRX       9
#define FocuserTX       10

// ST4 interface
#define ST4RAe  14                      // ST4 REast
#define ST4DEs  15                      // ST4 South
#define ST4DEn  16                      // ST4 North
#define ST4RAw  17                      // ST4 West
#define LEDPin 23 
#define MaxLED 16
#endif

#if VERSION == 240
#define AxisDriver      3                 // Select your driver 2 for the TMC2130, 3 for the TMC5160
#define Axis1StepPin    22                // Pin 22 (Step)
#define Axis1DirPin     2                 // Pin 3 (Dir)
#define Axis1CSPin      21                // Pin 5 (CS)
#define Axis1EnablePin  3                 // Pin 5 (enable)
#define Axis2StepPin    20                // Pin 6 (Step)
#define Axis2DirPin     4                 // Pin 19 (Dir)
#define Axis2CSPin      19                // Pin 20 (CS)
#define Axis2EnablePin  5                 // Pin 18 (enable)
#define Axis1Reverse    1
#define Axis2Reverse    1
// Pin 11 12 13 are used!! for SPI
//Focuser Interface
#define FocuserRX       9
#define FocuserTX       10
// ST4 interface
#define ST4RAe  14                      // ST4 REast
#define ST4DEs  15                      // ST4 South
#define ST4DEn  16                      // ST4 North
#define ST4RAw  17                      // ST4 West

// LED interface
#define LEDPin 23 
#define MaxLED 16
#endif
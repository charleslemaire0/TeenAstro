// -----------------------------------------------------------------------------------
// Configuration
#include "Pins.TeenAstro.h"

// turns debugging on, used during testing, default=OFF
#define DEBUG_OFF

// optional stepper driver Fault detection, just wire driver Fault signal to Pins 26 (Axis1) and 31 (Axis2), default=OFF (Teensy3.1 Pins 17,22)
// other settings are LOW and HIGH
#define AXIS1_FAULT_OFF
#define AXIS2_FAULT_OFF
// optional stepper driver Enable support is always on, just wire Enable to Pins 25 (Axis1) and 30 (Axis2) and OnStep will pull these HIGH (Teensy3.1 Pins 16,21)
// by default to disable stepper drivers on startup and when PRK_PARKED. An Align or UnPark will enable the drivers.  Adjust below if you need these pulled LOW to disable the drivers.
#define AXIS1_DISABLED_HIGH
#define AXIS2_DISABLED_HIGH

// ADJUST THE FOLLOWING TO MATCH YOUR MOUNT --------------------------------------------------------------------------------
#define StepsMaxRate              20 // this is the minimum number of micro-seconds between micro-steps
                                     // minimum* (fastest goto) is around 16 (Teensy3.1) or 32 (MegeoA2560), default=96 higher is ok
                                     // too low and OnStep communicates slowly and/or freezes as the motor timers use up all the MCU time
                                     // * = minimum can be lower, when both AXIS1/AXIS2_MODE_GOTO are used by AXIS1/AXIS2_STEP_GOTO times

#define BacklashTakeupRate        16 // backlash takeup rate (in multipules of the sidereal rate): too fast and your motors will stall,
                                     // too slow and the mount will be sluggish while it moves through the backlash
                                     // for the most part this doesn't need to be changed, but adjust when needed.  Default=25

#define MinAxis2EQ                   -360 // minimum allowed position for axis2
#define MaxAxis2EQ                   +360 // maximum allowed position for axis2
#define MinAxis2AZALT                -10 // minimum allowed position for axis2
#define MaxAxis2AZALT                 90 // maximum allowed position for axis2
#define DegreePastAZ                  10  // Alt/Az mounts only. +/- maximum allowed Azimuth, default = 0.  Allowed range is 0 to 30

// -----------------------------------------------------------------------------------
// Configuration
#include "Pins.TeenAstro.h"

// turns debugging on, used during testing, default=OFF
#define DEBUG_OFF
// optional stepper driver Fault detection, just wire driver Fault signal to Pins 26 (Axis1) and 31 (Axis2), default=OFF (Teensy3.1 Pins 17,22)
// other settings are LOW and HIGH
#define AXIS1_FAULT_OFF
#define AXIS2_FAULT_OFF
// optional stepper driver Enable support is always on, just wire Enable to Pins 25 (Axis1) and 30 (Axis2) and TeenAstro will pull these HIGH (Teensy3.1 Pins 16,21)
// by default to disable stepper drivers on startup and when PRK_PARKED. An Align or UnPark will enable the drivers.  Adjust below if you need these pulled LOW to disable the drivers.
#define AXIS1_DISABLED_HIGH
#define AXIS2_DISABLED_HIGH
// ADJUST THE FOLLOWING TO MATCH YOUR MOUNT --------------------------------------------------------------------------------
#define StepsMinInterval          16 // this is the minimum number of micro-seconds between micro-steps                        
#define StepsMaxInterval      100000 // this is the maximum number of micro-seconds between micro-steps
#define BacklashTakeupRate        16 // backlash takeup rate (in multipules of the sidereal rate): too fast and your motors will stall,
                                     // too slow and the mount will be sluggish while it moves through the backlash
                                     // for the most part this doesn't need to be changed, but adjust when needed.  Default=25

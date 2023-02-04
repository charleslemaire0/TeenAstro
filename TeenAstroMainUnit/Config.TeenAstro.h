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


#if defined(ARDUINO_TEENSY40) || defined(ARDUINO_TEENSY_MICROMOD)
#define StepsMinInterval          4 // this is the minimum number of micro-seconds between micro-steps                        
#else
#define StepsMinInterval         16 // this is the minimum number of micro-seconds between micro-steps 
#endif
#define StepsMaxInterval      100000 // this is the maximum number of micro-seconds between micro-steps


#ifdef MOUNT_TEMPLATE
#include "Config.Mount.Template.h"
#elif defined MOUNT_ZH_1
#include "Config.Mount.ZH-1.h"
#else
#include "Config.Mount.Universal.h"
#endif
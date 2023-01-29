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


#if MOUNT_TEMPLATE == 1
    #include "Config.Mount.ZH-1.h"
#else
    #include "Config.Mount.Universal.h"
#endif


#define FirmwareSubName "ZH-1"

// MOUNT_TYPE_GEM = 1, MOUNT_TYPE_FORK = 2, MOUNT_TYPE_ALTAZM = 3, MOUNT_TYPE_FORK_ALT = 4
#define D_mountType 4

#define D_motorA1gear 16
#define D_motorA1stepRot 400
#define D_motorA1micro 8
#define D_motorA1reverse 0
#define D_motorA1highCurr 1600
#define D_motorA1lowCurr 1200
#define D_motorA1silent 1

#define D_motorA2gear 16
#define D_motorA2stepRot 200
#define D_motorA2micro 8
#define D_motorA2reverse 0
#define D_motorA2highCurr 1600
#define D_motorA2lowCurr 1200
#define D_motorA2silent 1

#define D_encoderA1plusePerDegree 400
#define D_encoderA1reverse 0

#define D_encoderA2plusePerDegree 400
#define D_encoderA2reverse 0
#pragma once
//uncomment the version PCB you are using

#include "FirmwareDef.h"

#if VERSION == 220
#define Focus_Serial Serial2
#define GNSS_Serial  Serial3
#ifdef AxisDriver
#undef AxisDriver
#endif
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
#define Axis1Reverse    0
#define Axis2Reverse    0

//Focuser Interface
#define FocuserRX       26
#define FocuserTX       31

// ST4 interface
#define ST4RAe          14                // ST4 REast
#define ST4DEs          15                // ST4 South
#define ST4DEn          16                // ST4 North
#define ST4RAw          17                // ST4 West
#define LEDPin          33
#define MaxLED          255
#endif

#if VERSION == 230
#define Focus_Serial Serial2
#define GNSS_Serial  Serial3
#ifdef AxisDriver
#undef AxisDriver
#endif
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
#define ST4RAe          14                // ST4 REast
#define ST4DEs          15                // ST4 South
#define ST4DEn          16                // ST4 North
#define ST4RAw          17                // ST4 West
#define LEDPin          23
#define MaxLED          16
#endif

#if VERSION == 240
#define Focus_Serial Serial2
#define GNSS_Serial  Serial3
#ifndef AxisDriver
#define AxisDriver      3                 // Select your driver 2 for the TMC2130, 3 for the TMC5160, 4 for the TMC2160
#endif
#define Axis1StepPin    22                // Pin 22 (Step)
#define Axis1DirPin     2                 // Pin 3 (Dir)
#define Axis1CSPin      21                // Pin 5 (CS)
#define Axis1EnablePin  3                 // Pin 5 (enable)
#define Axis2StepPin    20                // Pin 6 (Step)
#define Axis2DirPin     4                 // Pin 19 (Dir)
#define Axis2CSPin      19                // Pin 20 (CS)
#define Axis2EnablePin  5                 // Pin 18 (enable)
#define RETICULE_LED_PINS 6  
#define PPS             18
#define Axis1Reverse    1
#define Axis2Reverse    1
// Pin 11 12 13 are used!! for SPI
//Focuser Interface
#define FocuserRX       9
#define FocuserTX       10
// ST4 interface
#define ST4RAe          14                // ST4 REast
#define ST4DEs          15                // ST4 South
#define ST4DEn          16                // ST4 North
#define ST4RAw          17                // ST4 West

// LED interface
#define LEDPin          23 
#define MaxLED          16
#endif

#if VERSION == 245                      // Teensy 3.2, beta board, not released, small batch in test - lordzurp

#define Focus_Serial Serial2
#define GNSS_Serial  Serial3
#ifndef AxisDriver
#define AxisDriver      3                 // Select your driver 2 for the TMC2130, 3 for the TMC5160, 4 for the TMC2160
#endif
#define Axis1StepPin    22                // Pin 22 (Step)
#define Axis1DirPin     2                 // Pin 3 (Dir)
#define Axis1CSPin      21                // Pin 5 (CS)
#define Axis1EnablePin  3                 // Pin 5 (enable)
#define Axis2StepPin    20                // Pin 6 (Step)
#define Axis2DirPin     4                 // Pin 19 (Dir)
#define Axis2CSPin      19                // Pin 20 (CS)
#define Axis2EnablePin  5                 // Pin 18 (enable)
#define RETICULE_LED_PINS 6  
#define PPS             18
#define Axis1Reverse    1
#define Axis2Reverse    1
// Pin 11 12 13 are used!! for SPI
//Focuser Interface
#define FocuserRX       9
#define FocuserTX       10
// ST4 interface
#define ST4RAe          14                // ST4 REast
#define ST4DEs          15                // ST4 South
#define ST4DEn          16                // ST4 North
#define ST4RAw          17                // ST4 West

// LED interface
#define LEDPin          23 
#define MaxLED          16

#endif

#if VERSION == 246                      // Teensy 4.0, beta board, not released, small batch in test - lordzurp

#define Focus_Serial Serial2
#define GNSS_Serial  Serial3
#ifndef AxisDriver
#define AxisDriver      3                 // Select your driver 2 for the TMC2130, 3 for the TMC5160, 4 for the TMC2160
#endif
#define Axis1StepPin    22                // Pin 22 (Step)
#define Axis1DirPin     2                 // Pin 3 (Dir)
#define Axis1CSPin      21                // Pin 5 (CS)
#define Axis1EnablePin  3                 // Pin 5 (enable)
#define Axis2StepPin    20                // Pin 6 (Step)
#define Axis2DirPin     4                 // Pin 19 (Dir)
#define Axis2CSPin      19                // Pin 20 (CS)
#define Axis2EnablePin  5                 // Pin 18 (enable)
#define RETICULE_LED_PINS 6  
#define PPS             18
#define Axis1Reverse    1
#define Axis2Reverse    1
// Pin 11 12 13 are used!! for SPI
//Focuser Interface
#define FocuserRX       9
#define FocuserTX       10
// ST4 interface
#define ST4RAe          14                // ST4 REast
#define ST4DEs          15                // ST4 South
#define ST4DEn          16                // ST4 North
#define ST4RAw          17                // ST4 West

// LED interface
#define LEDPin          23 
#define MaxLED          16

#endif

#if VERSION == 250

    // GNSS
    #define GNSS_Serial  Serial2
    #define PPS             18
    
    #ifndef AxisDriver
    #define AxisDriver      3                 // Select your driver 2 for the TMC2130, 3 for the TMC5160, 4 for the TMC2160
    #endif

    // Motor 1 : RA / AZ
    #define Axis1Reverse    1

    #define Axis1StepPin    22                // Pin 22 (Motor 1 Step)
    #define Axis1DirPin     2                 // Pin 2 (Motor 1 Dir)
    #define Axis1CSPin      9                 // Pin 9 (Motor 1 CS)
    #define Axis1EnablePin  3                 // Pin 3 (Motor 1 enable)

    // Motor 2 : Dec / Alt
    #define Axis2Reverse    1

    #define Axis2StepPin    20                // Pin 20 (Motor 2 Step)
    #define Axis2DirPin     4                 // Pin 4 (Motor 2 Dir)
    #define Axis2CSPin      10                // Pin 10 (Motor 2 CS)
    #define Axis2EnablePin  5                 // Pin 5 (Motor 2 enable)

    // encoder interface
    #define EA1A             25
    #define EA1B             27
    #define EA2A             31
    #define EA2B             33

    //Focuser Interface
    #define Focus_Serial Serial3
    #define FocuserRX       15
    #define FocuserTX       14
    
    // ST4 interface
    #define ST4RAe          16                 // ST4 REast
    #define ST4DEs          17                 // ST4 South
    #define ST4DEn          18                 // ST4 North
    #define ST4RAw          19                 // ST4 West

    // LED interface
    #define LEDPin          23 
    #define MaxLED          16
    #define RETICULE_LED_PINS 6  

    #define LimitSensor     21

#endif



#if VERSION == 259                       // teensy micromod, beta board, not released - lordzurp

    // GNSS
    #define GNSS_Serial  Serial2
    #define PPS             10

    #ifndef AxisDriver
    #define AxisDriver      4                 // Select your driver 2 for the TMC2130, 3 for the TMC5160, 4 for the TMC2660
    #endif

    // Motor 1 : RA / AZ
    #define Axis1Reverse    1

    #define Axis1StepPin    32              // Pin 32 (Motor 1 Step)
    #define Axis1DirPin     26              // Pin 26 (Motor 1 Dir)
    #define Axis1CSPin      9               // Pin 9 (Motor 1 CS)
    #define Axis1EnablePin  6               // Pin 6 (Motor 1 enable)

    // Motor 2 : Dec / Alt
    #define Axis2Reverse    1

    #define Axis2StepPin    35              // Pin 35 (Motor 2 Step)
    #define Axis2DirPin     34              // Pin 34 (Motor 2 Dir)
    #define Axis2CSPin      38              // Pin 38 (Motor 2 CS)
    #define Axis2EnablePin  39              // Pin 39 (Motor 2 enable)

    // LED interface
    #define LEDPin          27
    #define MaxLED          16
    #define RETICULE_LED_PINS 28

    // **************
    // *** UNUSED ***
    // **************
        // ST4 interface
        #define ST4RAe          41                // ST4 REast
        #define ST4DEs          42                // ST4 South
        #define ST4DEn          43                // ST4 North
        #define ST4RAw          44                // ST4 West

        //Focuser Interface
        #define Focus_Serial Serial3
        #define FocuserRX       15
        #define FocuserTX       14
#endif

#if VERSION == 260                          // teensy micromod, release candidate board - lordzurp

    // GNSS
    #define GNSS_Serial  Serial4
    #define PPS             5

    #ifndef AxisDriver
    #define AxisDriver      4                 // Select your driver 2 for the TMC2130, 3 for the TMC5160, 4 for the TMC2660
    #endif

    // Motor 1 : RA / AZ
    #define Axis1Reverse    1

    #define Axis1StepPin    35              // Pin 35 (Motor 1 Step)
    #define Axis1DirPin     34              // Pin 34 (Motor 1 Dir)
    #define Axis1CSPin      6               // Pin 6 (Motor 1 CS)
    #define Axis1EnablePin  9               // Pin 9 (Motor 1 Enable)

    // Motor 2 : Dec / Alt
    #define Axis2Reverse    1

    #define Axis2StepPin    39              // Pin 39 (Motor 2 Step)
    #define Axis2DirPin     37              // Pin 37 (Motor 2 Dir)
    #define Axis2CSPin      32              // Pin 32 (Motor 2 CS)
    #define Axis2EnablePin  26              // Pin 26 (Motor 2 Enable)

    // encoder interface
    #define EA1A             2
    #define EA1B             30
    #define EA2A             33
    #define EA2B             31

    // LED interface
    #define LEDPin          27
    #define MaxLED          16
    #define RETICULE_LED_PINS 28

    // ==============
    // =   UNUSED   =
    // ==============
        // ST4 interface
        #define ST4RAe          41                // ST4 REast
        #define ST4DEs          42                // ST4 South
        #define ST4DEn          43                // ST4 North
        #define ST4RAw          44                // ST4 West

        //Focuser Interface
        #define Focus_Serial Serial3
        #define FocuserRX       15
        #define FocuserTX       14
#endif

#define HASST4 (defined ST4RAw && defined ST4RAe && defined ST4DEn && defined ST4DEs)
#define HASEncoder (defined EA1A && defined EA1B && defined EA2A && defined EA2B)

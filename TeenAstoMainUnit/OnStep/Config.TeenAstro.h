// -----------------------------------------------------------------------------------
// Configuration
#include "Pins.TeenAstro.h"





// -------------------------------------------------------------------------------------------------------------------------
// ADJUST THE FOLLOWING TO CONFIGURE YOUR CONTROLLER FEATURES --------------------------------------------------------------

// turns ethernet on for W5100 adapters, default=OFF (if _ON you must also uncomment the #include "EthernetPlus.h" line near the beginning of OnStep.ino for now)
// configure the interface IP address and subnet mask in Ethernet.ino
// see forum below for good technical info. about using an Arduino Ethernet Shield:
// https://forum.pjrc.com/threads/25050-Teensy-3-0-3-1-W5100-ethernet-shield
#define W5100_OFF
// default IP,Gateway,subnet are in the Network.ino file
// if ethernet is available DHCP is used to obtain the IP address (default addresses are overridden), default=OFF
#define ETHERNET_USE_DHCP_OFF

// turns debugging on, used during testing, default=OFF
#define DEBUG_OFF

// Mount type, default is _GEM (German Equatorial) other options are _FORK, _FORK_ALT.  _FORK switches off Meridian Flips after (1, 2 or 3 star) alignment is done.  _FORK_ALT disables Meridian Flips (1 star align.)
// _ALTAZM is for Alt/Azm mounted 'scopes (1 star align only.)

// ST4 interface on pins 47, 49, 51, 53.  Pin 47 is RA- (West), Pin 49 is Dec- (South), Pin 51 is Dec+ (North), Pin 53 is RA+ (East.)  Teensy3.1 pins 24, 25, 26, 27.
// ST4_ON enables the interface.  ST4_PULLUP enables the interface and any internal pullup resistors.
// It is up to you to create an interface that meets the electrical specifications of any connected device, use at your own risk.
// ST4_ALTERNATE_PINS_ON moves the interface (Mega2560 only) to pins 43, 45, 47, 49.  Pin 43 is Dec- (South), Pin 45 is Dec+ (North), Pin 47 is RA- (West), Pin 49 is RA+ (East.)
// ST4_ALTERNATE_PINS_ON is required for Steve's ST4 board and is also required if the ST4 interface is to be used alongside the Arduino Ethernet Shield
#define ST4_OFF
#define ST4_ALTERNATE_PINS_OFF
// PPS sense rising edge on pin 21 for optional precision clock source (GPS, for example), default=OFF (Teensy3.1 Pin 23)
#define PPS_SENSE_OFF
#define LIMIT_SENSE_OFF
// light status LED by sink to ground (pin 9) and source +5V (pin 8), default=ON
// _ON and OnStep keeps this illuminated to indicate that the controller is active.  When sidereal tracking this LED will rapidly flash
#define STATUS_LED_PINS_OFF
// lights 2nd status LED by sink to ground (pin 10), default=OFF, must be off for Teensy3.1 (pin 7)
// _ON sets this to blink at 1 sec intervals when PPS is synced
// n sets this to dimly light a polar finder reticle, for example I use STATUS_LED2_PINS 250
// The W5100 Ethernet adapter uses pin 10 for CS, so if W5100_ON is used this must be _OFF
#define STATUS_LED2_PINS_OFF
// lights reticule LED by sink to ground (pin 44), default=OFF.  Defaults to pin 9 on the Teensy3.1 (STATUS_LED_PINS must be _ON)
// RETICULE_LED_PINS n, where n=0 to 255 activates this feature and sets default brightness
#define RETICULE_LED_PINS_OFF

// optional stepper driver Fault detection, just wire driver Fault signal to Pins 26 (Axis1) and 31 (Axis2), default=OFF (Teensy3.1 Pins 17,22)
// other settings are LOW and HIGH
#define AXIS1_FAULT_OFF
#define AXIS2_FAULT_OFF
// optional stepper driver Enable support is always on, just wire Enable to Pins 25 (Axis1) and 30 (Axis2) and OnStep will pull these HIGH (Teensy3.1 Pins 16,21)
// by default to disable stepper drivers on startup and when Parked. An Align or UnPark will enable the drivers.  Adjust below if you need these pulled LOW to disable the drivers.
#define AXIS1_DISABLED_HIGH
#define AXIS2_DISABLED_HIGH

// optionally adjust tracking rate to compensate for atmospheric refraction, default=OFF (limited testing done)
// can be turned on/off with the :Te# and :Td# commands regardless of this setting
#define TRACK_REFRACTION_RATE_DEFAULT_OFF

// use seperate pulse-guide rate so centering and guiding don't disturb each other, default=ON
#define SEPERATE_PULSE_GUIDE_RATE_ON

// ADJUST THE FOLLOWING TO MATCH YOUR MOUNT --------------------------------------------------------------------------------
#define MaxRate                   20 // this is the minimum number of micro-seconds between micro-steps
                                     // minimum* (fastest goto) is around 16 (Teensy3.1) or 32 (Mega2560), default=96 higher is ok
                                     // too low and OnStep communicates slowly and/or freezes as the motor timers use up all the MCU time
                                     // * = minimum can be lower, when both AXIS1/AXIS2_MODE_GOTO are used by AXIS1/AXIS2_STEP_GOTO times

#define BacklashTakeupRate        60 // backlash takeup rate (in multipules of the sidereal rate): too fast and your motors will stall,
                                     // too slow and the mount will be sluggish while it moves through the backlash
                                     // for the most part this doesn't need to be changed, but adjust when needed.  Default=25




// Tak EM10         : (14400*360)/144 = 36000


#ifdef EM200b
#define GearAxis1 1800.
#define StepRotAxis1 200
#define MicroAxis1 16
#define GearAxis2 1800.
#define StepRotAxis2 200
#define MicroAxis2 16

// Tak EM10         : (14400*360)/144 = 36000
#define REVERSE_AXIS1_ON          // reverse the direction of movement for the HA/RA axis, adjust as needed or reverse your wiring so things move in the right direction
#define REVERSE_AXIS2_OFF        // reverse the direction of movement for the Dec axis (both reversed for my EM10b, both normal for G11)
#endif

#ifdef T400
#define GearAxis1 4608.
#define StepRotAxis1 200
#define MicroAxis1 16

#define GearAxis2 4608.
#define StepRotAxis2 200
#define MicroAxis2 16

#define StepsPerWormRotationAxis1 32000
#define PECBufferSize           824  // PEC, buffer size, max should be no more than 3384, your required buffer size >= StepsPerAxis1WormRotation/(StepsPerDegeeAxis1/240)
// for the most part this doesn't need to be changed, but adjust when needed.  824 seconds is the default.  Ignored on Alt/Azm mounts.
#define REVERSE_AXIS1_ON          // reverse the direction of movement for the HA/RA axis, adjust as needed or reverse your wiring so things move in the right direction
#define REVERSE_AXIS2_ON        // reverse the direction of movement for the Dec axis (both reversed for my EM10b, both normal for G11)
#endif

#define MinDec                   -91 // minimum allowed declination, default = -91 (off)  Ignored on Alt/Azm mounts.
#define MaxDec                   +91 // maximum allowed declination, default =  91 (off)  Ignored on Alt/Azm mounts.
                                     // For example, a value of +80 would stop gotos/tracking near the north celestial pole.
                                     // For a Northern Hemisphere user, this would stop tracking when the mount is in the polar home position but
                                     // that can be easily worked around by doing an alignment once and saving a park position (assuming a 
                                     // fork/yolk mount with meridian flips turned off by setting the minutesPastMeridian values to cover the whole sky)
#define MaxAzm                   180 // Alt/Az mounts only. +/- maximum allowed Azimuth, default =  180.  Allowed range is 180 to 360



// THAT'S IT FOR USER CONFIGURATION!

// -------------------------------------------------------------------------------------------------------------------------



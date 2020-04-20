#pragma once
#include <Arduino.h>
#include <math.h>
#include <TeenAstroCoordConv.hpp>
#include <TeenAstroMath.h>
#include <TinyGPS++.h>
#include <TeenAstroStepper.h>
#include "Config.TeenAstro.h"
#include "timerLoop.hpp"
#include "TelTimer.hpp"
#include "Site.hpp"
#include "FPoint.h"
#include "Command.h"
#include "Config.TeenAstro.h"
#include "EEPROM_adress.h"
#include "XEEPROM.hpp"

TinyGPSPlus gps;
CoordConv alignment;
bool hasStarAlignment = false;
enum Mount { MOUNT_UNDEFINED, MOUNT_TYPE_GEM, MOUNT_TYPE_FORK, MOUNT_TYPE_ALTAZM, MOUNT_TYPE_FORK_ALT };
enum PierSide { PIER_NOTVALID, PIER_EAST, PIER_WEST };
enum MeridianFlip { FLIP_NEVER, FLIP_ALIGN, FLIP_ALWAYS };
enum CheckMode { CHECKMODE_GOTO, CHECKMODE_TRACKING };
enum ParkState { PRK_UNPARKED, PRK_PARKING, PRK_PARKED, PRK_FAILED, PRK_UNKNOW };

ParkState parkStatus = PRK_UNPARKED;
boolean parkSaved = false;
boolean atHome = true;
boolean homeMount = false;
MeridianFlip meridianFlip = FLIP_NEVER;
Mount mountType = MOUNT_TYPE_GEM;
byte maxAlignNumStar = 0;
boolean hasFocuser = false;
boolean hasGNSS = true;
boolean refraction = true;
boolean correct_tracking = false;
// 86164.09 sidereal seconds = 1.00273 clock seconds per sidereal second)
double                  siderealInterval = 15956313.0;
const double            masterSiderealInterval = 15956313.0;

// default = 15956313 ticks per sidereal hundredth second, where a tick is 1/16 uS
// this is stored in XEEPROM.which is updated/adjusted with the ":T+#" and ":T-#" commands
// a higher number here means a longer count which slows down the sidereal clock
const double            HzCf = 16000000.0 / 60.0;   // conversion factor to go to/from Hz for sidereal interval
volatile double         SiderealRate;               // based on the siderealInterval, this is the time between steps for sidereal tracking
volatile double         TakeupRate;                 // this is the takeup rate for synchronizing the target and actual positions when needed


double                  maxRate = MaxRate * 16L;
float                   pulseGuideRate = 0.25; //in sideral Speed
double                  DegreesForAcceleration = 3;


//Timers
volatile double         timerRateAxis1 = 0;
volatile double         timerRateBacklashAxis1 = 0;
volatile boolean        inbacklashAxis1 = false;
boolean                 faultAxis1 = false;
volatile double         timerRateAxis2 = 0;
volatile double         timerRateBacklashAxis2 = 0;
volatile boolean        inbacklashAxis2 = false;
boolean                 faultAxis2 = false;

//Motor Axis1
unsigned int GearAxis1;
unsigned int StepRotAxis1;
byte MicroAxis1;
bool ReverseAxis1;
u_int8_t HighCurrAxis1;
u_int8_t LowCurrAxis1;

//Motor Axis2
unsigned int GearAxis2;
unsigned int StepRotAxis2;
uint8_t MicroAxis2;
bool ReverseAxis2;
u_int8_t HighCurrAxis2;
u_int8_t LowCurrAxis2;

//tracking rate
#define default_tracking_rate   1
volatile double         trackingTimerRateAxis1 = default_tracking_rate;
volatile double         trackingTimerRateAxis2 = default_tracking_rate;
volatile double         guideTimerRateAxis1 = 0.0;
volatile double         guideTimerRateAxis2 = 0.0;

// backlash control
int backlashAxis1 = 0;
int backlashAxis2 = 0;
volatile int    StepsBacklashAxis1 = 0;
volatile int    StepsBacklashAxis2 = 0;
volatile int    blAxis1 = 0;
volatile int    blAxis2 = 0;


//geometry Axis1
long StepsPerRotAxis1; // calculated as    :  stepper_steps * micro_steps * gear_reduction1 * (gear_reduction2/360)
double StepsPerDegreeAxis1;
double StepsPerSecondAxis1;
long halfRotAxis1;
long quaterRotAxis1;
long poleStepAxis1;
long homeStepAxis1;

//geometry Axis2
long StepsPerRotAxis2; // calculated as    :  stepper_steps * micro_steps * gear_reduction1 * (gear_reduction2/360)
double StepsPerDegreeAxis2;
double StepsPerSecondAxis2;
long halfRotAxis2;
long quaterRotAxis2;
long poleStepAxis2;
long homeStepAxis2;

volatile double         timerRateRatio;
volatile boolean        useTimerRateRatio;

#define BreakDistAxis1              (2L)
#define BreakDistAxis2              (2L)

volatile double         AccAxis1 = 0; //acceleration in steps per second square
volatile double         AccAxis2 = 0; //acceleration in steps per second square

IntervalTimer           itimer3;
void                    TIMER3_COMPA_vect(void);

IntervalTimer           itimer4;
void                    TIMER4_COMPA_vect(void);


PierSide newTargetPierSide = PIER_NOTVALID;


//Target and position Axis 1
volatile long       posAxis1;    // hour angle position in steps
volatile long       deltaTargetAxis1;
volatile long       startAxis1;  // hour angle of goto start position in steps
volatile fixed_t    targetAxis1; // hour angle of goto end   position in steps
volatile byte       dirAxis1;    // stepping direction + or -
#define stepAxis1   1


//Target and position Axis 2
volatile long       posAxis2;     // declination position in steps
volatile long       deltaTargetAxis2;
volatile long       startAxis2;   // declination of goto start position in steps
volatile fixed_t    targetAxis2;  // declination of goto end   position in steps
volatile byte       dirAxis2;     // stepping direction + or -
#define stepAxis2   1


//Targets
double              newTargetAlt = 0.0;                     // holds the altitude for goTos
double              newTargetAzm = 0.0;                     // holds the azmiuth for goTos
double              newTargetDec;                           // holds the Dec for goTos
double              newTargetRA;                            // holds the RA for goTos

double              currentAzm = 0;                         // the current Azimuth
double              currentAlt = 45;                        // the current altitude

//Limits
int                 minAlt;                                 // the minimum altitude, in degrees, for goTo's (so we don't try to point too low)
int                 maxAlt;                                 // the maximum altitude, in degrees, for goTo's (to keep the telescope tube away from the mount/tripod)
long                minutesPastMeridianGOTOE;               // for goto's, how far past the meridian to allow before we do a flip (if on the East side of the pier)- one hour of RA is the default = 60.  Sometimes used for Fork mounts in Align mode.  Ignored on Alt/Azm mounts.
long                minutesPastMeridianGOTOW;               // as above, if on the West side of the pier.  If left alone, the mount will stop tracking when it hits the this limit.  Sometimes used for Fork mounts in Align mode.  Ignored on Alt/Azm mounts.
double              underPoleLimitGOTO;                     // maximum allowed hour angle (+/-) under the celestial pole. OnStep will flip the mount and move the Dec. >90 degrees (+/-) once past this limit.  Sometimes used for Fork mounts in Align mode.  Ignored on Alt/Azm mounts.
//                                                          // If left alone, the mount will stop tracking when it hits this limit.  Valid range is 7 to 11 hours.

#define HADirNCPInit    0
#define HADirSCPInit    1
volatile byte   HADir = HADirNCPInit;

// Status ------------------------------------------------------------------------------------------------------------------
enum Errors
{
  ERR_NONE,
  ERR_MOTOR_FAULT,
  ERR_ALT,
  ERR_LIMIT_SENSE,
  ERR_AXIS2,
  ERR_AZM,
  ERR_UNDER_POLE,
  ERR_MERIDIAN,
  ERR_SYNC
};

Errors lastError = ERR_NONE;
Errors StartLoopError = ERR_NONE;

//Command Precision
boolean highPrecision = true;

volatile bool movingTo = false;

bool doSpiral = false;

// Tracking
#define TrackingSolar 0.99726956632
#define TrackingLunar 0.96236513150
bool lastSideralTracking = false;
volatile bool sideralTracking = false;
enum SID_Mode
{
  SIDM_STAR,
  SIDM_SUN,
  SIDM_MOON
};
volatile SID_Mode sideralMode = SIDM_STAR;

//Guiding
enum Guiding { GuidingOFF, GuidingPulse, GuidingST4, GuidingRecenter };
volatile Guiding GuidingState = GuidingOFF;
unsigned long lastSetTrakingEnable = millis();
unsigned long lastSecurityCheck = millis();

boolean abortSlew = false;

// Command processing -------------------------------------------------------------------------------------------------------
#define BAUD 57600

boolean commandError = false;
boolean quietReply = false;

char reply[50];
char command[3];
char parameter[25];
byte bufferPtr = 0;

// for serial 0
char command_serial_zero[25];
char parameter_serial_zero[25];
byte bufferPtr_serial_zero = 0;
// for serial 1
char command_serial_one[25];
char parameter_serial_one[25];
byte bufferPtr_serial_one = 0;

// serial speed
unsigned long   baudRate[10] =
{
  115200, 56700, 38400, 28800, 19200, 14400, 9600, 4800, 2400, 1200
};

//
Motor motorAxis1;
Motor motorAxis2;

// tracking and PEC, fractional steps
fixed_t         fstepAxis1;
fixed_t         fstepAxis2;

// guide command
#define GuideRate1x     2
#define GuideRate16x    5
#define GuideRateMax    9
#define GuideRateNone   255
#define RG 0
#define RC 4
#define RM 6
#define RS 9

double          guideRates[10] =
{
  0.25 , 0.5 , 1.0 , 2.0 , 4.0 , 16.0, 32.0 , 64.0, 64.0, 64.0
};

//.25X .5x 1x 2x 4x  8x 24x 48x half-MaxRate MaxRate
volatile byte   activeGuideRate = GuideRateNone;

volatile byte   guideDirAxis1 = 0;
long            guideDurationAxis1 = -1;
unsigned long   guideDurationLastAxis1 = 0;
volatile byte   guideDirAxis2 = 0;
long            guideDurationAxis2 = -1;
unsigned long   guideDurationLastAxis2 = 0;

long            lasttargetAxis1 = 0;
long            debugv1 = 0;
boolean         axis1Enabled = false;
boolean         axis2Enabled = false;

double          guideTimerBaseRate = 0;
fixed_t         amountGuideAxis1;
fixed_t         guideAxis1;
fixed_t         amountGuideAxis2;
fixed_t         guideAxis2;

// Reticule control
#ifdef RETICULE_LED_PINS
int             reticuleBrightness = 255;
#endif

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
#include "Command.h"
#include "Config.TeenAstro.h"
#include "EEPROM_adress.h"
#include "XEEPROM.hpp"

TinyGPSPlus gps;
CoordConv alignment;
bool hasStarAlignment = false;
enum Mount { MOUNT_UNDEFINED, MOUNT_TYPE_GEM, MOUNT_TYPE_FORK, MOUNT_TYPE_ALTAZM, MOUNT_TYPE_FORK_ALT };
enum MeridianFlip { FLIP_NEVER, FLIP_ALIGN, FLIP_ALWAYS };
enum CheckMode { CHECKMODE_GOTO, CHECKMODE_TRACKING };
enum ParkState { PRK_UNPARKED, PRK_PARKING, PRK_PARKED, PRK_FAILED, PRK_UNKNOW };

ParkState parkStatus = PRK_UNPARKED;
bool parkSaved = false;
bool atHome = true;
bool homeMount = false;
MeridianFlip meridianFlip = FLIP_NEVER;
Mount mountType = MOUNT_TYPE_GEM;
byte maxAlignNumStar = 0;
bool hasFocuser = false;
bool hasGNSS = true;
bool refraction = true;
bool correct_tracking = false;
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
bool                    faultAxis1 = false;
volatile double         timerRateAxis2 = 0;
volatile double         timerRateBacklashAxis2 = 0;
bool                    faultAxis2 = false;

// -----------------------------------------------------------------------------------------------------------------------------
// Refraction rate tracking
// az_deltaAxis1/az_deltaAxis2 are in arc-seconds/second
double  az_deltaAxis1 = 15., az_deltaAxis2 = 0.;
double  az_deltaRateScale = 1.;


//Motor
class MotorAxis
{
public:
  unsigned int gear;
  unsigned int stepRot;
  byte micro;
  bool reverse;
  u_int8_t highCurr;
  u_int8_t lowCurr;
  Driver driver;

};
MotorAxis MA1;
MotorAxis MA2;


//tracking rate
#define default_tracking_rate   1
volatile double         trackingTimerRateAxis1 = default_tracking_rate;
volatile double         trackingTimerRateAxis2 = default_tracking_rate;
volatile double         guideTimerRateAxis1 = 0.0;
volatile double         guideTimerRateAxis2 = 0.0;

// backlash control
struct backlash
{
  int inSeconds;
  volatile int inSteps;
  volatile bool correcting;
  volatile int movedSteps;
};
backlash bl_Axis1 = { 0,0,0 };
backlash bl_Axis2 = { 0,0,0 };


//geometry Axis
class GeoAxis
{
public:
  long   stepsPerRot; // calculated as    :  stepper_steps * micro_steps * gear_reduction1 * (gear_reduction2/360)
  double stepsPerDegree;
  double stepsPerSecond;
  long   halfRot;   //in steps
  long   quaterRot; //in steps
  long   poleDef;   //in steps
  long   homeDef;   //in steps
  long   breakDist; //in steps
};

GeoAxis GA1;
GeoAxis GA2;

volatile double         timerRateRatio;

volatile double     AccAxis1 = 0; //acceleration in steps per second square
volatile double     AccAxis2 = 0; //acceleration in steps per second square


//Target and position Axis 1
volatile long       posAxis1;    // hour angle position in steps
volatile long       deltaTargetAxis1;
volatile long       startAxis1;  // hour angle of goto start position in steps
volatile double     targetAxis1; // hour angle of goto end   position in steps
volatile bool       dirAxis1;    // stepping direction + or -
double              fstepAxis1;  // amount of steps for Tracking
#define stepAxis1   1

//Target and position Axis 2
volatile long       posAxis2;     // declination position in steps
volatile long       deltaTargetAxis2;
volatile long       startAxis2;   // declination of goto start position in steps
volatile double     targetAxis2;  // declination of goto end   position in steps
volatile bool       dirAxis2;     // stepping direction + or -
double              fstepAxis2;   // amount of steps for Tracking
#define stepAxis2   1


//Targets
PierSide newTargetPierSide = PIER_NOTVALID;

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

#define HADirNCPInit    false
#define HADirSCPInit    true
volatile bool   HADir = HADirNCPInit;

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
bool highPrecision = true;

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

bool abortSlew = false;

// Command processing -------------------------------------------------------------------------------------------------------
#define BAUD 57600

char reply[50];
char command[28];

// serial speed
unsigned long   baudRate[10] =
{
  115200, 56700, 38400, 28800, 19200, 14400, 9600, 4800, 2400, 1200
};



// guide command
#define GuideRateRG   0
#define GuideRateRC   1
#define GuideRateRM   2
#define GuideRateRS   3
#define GuideRateRX   4

#define DefaultR0 1
#define DefaultR1 4
#define DefaultR2 16
#define DefaultR3 64
#define DefaultR4 64
double          guideRates[5] =
{
  DefaultR0 , DefaultR1 , DefaultR2 ,  DefaultR3 , DefaultR4
};

volatile byte   activeGuideRate = GuideRateRS;

volatile byte   guideDirAxis1 = 0;
long            guideDurationAxis1 = -1;
unsigned long   guideDurationLastAxis1 = 0;
volatile byte   guideDirAxis2 = 0;
long            guideDurationAxis2 = -1;
unsigned long   guideDurationLastAxis2 = 0;

long            lasttargetAxis1 = 0;
long            debugv1 = 0;
bool            axis1Enabled = false;
bool            axis2Enabled = false;

double          guideTimerBaseRate = 0;
double          amountGuideAxis1;
double          amountGuideAxis2;

// Reticule control
#ifdef RETICULE_LED_PINS
int             reticuleBrightness = 255;
#endif

long distStepAxis1(long* start, long* end)
{
  return *end - *start;
}
long distStepAxis1(volatile long* start, volatile long* end)
{
  return *end -* start;
}
long distStepAxis1(volatile long* start, volatile double* end)
{
  return *end - *start;
}
long distStepAxis2(long* start, long* end)
{
  return *end - *start;
}
long distStepAxis2(volatile long* start, volatile long* end)
{
  return *end - *start;
}
long distStepAxis2(volatile long* start, volatile double* end)
{
  return *end - *start;
}

void updateDeltaTarget()
{
  cli();
  deltaTargetAxis1 = (long)targetAxis1 - posAxis1;
  deltaTargetAxis2 = (long)targetAxis2 - posAxis2;
  sei();
}
void updateDeltaTargetAxis1()
{
  cli();
  deltaTargetAxis1 = (long)targetAxis1 - posAxis1;
  sei();
}
void updateDeltaTargetAxis2()
{
  cli();
  deltaTargetAxis2 = (long)targetAxis2 - posAxis2;
  sei();
}
bool atTargetAxis1(bool update = false)
{
  if (update)
    updateDeltaTargetAxis1();
  return abs(deltaTargetAxis1) < GA1.breakDist;
}
bool atTargetAxis2(bool update = false)
{
  if (update)
    updateDeltaTargetAxis2();
  return abs(deltaTargetAxis2) < GA2.breakDist;
}
PierSide GetPierSide()
{
  cli(); long pos = posAxis2; sei();
  return -GA2.quaterRot <= pos && pos <= GA2.quaterRot ? PIER_EAST : PIER_WEST;
}

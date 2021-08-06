#pragma once
#include <Arduino.h>
#include <math.h>
#include <TeenAstroCoordConv.hpp>
#include <TeenAstroMath.h>
#include <TinyGPS++.h>
#include "Config.TeenAstro.h"
#include "timerLoop.hpp"
#include "TelTimer.hpp"
#include "Site.hpp"
#include "Command.h"
#include "Config.TeenAstro.h"
#include "EEPROM_adress.h"
#include "XEEPROM.hpp"
#include "Axis.hpp"

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
bool DecayModeTrack = false;
MeridianFlip meridianFlip = FLIP_NEVER;
Mount mountType = MOUNT_TYPE_GEM;
byte maxAlignNumStar = 0;
bool apparentPole = true;
bool hasFocuser = false;
bool hasGNSS = true;
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

double                  maxRate = StepsMaxRate * 16L;
float                   pulseGuideRate = 0.25; //in sideral Speed
double                  DegreesForAcceleration = 3;

double              az_deltaRateScale = 1.;

MotorAxis           motorA1;
MotorAxis           motorA2;

backlash            backlashA1 = { 0,0,0,0 };
backlash            backlashA2 = { 0,0,0,0 };

GeoAxis             geoA1;
GeoAxis             geoA2;

volatile double     timerRateRatio;
StatusAxis          staA1;
StatusAxis          staA2;

PierSide            newTargetPierSide = PIER_NOTVALID;

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
  ERR_MERIDIAN
};
Errors lastError = ERR_NONE;


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
double  guideRates[5] =
{
  DefaultR0 , DefaultR1 , DefaultR2 ,  DefaultR3 , DefaultR4
};

volatile byte   activeGuideRate = GuideRateRS;

GuideAxis guideA1 = { 0,0,0,0,0 };
GuideAxis guideA2 = { 0,0,0,0,0 };

long            lasttargetAxis1 = 0;
long            debugv1 = 0;

double          guideTimerBaseRate = 0;

// Reticule control
#ifdef RETICULE_LED_PINS
int             reticuleBrightness = 255;
#endif

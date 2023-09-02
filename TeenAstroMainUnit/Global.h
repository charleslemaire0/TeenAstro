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
#include "Refraction.hpp"
#include "Axis.hpp"
#include "AxisEncoder.hpp"
#include "TeenAstroLA3.hpp"
#include "TeenAstroCoord_LO.hpp"
#include "TeenAstroCoord_EQ.hpp"
#include "TeenAstroCoord_HO.hpp"
#include "TeenAstroCoord_IN.hpp"

TinyGPSPlus gps;
CoordConv alignment;
bool hasStarAlignment = false;
bool TrackingCompForAlignment = false;

typedef double interval;
typedef double speed;

enum Mount { MOUNT_UNDEFINED, MOUNT_TYPE_GEM, MOUNT_TYPE_FORK, MOUNT_TYPE_ALTAZM, MOUNT_TYPE_FORK_ALT };
enum EncoderSync {ES_OFF, ES_60, ES_30, ES_15, ES_8, ES_4, ES_2, ES_ALWAYS };
enum Pushto {PT_OFF, PT_RADEC, PT_ALTAZ};
enum MeridianFlip { FLIP_NEVER, FLIP_ALIGN, FLIP_ALWAYS };
enum CheckMode { CHECKMODE_GOTO, CHECKMODE_TRACKING };
enum ParkState { PRK_UNPARKED, PRK_PARKING, PRK_PARKED, PRK_FAILED, PRK_UNKNOW };
enum RateCompensation { RC_UNKOWN = -1, RC_NONE, RC_ALIGN_RA, RC_ALIGN_BOTH, RC_FULL_RA, RC_FULL_BOTH };
enum TrackingCompensation {TC_NONE, TC_RA, TC_BOTH};

ParkState parkStatus = ParkState::PRK_UNPARKED;
bool parkSaved = false;
bool homeSaved = false;
bool atHome = true;
bool homeMount = false;
bool DecayModeTrack = false;
MeridianFlip meridianFlip = MeridianFlip::FLIP_NEVER;
Mount mountType = Mount::MOUNT_TYPE_GEM;
char mountName[maxNumMount][15];
bool isMountTypeFix = false;

byte maxAlignNumStar = 0;
bool autoAlignmentBySync = false;

Pushto PushtoStatus = Pushto::PT_OFF;
bool hasFocuser = false;
bool hasGNSS = true;


double temperature = 10;
double pressure = 110;

RefractionFlags doesRefraction;
 
LA3::RefrOpt RefrOptForPole()
{
  return { doesRefraction.forPole, temperature, pressure };
}
LA3::RefrOpt RefrOptForGoto()
{
  return { doesRefraction.forGoto, temperature, pressure };
}
LA3::RefrOpt RefrOptForTracking()
{
  return { doesRefraction.forTracking, temperature, pressure };
}

TrackingCompensation tc = TrackingCompensation::TC_NONE;
// 86164.09 sidereal seconds = 1.00273 clock seconds per sidereal second)
double                  siderealClockSpeed = 997269.5625;
const double            mastersiderealClockSpeed = 997269.5625;

// default = 997269.5625 ticks per sidereal hundredth second, where a tick is 1 uS
// this is stored in XEEPROM.which is updated/adjusted with the ":T+#" and ":T-#" commands
// a higher number here means a longer count which slows down the sidereal clock

const double            masterClockSpeed = 1000000;    // reference frequence for tick
const double            HzCf = masterClockSpeed / 60.0;   // conversion factor to go to/from Hz for sidereal interval
            
bool reboot_unit = false;

interval                minInterval1 = StepsMinInterval;
interval                maxInterval1 = StepsMaxInterval;
interval                minInterval2 = StepsMinInterval;
interval                maxInterval2 = StepsMaxInterval;

float                   pulseGuideRate = 0.25; //in sideral Speed
double                  DegreesForAcceleration = 3;

MotorAxis           motorA1;
MotorAxis           motorA2;

EncoderSync         EncodeSyncMode = EncoderSync::ES_OFF;
EncoderAxis         encoderA1;
EncoderAxis         encoderA2;

GeoAxis             geoA1;
GeoAxis             geoA2;

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
double              underPoleLimitGOTO;                     // maximum allowed hour angle (+/-) under the celestial pole. Telescop will flip the mount and move the Dec. >90 degrees (+/-) once past this limit.  Sometimes used for Fork mounts in Align mode.  Ignored on Alt/Azm mounts.
int                 distanceFromPoleToKeepTrackingOn;       // tracking off 6 hours after transit if sistanceFromPole > distanceFromPoleToKeepTrackingOn

                                                           
//                                                          // If left alone, the mount will stop tracking when it hits this limit.  Valid range is 7 to 11 hours.

#define HADirNCPInit    false
#define HADirSCPInit    true

volatile bool   HADir = HADirNCPInit;

// Status ------------------------------------------------------------------------------------------------------------------
enum ErrorsTraking
{
  ERRT_NONE,
  ERRT_MOTOR_FAULT,
  ERRT_ALT,
  ERRT_LIMIT_SENSE,
  ERRT_AXIS1,
  ERRT_AXIS2,
  ERRT_UNDER_POLE,
  ERRT_MERIDIAN
};

ErrorsTraking lastError = ErrorsTraking::ERRT_NONE;

enum ErrorsGoTo
{
  ERRGOTO_NONE,
  ERRGOTO_BELOWHORIZON,
  ERRGOTO_NOOBJECTSELECTED,
  ERRGOTO_SAMESIDE,
  ERRGOTO_PARKED,
  ERRGOTO_SLEWING,
  ERRGOTO_LIMITS,
  ERRGOTO_GUIDINGBUSY,
  ERRGOTO_ABOVEOVERHEAD,
  ERRGOTO_MOTOR,
  ERRGOTO____,
  ERRGOTO_MOTOR_FAULT,
  ERRGOTO_ALT,
  ERRGOTO_LIMIT_SENSE,
  ERRGOTO_AXIS1,
  ERRGOTO_AXIS2,
  ERRGOTO_UNDER_POLE,
  ERRGOTO_MERIDIAN
};

//Command Precision
bool highPrecision = true;

volatile bool movingTo = false;

bool doSpiral = false;
double SpiralFOV = 1;
// Tracking
#define TrackingStar  1
#define TrackingSolar 0.99726956632
#define TrackingLunar 0.96236513150
bool lastSideralTracking = false;
volatile bool sideralTracking = false;
enum SID_Mode
{
  SIDM_STAR,
  SIDM_SUN,
  SIDM_MOON,
  SIDM_TARGET
};

volatile SID_Mode sideralMode = SID_Mode::SIDM_STAR;

double  RequestedTrackingRateHA = TrackingStar; // in RA seconds per sideral second
double  RequestedTrackingRateDEC = 0; // in DEC seconds per sideral second
long    storedTrakingRateRA = 0;
long    storedTrakingRateDEC = 0;
//Guiding

enum Guiding { GuidingOFF, GuidingPulse, GuidingST4, GuidingRecenter, GuidingAtRate };
volatile Guiding GuidingState = Guiding::GuidingOFF;
Guiding lastGuidingState = Guiding::GuidingOFF;
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

enum GuideRate {RG,RC,RM,RS,RX};
// guide command


#define DefaultR0 1
#define DefaultR1 4
#define DefaultR2 16
#define DefaultR3 64
#ifndef DefaultR4
  #define DefaultR4 600
#endif
double  guideRates[5] =
{
  DefaultR0 , DefaultR1 , DefaultR2 ,  DefaultR3 , DefaultR4
};

volatile byte activeGuideRate = GuideRate::RX;
volatile byte recenterGuideRate = activeGuideRate;

GuideAxis guideA1;
GuideAxis guideA2;

// Reticule control
#ifdef RETICULE_LED_PINS
int             reticuleBrightness = 255;
#endif

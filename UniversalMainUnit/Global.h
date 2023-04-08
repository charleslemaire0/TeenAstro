
#define UNUSED(x) __attribute__((unused)) x


enum MeridianFlip { FLIP_NEVER, FLIP_ALIGN, FLIP_ALWAYS };
enum CheckMode { CHECKMODE_GOTO, CHECKMODE_TRACKING };
enum ParkState { PRK_UNPARKED, PRK_PARKING, PRK_PARKED, PRK_FAILED, PRK_UNKNOW };
enum RateCompensation { RC_UNKOWN = -1, RC_NONE, RC_ALIGN_RA, RC_ALIGN_BOTH, RC_FULL_RA, RC_FULL_BOTH };
enum TrackingCompensation {TC_NONE, TC_RA, TC_BOTH};
enum ErrorsGoTo
{
  ERRGOTO_NONE,
  ERRGOTO_BELOWHORIZON,
  ERRGOTO_NOOBJECTSELECTED,
  ERRGOTO_SAMESIDE,
  ERRGOTO_PARKED,
  ERRGOTO_SLEWING,
  ERRGOTO_LIMITS,
  ERRGOTO_GUIDING,
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
enum ErrorsTracking
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
enum Guiding { GuidingOFF, GuidingPulse, GuidingST4, GuidingRecenter, GuidingAtRate };
enum GuideRate {RG,RC,RM,RS,RXX};
enum SID_Mode
{
  SIDM_STAR,
  SIDM_SUN,
  SIDM_MOON,
  SIDM_TARGET
};

// Tracking
#define TrackingStar  1
#define TrackingSolar 0.99726956632
#define TrackingLunar 0.96236513150

#define DefaultR0 1
#define DefaultR1 4
#define DefaultR2 16
#define DefaultR3 64
#define DefaultR4 600

#include <math.h>
#include "FirmwareDef.h"
#include "HAL_TeenAstro.h"
#include "XEEPROM.hpp"
#include "EEPROM_init.h"
#include "EEPROM_address.h"
#include "timerLoop.hpp"
#include "TelTimer.hpp"
#include "Command.h"
#include "ValueToString.h"
#include "TMCStepper.h"
#include "MotionControl.h"
#include "Mount.h"
#include "StepDir.h"
#include "Mc5160.h"
#include "MotorDriver.h"
#include "Axis.hpp"
#include "Site.hpp"
#include "Refraction.hpp"
#include "TeenAstroCoordConv.hpp"
#include "Limit.h"
#include "Astro.h"
#include "Home.h"
#include "Park.h"
#include "Control.h"
#include "Monitor.h"
#include "Guiding.h"

// TeenAstroMainUnit.h
void initMotors(bool);
void readEEPROMmotor(void);
void writeDefaultEEPROMmotor(void);
void updateRatios(bool, bool);
void updateSidereal(void);
void beginTimers(void);
void SafetyCheck(bool);



#define BAUD 57600


#ifdef TEENASTRO_MAIN_UNIT
#define GLOBAL
#else
#define GLOBAL extern 
#endif


// FreeRTOS constants and structures
#define MON_TASK_PRTY     2
#define CTRL_TASK_PRTY    4
#define CMD_TASK_PRTY     6
#define MON_TASK_PERIOD 100  // milliseconds
#define CMD_TASK_PERIOD  1
#define CTRL_TASK_PERIOD 1 

GLOBAL QueueHandle_t controlQueue;
GLOBAL SemaphoreHandle_t hwMutex;       // to prevent concurrent hardware accesses 
GLOBAL SemaphoreHandle_t swMutex;       // to prevent concurrent global variable accesses 
GLOBAL EventGroupHandle_t mountEvents;  // abort etc.

// One sidereal day is 86164.09 clock seconds
// One sidereal second is 86164.09 / 86400 = 0.9972... clock second
// siderealClockSpeed = number of 1/16µS ticks in one sidereal second = (16 * 1000000 * 0.9972) = 15956313 
#define TICKS_PER_SEC 16000000
#define mastersiderealClockSpeed 15956313.0
#define SIDEREAL_SECOND (mastersiderealClockSpeed / TICKS_PER_SEC)
#define HzCf  (TICKS_PER_SEC / 60.0)   // conversion factor to go to/from Hz for sidereal interval
GLOBAL double  siderealClockSpeed;

GLOBAL char reply[50];
GLOBAL char command[28];

GLOBAL T_Serial S_SHC;
GLOBAL T_Serial S_USB;

GLOBAL MotorDriver motorA1;
GLOBAL MotorDriver motorA2;
GLOBAL GeoAxis geoA1;
GLOBAL GeoAxis geoA2;

GLOBAL ParkState           parkStatus;
GLOBAL Mount               mount;
GLOBAL PierSide            newTargetPierSide;

// Flags
GLOBAL bool highPrecision ;
GLOBAL bool doSpiral ;
GLOBAL bool lastsiderealTracking ;
GLOBAL bool hasStarAlignment ;
GLOBAL RefractionFlags doesRefraction;

GLOBAL bool parkSaved ;
GLOBAL bool homeSaved ;
GLOBAL bool autoAlignmentBySync ;
GLOBAL bool TrackingCompForAlignment ;
GLOBAL bool DecayModeTrack;

GLOBAL TrackingCompensation tc;
GLOBAL volatile SID_Mode   siderealMode;
GLOBAL siteDefinition      localSite;
GLOBAL ErrorsTracking       lastError;

GLOBAL double              newTargetAlt;
GLOBAL double              newTargetAzm;
GLOBAL double              newTargetDec;
GLOBAL double              newTargetRA; 
GLOBAL double              deltaAlt;
GLOBAL HorCoords           currentAzAlt, prevAzAlt;


// Tracking
GLOBAL long                lasttargetAxis1;
GLOBAL double              RequestedTrackingRateHA; // in RA seconds per sidereal second
GLOBAL double              RequestedTrackingRateDEC; // in DEC seconds per sidereal second
GLOBAL long                storedTrackingRateRA;
GLOBAL long                storedTrackingRateDEC;
GLOBAL unsigned long       lastSetTrackingEnable;

//Guiding
GLOBAL volatile Guiding    GuidingState;
GLOBAL Guiding             lastGuidingState;
GLOBAL volatile byte       activeGuideRate;
GLOBAL double              guideTimerBaseRate1;
GLOBAL double              guideTimerBaseRate2;
GLOBAL double  guideRates[5];
GLOBAL GuideAxis           guideA1;
GLOBAL GuideAxis           guideA2;


GLOBAL CoordConv           alignment;
GLOBAL Limits              limits;
GLOBAL DateTimeTimers      rtk;

GLOBAL uint8_t             currentMount;
GLOBAL uint8_t             currentSite;
GLOBAL uint8_t             AxisDriver;
GLOBAL char                mountNames[maxNumMounts][MountNameLen];
GLOBAL bool                reboot_unit;
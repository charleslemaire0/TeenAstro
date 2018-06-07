#pragma once
#include "Config.TeenAstro.h"
#include "TelTimer.h"
#include "Site.h"
#include "FPoint.h"

timerLoop tlp;
GNSSPPS         pps;
DateTimeTimers  rtk;
// Location ----------------------------------------------------------------------------------------------------------------
siteDefinition      localSite;


#define PierSideEast     1
#define PierSideFlipEW1  2
#define PierSideFlipEW2  3
#define PierSideFlipEW3  4
#define PierSideWest     5
#define PierSideFlipWE1  6
#define PierSideFlipWE2  7
#define PierSideFlipWE3  8

#define CheckModeGOTO        0
#define CheckModeTracking    1

#define NotParked   0
#define Parking     1
#define Parked      2
#define ParkFailed  3
byte parkStatus = NotParked;
boolean parkSaved = false;
boolean atHome = true;
boolean homeMount = false;
byte pierSide = PierSideEast;



// 86164.09 sidereal seconds = 1.00273 clock seconds per sidereal second)
long                    siderealInterval = 15956313L;
long                    masterSiderealInterval = siderealInterval;

// default = 15956313 ticks per sidereal hundredth second, where a tick is 1/16 uS
// this is stored in EEPROM which is updated/adjusted with the ":T+#" and ":T-#" commands
// a higher number here means a longer count which slows down the sidereal clock
double                  HzCf = 16000000.0 / 60.0;   // conversion factor to go to/from Hz for sidereal interval
volatile long           SiderealRate;               // based on the siderealInterval, this is the time between steps for sidereal tracking
volatile long           TakeupRate;                 // this is the takeup rate for synchronizing the target and actual positions when needed

// Tracking and rate control
#ifdef MOUNT_TYPE_ALTAZM
#define refraction_enable   false                   // refraction isn't allowed in Alt/Azm mode
#else
#define refraction_enable   true                    // refraction allowed
#endif
#ifdef TRACK_REFRACTION_RATE_DEFAULT_ON
boolean                 refraction = refraction_enable;
#else
boolean                 refraction = false;
#endif
boolean                 onTrack = false;

long                    maxRate = MaxRate * 16L;
volatile long           timerRateAxis1 = 0;
volatile long           timerRateBacklashAxis1 = 0;
volatile boolean        inbacklashAxis1 = false;
boolean                 faultAxis1 = false;
volatile long           timerRateAxis2 = 0;
volatile long           timerRateBacklashAxis2 = 0;
volatile boolean        inbacklashAxis2 = false;
boolean                 faultAxis2 = false;


unsigned int GearAxis1; //2000
unsigned int StepRotAxis1;
byte MicroAxis1;
bool ReverseAxis1;
bool CoolStep1;
unsigned int GearAxis2;//1800
unsigned int StepRotAxis2;
byte MicroAxis2;
bool ReverseAxis2;
bool CoolStep2;
u_int8_t HighCurrAxis1;
u_int8_t LowCurrAxis1;
u_int8_t HighCurrAxis2;
u_int8_t LowCurrAxis2;

#define default_tracking_rate   1
volatile double         trackingTimerRateAxis1 = default_tracking_rate;
volatile double         trackingTimerRateAxis2 = default_tracking_rate;
volatile double         guideTimerRateAxis1 = 0.0;
volatile double         guideTimerRateAxis2 = 0.0;

// backlash control
volatile int    backlashAxis1 = 0;
volatile int    backlashAxis2 = 0;
volatile int    blAxis1 = 0;
volatile int    blAxis2 = 0;

long StepsPerRotAxis1; // calculated as    :  stepper_steps * micro_steps * gear_reduction1 * (gear_reduction2/360)
long StepsPerRotAxis2; // calculated as    :  stepper_steps * micro_steps * gear_reduction1 * (gear_reduction2/360)
double StepsPerDegreeAxis1;
double StepsPerDegreeAxis2;
double StepsPerSecondAxis1;
double StepsPerSecondAxis2;

long halfRotAxis1;
long quaterRotAxis1;
long halfRotAxis2;
long quaterRotAxis2;

long celestialPoleStepAxis1;
long celestialPoleStepAxis2;

volatile double         timerRateRatio;
volatile boolean        useTimerRateRatio;

#define BreakDistAxis1              (2L)
#define BreakDistAxis2              (2L)
volatile double         StepsForRateChangeAxis1 = 0;
volatile double         StepsForRateChangeAxis2 = 0;


IntervalTimer           itimer3;
void                    TIMER3_COMPA_vect(void);

IntervalTimer           itimer4;
void                    TIMER4_COMPA_vect(void);


// fix UnderPoleLimit for fork mounts
#if defined(MOUNT_TYPE_FORK) || defined(MOUNT_TYPE_FORK_ALT)
#undef UnderPoleLimit
#define UnderPoleLimit  12
#endif

// either 0 or (fabs(latitude))
#define AltAzmDecStartPos   (fabs(latitude))

byte newTargetPierSide = 0;

volatile long       posAxis1;    // hour angle position in steps
double              deltaSyncAxis1;
long                trueAxis1;   // correction to above for motor shaft position steps
volatile long       startAxis1;  // hour angle of goto start position in steps
volatile fixed_t    targetAxis1; // hour angle of goto end   position in steps
volatile byte       dirAxis1;    // stepping direction + or -
double              newTargetRA; // holds the RA for goTos
fixed_t             origTargetAxis1;
#if defined(AXIS1_MODE) && defined(AXIS1_MODE_GOTO)
volatile long       stepAxis1 = 1;
#else
#define stepAxis1   1
#endif

volatile long       posAxis2;     // declination position in steps
double              deltaSyncAxis2;
long                trueAxis2;    // correction to above for motor shaft position steps
volatile long       startAxis2;   // declination of goto start position in steps
volatile fixed_t    targetAxis2;  // declination of goto end   position in steps
volatile byte       dirAxis2;     // stepping direction + or -
double              newTargetDec; // holds the Dec for goTos
long                origTargetAxis2;
#if defined(AXIS2_MODE) && defined(AXIS2_MODE_GOTO)
volatile long       stepAxis2 = 1;
#else
#define stepAxis2   1
#endif
double              newTargetAlt = 0.0, newTargetAzm = 0.0; // holds the altitude and azmiuth for slews
double              currentAlt = 45;                        // the current altitude
int                 minAlt;                                 // the minimum altitude, in degrees, for goTo's (so we don't try to point too low)
int                 maxAlt;                                 // the maximum altitude, in degrees, for goTo's (to keep the telescope tube away from the mount/tripod)
bool                autoContinue = false;                   // automatically do a meridian flip and continue when we hit the MinutesPastMeridianW

                                            // Stepper/position/rate ----------------------------------------------------------------------------------------------------
#define CLR(x, y)   (x &= (~(1 << y)))
#define SET(x, y)   (x |= (1 << y))
#define TGL(x, y)   (x ^= (1 << y))

                                            // I set the pin usage to facilitate easy connection of jumper cables
                                            // for now, the #defines below are used to program the port modes using the standard Arduino library
                                            // defines for direct port control

#if defined(AXIS1_DISABLED_HIGH)
#define Axis1_Disabled  HIGH
#define Axis1_Enabled   LOW
#endif
#if defined(AXIS1_DISABLED_LOW)
#define Axis1_Disabled  LOW
#define Axis1_Enabled   HIGH
#endif
#if defined(AXIS2_DISABLED_HIGH)
#define Axis2_Disabled  HIGH
#define Axis2_Enabled   LOW
#endif
#if defined(AXIS2_DISABLED_LOW)
#define Axis2_Disabled  LOW
#define Axis2_Enabled   HIGH
#endif
#define DecDirEInit 1
#define DecDirWInit 0
volatile byte   DecDir = DecDirEInit;
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
  ERR_DEC,
  ERR_AZM,
  ERR_UNDER_POLE,
  ERR_MERIDIAN,
  ERR_SYNC
};

Errors lastError = ERR_NONE;

boolean highPrecision = true;

#define TrackingOFF                0
#define TrackingON                 1
#define TrackingMoveTo             2

#define TrackingSolar 0.99726956632
#define TrackingLunar 0.96236513150

volatile byte trackingState = TrackingOFF;
unsigned long lastSetTrakingEnable = millis();
unsigned long lastSecurityCheck = millis();
byte abortTrackingState = TrackingOFF;
volatile byte lastTrackingState = TrackingOFF;
boolean abortSlew = false;

#define MeridianFlipNever   0
#define MeridianFlipAlign   1
#define MeridianFlipAlways  2
#ifdef MOUNT_TYPE_GEM
byte meridianFlip = MeridianFlipAlways;
#endif
#ifdef MOUNT_TYPE_FORK
byte meridianFlip = MeridianFlipAlign;
#endif
#ifdef MOUNT_TYPE_FORK_ALT
byte meridianFlip = MeridianFlipNever;
#endif
#ifdef MOUNT_TYPE_ALTAZM
byte meridianFlip = MeridianFlipNever;
#endif


// Command processing -------------------------------------------------------------------------------------------------------
#define BAUD    57600

boolean commandError = false;
boolean quietReply = false;

char reply[50];

char command[3];
char parameter[25];
byte bufferPtr = 0;

// for bluetooth/serial 0
char command_serial_zero[25];
char parameter_serial_zero[25];
byte bufferPtr_serial_zero = 0;

char Serial_recv_buffer[256] = "";
volatile byte Serial_recv_tail = 0;
volatile byte Serial_recv_head = 0;
char Serial_xmit_buffer[50] = "";
byte Serial_xmit_index = 0;

// for bluetooth/serial 1
char command_serial_one[25];
char parameter_serial_one[25];
byte bufferPtr_serial_one = 0;

char Serial1_recv_buffer[256] = "";
volatile byte Serial1_recv_tail = 0;
volatile byte Serial1_recv_head = 0;
char Serial1_xmit_buffer[50] = "";
byte Serial1_xmit_index = 0;

// for ethernet
char command_ethernet[25];
char parameter_ethernet[25];
byte bufferPtr_ethernet = 0;

// Misc ---------------------------------------------------------------------------------------------------------------------
#define Rad 57.29577951

// align
#if defined(MOUNT_TYPE_GEM)
#define MAX_NUM_ALIGN_STARS '3'
#elif defined(MOUNT_TYPE_FORK)
#define MAX_NUM_ALIGN_STARS '3'
#elif defined(MOUNT_TYPE_FORK_ALT)
#define MAX_NUM_ALIGN_STARS '1'
#elif defined(MOUNT_TYPE_ALTAZM)
#define MAX_NUM_ALIGN_STARS '3'
#else
#endif

// serial speed
unsigned long   baudRate[10] =
{
  115200, 56700, 38400, 28800, 19200, 14400, 9600, 4800, 2400, 1200
};



// tracking and PEC, fractional steps
fixed_t         fstepAxis1;
fixed_t         fstepAxis2;

// guide command
#define GuideRate1x     2
#define GuideRate16x    5
#define GuideRateMax    9
#define GuideRateNone   255


#define slewRate        (1.0 / (((double) StepsPerDegreeAxis1 * (MaxRate / 1000000.0))) * 3600.0)/15.0
#define halfSlewRate    (slewRate / 2.0)
double          guideRates[10] =
{
  0.25 * 15, 0.5 * 15, 1.0 * 15, 2.0 * 15, 4.0 * 15, 16.0 * 15, 32.0 * 15, 64.0 * 15, halfSlewRate, slewRate
};


//                      .25X .5x 1x 2x 4x  8x 24x 48x half-MaxRate MaxRate
byte            currentGuideRate = GuideRate16x;
byte            currentPulseGuideRate = GuideRate1x;
volatile byte   activeGuideRate = GuideRateNone;

volatile byte   guideDirAxis1 = 0;
long            guideDurationHA = -1;
unsigned long   guideDurationLastHA = 0;
volatile byte   guideDirAxis2 = 0;
long            guideDurationDec = -1;
unsigned long   guideDurationLastDec = 0;

long            lasttargetAxis1 = 0;
long            debugv1 = 0;
boolean         axis1Enabled = false;
boolean         axis2Enabled = false;

double          guideTimerBaseRate = 0;
fixed_t         amountGuideHA;
fixed_t         guideHA;
fixed_t         amountGuideDec;
fixed_t         guideDec;

// Reticule control
#ifdef RETICULE_LED_PINS
int             reticuleBrightness = RETICULE_LED_PINS;
#endif
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


enum Mount { MOUNT_UNDEFINED, MOUNT_TYPE_GEM, MOUNT_TYPE_FORK, MOUNT_TYPE_ALTAZM, MOUNT_TYPE_FORK_ALT};

#define MeridianFlipNever   0
#define MeridianFlipAlign   1
#define MeridianFlipAlways  2


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
byte meridianFlip = MeridianFlipNever;
byte mountType = MOUNT_TYPE_GEM;
byte maxAlignNumStar = 0;

boolean refraction_enable = false;
boolean onTrack = false;

// 86164.09 sidereal seconds = 1.00273 clock seconds per sidereal second)
long                    siderealInterval = 15956313L;
long                    masterSiderealInterval = siderealInterval;

// default = 15956313 ticks per sidereal hundredth second, where a tick is 1/16 uS
// this is stored in EEPROM which is updated/adjusted with the ":T+#" and ":T-#" commands
// a higher number here means a longer count which slows down the sidereal clock
double                  HzCf = 16000000.0 / 60.0;   // conversion factor to go to/from Hz for sidereal interval
volatile long           SiderealRate;               // based on the siderealInterval, this is the time between steps for sidereal tracking
volatile long           TakeupRate;                 // this is the takeup rate for synchronizing the target and actual positions when needed


long                    maxRate = MaxRate * 16L;
float                   pulseGuideRate = 0.25; //in sideral Speed
double                  DegreesForAcceleration = 3;
double                  DegreesForRapidStop = 0.5 *DegreesForAcceleration;

volatile long           timerRateAxis1 = 0;
volatile long           timerRateBacklashAxis1 = 0;
volatile boolean        inbacklashAxis1 = false;
boolean                 faultAxis1 = false;
volatile long           timerRateAxis2 = 0;
volatile long           timerRateBacklashAxis2 = 0;
volatile boolean        inbacklashAxis2 = false;
boolean                 faultAxis2 = false;


#ifdef TRACK_REFRACTION_RATE_DEFAULT_ON
boolean                 refraction = refraction_enable;
#else
boolean                 refraction = false;
#endif

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
long                minutesPastMeridianGOTOE;               // for goto's, how far past the meridian to allow before we do a flip (if on the East side of the pier)- one hour of RA is the default = 60.  Sometimes used for Fork mounts in Align mode.  Ignored on Alt/Azm mounts.
long                minutesPastMeridianGOTOW;               // as above, if on the West side of the pier.  If left alone, the mount will stop tracking when it hits the this limit.  Sometimes used for Fork mounts in Align mode.  Ignored on Alt/Azm mounts.
double              underPoleLimitGOTO;                     // maximum allowed hour angle (+/-) under the celestial pole. OnStep will flip the mount and move the Dec. >90 degrees (+/-) once past this limit.  Sometimes used for Fork mounts in Align mode.  Ignored on Alt/Azm mounts.
//                                                          // If left alone, the mount will stop tracking when it hits this limit.  Valid range is 7 to 11 hours.
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
#define GuidingOFF                 0
#define GuidingPulse               1
#define GuidingRecenter            2

#define TrackingSolar 0.99726956632
#define TrackingLunar 0.96236513150

volatile byte trackingState = TrackingOFF;
volatile byte GuidingState  = GuidingOFF;
unsigned long lastSetTrakingEnable = millis();
unsigned long lastSecurityCheck = millis();
byte abortTrackingState = TrackingOFF;
volatile byte lastTrackingState = TrackingOFF;
boolean abortSlew = false;

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


double          guideRates[10] =
{
  0.25 , 0.5 , 1.0 , 2.0 , 4.0 , 16.0, 32.0 , 64.0, 64.0, 64.0
};

//                      .25X .5x 1x 2x 4x  8x 24x 48x half-MaxRate MaxRate
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

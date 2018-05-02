#pragma once
#include "Config.h"
#include "TelTimer.h"

timerLoop tlp;
GNSSPPS         pps;
DateTimeTimers  rtk;




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

boolean configdone = false;

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


volatile double         timerRateRatio = (StepsPerSecondAxis1 / StepsPerSecondAxis2);
volatile boolean        useTimerRateRatio = (StepsPerRotAxis1 != StepsPerRotAxis2);

#define BreakDistAxis1              (2L)
#define BreakDistAxis2              (2L)
volatile double         StepsForRateChangeAxis1 = 0;
volatile double         StepsForRateChangeAxis2 = 0;


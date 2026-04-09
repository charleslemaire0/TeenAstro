#pragma once
#include "MountTypes.h"

struct MountTracking {
  double siderealClockSpeed;
  TrackingCompensation trackComp;
  bool lastSideralTracking;
  volatile bool sideralTracking;   // used from ISR/timer
  volatile SID_Mode sideralMode;  // used from ISR/timer
  double RequestedTrackingRateHA;
  double RequestedTrackingRateDEC;
  long storedTrakingRateRA;
  long storedTrakingRateDEC;
  unsigned long lastSetTrakingEnable;
  unsigned long lastSecurityCheck;
  bool abortSlew;
  bool doSpiral;
  double SpiralFOV;
  // GOTO_NONE, GOTO_EQ, GOTO_ALTAZ, GOTO_FLIP_PIER_SIDE (CommandEnums.h); :GXAS# byte 100 bits 5-7
  volatile GotoState gotoState;
};

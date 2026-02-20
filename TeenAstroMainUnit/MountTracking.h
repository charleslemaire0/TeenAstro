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
  volatile bool movingTo;  // used from ISR/timer
};

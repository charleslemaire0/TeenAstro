#pragma once
/**
 * Operational limits: altitude, meridian (GOTO/tracking), under-pole, distance-from-pole.
 * Holds limit data and checking logic (pole, meridian, withinLimit). Per-axis min/max in steps
 * are in GeoAxis and MountAxisLimitManager.
 */
#include "MountTypes.h"
#include <TeenAstroMath.h>

class Mount;

class MountLimits {
public:
  explicit MountLimits(class Mount& mount);

  long getMeridianEastLimit() const;
  long getMeridianWestLimit() const;
  bool checkAltitudeLimits(double alt) const;
  bool checkPole(long axis1, long axis2, CheckMode mode) const;
  bool checkMeridian(long axis1, long axis2, CheckMode mode) const;
  bool withinLimit(long axis1, long axis2) const;

  PoleSide getPoleSideFromAxis2(long axis2) const;
  void getAxisPositions(long& axis1, long& axis2) const;

  void initLimit();
  void resetEELimit();
  void forceResetEELimit();
  bool initLimitMinAxis1();
  bool initLimitMaxAxis1();
  bool initLimitMinAxis2();
  bool initLimitMaxAxis2();

  int minAlt;
  int maxAlt;
  long minutesPastMeridianGOTOE;
  long minutesPastMeridianGOTOW;
  double underPoleLimitGOTO;
  int distanceFromPoleToKeepTrackingOn;

private:
  Mount& mount_;
};

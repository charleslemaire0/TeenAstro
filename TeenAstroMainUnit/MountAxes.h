#pragma once
#include "MountTypes.h"
#include "Axis.hpp"
#include "TeenAstroCoord_IN.hpp"
#include <TeenAstroMath.h>

class MountAxes {
public:
  void updateDeltaTarget();
  void updateDeltaStart();
  void enable(bool enable);
  void applyTrackingRates();
  void setAcceleration(double minInterval1, double minInterval2, double degreesForAccel);
  void setSidereal(double siderealClockSpeed, double stepsPerSecond1, double stepsPerSecond2,
    double masterClockSpeed, int backlashRate1, int backlashRate2);

  PoleSide getPoleSideFromAxis2(long axis2) const;
  void getAxisPositions(long& axis1, long& axis2) const;
  void stepToAngle(long Axis1, long Axis2, double* AngleAxis1, double* AngleAxis2, PoleSide* Side) const;
  void angle2Step(double AngleAxis1, double AngleAxis2, PoleSide Side, long* Axis1, long* Axis2) const;
  void syncAxis(const long* axis1, const long* axis2);
  void getInstrDeg(double* A1, double* A2, double* A3) const;
  Coord_IN getInstrTarget() const;
  GeoAxis geoA1;
  GeoAxis geoA2;
  StatusAxis staA1;
  StatusAxis staA2;
};

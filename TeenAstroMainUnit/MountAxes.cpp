#include "MountAxes.h"
#include "MainUnit.h"
#include <Arduino.h>

void MountAxes::updateDeltaTarget()
{
  staA1.updateDeltaTarget();
  staA2.updateDeltaTarget();
}

void MountAxes::updateDeltaStart()
{
  staA1.updateDeltaStart();
  staA2.updateDeltaStart();
}

void MountAxes::enable(bool en)
{
  staA1.enable = en;
  staA2.enable = en;
}

void MountAxes::applyTrackingRates()
{
  cli();
  staA1.CurrentTrackingRate = staA1.RequestedTrackingRate;
  staA2.CurrentTrackingRate = staA2.RequestedTrackingRate;
  staA1.fstep = geoA1.stepsPerCentiSecond * staA1.CurrentTrackingRate;
  staA2.fstep = geoA2.stepsPerCentiSecond * staA2.CurrentTrackingRate;
  sei();
}

void MountAxes::setAcceleration(double minInterval1, double minInterval2, double degreesForAccel)
{
  double Vmax1 = interval2speed(minInterval1);
  double Vmax2 = interval2speed(minInterval2);
  cli();
  staA1.acc = Vmax1 / (4. * degreesForAccel * geoA1.stepsPerDegree) * Vmax1;
  staA2.acc = Vmax2 / (4. * degreesForAccel * geoA2.stepsPerDegree) * Vmax2;
  sei();
}

void MountAxes::setSidereal(double siderealClockSpeed, double stepsPerSecond1, double stepsPerSecond2,
  double masterClockSpeed, int backlashRate1, int backlashRate2)
{
  cli();
  staA1.setSidereal(siderealClockSpeed, stepsPerSecond1, masterClockSpeed, backlashRate1);
  staA2.setSidereal(siderealClockSpeed, stepsPerSecond2, masterClockSpeed, backlashRate2);
  sei();
}

PoleSide MountAxes::getPoleSideFromAxis2(long axis2) const
{
  return -geoA2.quaterRot <= axis2 && axis2 <= geoA2.quaterRot ? POLE_UNDER : POLE_OVER;
}
void MountAxes::getAxisPositions(long& axis1, long& axis2) const
{
  cli();
  axis1 = staA1.pos;
  axis2 = staA2.pos;
  sei();
}
void MountAxes::stepToAngle(long Axis1, long Axis2, double* AngleAxis1, double* AngleAxis2, PoleSide* Side) const
{
  if (Axis2 > geoA2.poleDef)
  {
    Axis2 = geoA2.poleDef - (Axis2 - geoA2.poleDef);
    Axis1 -= geoA1.halfRot;
    *Side = PoleSide::POLE_OVER;
  }
  else if (Axis2 < -geoA2.poleDef)
  {
    Axis2 = -geoA2.poleDef - (Axis2 + geoA2.poleDef);
    Axis1 -= geoA1.halfRot;
    *Side = PoleSide::POLE_OVER;
  }
  else
    *Side = PoleSide::POLE_UNDER;
  *AngleAxis1 = Axis1 / geoA1.stepsPerDegree;
  *AngleAxis2 = Axis2 / geoA2.stepsPerDegree;
}
void MountAxes::angle2Step(double AngleAxis1, double AngleAxis2, PoleSide Side, long* Axis1, long* Axis2) const
{
  *Axis1 = (long)(AngleAxis1 * geoA1.stepsPerDegree);
  *Axis2 = (long)(AngleAxis2 * geoA2.stepsPerDegree);
  if (Side >= POLE_OVER)
  {
    *Axis2 = geoA2.poleDef - (*Axis2 - geoA2.poleDef);
    *Axis1 += geoA1.halfRot;
    while (*Axis1 < -geoA1.halfRot) *Axis1 += geoA1.stepsPerRot;
    while (*Axis1 > geoA1.halfRot) *Axis1 -= geoA1.stepsPerRot;
  }
}
void MountAxes::syncAxis(const long* axis1, const long* axis2)
{
  cli();
  staA1.start = *axis1;
  staA2.start = *axis2;
  staA1.pos = *axis1;
  staA2.pos = *axis2;
  staA1.target = *axis1;
  staA2.target = *axis2;
  sei();
}
void MountAxes::getInstrDeg(double* A1, double* A2, double* A3) const
{
  long axis1, axis2;
  cli();
  axis1 = staA1.pos;
  axis2 = staA2.pos;
  sei();
  *A1 = axis1 / geoA1.stepsPerDegree;
  *A2 = axis2 / geoA2.stepsPerDegree;
  *A3 = 0.0;
}
Coord_IN MountAxes::getInstrTarget() const
{
  cli();
  double Axis1 = staA1.target / geoA1.stepsPerDegree;
  double Axis2 = staA2.target / geoA2.stepsPerDegree;
  double Axis3 = 0.0;
  sei();
  return Coord_IN(Axis3 * DEG_TO_RAD, Axis2 * DEG_TO_RAD, Axis1 * DEG_TO_RAD);
}

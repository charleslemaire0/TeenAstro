// -----------------------------------------------------------------------------------------------------------------------------
// Astronomy related functions
#include "Global.h"
#include "ValueToString.h"


void updateDeltaTarget()
{
  staA1.updateDeltaTarget();
  staA2.updateDeltaTarget();
}

void updateDeltaStart()
{
  staA1.updateDeltaStart();
  staA2.updateDeltaStart();
}

PierSide GetPierSide()
{
  cli(); long pos = staA2.pos; sei();
  return -geoA2.quaterRot <= pos && pos <= geoA2.quaterRot ? PIER_EAST : PIER_WEST;
}

void ApplyTrackingRate()
{
  staA1.CurrentTrackingRate = staA1.RequestedTrackingRate;
  staA2.CurrentTrackingRate = staA2.RequestedTrackingRate;
  staA1.fstep = geoA1.stepsPerCentiSecond * staA1.CurrentTrackingRate;
  staA2.fstep = geoA2.stepsPerCentiSecond * staA2.CurrentTrackingRate;
}

void SetTrackingRate(double rHA, double rDEC)
{
  RequestedTrackingRateHA = rHA;
  RequestedTrackingRateDEC = rDEC;
  computeTrackingRate(true);
}

void computeTrackingRate(bool apply)
{
  //reset SideralMode if it is equal to sideralspeed
  if (RequestedTrackingRateHA == 1 && RequestedTrackingRateDEC == 0)
  {
    sideralMode = SIDM_STAR;
  }
  if (isAltAZ() || sideralMode == SIDM_TARGET)
  {
    do_compensation_calc();
  }
  else if (tc == TC_NONE)
  {
    staA1.RequestedTrackingRate = RequestedTrackingRateHA;
    staA2.RequestedTrackingRate = 0;
  }
  else if (doesRefraction.forTracking || TrackingCompForAlignment)
  {
    do_compensation_calc();
    if (tc == TC_RA)
    {
      staA2.RequestedTrackingRate = 0;
    }
  }
  else
  {
    staA1.RequestedTrackingRate = RequestedTrackingRateHA;
    staA2.RequestedTrackingRate = 0;
  }
  if (apply)
  {
    ApplyTrackingRate();
  }
}

void RateFromMovingTarget(const double &HA_prev, const double &Dec_prev,
  const double &HA_next, const double &Dec_next,
  const double &TimeRange, const PierSide &side, const bool &refr, 
  double &A1_trackingRate, double &A2_trackingRate)
{
  double Axis1_tmp, Axis2_tmp = 0.;
  double Azm_tmp, Alt_tmp = 0.;
  long axis1_before, axis1_after = 0;
  long axis2_before, axis2_after = 0;
  double axis1_delta, axis2_delta = 0;

  EquToHor(HA_prev, Dec_prev, refr, &Azm_tmp, &Alt_tmp, localSite.cosLat(), localSite.sinLat());
  alignment.toAxisDeg(Axis1_tmp, Axis2_tmp, Azm_tmp, Alt_tmp);
  Angle2Step(Axis1_tmp, Axis2_tmp, side, &axis1_before, &axis2_before);

  EquToHor(HA_next, Dec_next, refr, &Azm_tmp, &Alt_tmp, localSite.cosLat(), localSite.sinLat());
  alignment.toAxisDeg(Axis1_tmp, Axis2_tmp, Azm_tmp, Alt_tmp);
  Angle2Step(Axis1_tmp, Axis2_tmp, side, &axis1_after, &axis2_after);

  axis1_delta = distStepAxis1(&axis1_before, &axis1_after) / geoA1.stepsPerDegree;
  while (axis1_delta < -180) axis1_delta += 360.;
  while (axis1_delta >= 180) axis1_delta -= 360.;

  axis2_delta = distStepAxis2(&axis2_before, &axis2_after) / geoA2.stepsPerDegree;
  while (axis2_delta < -180) axis2_delta += 360.;
  while (axis2_delta >= 180) axis2_delta -= 360.;


  A1_trackingRate = 0.5 * axis1_delta * (3600. / (TimeRange * 15));
  A2_trackingRate = 0.5 * axis2_delta * (3600. / (TimeRange * 15));

}

void do_compensation_calc()
{
  // 1 arc-min ahead of and behind the current Equ position, used for rate calculation

  const double TimeRange = 60;
  double HA_prev, HA_next, HA_now = 0.;
  double Dec_prev, Dec_next, Dec_now = 0.;
  double DriftHA, DriftDEC = 0;
  double RateA1,RateA2 = 0;

  // turn off if not tracking at sidereal rate
  if (!sideralTracking)
  {
    staA1.RequestedTrackingRate = 0.;
    staA2.RequestedTrackingRate = 0.;
    return;
  }

  PierSide side_tmp = GetPierSide();
  DriftHA = RequestedTrackingRateHA * TimeRange * 15;
  DriftHA /= 3600;
  DriftDEC = RequestedTrackingRateDEC * TimeRange;
  DriftDEC /= 3600;

  // if moving to a target select target as reference position if not select current position
  if (movingTo)
    getEquTarget(&HA_now, &Dec_now, localSite.cosLat(), localSite.sinLat(), true);
  else
    getEqu(&HA_now, &Dec_now, localSite.cosLat(), localSite.sinLat(), true);

  // look ahead of the position
  HA_prev = HA_now - DriftHA;
  Dec_prev = Dec_now - DriftDEC;

  // look behind the position
  HA_next = HA_now + DriftHA;
  Dec_next = Dec_now + DriftDEC;

  RateFromMovingTarget(HA_prev, Dec_prev, HA_next, Dec_next,
    TimeRange, side_tmp, doesRefraction.forTracking,
    RateA1, RateA2);

  //Limite rate up to 16 time the sidereal speed 
  staA1.RequestedTrackingRate = min(max(RateA1, -16), 16);
  staA2.RequestedTrackingRate = min(max(RateA2, -16), 16);
}

void initMaxRate()
{
  double maxslewEEPROM = XEEPROM.readInt(getMountAddress(EE_maxRate));
  SetRates(maxslewEEPROM);          // set the new acceleration rate
}

// Acceleration rate calculation
void SetRates(double maxslewrate)
{
  // set the new acceleration rate

  double fact1 = masterClockSpeed / geoA1.stepsPerSecond;
  double fact2 = masterClockSpeed / geoA2.stepsPerSecond;
  minInterval1 = max(fact1 / maxslewrate, StepsMinInterval);
  minInterval2 = max(fact2 / maxslewrate, StepsMinInterval);
  double maxslewCorrected = min(fact1 / minInterval1, fact2 / minInterval2);
  if (abs(maxslewrate - maxslewCorrected) > 2)
  {
    XEEPROM.writeInt(getMountAddress(EE_maxRate), (int)maxslewCorrected);
  }
  minInterval1 = fact1 / maxslewCorrected;
  minInterval2 = fact2 / maxslewCorrected;
  guideRates[4] = maxslewCorrected;
  if (guideRates[3] >= maxslewCorrected)
  {
    guideRates[3] = maxslewCorrected/2;
    XEEPROM.write(getMountAddress(EE_Rate3), guideRates[3]);
  }
  resetGuideRate();
  SetAcceleration();
}

void SetAcceleration()
{
  double Vmax1 = interval2speed(minInterval1);
  double Vmax2 = interval2speed(minInterval2);
  cli();
  staA1.acc = Vmax1 / (4. * DegreesForAcceleration * geoA1.stepsPerDegree) * Vmax1;
  staA2.acc = Vmax2 / (4. * DegreesForAcceleration * geoA2.stepsPerDegree) * Vmax2;
  sei();
}

// calculates the tracking speed for move commands
void enableGuideRate(int g)
{
  if (g < 0) g = 0;
  if (g > 4) g = 4;
  if (activeGuideRate != g)
  {
    // if we reset the guide rate it cancels the current pulse guiding
    activeGuideRate = g;
    guideA1.enableAtRate(guideRates[g]);
    guideA2.enableAtRate(guideRates[g]);
  }
}


void enableST4GuideRate()
{
  enableGuideRate(0);
}

void enableRecenterGuideRate()
{
  enableGuideRate(recenterGuideRate);
}

void resetGuideRate()
{
  enableGuideRate(activeGuideRate);
}

bool isAltAZ()
{
  return mountType == MOUNT_TYPE_ALTAZM || mountType == MOUNT_TYPE_FORK_ALT;
}
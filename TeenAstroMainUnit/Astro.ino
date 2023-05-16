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

void RateFromMovingTarget( Coord_EQ &EQprev,  Coord_EQ &EQnext,
  const double &TimeRange, const PierSide &side, const bool &refr, 
  double &A1_trackingRate, double &A2_trackingRate)
{
  
  long axis1_before, axis1_after = 0;
  long axis2_before, axis2_after = 0;
  double axis1_delta, axis2_delta = 0;

  LA3::RefrOpt rop = { doesRefraction.forTracking, 10, 101 };
  
  Coord_IN INprev = EQprev.To_Coord_IN(*localSite.latitude() * DEG_TO_RAD, rop, alignment.Tinv);
  Angle2Step(INprev.Axis1() * RAD_TO_DEG, INprev.Axis2() * RAD_TO_DEG, side, geoA1.poleDef, &axis1_before, &axis2_before);

  Coord_IN INnext = EQnext.To_Coord_IN(*localSite.latitude() * DEG_TO_RAD, rop, alignment.Tinv);
  Angle2Step(INnext.Axis1() * RAD_TO_DEG, INnext.Axis2() * RAD_TO_DEG, side, geoA1.poleDef, &axis1_after, &axis2_after);


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
  double HA_now = 0.;
  double Dec_now = 0.;
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
  {
    Coord_EQ EQ_T = getEquTarget(*localSite.latitude() * DEG_TO_RAD);
    HA_now = EQ_T.Ha() * RAD_TO_DEG;
    Dec_now = EQ_T.Dec() * RAD_TO_DEG;
  }
  else
  {
    Coord_EQ EQ_T = getEqu(*localSite.latitude() * DEG_TO_RAD);
    HA_now = EQ_T.Ha() * RAD_TO_DEG;
    Dec_now = EQ_T.Dec() * RAD_TO_DEG;
  }


  // look ahead of the position
  Coord_EQ EQ_prev(0, (Dec_now - DriftDEC) * DEG_TO_RAD, (HA_now - DriftHA) * DEG_TO_RAD);

  // look behind the position
  Coord_EQ EQ_next(0, (Dec_now + DriftDEC) * DEG_TO_RAD, (HA_now + DriftHA) * DEG_TO_RAD);

  RateFromMovingTarget(EQ_prev, EQ_next,
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
void enableGuideRate(int g, bool force = false)
{
  if (g < 0) g = 0;
  if (g > 4) g = 4;
  if (activeGuideRate != g || force)
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
  enableGuideRate(activeGuideRate, true);
}

bool isAltAZ()
{
  return mountType == MOUNT_TYPE_ALTAZM || mountType == MOUNT_TYPE_FORK_ALT;
}
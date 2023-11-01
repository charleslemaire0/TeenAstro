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

PoleSide GetPoleSide()
{
  long axis1, axis2;
  setAtMount(axis1, axis2);
  return -geoA2.poleDef <= axis2 && axis2 <= geoA2.poleDef ? POLE_UNDER : POLE_OVER;
}

bool TelescopeBusy()
{
  return movingTo || GuidingState != Guiding::GuidingOFF;
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
  double sign = localSite.northHemisphere() ? 1 : -1;

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
    staA1.RequestedTrackingRate = sign * RequestedTrackingRateHA;
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
    staA1.RequestedTrackingRate = sign * RequestedTrackingRateHA;
    staA2.RequestedTrackingRate = 0;
  }
  if (apply)
  {
    ApplyTrackingRate();
  }
}

void RateFromMovingTarget( Coord_EQ &EQprev,  Coord_EQ &EQnext,
  const double &TimeRange, const PoleSide &side, const bool &refr, 
  double &A1_trackingRate, double &A2_trackingRate)
{
  
  long axis1_before, axis1_after = 0;
  long axis2_before, axis2_after = 0;
  double axis1_delta, axis2_delta = 0;

  LA3::RefrOpt rop = { doesRefraction.forTracking, 10, 101 };
  
  Coord_IN INprev = EQprev.To_Coord_IN(*localSite.latitude() * DEG_TO_RAD, rop, alignment.Tinv);
  Angle2Step(INprev.Axis1() * RAD_TO_DEG, INprev.Axis2() * RAD_TO_DEG, side, &axis1_before, &axis2_before);

  Coord_IN INnext = EQnext.To_Coord_IN(*localSite.latitude() * DEG_TO_RAD, rop, alignment.Tinv);
  Angle2Step(INnext.Axis1() * RAD_TO_DEG, INnext.Axis2() * RAD_TO_DEG, side, &axis1_after, &axis2_after);


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

  PoleSide side_tmp = GetPoleSide();
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
  double maxslewEEPROM = XEEPROM.readUShort(getMountAddress(EE_maxRate));
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
    XEEPROM.writeUShort(getMountAddress(EE_maxRate), (int)maxslewCorrected);
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

// safety checks,
// keeps mount from tracking past the meridian limit, past the underPoleLimit,
// below horizon limit, above the overhead limit, or past the Dec limits
void SafetyCheck(const bool forceTracking)
{
  // basic check to see if we're not at home
  PoleSide currentSide = GetPoleSide();
  long axis1, axis2;
  setAtMount(axis1, axis2);

  if (atHome)
    atHome = !sideralTracking;

  if (!geoA1.withinLimit(axis1))
  {
    lastError = ERRT_AXIS1;
    if (movingTo)
      abortSlew = true;
    else if (!forceTracking)
      sideralTracking = false;
    return;
  }
  else if (lastError == ERRT_AXIS1)
  {
    lastError = ERRT_NONE;
  }

  if (!geoA2.withinLimit(axis2))
  {
    lastError = ERRT_AXIS2;
    if (movingTo)
      abortSlew = true;
    else if (!forceTracking)
      sideralTracking = false;
    return;
  }
  else if (lastError == ERRT_AXIS2)
  {
    lastError = ERRT_NONE;
  }

  if (mountType == MOUNT_TYPE_GEM)
  {
    if (!checkMeridian(axis1, axis2, CHECKMODE_TRACKING))
    {
      if ((staA1.dir && currentSide == POLE_OVER) || (!staA1.dir && currentSide == POLE_UNDER))
      {
        lastError = ERRT_MERIDIAN;
        if (movingTo)
        {
          abortSlew = true;
        }
        if (currentSide >= POLE_OVER && !forceTracking)
          sideralTracking = false;
        return;
      }
      else if (lastError == ERRT_MERIDIAN)
      {
        lastError = ERRT_NONE;
      }
    }
    else if (lastError == ERRT_MERIDIAN)
    {
      lastError = ERRT_NONE;
    }

    if (!checkPole(axis1, axis2, CHECKMODE_TRACKING))
    {
      if ((staA1.dir && currentSide == POLE_UNDER) || (!staA1.dir && currentSide == POLE_OVER))
      {
        lastError = ERRT_UNDER_POLE;
        if (movingTo)
          abortSlew = true;
        if (currentSide == POLE_UNDER && !forceTracking)
          sideralTracking = false;
        return;
      }
      else if (lastError == ERRT_UNDER_POLE)
      {
        lastError = ERRT_NONE;
      }
    }
    else if (lastError == ERRT_UNDER_POLE)
    {
      lastError = ERRT_NONE;
    }
  }
  if (atHome && lastError != ERRT_NONE)
  {
    unsetHome();
    syncAtHome();
  }
  if (parkStatus == PRK_PARKED && lastError != ERRT_NONE)
  {
    unsetPark();
    syncAtHome();
  }
}


//enable Axis 
void enable_Axis(bool enable)
{
  if (enable)
  {
    staA1.enable = true;
    staA2.enable = true;
  }
  else
  {
    staA1.enable = false;
    staA2.enable = false;
  }
}

void updateRatios(bool deleteAlignment, bool deleteHP)
{
  cli();
  geoA1.setstepsPerRot((double)motorA1.gear / 1000.0 * motorA1.stepRot * pow(2, motorA1.micro));
  geoA2.setstepsPerRot((double)motorA2.gear / 1000.0 * motorA2.stepRot * pow(2, motorA2.micro));
  staA1.setBacklash_inSteps(motorA1.backlashAmount, geoA1.stepsPerArcSecond);
  staA2.setBacklash_inSteps(motorA2.backlashAmount, geoA2.stepsPerArcSecond);
  sei();

  guideA1.init(&geoA1.stepsPerCentiSecond, guideRates[activeGuideRate]);
  guideA2.init(&geoA2.stepsPerCentiSecond, guideRates[activeGuideRate]);

  initCelestialPole();
  initTransformation(deleteAlignment);
  if (deleteHP)
  {
    unsetPark();
    unsetHome();
  }
  initLimit();
  initHome();
  updateSideral();
  initMaxRate();
}

void updateSideral()
{
  cli();
  staA1.setSidereal(siderealClockSpeed, geoA1.stepsPerSecond, masterClockSpeed, motorA1.backlashRate);
  staA2.setSidereal(siderealClockSpeed, geoA2.stepsPerSecond, masterClockSpeed, motorA2.backlashRate);
  sei();

  SetTrackingRate(default_tracking_rate, 0);

  // initialize the sidereal clock, RA, and Dec
  SetsiderealClockSpeed(siderealClockSpeed);
}


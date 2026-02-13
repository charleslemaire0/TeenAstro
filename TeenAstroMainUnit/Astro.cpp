// -----------------------------------------------------------------------------------------------------------------------------
// Astronomy related functions
#include "Global.h"
#include "ValueToString.h"


void updateDeltaTarget()
{
  mount.staA1.updateDeltaTarget();
  mount.staA2.updateDeltaTarget();
}

void updateDeltaStart()
{
  mount.staA1.updateDeltaStart();
  mount.staA2.updateDeltaStart();
}

PoleSide GetPoleSide()
{
  long axis1, axis2;
  setAtMount(axis1, axis2);
  return -mount.geoA2.poleDef <= axis2 && axis2 <= mount.geoA2.poleDef ? POLE_UNDER : POLE_OVER;
}

PoleSide GetTargetPoleSide()
{
  long axis2;
  cli();
  axis2 = mount.staA2.target;
  sei();
  return getPoleSide(axis2);
}

bool TelescopeBusy()
{
  return mount.movingTo || mount.GuidingState != Guiding::GuidingOFF;
}

void ApplyTrackingRate()
{
  cli();
  mount.staA1.CurrentTrackingRate = mount.staA1.RequestedTrackingRate;
  mount.staA2.CurrentTrackingRate = mount.staA2.RequestedTrackingRate;
  mount.staA1.fstep = mount.geoA1.stepsPerCentiSecond * mount.staA1.CurrentTrackingRate;
  mount.staA2.fstep = mount.geoA2.stepsPerCentiSecond * mount.staA2.CurrentTrackingRate;
  sei();
}

void SetTrackingRate(double rHA, double rDEC)
{
  mount.RequestedTrackingRateHA = rHA;
  mount.RequestedTrackingRateDEC = rDEC;
  computeTrackingRate(true);
}

void computeTrackingRate(bool apply)
{
  //reset SideralMode if it is equal to sideralspeed
  

  if (mount.RequestedTrackingRateHA == 1 && mount.RequestedTrackingRateDEC == 0)
  {
    mount.sideralMode = SIDM_STAR;
  }
  if (isAltAZ())
  {
    do_compensation_calc();
  }
  else if (environment.doesRefraction.forTracking || hasStarAlignment )
  {
    do_compensation_calc();
    if (mount.trackComp == TC_RA)
    {
      mount.staA2.RequestedTrackingRate = 0;
    }
  }
  else
  {
    double sign = localSite.northHemisphere() ? 1 : -1;
    mount.staA1.RequestedTrackingRate = sign * mount.RequestedTrackingRateHA;
    sign = GetTargetPoleSide() == POLE_UNDER ? 1 : -1;
    mount.staA2.RequestedTrackingRate = sign * mount.RequestedTrackingRateDEC/15;
    if (mount.trackComp == TC_RA)
    {
      mount.staA2.RequestedTrackingRate = 0;
    }
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

  LA3::RefrOpt rop = { environment.doesRefraction.forTracking, 10, 101 };
  
  Coord_IN INprev = EQprev.To_Coord_IN(*localSite.latitude() * DEG_TO_RAD, rop, alignment.Tinv);
  Angle2Step(INprev.Axis1() * RAD_TO_DEG, INprev.Axis2() * RAD_TO_DEG, side, &axis1_before, &axis2_before);

  Coord_IN INnext = EQnext.To_Coord_IN(*localSite.latitude() * DEG_TO_RAD, rop, alignment.Tinv);
  Angle2Step(INnext.Axis1() * RAD_TO_DEG, INnext.Axis2() * RAD_TO_DEG, side, &axis1_after, &axis2_after);


  axis1_delta = distStepAxis1(&axis1_before, &axis1_after) / mount.geoA1.stepsPerDegree;
  while (axis1_delta < -180) axis1_delta += 360.;
  while (axis1_delta >= 180) axis1_delta -= 360.;

  axis2_delta = distStepAxis2(&axis2_before, &axis2_after) / mount.geoA2.stepsPerDegree;
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
  if (!mount.sideralTracking)
  {
    mount.staA1.RequestedTrackingRate = 0.;
    mount.staA2.RequestedTrackingRate = 0.;
    return;
  }

  PoleSide side_tmp;
  DriftHA = mount.RequestedTrackingRateHA * TimeRange * 15;
  DriftHA /= 3600;
  DriftDEC = mount.RequestedTrackingRateDEC * TimeRange ;
  DriftDEC /= 3600;

  // if moving to a target select target as reference position if not select current position
  if (mount.movingTo)
  {
    Coord_EQ EQ_T = getEquTarget(*localSite.latitude() * DEG_TO_RAD);
    HA_now = EQ_T.Ha() * RAD_TO_DEG;
    Dec_now = EQ_T.Dec() * RAD_TO_DEG;
    side_tmp = GetTargetPoleSide();
  }
  else
  {
    Coord_EQ EQ_T = getEqu(*localSite.latitude() * DEG_TO_RAD);
    HA_now = EQ_T.Ha() * RAD_TO_DEG;
    Dec_now = EQ_T.Dec() * RAD_TO_DEG;
    side_tmp = GetPoleSide();
  }


  // look ahead of the position
  Coord_EQ EQ_prev(0, (Dec_now - DriftDEC) * DEG_TO_RAD, (HA_now - DriftHA) * DEG_TO_RAD);

  // look behind the position
  Coord_EQ EQ_next(0, (Dec_now + DriftDEC) * DEG_TO_RAD, (HA_now + DriftHA) * DEG_TO_RAD);

  RateFromMovingTarget(EQ_prev, EQ_next,
    TimeRange, side_tmp, environment.doesRefraction.forTracking,
    RateA1, RateA2);

  //Limite rate up to 16 time the sidereal speed 
  mount.staA1.RequestedTrackingRate = min(max(RateA1, -16), 16);
  mount.staA2.RequestedTrackingRate = min(max(RateA2, -16), 16);
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

  double fact1 = masterClockSpeed / mount.geoA1.stepsPerSecond;
  double fact2 = masterClockSpeed / mount.geoA2.stepsPerSecond;
  mount.minInterval1 = max(fact1 / maxslewrate, StepsMinInterval);
  mount.minInterval2 = max(fact2 / maxslewrate, StepsMinInterval);
  double maxslewCorrected = min(fact1 / mount.minInterval1, fact2 / mount.minInterval2);
  if (abs(maxslewrate - maxslewCorrected) > 2)
  {
    XEEPROM.writeUShort(getMountAddress(EE_maxRate), (int)maxslewCorrected);
  }
  mount.minInterval1 = fact1 / maxslewCorrected;
  mount.minInterval2 = fact2 / maxslewCorrected;
  mount.guideRates[4] = maxslewCorrected;
  if (mount.guideRates[3] >= maxslewCorrected)
  {
    mount.guideRates[3] = maxslewCorrected/2;
    XEEPROM.write(getMountAddress(EE_Rate3), mount.guideRates[3]);
  }
  resetGuideRate();
  SetAcceleration();
}

void SetAcceleration()
{
  double Vmax1 = interval2speed(mount.minInterval1);
  double Vmax2 = interval2speed(mount.minInterval2);
  cli();
  mount.staA1.acc = Vmax1 / (4. * mount.DegreesForAcceleration * mount.geoA1.stepsPerDegree) * Vmax1;
  mount.staA2.acc = Vmax2 / (4. * mount.DegreesForAcceleration * mount.geoA2.stepsPerDegree) * Vmax2;
  sei();
}

// calculates the tracking speed for move commands
void enableGuideRate(int g, bool force = false)
{
  if (g < 0) g = 0;
  if (g > 4) g = 4;
  if (mount.activeGuideRate != g || force)
  {
    // if we reset the guide rate it cancels the current pulse guiding
    mount.activeGuideRate = g;
    mount.guideA1.enableAtRate(mount.guideRates[g]);
    mount.guideA2.enableAtRate(mount.guideRates[g]);
  }
}


void enableST4GuideRate()
{
  enableGuideRate(0);
}

void enableRecenterGuideRate()
{
  enableGuideRate(mount.recenterGuideRate);
}

void resetGuideRate()
{
  enableGuideRate(mount.activeGuideRate, true);
}

bool isAltAZ()
{
  return mount.mountType == MOUNT_TYPE_ALTAZM || mount.mountType == MOUNT_TYPE_FORK_ALT;
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

  if (mount.atHome)
    mount.atHome = !mount.sideralTracking;

  if (!mount.geoA1.withinLimit(axis1))
  {
    mount.lastError = ERRT_AXIS1;
    if (mount.movingTo)
      mount.abortSlew = true;
    else if (!forceTracking)
      mount.sideralTracking = false;
    return;
  }
  else if (mount.lastError == ERRT_AXIS1)
  {
    mount.lastError = ERRT_NONE;
  }

  if (!mount.geoA2.withinLimit(axis2))
  {
    mount.lastError = ERRT_AXIS2;
    if (mount.movingTo)
      mount.abortSlew = true;
    else if (!forceTracking)
      mount.sideralTracking = false;
    return;
  }
  else if (mount.lastError == ERRT_AXIS2)
  {
    mount.lastError = ERRT_NONE;
  }

  if (mount.mountType == MOUNT_TYPE_GEM)
  {
    if (!checkMeridian(axis1, axis2, CHECKMODE_TRACKING))
    {
      if ((mount.staA1.dir && currentSide == POLE_OVER) || (!mount.staA1.dir && currentSide == POLE_UNDER))
      {
        mount.lastError = ERRT_MERIDIAN;
        if (mount.movingTo)
        {
          mount.abortSlew = true;
        }
        if (currentSide >= POLE_OVER && !forceTracking)
          mount.sideralTracking = false;
        return;
      }
      else if (mount.lastError == ERRT_MERIDIAN)
      {
        mount.lastError = ERRT_NONE;
      }
    }
    else if (mount.lastError == ERRT_MERIDIAN)
    {
      mount.lastError = ERRT_NONE;
    }

    if (!checkPole(axis1, axis2, CHECKMODE_TRACKING))
    {
      if ((mount.staA1.dir && currentSide == POLE_UNDER) || (!mount.staA1.dir && currentSide == POLE_OVER))
      {
        mount.lastError = ERRT_UNDER_POLE;
        if (mount.movingTo)
          mount.abortSlew = true;
        if (currentSide == POLE_UNDER && !forceTracking)
          mount.sideralTracking = false;
        return;
      }
      else if (mount.lastError == ERRT_UNDER_POLE)
      {
        mount.lastError = ERRT_NONE;
      }
    }
    else if (mount.lastError == ERRT_UNDER_POLE)
    {
      mount.lastError = ERRT_NONE;
    }
  }
  if (mount.atHome && mount.lastError != ERRT_NONE)
  {
    unsetHome();
    syncAtHome();
  }
  if (mount.parkStatus == PRK_PARKED && mount.lastError != ERRT_NONE)
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
    mount.staA1.enable = true;
    mount.staA2.enable = true;
  }
  else
  {
    mount.staA1.enable = false;
    mount.staA2.enable = false;
  }
}

void updateRatios(bool deleteAlignment, bool deleteHP)
{

  if (mount.enableMotor)
  {
    cli();
    mount.geoA1.setstepsPerRot((double)mount.motorA1.gear / 1000.0 * mount.motorA1.stepRot * pow(2, mount.motorA1.micro));
    mount.geoA2.setstepsPerRot((double)mount.motorA2.gear / 1000.0 * mount.motorA2.stepRot * pow(2, mount.motorA2.micro));
    mount.staA1.setBacklash_inSteps(mount.motorA1.backlashAmount, mount.geoA1.stepsPerArcSecond);
    mount.staA2.setBacklash_inSteps(mount.motorA2.backlashAmount, mount.geoA2.stepsPerArcSecond);
    mount.staA1.target = mount.staA1.pos;
    mount.staA2.target = mount.staA2.pos;
    sei();
  }
  else
  {
    cli();
    mount.geoA1.setstepsPerRot(mount.encoderA1.pulsePerDegree * 360);
    mount.geoA2.setstepsPerRot(mount.encoderA2.pulsePerDegree * 360);
    mount.staA1.setBacklash_inSteps(mount.motorA1.backlashAmount, mount.geoA1.stepsPerArcSecond);
    mount.staA2.setBacklash_inSteps(mount.motorA2.backlashAmount, mount.geoA2.stepsPerArcSecond);
    mount.staA1.target = mount.staA1.pos;
    mount.staA2.target = mount.staA2.pos;
    sei();
  }


  mount.guideA1.init(&mount.geoA1.stepsPerCentiSecond, mount.guideRates[mount.activeGuideRate]);
  mount.guideA2.init(&mount.geoA2.stepsPerCentiSecond, mount.guideRates[mount.activeGuideRate]);

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
  mount.staA1.setSidereal(mount.siderealClockSpeed, mount.geoA1.stepsPerSecond, masterClockSpeed, mount.motorA1.backlashRate);
  mount.staA2.setSidereal(mount.siderealClockSpeed, mount.geoA2.stepsPerSecond, masterClockSpeed, mount.motorA2.backlashRate);
  sei();

  SetTrackingRate(default_tracking_rate, 0);

  // initialize the sidereal clock, RA, and Dec
  SetsiderealClockSpeed(mount.siderealClockSpeed);
}


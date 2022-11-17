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

// staA.trackingTimerRate are x the sidereal rate
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

void do_compensation_calc()
{
  // 1 arc-min ahead of and behind the current Equ position, used for rate calculation

  const double TimeRange = 60;
  long axis1_before, axis1_after = 0;
  long axis2_before, axis2_after = 0;
  double Axis1_tmp, Axis2_tmp = 0.;
  double HA_tmp, HA_now = 0.;
  double Dec_tmp, Dec_now = 0.;
  double Azm_tmp, Alt_tmp = 0.;
  double DriftHA = 0;
  double DriftDEC = 0;
  double axis1_delta, axis2_delta = 0;

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
  HA_tmp = HA_now - DriftHA;
  Dec_tmp = Dec_now - DriftDEC;
  EquToHor(HA_tmp, Dec_tmp, doesRefraction.forTracking, &Azm_tmp, &Alt_tmp, localSite.cosLat(), localSite.sinLat());
  alignment.toInstrumentalDeg(Axis1_tmp, Axis2_tmp, Azm_tmp, Alt_tmp);
  InstrtoStep(Axis1_tmp, Axis2_tmp, side_tmp, &axis1_before, &axis2_before);

  // look behind the position
  HA_tmp = HA_now + DriftHA;
  Dec_tmp = Dec_now + DriftDEC;

  EquToHor(HA_tmp, Dec_tmp, doesRefraction.forTracking, &Azm_tmp, &Alt_tmp, localSite.cosLat(), localSite.sinLat());
  alignment.toInstrumentalDeg(Axis1_tmp, Axis2_tmp, Azm_tmp, Alt_tmp);
  InstrtoStep(Axis1_tmp, Axis2_tmp, side_tmp, &axis1_after, &axis2_after);

  axis1_delta = distStepAxis1(&axis1_before, &axis1_after) / geoA1.stepsPerDegree;
  while (axis1_delta < -180) axis1_delta += 360.;
  while (axis1_delta >= 180) axis1_delta -= 360.;

  axis2_delta = distStepAxis2(&axis2_before, &axis2_after) / geoA2.stepsPerDegree;
  while (axis2_delta < -180) axis2_delta += 360.;
  while (axis2_delta >= 180) axis2_delta -= 360.;

  staA1.RequestedTrackingRate = 0.5 * axis1_delta * (3600. / (TimeRange * 15));
  staA2.RequestedTrackingRate = 0.5 * axis2_delta * (3600. / (TimeRange * 15));


  //Limite rate up to 8 time the sidereal speed 
  //staA1.RequestedTrackingRate = min(max(staA1.RequestedTrackingRate, -8), 8);
  //staA2.RequestedTrackingRate = min(max(staA2.RequestedTrackingRate, -8), 8);
}

void initMaxRate()
{
  double maxslewEEPROM = XEEPROM.readInt(EE_maxRate);
  SetRates(maxslewEEPROM);          // set the new acceleration rate
}

// Acceleration rate calculation
void SetRates(double maxslewrate)
{
  // set the new acceleration rate

  double fact1 = masterClockRate / geoA1.stepsPerSecond;
  double fact2 = masterClockRate / geoA2.stepsPerSecond;
  minInterval1 = max(fact1 / maxslewrate, StepsMinInterval );
  minInterval2 = max(fact2 / maxslewrate, StepsMinInterval );
  double maxslewCorrected = min(fact1 / minInterval1, fact2 / minInterval2);
  if (abs(maxslewrate - maxslewCorrected) > 2)
  {
    XEEPROM.writeInt(EE_maxRate, (int)maxslewCorrected);
  }
  minInterval1 = fact1 / maxslewCorrected;
  minInterval2 = fact2 / maxslewCorrected;
  guideRates[4] = maxslewCorrected;
  if (guideRates[3] >= maxslewCorrected)
  {
    guideRates[3] = 64;
    XEEPROM.write(EE_Rate3, guideRates[3]);
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
void enableGuideRate(int g, bool force)
{
  // don't do these lengthy calculations unless we have to

  if (g < 0) g = 0;
  if (g > 4) g = 4;
  if (!force && (guideTimerBaseRate1 == guideRates[g] && guideTimerBaseRate2 == guideTimerBaseRate1)) return;

  activeGuideRate = g;

  // this enables the guide rate
  guideTimerBaseRate1 = guideRates[g];
  guideTimerBaseRate2 = guideTimerBaseRate1;
  cli();
  guideA1.amount = guideTimerBaseRate1 * geoA1.stepsPerCentiSecond;
  guideA2.amount = guideTimerBaseRate2 * geoA2.stepsPerCentiSecond;
  sei();
}

void enableGuideAtRate(int axis, double rate)
{
  if (axis == 1 && guideTimerBaseRate1 != rate)
  {
    guideTimerBaseRate1 = rate;
    cli();
    guideA1.amount = guideTimerBaseRate1 * geoA1.stepsPerCentiSecond;
    sei();
  }
  else if (axis == 2 && guideTimerBaseRate2 != rate)
  {
    guideTimerBaseRate2 = rate;
    cli();
    guideA2.amount = guideTimerBaseRate2 * geoA2.stepsPerCentiSecond;
    sei();
  }
}

void enableST4GuideRate()
{
  if (guideTimerBaseRate1 != guideRates[0] || guideTimerBaseRate2 != guideTimerBaseRate1)
  {
    guideTimerBaseRate1 = guideRates[0];
    guideTimerBaseRate2 = guideTimerBaseRate1;
    cli();
    guideA1.amount = guideTimerBaseRate1 * geoA1.stepsPerCentiSecond;
    guideA2.amount = guideTimerBaseRate2 * geoA2.stepsPerCentiSecond;
    sei();
  }
}

void resetGuideRate()
{
  enableGuideRate(activeGuideRate, true);
}

void enableRateAxis1(double vRate)
{
  cli();
  guideA1.amount = abs(vRate) * geoA1.stepsPerCentiSecond;
  guideA1.timerRate = vRate;
  sei();
}

void enableRateAxis2(double vRate)
{
  cli();
  guideA2.amount = abs(vRate) * geoA2.stepsPerCentiSecond;
  guideA2.timerRate = vRate;
  sei();
}

bool isAltAZ()
{
  return mountType == MOUNT_TYPE_ALTAZM || mountType == MOUNT_TYPE_FORK_ALT;
}
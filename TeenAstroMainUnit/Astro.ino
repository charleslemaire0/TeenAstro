// -----------------------------------------------------------------------------------------------------------------------------
// Astronomy related functions
#include "Global.h"

#include "ValueToString.h"


void updateDeltaTarget()
{
  cli();
  staA1.deltaTarget = (long)staA1.target - staA1.pos;
  staA2.deltaTarget = (long)staA2.target - staA2.pos;
  sei();
}

void updateDeltaTargetAxis1()
{
  cli();
  staA1.deltaTarget = (long)staA1.target - staA1.pos;
  sei();
}

void updateDeltaTargetAxis2()
{
  cli();
  staA2.deltaTarget = (long)staA2.target - staA2.pos;
  sei();
}

bool atTargetAxis1(bool update = false)
{
  if (update)
    updateDeltaTargetAxis1();
  return geoA1.atTarget(staA1.deltaTarget);
}

bool atTargetAxis2(bool update = false)
{
  if (update)
    updateDeltaTargetAxis2();
  return geoA2.atTarget(staA2.deltaTarget);
}

PierSide GetPierSide()
{
  cli(); long pos = staA2.pos; sei();
  return -geoA2.quaterRot <= pos && pos <= geoA2.quaterRot ? PIER_EAST : PIER_WEST;
}

// staA1.trackingTimerRate/2 are x the sidereal rate
void SetDeltaTrackingRate()
{
  staA1.trackingTimerRate = staA1.az_delta / 15.;
  staA2.trackingTimerRate = staA2.az_delta / 15.;

  staA1.fstep = geoA1.stepsPerCentiSecond * staA1.trackingTimerRate;
  staA2.fstep = geoA2.stepsPerCentiSecond * staA2.trackingTimerRate;
}

void SetTrackingRate(double r)
{
  az_deltaRateScale = r;
  if (!isAltAZ())
  {
    staA1.az_delta = r * 15.;
    staA2.az_delta = 0.;
  }
  SetDeltaTrackingRate();
}

double GetTrackingRate()
{
  return az_deltaRateScale;
}

#define AltAzTrackingRange  1  // distance in arc-min (20) ahead of and behind the current Equ position, used for rate calculation
bool do_compensation_calc()
{
  bool done = false;

  static long axis1_before, axis1_after = 0;
  static long axis2_before, axis2_after = 0;
  static double Axis1_tmp, Axis2_tmp = 0.;
  static double HA_tmp, HA_now = 0.;
  static double Dec_tmp, Dec_now = 0.;
  static double Azm_tmp, Alt_tmp = 0.;
  static int    az_step = 0;

  // turn off if not tracking at sidereal rate
  if (!sideralTracking)
  {
    staA1.az_delta = 0.;
    staA2.az_delta = 0.;
    az_step = 0;
    return true;
  }
  az_step++;
  // convert units, get ahead of and behind current position
  switch (az_step)
  {
  case 1:
    if (movingTo)
      getEquTarget(&HA_now, &Dec_now, localSite.cosLat(), localSite.sinLat(), true);
    else
      getEqu(&HA_now, &Dec_now, localSite.cosLat(), localSite.sinLat(), true);
    break;
  case 10:
    // look ahead of the current position
    HA_tmp = (HA_now - (AltAzTrackingRange / 60.));
    Dec_tmp = Dec_now;
    break;
  case 15:
    EquToHorApp(HA_tmp, Dec_tmp, &Azm_tmp, &Alt_tmp, localSite.cosLat(), localSite.sinLat());
    alignment.toInstrumentalDeg(Axis1_tmp, Axis2_tmp, Azm_tmp, Alt_tmp);
    InstrtoStep(Axis1_tmp, Axis2_tmp, GetPierSide(), &axis1_before, &axis2_before);
    break;
  case 110:
    // look behind the current position
    HA_tmp = (HA_now + (AltAzTrackingRange / 60.));
    Dec_tmp = Dec_now;
    break;
  case 115:
    EquToHorApp(HA_tmp, Dec_tmp, &Azm_tmp, &Alt_tmp, localSite.cosLat(), localSite.sinLat());
    alignment.toInstrumentalDeg(Axis1_tmp, Axis2_tmp, Azm_tmp, Alt_tmp);
    InstrtoStep(Axis1_tmp, Axis2_tmp, GetPierSide(), &axis1_after, &axis2_after);
    break;
  case 120:
    // we have both -0.5hr and +0.5hr values // calculate tracking rate deltas'
               // handle coordinate wrap
    if ((axis1_after < -geoA1.halfRot) && (axis1_before > geoA1.halfRot)) axis1_after += 2 * geoA1.halfRot;
    if ((axis1_before < -geoA1.halfRot) && (axis1_after > geoA1.halfRot)) axis1_after += 2 * geoA1.halfRot;
    // set rates

    staA1.az_delta = (distStepAxis1(&axis1_before, &axis1_after) / geoA1.stepsPerDegree * (15. / (AltAzTrackingRange / 60.)) / 2.) * az_deltaRateScale;
    staA2.az_delta = (distStepAxis2(&axis2_before, &axis2_after) / geoA2.stepsPerDegree * (15. / (AltAzTrackingRange / 60.)) / 2.) * az_deltaRateScale;
    // override for special case of near a celestial pole
    //if (90.0 - fabs(Dec_now) <= 0.5)
    //{
    //  staA1.az_delta = 0.0;
    //  staA2.az_delta = 0.0;
    //}
    break;
  case 200:
    az_step = 0;
    done = true;
    break;
  }
  return done;
}

void initMaxRate()
{
  double maxslewEEPROM = XEEPROM.readInt(EE_maxRate);
  double maxslewCorrected = SetRates(maxslewEEPROM);          // set the new acceleration rate
  if (abs(maxslewEEPROM - maxslewCorrected) > 2)
  {
    XEEPROM.writeInt(EE_maxRate, (int)maxslewCorrected);
  }
}

// Acceleration rate calculation
double SetRates(double maxslewrate)
{
  // set the new acceleration rate
  double stpdg = min(geoA1.stepsPerDegree, geoA2.stepsPerDegree);
  double fact = 3600. / 15. * 1. / (stpdg * 1. / 16. / 1000000.);
  maxRate = max(fact / maxslewrate, (MaxRate / 2L) * 16L);
  maxslewrate = fact / maxRate;
  guideRates[4] = maxslewrate;
  if (guideRates[3] >= maxslewrate)
  {
    guideRates[3] = 64;
    XEEPROM.write(EE_Rate3, guideRates[3]);
  }
  resetGuideRate();
  SetAcceleration();
  return maxslewrate;
}

void SetAcceleration()
{
  double Vmax = getV(maxRate);
  cli();
  staA1.acc = Vmax / (2. * DegreesForAcceleration * geoA1.stepsPerDegree)*Vmax;
  staA2.acc = Vmax / (2. * DegreesForAcceleration * geoA2.stepsPerDegree)*Vmax;
  sei();
}

// calculates the tracking speed for move commands
void enableGuideRate(int g, bool force)
{
  // don't do these lengthy calculations unless we have to

  if (g < 0) g = 0;
  if (g > 4) g = 4;
  if (!force && guideTimerBaseRate == guideRates[g]) return;

  activeGuideRate = g;

  // this enables the guide rate
  guideTimerBaseRate = guideRates[g];

  cli();
  guideA1.amount = guideTimerBaseRate * geoA1.stepsPerCentiSecond;
  guideA2.amount = guideTimerBaseRate * geoA2.stepsPerCentiSecond;
  sei();
}

void enableST4GuideRate()
{
  if (guideTimerBaseRate != guideRates[0])
  {
    guideTimerBaseRate = guideRates[0];
    cli();
    guideA1.amount = guideTimerBaseRate * geoA1.stepsPerCentiSecond;
    guideA2.amount = guideTimerBaseRate * geoA2.stepsPerCentiSecond;
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
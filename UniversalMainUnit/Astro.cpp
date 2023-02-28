// -----------------------------------------------------------------------------------------------------------------------------
// Astronomy related functions
#include "Global.h"
#include "ValueToString.h"



void computeTrackingRate(bool apply)
{  
#if 0  
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
#endif  
}

void do_compensation_calc()
{
#if 0
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
  double axis1_delta, axis2_delta= 0;

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

  staA1.RequestedTrackingRate = 0.5 * axis1_delta * (3600. / (TimeRange*15));
  staA2.RequestedTrackingRate = 0.5 * axis2_delta * (3600. / (TimeRange*15));


  //Limite rate up to 8 time the sidereal speed 
  //staA1.RequestedTrackingRate = min(max(staA1.RequestedTrackingRate, -8), 8);
  //staA2.RequestedTrackingRate = min(max(staA2.RequestedTrackingRate, -8), 8);
#endif
}


/*
 * initMaxSpeed
 * Read max speed from EEPROM, correct if it exceeds processor capability
 * and write it back to EEPROM 
 */
void initMaxSpeed()
{
  long maxSlewEEPROM = XEEPROM.readInt(EE_maxRate);     // in multiples of sidereal speed

  double maxAxis1Speed = geoA1.stepsPerSecond * maxSlewEEPROM;
  double maxAxis2Speed = geoA2.stepsPerSecond * maxSlewEEPROM;

  mount.maxSpeed = fmin(maxAxis1Speed, fmin(maxAxis2Speed, MAX_TEENASTRO_SPEED)); // steps per second
  long maxSlewCorrected = mount.maxSpeed / geoA1.stepsPerSecond; 

  // If the speed read from the EEPROM is too high, correct it
  if (maxSlewEEPROM > maxSlewCorrected)
  {
    XEEPROM.writeInt(EE_maxRate, (int) (maxSlewCorrected));
  }
  guideRates[4] = maxSlewEEPROM;
  if (guideRates[3] >= maxSlewEEPROM)   // also correct the next higher speed??
  {
    guideRates[3] = 64;
    XEEPROM.write(EE_Rate3, guideRates[3]);
  }
  SetAcceleration();
}

/*
 *  Acceleration in steps per second^2 is:
 *  square of speed (in steps/sec) / 2 * accelDistance (in steps) 
 */
void SetAcceleration()
{
  double aMax1, aMax2;
  aMax1 = (pow(mount.maxSpeed, 2)) / (2 * mount.DegreesForAcceleration * geoA1.stepsPerDegree);
  motorA1.setAmax(aMax1);

  aMax2 = (pow(mount.maxSpeed, 2)) / (2 * mount.DegreesForAcceleration * geoA2.stepsPerDegree);
  motorA2.setAmax(aMax2);
}

// calculates the slew speed for move commands
void enableGuideRate(int g, bool force)
{
  if (g < 0) g = 0;
  if (g > 4) g = 4;
  if (!force && (guideTimerBaseRate1 == guideRates[g] && guideTimerBaseRate2 == guideTimerBaseRate1)) return;

  activeGuideRate = g;  // global variable

  unsigned msg[CTL_MAX_MESSAGE_SIZE];
  msg[0] = CTL_MSG_SET_SLEW_SPEED;
  memcpy (&msg[1], &guideRates[g], sizeof(double)); 
  xQueueSend( controlQueue, &msg, 0);
}

bool isAltAz()
{
  return mount.mP->type == MOUNT_TYPE_ALTAZM || mount.mP->type == MOUNT_TYPE_FORK_ALT;
}

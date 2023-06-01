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



/*
 * initMaxSpeed
 * Read max speed from EEPROM, correct if it exceeds processor capability
 * and write it back to EEPROM 
 */
void initMaxSpeed()
{
  long maxSlewEEPROM = XEEPROM.readInt(getMountAddress(EE_maxRate));     // in multiples of sidereal speed

  double maxAxis1Speed = geoA1.stepsPerSecond * maxSlewEEPROM;
  double maxAxis2Speed = geoA2.stepsPerSecond * maxSlewEEPROM;

#if 0
	// This is not needed with Motion Controller mode - disable for the time being
  mount.maxSpeed = fmin(maxAxis1Speed, fmin(maxAxis2Speed, MAX_TEENASTRO_SPEED)); // steps per second
  long maxSlewCorrected = mount.maxSpeed / geoA1.stepsPerSecond; 
  // If the speed read from the EEPROM is too high, correct it
  if (maxSlewEEPROM > maxSlewCorrected)
  {
    XEEPROM.writeInt(getMountAddress(EE_maxRate), (int) (maxSlewCorrected));
  }
#else
  mount.maxSpeed = fmin(maxAxis1Speed, maxAxis2Speed); // steps per second  
#endif  

  guideRates[RXX] = maxSlewEEPROM;
  if (guideRates[RS] >= maxSlewEEPROM)   // also correct the next higher speed??
  {
    guideRates[RS] = 64;
    XEEPROM.write(getMountAddress(EE_Rate3), guideRates[3]);
  }
 setSlewSpeed(guideRates[RXX]);
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
void enableGuideRate(int g)
{
  if (g < 0) g = 0;
  if (g > 4) g = 4;
  activeGuideRate = g;  // global variable

  unsigned msg[CTL_MAX_MESSAGE_SIZE];
  msg[0] = CTL_MSG_SET_SLEW_SPEED;
  memcpy (&msg[1], &guideRates[g], sizeof(double)); 
  xQueueSend( controlQueue, &msg, 0);
}

void enableST4GuideRate()
{
  enableGuideRate(0);
}

void resetGuideRate()
{
  enableGuideRate(activeGuideRate);
}
bool isAltAz()
{
  return mount.mP->type == MOUNT_TYPE_ALTAZM || mount.mP->type == MOUNT_TYPE_FORK_ALT;
}

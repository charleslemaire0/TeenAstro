// -----------------------------------------------------------------------------------------------------------------------------
// Astronomy related functions
#include "Global.h"
#include "ValueToString.h"



/*
 * initMaxSpeed
 * Read max speed from EEPROM, correct if it exceeds processor capability
 * and write it back to EEPROM 
 */
void initMaxSpeed()
{
  long maxSlewEEPROM = XEEPROM.readInt(getMountAddress(EE_maxRate));     // in multiples of sidereal speed

  long maxStepsPerSecond = fmax(geoA1.stepsPerSecond, geoA2.stepsPerSecond);
  mount.maxSpeed = maxSlewEEPROM * maxStepsPerSecond;

  // For StepDir drivers only, if the speed read from the EEPROM is too high for the processor, correct it
  if ((motorA1.drvType == DRV_STEPDIR) || (motorA2.drvType == DRV_STEPDIR))
  {
    if (mount.maxSpeed > MAX_TEENASTRO_SPEED)
    {
      mount.maxSpeed = MAX_TEENASTRO_SPEED;
      maxSlewEEPROM = MAX_TEENASTRO_SPEED / maxStepsPerSecond;
      XEEPROM.writeInt(getMountAddress(EE_maxRate), (int) (maxSlewEEPROM));
    }
  } 

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

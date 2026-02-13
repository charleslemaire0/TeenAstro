// -----------------------------------------------------------------------------------
// functions related to Homing the mount
#include "Global.h"

bool setHome()
{
  if ((mount.parkStatus == PRK_UNPARKED) && !mount.movingTo)
  {
    mount.lastSideralTracking = mount.sideralTracking;
    mount.sideralTracking = false;

    // don't worry about moving around: during parking pec is turned off and backlash is cleared (0) so that mount.staA1.target/targetAxis2=mount.staA1.pos/mount.staA2.pos
    // this should handle getting us back to the home position for micro-step modes up to 256X
    // if sync anywhere is enabled use the corrected location

    long h = (mount.staA1.target / 1024L) * 1024L;
    long d = (mount.staA2.target / 1024L) * 1024L;

    h /= pow(2, mount.motorA1.micro);
    d /= pow(2, mount.motorA2.micro);
    // store our position
    XEEPROM.writeLong(getMountAddress(EE_homePosAxis1), h);
    XEEPROM.writeLong(getMountAddress(EE_homePosAxis2), d);
    XEEPROM.write(getMountAddress(EE_homeSaved), 1);
    initHome();
    mount.sideralTracking = mount.lastSideralTracking;
    return true;
  }

  return false;
}

// unset home position flag
void unsetHome()
{
  XEEPROM.update(getMountAddress(EE_homeSaved), 0);
  initHome();
}

// moves telescope to the home position, then stops tracking
bool goHome()
{
  if (!mount.enableMotor) return false;
  if ((mount.parkStatus != PRK_UNPARKED) && (mount.parkStatus != PRK_PARKING)) return false; // fail, moving to home not allowed if PRK_PARKED
  if (mount.lastError != ERRT_NONE) return false;                                // fail, cannot move if there are errors
  if (TelescopeBusy()) return false;                     
  // stop tracking
  mount.lastSideralTracking = false;
  mount.sideralTracking = false;
  GotoAxis(&mount.geoA1.homeDef, &mount.geoA2.homeDef);
  mount.homeMount = true;
  return true;
}


void finalizeHome()
{ 
  if (mount.backlashStatus == DONE)
  {
    mount.backlashStatus = INIT;
  }
  parkClearBacklash();
  if (mount.backlashStatus == DONE)
  {
    mount.homeMount = false;
    mount.movingTo = false;
    syncAtHome();
    // disable the stepper drivers
    enable_Axis(false);
  }
}

// resets telescope home position; user manually moves home position,
bool syncAtHome()
{
  if (TelescopeBusy()) return false;  // fail, forcing home not allowed during a move
  // default values for state variables
  mount.staA2.dir = true;
  mount.staA1.dir = true;
  mount.newTargetRA = 0;
  mount.newTargetDec = 0;
  mount.newTargetAlt = 0;
  mount.newTargetAzm = 0;
  mount.lastError = ErrorsTraking::ERRT_NONE;
  // reset tracking and rates
  mount.staA1.resetToSidereal();
  mount.staA2.resetToSidereal();
  mount.parkStatus = ParkState::PRK_UNPARKED;
  XEEPROM.update(getMountAddress(EE_parkStatus), mount.parkStatus);
  // clear pulse-guiding state
  mount.guideA1.setIdle();
  mount.guideA1.duration = 0UL;
  mount.guideA1.durationLast = 0UL;
  mount.guideA2.setIdle();
  mount.guideA2.duration = 0UL;
  mount.guideA2.durationLast = 0UL;
  // update starting coordinates to reflect NCP or SCP polar home position
  syncAxis(&mount.geoA1.homeDef, &mount.geoA2.homeDef);
  // initialize/disable the stepper drivers
  DecayModeTracking();
  mount.sideralTracking = false;
  mount.atHome = true;
  syncEwithT();
  return true;
}

// init the telescope home position;  if defined use the user defined home position
void initHome()
{
  mount.homeSaved = XEEPROM.read(getMountAddress(EE_homeSaved));
  if (mount.homeSaved)
  {
    mount.geoA1.homeDef = XEEPROM.readLong(getMountAddress(EE_homePosAxis1))*pow(2, mount.motorA1.micro);
    mount.geoA2.homeDef = XEEPROM.readLong(getMountAddress(EE_homePosAxis2))*pow(2, mount.motorA2.micro);
    mount.homeSaved  &= withinLimit(mount.geoA1.homeDef, mount.geoA2.homeDef);
    if (!mount.homeSaved)
    {
      XEEPROM.write(getMountAddress(EE_homeSaved), 0);
    }
  }

  if(!mount.homeSaved)
  {
    if (isAltAZ())
    {
      mount.geoA1.homeDef = mount.geoA1.poleDef;
      mount.geoA2.homeDef = 0;
    }
    else
    {
      mount.geoA1.homeDef = mount.geoA1.poleDef;
      mount.geoA2.homeDef = mount.geoA2.poleDef;
    }
  }
}

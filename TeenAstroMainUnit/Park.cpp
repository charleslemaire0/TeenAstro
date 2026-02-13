// -----------------------------------------------------------------------------------
// functions related to PRK_PARKING the mount
#include "Global.h"

// sets the park postion as the current position
bool setPark()
{
  if ((mount.parkStatus == PRK_UNPARKED) && !TelescopeBusy())
  {
    mount.lastSideralTracking = mount.sideralTracking;
    mount.sideralTracking = false;

    // don't worry about moving around: during parking pec is turned off and backlash is cleared (0) so that mount.staA1.target/targetAxis2=mount.staA1.pos/mount.staA2.pos
    // this should handle getting us back to the home position for micro-step modes up to 256X
    // if sync anywhere is enabled use the corrected location

    long    h = (mount.staA1.target / 1024L) * 1024L;
    long    d = (mount.staA2.target / 1024L) * 1024L;
    h /= pow(2, mount.motorA1.micro);
    d /= pow(2, mount.motorA2.micro);
    // store our position
    XEEPROM.writeLong(getMountAddress(EE_posAxis1), h);
    XEEPROM.writeLong(getMountAddress(EE_posAxis2), d);

    //// and the align
    saveAlignModel();
    mount.parkSaved = true;
    XEEPROM.write(getMountAddress(EE_parkSaved), mount.parkSaved);
    mount.sideralTracking = mount.lastSideralTracking;
    return true;
  }

  return false;
}

// unset parkposition flag
void unsetPark()
{
  if (mount.parkSaved)
  {
    mount.parkSaved = false;
    XEEPROM.write(getMountAddress(EE_parkSaved), mount.parkSaved);
  }
}

void saveAlignModel()
{
  // and store our corrections
  float t11 = 0, t12 = 0, t13 = 0, t21 = 0, t22 = 0, t23 = 0, t31 = 0, t32 = 0, t33 = 0;
  XEEPROM.write(getMountAddress(EE_Tvalid), hasStarAlignment);
  if (hasStarAlignment)
  {
    alignment.getT(t11, t12, t13, t21, t22, t23, t31, t32, t33);
  }
  XEEPROM.writeFloat(getMountAddress(EE_T11), t11);
  XEEPROM.writeFloat(getMountAddress(EE_T12), t12);
  XEEPROM.writeFloat(getMountAddress(EE_T13), t13);
  XEEPROM.writeFloat(getMountAddress(EE_T21), t21);
  XEEPROM.writeFloat(getMountAddress(EE_T22), t22);
  XEEPROM.writeFloat(getMountAddress(EE_T23), t23);
  XEEPROM.writeFloat(getMountAddress(EE_T31), t31);
  XEEPROM.writeFloat(getMountAddress(EE_T32), t32);
  XEEPROM.writeFloat(getMountAddress(EE_T33), t33);
  return;
}

// takes up backlash and returns to the current position
void parkClearBacklash()
{
  static long LastIntervalAxis1, LastIntervalAxis2;
  // backlash takeup rate
  if ((mount.staA1.backlash_inSteps == 0 && mount.staA2.backlash_inSteps == 0))
  {
    mount.backlashStatus = BacklashPhase::DONE;
    return;
  }
  switch (mount.backlashStatus)
  {
  case BacklashPhase::INIT:
  {
    cli();
    LastIntervalAxis1 = mount.staA1.interval_Step_Cur;
    LastIntervalAxis2 = mount.staA2.interval_Step_Cur;
    sei();
    //start by moving fully into the backlash
    long axis1Target = mount.staA1.target + mount.staA1.backlash_inSteps;
    long axis2Target = mount.staA2.target + mount.staA2.backlash_inSteps;
    GotoAxis(&axis1Target, &axis2Target);
    mount.backlashStatus = BacklashPhase::MOVE_IN;
    return;
  }
  break;
  case BacklashPhase::MOVE_IN:
  {
    updateDeltaTarget();
    if (mount.staA1.backlash_movedSteps == mount.staA1.backlash_inSteps && mount.staA1.deltaTarget == 0 &&
      mount.staA2.backlash_movedSteps == mount.staA2.backlash_inSteps && mount.staA2.deltaTarget == 0)
    {
      // then reverse direction and take it all up
      long axis1Target = mount.staA1.target - mount.staA1.backlash_inSteps;
      long axis2Target = mount.staA2.target - mount.staA2.backlash_inSteps;
      GotoAxis(&axis1Target, &axis2Target);
      mount.backlashStatus = MOVE_OUT;
    }
    return;
  }
  break;
  case  BacklashPhase::MOVE_OUT:
  {
    updateDeltaTarget();
    if (mount.staA1.backlash_movedSteps == 0 && mount.staA1.deltaTarget == 0 &&
      mount.staA2.backlash_movedSteps == 0 && mount.staA2.deltaTarget == 0)
    {
      // we arrive back at the exact same position so ftargetAxis1/Dec don't need to be touched
      // move at the previous speed
      cli();
      mount.staA1.interval_Step_Cur = LastIntervalAxis1;
      mount.staA2.interval_Step_Cur = LastIntervalAxis2;
      sei();
      mount.backlashStatus = DONE;
    }
    return;
  }
  break;
  default:
    break;
  }
}

void finalizePark()
{
  if (mount.backlashStatus == DONE)
  {
    mount.backlashStatus = INIT;
  }
  parkClearBacklash();
  if (mount.backlashStatus == DONE)
  {
    mount.movingTo = false;
    mount.parkStatus = PRK_PARKED;// success, we're parked 
    enable_Axis(false);// disable the stepper drivers
    XEEPROM.write(getMountAddress(EE_parkStatus), mount.parkStatus);
  }
}


// moves the telescope to the park position, stops tracking
byte park()
{
  // Gets park position and moves the mount there
  if (mount.parkStatus == PRK_PARKED)
  {
    return 0;
  }
  if (!mount.parkSaved)
  {
    return 1;
  }
  if (mount.parkStatus != PRK_UNPARKED)
  {
    return 2;
  }
  if (TelescopeBusy())
  {
    return 3;
  }
  if (mount.lastError != ERRT_NONE)
  {
    return 4;
  }
  if (!mount.enableMotor)
  {
    return 5;
  }

  // get the position we're supposed to park at
  long    h = XEEPROM.readLong(getMountAddress(EE_posAxis1));
  long    d = XEEPROM.readLong(getMountAddress(EE_posAxis2));
  h *= pow(2, mount.motorA1.micro);
  d *= pow(2, mount.motorA2.micro);
  // stop tracking
  mount.lastSideralTracking = false;
  mount.sideralTracking = false;
  // record our status
  mount.parkStatus = PRK_PARKING;
  XEEPROM.write(getMountAddress(EE_parkStatus), mount.parkStatus);
  GotoAxis(&h, &d);
  return 0;
}

// returns a parked telescope to operation, you must set date and time before calling this.  it also
bool syncAtPark()
{
  if (!mount.parkSaved)
  {
    return false;
  }
  mount.atHome = false;
  // enable the stepper drivers
  mount.staA1.enable = true;
  mount.staA2.enable = true;
  delay(10);

  // get our position
  long axis1, axis2;
  axis1 = XEEPROM.readLong(getMountAddress(EE_posAxis1));
  axis2 = XEEPROM.readLong(getMountAddress(EE_posAxis2));
  axis1 *= pow(2, mount.motorA1.micro);
  axis2 *= pow(2, mount.motorA2.micro);
  syncAxis(&axis1, &axis2);
  // set Meridian Flip behaviour to match mount type
  mount.meridianFlip = mount.mountType == MOUNT_TYPE_GEM ? FLIP_ALWAYS : FLIP_NEVER;
  syncEwithT();
  DecayModeTracking();
  return true;
}

//initialisation at park
bool iniAtPark()
{
  mount.parkSaved = XEEPROM.read(getMountAddress(EE_parkSaved));
  if (!mount.parkSaved)
  {
    mount.parkStatus = PRK_UNPARKED;
    return false;
  }
  byte parkStatusRead = XEEPROM.read(getMountAddress(EE_parkStatus));
  bool ok = false;
  switch (parkStatusRead)
  {
  case PRK_PARKED:
    if (syncAtPark())
    {
      mount.parkStatus = PRK_PARKED;
      ok = true;
    }
    else
    {
      mount.parkStatus = PRK_UNPARKED;
      XEEPROM.write(getMountAddress(EE_parkStatus), PRK_UNPARKED);
    }
    break;
  case PRK_UNPARKED:
    mount.parkStatus = PRK_UNPARKED;
    return false;
    break;
  default:
    mount.parkStatus = PRK_UNPARKED;
    XEEPROM.write(getMountAddress(EE_parkStatus), PRK_UNPARKED);
    break;
  }
  return ok;
}

// depends on the latitude, longitude, and timeZone; but those are stored and recalled automatically
void unpark()
{
  if (mount.parkStatus == PRK_UNPARKED)
    return;
  // update our status, we're not parked anymore
  mount.parkStatus = PRK_UNPARKED;
  XEEPROM.write(getMountAddress(EE_parkStatus), mount.parkStatus);
  // start tracking the sky
  if (mount.enableMotor)
  {
    StartSideralTracking();
  }
  return;
}

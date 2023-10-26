// -----------------------------------------------------------------------------------
// functions related to PRK_PARKING the mount

// sets the park postion as the current position
bool setPark()
{
  if ((parkStatus == PRK_UNPARKED) && !TelescopeBusy())
  {
    lastSideralTracking = sideralTracking;
    sideralTracking = false;

    // don't worry about moving around: during parking pec is turned off and backlash is cleared (0) so that staA1.target/targetAxis2=staA1.pos/staA2.pos
    // this should handle getting us back to the home position for micro-step modes up to 256X
    // if sync anywhere is enabled use the corrected location

    long    h = (staA1.target / 1024L) * 1024L;
    long    d = (staA2.target / 1024L) * 1024L;
    h /= pow(2, motorA1.micro);
    d /= pow(2, motorA2.micro);
    // store our position
    XEEPROM.writeLong(getMountAddress(EE_posAxis1), h);
    XEEPROM.writeLong(getMountAddress(EE_posAxis2), d);

    //// and the align
    saveAlignModel();
    parkSaved = true;
    XEEPROM.write(getMountAddress(EE_parkSaved), parkSaved);
    sideralTracking = lastSideralTracking;
    return true;
  }

  return false;
}

// unset parkposition flag
void unsetPark()
{
  if (parkSaved)
  {
    parkSaved = false;
    XEEPROM.write(getMountAddress(EE_parkSaved), parkSaved);
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
  if ((staA1.backlash_inSteps == 0 && staA2.backlash_inSteps == 0))
  {
    backlashStatus = BacklashPhase::DONE;
    return;
  }
  switch (backlashStatus)
  {
  case BacklashPhase::INIT:
  {
    cli();
    LastIntervalAxis1 = staA1.interval_Step_Cur;
    LastIntervalAxis2 = staA2.interval_Step_Cur;
    sei();
    //start by moving fully into the backlash
    long axis1Target = staA1.target + staA1.backlash_inSteps;
    long axis2Target = staA2.target + staA2.backlash_inSteps;
    GotoAxis(&axis1Target, &axis2Target);
    backlashStatus = BacklashPhase::MOVE_IN;
    return;
  }
  break;
  case BacklashPhase::MOVE_IN:
  {
    updateDeltaTarget();
    if (staA1.backlash_movedSteps == staA1.backlash_inSteps && staA1.deltaTarget == 0 &&
      staA2.backlash_movedSteps == staA2.backlash_inSteps && staA2.deltaTarget == 0)
    {
      // then reverse direction and take it all up
      long axis1Target = staA1.target - staA1.backlash_inSteps;
      long axis2Target = staA2.target - staA2.backlash_inSteps;
      GotoAxis(&axis1Target, &axis2Target);
      backlashStatus = MOVE_OUT;
    }
    return;
  }
  break;
  case  BacklashPhase::MOVE_OUT:
  {
    updateDeltaTarget();
    if (staA1.backlash_movedSteps == 0 && staA1.deltaTarget == 0 &&
      staA2.backlash_movedSteps == 0 && staA2.deltaTarget == 0)
    {
      // we arrive back at the exact same position so ftargetAxis1/Dec don't need to be touched
      // move at the previous speed
      cli();
      staA1.interval_Step_Cur = LastIntervalAxis1;
      staA2.interval_Step_Cur = LastIntervalAxis2;
      sei();
      backlashStatus = DONE;
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
  if (backlashStatus == DONE)
  {
    backlashStatus = INIT;
  }
  parkClearBacklash();
  if (backlashStatus == DONE)
  {
    parkStatus = PRK_PARKED;// success, we're parked 
    enable_Axis(false);// disable the stepper drivers
    XEEPROM.write(getMountAddress(EE_parkStatus), parkStatus);
  }
}


// moves the telescope to the park position, stops tracking
byte park()
{
  // Gets park position and moves the mount there
  if (parkStatus == PRK_PARKED)
  {
    return 0;
  }
  if (!parkSaved)
  {
    return 1;
  }
  if (parkStatus != PRK_UNPARKED)
  {
    return 2;
  }
  if (TelescopeBusy())
  {
    return 3;
  }
  if (lastError != ERRT_NONE)
  {
    return 4;
  }

  // get the position we're supposed to park at
  long    h = XEEPROM.readLong(getMountAddress(EE_posAxis1));
  long    d = XEEPROM.readLong(getMountAddress(EE_posAxis2));
  h *= pow(2, motorA1.micro);
  d *= pow(2, motorA2.micro);
  // stop tracking
  lastSideralTracking = false;
  sideralTracking = false;
  // record our status
  parkStatus = PRK_PARKING;
  XEEPROM.write(getMountAddress(EE_parkStatus), parkStatus);
  GotoAxis(&h, &d);
  return 0;
}

// returns a parked telescope to operation, you must set date and time before calling this.  it also
bool syncAtPark()
{
  if (!parkSaved)
  {
    return false;
  }
  atHome = false;
  // enable the stepper drivers
  staA1.enable = true;
  staA2.enable = true;
  delay(10);

  // get our position
  long axis1, axis2;
  axis1 = XEEPROM.readLong(getMountAddress(EE_posAxis1));
  axis2 = XEEPROM.readLong(getMountAddress(EE_posAxis2));
  axis1 *= pow(2, motorA1.micro);
  axis2 *= pow(2, motorA2.micro);
  syncAxis(&axis1, &axis2);
  // set Meridian Flip behaviour to match mount type
  meridianFlip = mountType == MOUNT_TYPE_GEM ? FLIP_ALWAYS : FLIP_NEVER;
  syncEwithT();
  DecayModeTracking();
  return true;
}

//initialisation at park
bool iniAtPark()
{
  parkSaved = XEEPROM.read(getMountAddress(EE_parkSaved));
  if (!parkSaved)
  {
    parkStatus = PRK_UNPARKED;
    return false;
  }
  byte parkStatusRead = XEEPROM.read(getMountAddress(EE_parkStatus));
  bool ok = false;
  switch (parkStatusRead)
  {
  case PRK_PARKED:
    if (syncAtPark())
    {
      parkStatus = PRK_PARKED;
      ok = true;
    }
    else
    {
      parkStatus = PRK_UNPARKED;
      XEEPROM.write(getMountAddress(EE_parkStatus), PRK_UNPARKED);
    }
    break;
  case PRK_UNPARKED:
    parkStatus = PRK_UNPARKED;
    return false;
    break;
  default:
    parkStatus = PRK_UNPARKED;
    XEEPROM.write(getMountAddress(EE_parkStatus), PRK_UNPARKED);
    break;
  }
  return ok;
}

// depends on the latitude, longitude, and timeZone; but those are stored and recalled automatically
void unpark()
{
  if (parkStatus == PRK_UNPARKED)
    return;
  // update our status, we're not parked anymore
  parkStatus = PRK_UNPARKED;
  XEEPROM.write(getMountAddress(EE_parkStatus), parkStatus);
  // start tracking the sky
  sideralTracking = true;
  return;
}



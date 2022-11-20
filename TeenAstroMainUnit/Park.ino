// -----------------------------------------------------------------------------------
// functions related to PRK_PARKING the mount

// sets the park postion as the current position
bool setPark()
{
  if ((parkStatus == PRK_UNPARKED) && !movingTo)
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
    XEEPROM.writeLong(EE_posAxis1, h);
    XEEPROM.writeLong(EE_posAxis2, d);

    //// and the align
    saveAlignModel();
    parkSaved = true;
    XEEPROM.write(EE_parkSaved, parkSaved);
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
    XEEPROM.write(EE_parkSaved, parkSaved);
  }
}

void saveAlignModel()
{
  // and store our corrections
  float t11 = 0, t12 = 0, t13 = 0, t21 = 0, t22 = 0, t23 = 0, t31 = 0, t32 = 0, t33 = 0;
  XEEPROM.write(EE_Tvalid, hasStarAlignment);
  if (hasStarAlignment)
  {
    alignment.getT(t11, t12, t13, t21, t22, t23, t31, t32, t33);
  }
  XEEPROM.writeFloat(EE_T11, t11);
  XEEPROM.writeFloat(EE_T12, t12);
  XEEPROM.writeFloat(EE_T13, t13);
  XEEPROM.writeFloat(EE_T21, t21);
  XEEPROM.writeFloat(EE_T22, t22);
  XEEPROM.writeFloat(EE_T23, t23);
  XEEPROM.writeFloat(EE_T31, t31);
  XEEPROM.writeFloat(EE_T32, t32);
  XEEPROM.writeFloat(EE_T33, t33);
  return;
}

// takes up backlash and returns to the current position
bool parkClearBacklash()
{
  // backlash takeup rate
  if (backlashA1.inSteps == 0 && backlashA2.inSteps == 0)
  {
    return true;
  }
  cli();
  long    LastTimerRateAxis1 = staA1.interval_Step_Cur;
  long    LastTimerRateAxis2 = staA2.interval_Step_Cur;
  staA1.interval_Step_Cur = backlashA1.interval_Step;
  staA2.interval_Step_Cur = backlashA2.interval_Step;
  sei();

  // figure out how long we'll have to wait for the backlash to clear (+50%)
  long    t;
  if (backlashA1.inSteps > backlashA2.inSteps)
    t = (long)(backlashA1.inSteps * 1500 / geoA1.stepsPerSecond);
  else
    t = (long)(backlashA2.inSteps * 1500 / geoA2.stepsPerSecond);
  t = (t / BacklashTakeupRate + 250) / 12;

  // start by moving fully into the backlash
  cli();
  staA1.target += backlashA1.inSteps;
  staA2.target += backlashA2.inSteps;
  sei();

  // wait until done or timed out
  for (int i = 0; i < 12; i++)
  {
    if (backlashA1.movedSteps != backlashA1.inSteps || staA1.pos != staA1.target ||
        backlashA2.movedSteps != backlashA2.inSteps || staA2.pos != staA2.target)
      delay(t);
  }

  // then reverse direction and take it all up
  cli();
  staA1.target-= backlashA1.inSteps;
  staA2.target-= backlashA2.inSteps;
  sei();


  // wait until done or timed out, plus a safety margin
  for (int i = 0; i < 24; i++)
  {
    updateDeltaTarget();
    if ((backlashA1.movedSteps != 0) || (staA1.deltaTarget != 0) ||
      (backlashA2.movedSteps != 0) || (staA2.deltaTarget != 0))
      delay(t);
  }

  // we arrive back at the exact same position so ftargetAxis1/Dec don't need to be touched
  // move at the previous speed
  cli();
  staA1.interval_Step_Cur = LastTimerRateAxis1;
  staA2.interval_Step_Cur = LastTimerRateAxis2;
  sei();

  // return true on success
  if ((backlashA1.movedSteps != 0) || (backlashA2.movedSteps != 0))
    return false;
  else
    return true;
}

// moves the telescope to the park position, stops tracking
byte park()
{
  // Gets park position and moves the mount there
  if (parkStatus == PRK_PARKED)
  {
    return 0;
  }
  if (lastError != ERRT_NONE)
  {
    return 4;
  }
  if (!movingTo)
  {
    if (parkStatus == PRK_UNPARKED)
    {
      if (parkSaved)
      {
        // get the position we're supposed to park at
        long    h = XEEPROM.readLong(EE_posAxis1);
        long    d = XEEPROM.readLong(EE_posAxis2);
        h *= pow(2, motorA1.micro);
        d *= pow(2, motorA2.micro);
        // stop tracking
        lastSideralTracking = false;
        sideralTracking = false;
        // record our status
        parkStatus = PRK_PARKING;
        XEEPROM.write(EE_parkStatus, parkStatus);
        goTo(h, d);
        return 0;
      }
      else
        return 1;   // no park position saved
    }
    else
      return 2;    // not parked
  }
  else
    return 3;     // already moving
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
  // get corrections
  //GeoAlign.readCoe();

  // get our position
  long axis1, axis2;
  axis1 = XEEPROM.readLong(EE_posAxis1);
  axis2 = XEEPROM.readLong(EE_posAxis2);
  axis1 *= pow(2, motorA1.micro);
  axis2 *= pow(2, motorA2.micro);
  cli();
  staA1.pos = axis1;
  staA1.target = axis1;
  staA2.pos = axis2;
  staA2.target = axis2;
  sei();
  // set Meridian Flip behaviour to match mount type
  meridianFlip = mountType == MOUNT_TYPE_GEM ? FLIP_ALWAYS : FLIP_NEVER;
  return true;
}

//initialisation at park
bool iniAtPark()
{
  parkSaved = XEEPROM.read(EE_parkSaved);
  if (!parkSaved)
  {
    parkStatus = PRK_UNPARKED;
    return false;
  }
  byte parkStatusRead = XEEPROM.read(EE_parkStatus);
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
      XEEPROM.write(EE_parkStatus, PRK_UNPARKED);
    }
    break;
  case PRK_UNPARKED:
    parkStatus = PRK_UNPARKED;
    return false;
    break;
  default:
    parkStatus = PRK_UNPARKED;
    XEEPROM.write(EE_parkStatus, PRK_UNPARKED);
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
  XEEPROM.write(EE_parkStatus, parkStatus);
  // start tracking the sky
  sideralTracking = true;
  return;
}



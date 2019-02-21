// -----------------------------------------------------------------------------------
// functions related to Parking the mount

// sets the park postion as the current position
boolean setPark()
{
  if ((parkStatus == NotParked) && (trackingState != TrackingMoveTo))
  {
    lastTrackingState = trackingState;
    trackingState = TrackingOFF;

    // don't worry about moving around: during parking pec is turned off and backlash is cleared (0) so that targetAxis1/targetAxis2=posAxis1/posAxis2
    // this should handle getting us back to the home position for micro-step modes up to 256X
    // if sync anywhere is enabled use the corrected location

    long    h = (((long)targetAxis1.part.m) / 1024L) * 1024L;
    long    d = (((long)targetAxis2.part.m) / 1024L) * 1024L;
    h /= pow(2, MicroAxis1);
    d /= pow(2, MicroAxis2);
    // store our position
    EEPROM_writeLong(EE_posAxis1, h);
    EEPROM_writeLong(EE_posAxis2, d);

    // and the align
    saveAlignModel();
    parkSaved = true;
    EEPROM.write(EE_parkSaved, parkSaved);
    trackingState = lastTrackingState;
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
    EEPROM.write(EE_parkSaved, parkSaved);
  }
}

boolean saveAlignModel()
{
  // and store our corrections
  GeoAlign.writeCoe();
  return true;
}

// takes up backlash and returns to the current position
boolean parkClearBacklash()
{
  // backlash takeup rate
  if (StepsBacklashAxis1 == 0 && StepsBacklashAxis2 == 0)
  {
    return true;
  }
  cli();
  long    LastTimerRateAxis1 = timerRateAxis1;
  long    LastTimerRateAxis2 = timerRateAxis2;
  timerRateAxis1 = timerRateBacklashAxis1;
  timerRateAxis2 = timerRateBacklashAxis2;
  sei();

  // figure out how long we'll have to wait for the backlash to clear (+50%)
  long    t;
  if (StepsBacklashAxis1 > StepsBacklashAxis2)
    t = ((long)StepsBacklashAxis1 * 1500) / (long)StepsPerSecondAxis1;
  else
    t = ((long)StepsBacklashAxis2 * 1500) / (long)StepsPerSecondAxis2;
  t = (t / BacklashTakeupRate + 250) / 12;

  // start by moving fully into the backlash
  cli();
  targetAxis1.part.m += StepsBacklashAxis1;
  targetAxis2.part.m += StepsBacklashAxis2;
  sei();

  // wait until done or timed out
  for (int i = 0; i < 12; i++)
  {
    if ((blAxis1 != StepsBacklashAxis1) || (posAxis1 != (long)targetAxis1.part.m) ||
      (blAxis2 != StepsBacklashAxis2) || (posAxis2 != (long)targetAxis2.part.m))
      delay(t);
  }

  // then reverse direction and take it all up
  cli();
  targetAxis1.part.m -= StepsBacklashAxis1;
  targetAxis2.part.m -= StepsBacklashAxis2;
  sei();


  // wait until done or timed out, plus a safety margin
  for (int i = 0; i < 24; i++)
  {
    updateDeltaTarget();
    if ((blAxis1 != 0) || (deltaTargetAxis1 != 0) ||
      (blAxis2 != 0) || (deltaTargetAxis2 != 0))
      delay(t);
  }

  // we arrive back at the exact same position so ftargetAxis1/Dec don't need to be touched
  // move at the previous speed
  cli();
  timerRateAxis1 = LastTimerRateAxis1;
  timerRateAxis2 = LastTimerRateAxis2;
  sei();

  // return true on success
  if ((blAxis1 != 0) || (blAxis2 != 0))
    return false;
  else
    return true;
}

// moves the telescope to the park position, stops tracking
byte park()
{
  // Gets park position and moves the mount there
  if (parkStatus == Parked)
  {
    return 0;
  }
  if (lastError != ERR_NONE)
  {
    return 4;
  }
  if (trackingState != TrackingMoveTo)
  {
    parkSaved = EEPROM.read(EE_parkSaved);
    if (parkStatus == NotParked)
    {
      if (parkSaved)
      {
        // get the position we're supposed to park at
        long    h = EEPROM_readLong(EE_posAxis1);
        long    d = EEPROM_readLong(EE_posAxis2);
        h *= pow(2, MicroAxis1);
        d *= pow(2, MicroAxis2);
        // stop tracking
        abortTrackingState = trackingState;
        lastTrackingState = TrackingOFF;
        trackingState = TrackingOFF;
        // record our status
        parkStatus = Parking;
        EEPROM.write(EE_parkStatus, parkStatus);
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
boolean syncAtPark()
{
  parkSaved = EEPROM.read(EE_parkSaved);
  if (!parkSaved)
  {
    return false;
  }
  atHome = false;
  // enable the stepper drivers
  axis1Enabled = true;
  axis2Enabled = true;
  delay(10);
  // get corrections
  GeoAlign.readCoe();

  // get our position
  int axis1, axis2;
  axis1 = EEPROM_readLong(EE_posAxis1);
  axis2 = EEPROM_readLong(EE_posAxis2);
  axis1 *= pow(2, MicroAxis1);
  axis2 *= pow(2, MicroAxis2);
  cli();
  posAxis1 = axis1;
  targetAxis1.part.m = axis1;
  targetAxis1.part.f = 0;
  posAxis2 = axis2;
  targetAxis2.part.m = axis2;
  targetAxis2.part.f = 0;
  sei();
  // see what side of the pier we're on
  CheckPierSide();
  // set Meridian Flip behaviour to match mount type
  meridianFlip = mountType == MOUNT_TYPE_GEM ? MeridianFlipAlways : MeridianFlipNever;
  return true;
}

boolean iniAtPark()
{
  if (trackingState != TrackingOFF)
    return false;
  parkStatus = EEPROM.read(EE_parkStatus);
  if (!(parkStatus == Parked ))
  {
    parkStatus = NotParked;
    EEPROM.write(EE_parkStatus, NotParked);
    return false;
  }
  return syncAtPark();
}

// depends on the latitude, longitude, and timeZone; but those are stored and recalled automatically
boolean unpark()
{
  if (parkStatus == NotParked)
    return true;
  bool ok = iniAtPark();
  if (!ok)
  {
    return false;
  }
  // update our status, we're not parked anymore
  parkStatus = NotParked;
  EEPROM.write(EE_parkStatus, parkStatus);
  // start tracking the sky
  trackingState = TrackingON;
  return true;
}

void syncPolarHome()
{
  // update starting coordinates to reflect NCP or SCP polar home position
  startAxis1 = celestialPoleStepAxis1;
  startAxis2 = celestialPoleStepAxis2;
  cli();
  targetAxis1.part.m = startAxis1;
  targetAxis1.part.f = 0;
  posAxis1 = startAxis1;
  targetAxis2.part.m = startAxis2;
  targetAxis2.part.f = 0;
  posAxis2 = startAxis2;
  sei();
  atHome = true;
}

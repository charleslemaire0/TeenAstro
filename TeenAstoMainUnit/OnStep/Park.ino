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
    long    ax1md =
      (
      (((long)targetAxis1.part.m) - trueAxis1) %
        1024L -
        ((long)targetAxis1.part.m) %
        1024L
        );
    long    ax2md =
      (
      (((long)targetAxis2.part.m) - trueAxis2) %
        1024L -
        ((long)targetAxis2.part.m) %
        1024L
        );
    long    h = (((long)targetAxis1.part.m) / 1024L) *
      1024L -
      ax1md;
    long    d = (((long)targetAxis2.part.m) / 1024L) *
      1024L -
      ax2md;


    // store our position
    EEPROM_writeLong(EE_posAxis1, h);
    EEPROM_writeLong(EE_posAxis2, d);
    EEPROM_writeLong(EE_trueAxis1, trueAxis1);
    EEPROM_writeLong(EE_trueAxis2, trueAxis2);

    // and the align
    saveAlignModel();

    // and remember what side of the pier we're on
    EEPROM.write(EE_pierSide, pierSide);
    parkSaved = true;
    EEPROM.write(EE_parkSaved, parkSaved);

    trackingState = lastTrackingState;
    return true;
  }

  return false;
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
  if (backlashAxis1 == 0 && backlashAxis2 == 0)
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
  if (backlashAxis1 > backlashAxis2)
    t = ((long)backlashAxis1 * 1500) / (long)StepsPerSecondAxis1;
  else
    t = ((long)backlashAxis2 * 1500) / (long)StepsPerSecondAxis1;
  t = (t / BacklashTakeupRate + 250) / 12;

  // start by moving fully into the backlash
  cli();
  targetAxis1.part.m += backlashAxis1;
  targetAxis2.part.m += backlashAxis2;
  sei();

  // wait until done or timed out
  for (int i = 0; i < 12; i++)
  {
    if ((blAxis1 != backlashAxis1) || (posAxis1 != (long)targetAxis1.part.m) ||
      (blAxis2 != backlashAxis2) || (posAxis2 != (long)targetAxis2.part.m))
      delay(t);
  }

  // then reverse direction and take it all up
  cli();
  targetAxis1.part.m -= backlashAxis1;
  targetAxis2.part.m -= backlashAxis2;
  sei();

  // wait until done or timed out, plus a safety margin
  for (int i = 0; i < 24; i++)
  {
    if ((blAxis1 != 0) || (posAxis1 != (long)targetAxis1.part.m) ||
      (blAxis2 != 0) || (posAxis2 != (long)targetAxis2.part.m))
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
        double HaP = (double)h / StepsPerDegreeAxis1;

        byte    gotoPierSide = EEPROM.read(EE_pierSide);

        byte side = predictSideOfPier(HaP, gotoPierSide);
        if (side == 0)
        {
          parkStatus = ParkFailed;
          EEPROM.write(EE_parkStatus, parkStatus);
          return -1; //fail, outside limit
        }
        // stop tracking
        abortTrackingState = trackingState;
        lastTrackingState = TrackingOFF;
        trackingState = TrackingOFF;

        // record our status
        parkStatus = Parking;
        EEPROM.write(EE_parkStatus, parkStatus);

        bool doflip = !(pierSide == gotoPierSide);
        if (doflip)
        {
          if (pierSide == PierSideEast)
            pierSide = PierSideFlipEW1;
          else
            pierSide = PierSideFlipWE1;
        }
        // now, slew to this target HA,Dec
        goTo(h, d);

        return 0;
      }
      else
        return 1;   // no park position saved
    }
    else
      return 2;       // not parked
  }
  else
    return 3;           // already moving
}

// returns a parked telescope to operation, you must set date and time before calling this.  it also
boolean syncAtPark()
{
  parkSaved = EEPROM.read(EE_parkSaved);
  if (!parkSaved)
  {
    return false;
  }
  // enable the stepper drivers
  axis1Enabled = true;
  axis2Enabled = true;
  delay(10);
  // get corrections
  GeoAlign.readCoe();

  // get our position
  int axis1, axis2, taxis1, taxis2;
  axis1 = EEPROM_readLong(EE_posAxis1);
  axis2 = EEPROM_readLong(EE_posAxis2);
  taxis1 = EEPROM_readLong(EE_trueAxis1);
  taxis2 = EEPROM_readLong(EE_trueAxis2);
  cli();
  posAxis1 = axis1;
  targetAxis1.part.m = axis1;
  targetAxis1.part.f = 0;
  posAxis2 = axis2;
  targetAxis2.part.m = axis2;
  targetAxis2.part.f = 0;
  trueAxis1 = taxis1;
  trueAxis2 = taxis2;
  sei();

  // see what side of the pier we're on
  pierSide = EEPROM.read(EE_pierSide);
  if (pierSide == PierSideWest)
    DecDir = DecDirWInit;
  else
    DecDir = DecDirEInit;

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
  trueAxis1 = startAxis1;
  targetAxis2.part.m = startAxis2;
  targetAxis2.part.f = 0;
  posAxis2 = startAxis2;
  trueAxis2 = startAxis2;
  sei();
}

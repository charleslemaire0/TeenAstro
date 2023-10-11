// -----------------------------------------------------------------------------------
// functions related to Homing the mount

bool setHome()
{
  if ((parkStatus == PRK_UNPARKED) && !movingTo)
  {
    lastSideralTracking = sideralTracking;
    sideralTracking = false;

    // don't worry about moving around: during parking pec is turned off and backlash is cleared (0) so that staA1.target/targetAxis2=staA1.pos/staA2.pos
    // this should handle getting us back to the home position for micro-step modes up to 256X
    // if sync anywhere is enabled use the corrected location

    long h = (staA1.target / 1024L) * 1024L;
    long d = (staA2.target / 1024L) * 1024L;

    h /= pow(2, motorA1.micro);
    d /= pow(2, motorA2.micro);
    // store our position
    XEEPROM.writeLong(getMountAddress(EE_homePosAxis1), h);
    XEEPROM.writeLong(getMountAddress(EE_homePosAxis2), d);
    XEEPROM.write(getMountAddress(EE_homeSaved), 1);
    initHome();
    sideralTracking = lastSideralTracking;
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
  if ((parkStatus != PRK_UNPARKED) && (parkStatus != PRK_PARKING)) return false; // fail, moving to home not allowed if PRK_PARKED
  if (lastError != ERRT_NONE) return false;                                // fail, cannot move if there are errors
  if (movingTo) return false;                      // fail, moving to home not allowed during a move
  if (guideA1.isBusy() || guideA2.isBusy()) return false;                       // fail, moving to home not allowed while guiding
  cli();

  staA1.target = geoA1.homeDef;
  staA2.target = geoA2.homeDef;
  staA1.start = staA1.pos;
  staA2.start = staA2.pos;
  SetsiderealClockSpeed(siderealClockSpeed);
  sei();
  // stop tracking
  lastSideralTracking = false;
  sideralTracking = false;
  movingTo = true;
  homeMount = true;
  DecayModeGoto();
  return true;
}


void finalizeHome()
{ 
  if (backlashStatus == DONE)
  {
    backlashStatus = INIT;
  }
  parkClearBacklash();
  if (backlashStatus == DONE)
  {
    syncAtHome();
    homeMount = false;
    // disable the stepper drivers
    enable_Axis(false);
  }
}

// resets telescope home position; user manually moves home position,
bool syncAtHome()
{
  if (movingTo) return false;  // fail, forcing home not allowed during a move
  // default values for state variables
  staA2.dir = true;
  staA1.dir = true;
  newTargetRA = 0;
  newTargetDec = 0;
  newTargetAlt = 0;
  newTargetAzm = 0;
  lastError = ErrorsTraking::ERRT_NONE;
  // reset tracking and rates
  staA1.resetToSidereal();
  staA2.resetToSidereal();
  parkStatus = ParkState::PRK_UNPARKED;
  XEEPROM.update(getMountAddress(EE_parkStatus), parkStatus);
  // clear pulse-guiding state
  guideA1.setIdle();
  guideA1.duration = 0;
  guideA1.durationLast = 0;
  guideA2.setIdle();
  guideA2.duration = 0;
  guideA2.durationLast = 0;
  // update starting coordinates to reflect NCP or SCP polar home position

  staA1.start = geoA1.homeDef;
  staA2.start = geoA2.homeDef;
  cli();
  staA1.target = staA1.start;
  staA1.pos = staA1.start;
  staA2.target = staA2.start;
  staA2.pos = staA2.start;
  sei();
  // initialize/disable the stepper drivers
  DecayModeTracking();
  sideralTracking = false;
  atHome = true;
  syncEwithT();
  return true;
}

// init the telescope home position;  if defined use the user defined home position
void initHome()
{
  homeSaved = XEEPROM.read(getMountAddress(EE_homeSaved));
  if (homeSaved)
  {
    geoA1.homeDef = XEEPROM.readLong(getMountAddress(EE_homePosAxis1))*pow(2, motorA1.micro);
    geoA2.homeDef = XEEPROM.readLong(getMountAddress(EE_homePosAxis2))*pow(2, motorA2.micro);
    homeSaved  &= withinLimit(geoA1.homeDef, geoA2.homeDef);
    if (!homeSaved)
    {
      XEEPROM.write(getMountAddress(EE_homeSaved), 0);
    }
  }

  if(!homeSaved)
  {
    if (isAltAZ())
    {
      geoA1.homeDef = geoA1.poleDef;
      geoA2.homeDef = 0;
    }
    else
    {
      geoA1.homeDef = geoA1.poleDef;
      geoA2.homeDef = geoA2.poleDef;
    }
  }
}
// -----------------------------------------------------------------------------------
// functions related to Homing the mount
// moves telescope to the home position, then stops tracking
boolean goHome()
{
    if ((parkStatus != NotParked) && (parkStatus != Parking)) return false; // fail, moving to home not allowed if Parked
    if (lastError != ERR_NONE) return false;                                // fail, cannot move if there are errors
    if (trackingState == TrackingMoveTo) return false;                      // fail, moving to home not allowed during a move
    if (guideDirAxis1 || guideDirAxis2) return false;                       // fail, moving to home not allowed while guiding
    cli();
       
    targetAxis1.part.m = celestialPoleStepAxis1;
    targetAxis1.part.f = 0;
    targetAxis2.part.m = celestialPoleStepAxis2;
    targetAxis2.part.f = 0;

    startAxis1 = posAxis1;
    startAxis2 = posAxis2;

    abortTrackingState = trackingState;
    lastTrackingState = TrackingOFF;
    trackingState = TrackingMoveTo;
    SetSiderealClockRate(siderealInterval);
    sei();

    homeMount = true;

    DecayModeGoto();

    return true;
}

// sets telescope home position; user manually moves to Hour Angle 90 and Declination 90 (CWD position),

// then the first gotoEqu will set the pier side and turn on tracking
boolean setHome()
{
    if (trackingState == TrackingMoveTo) return false;  // fail, forcing home not allowed during a move

    // default values for state variables
    pierSide = PierSideEast;
    dirAxis2 = 1;
    DecDir = DecDirEInit;
    if (*localSite.latitude() > 0)
        HADir = HADirNCPInit;
    else
        HADir = HADirSCPInit;
    dirAxis1 = 1;
    newTargetRA = 0;
    newTargetDec = 0;
    newTargetAlt = 0;
    newTargetAzm = 0;
    origTargetAxis1.fixed = 0;
    origTargetAxis2 = 0;

    // reset pointing model
    alignNumStars = 0;
    alignThisStar = 0;
    if (mountType == MOUNT_TYPE_ALTAZM)
      Align.init();

    GeoAlign.init();

    // reset meridian flip control
if (mountType==MOUNT_TYPE_GEM)
    meridianFlip = MeridianFlipAlways;
else if (mountType==MOUNT_TYPE_FORK)
    meridianFlip = MeridianFlipAlign;
else if (mountType == MOUNT_TYPE_FORK_ALT)
    meridianFlip = MeridianFlipNever;
else if (mountType == MOUNT_TYPE_ALTAZM)
    meridianFlip = MeridianFlipNever;

    // where we are
    homeMount = false;
    atHome = true;
    lastError = ERR_NONE;

    // reset tracking and rates
    trackingState = TrackingOFF;
    lastTrackingState = TrackingOFF;
    timerRateAxis1 = SiderealRate;
    timerRateAxis2 = SiderealRate;

    // not parked, but don't wipe the park position if it's saved - we can still use it
    parkStatus = NotParked;
    EEPROM.write(EE_parkStatus, parkStatus);

    // the polar home position
    startAxis1 = celestialPoleStepAxis1;
    startAxis2 = celestialPoleStepAxis2;

    // clear pulse-guiding state
    guideDirAxis1 = 0;
    guideDurationHA = 0;
    guideDurationLastHA = 0;
    guideDirAxis2 = 0;
    guideDurationDec = 0;
    guideDurationLastDec = 0;
    enable_Axis(false);

    cli();
    targetAxis1.part.m = startAxis1;
    targetAxis1.part.f = 0;
    posAxis1 = startAxis1;
    trueAxis1 = startAxis1;
    targetAxis2.part.m = startAxis2;
    targetAxis2.part.f = 0;
    posAxis2 = startAxis2;
    trueAxis2 = startAxis2;
    blAxis1 = 0;
    blAxis2 = 0;
    sei();

    // initialize/disable the stepper drivers
    DecayModeTracking();

    return true;
}

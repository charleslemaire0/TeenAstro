// -----------------------------------------------------------------------------------
// functions related to Homing the mount
// moves telescope to the home position, then stops tracking
boolean goHome()
{
    if ((parkStatus != PRK_UNPARKED) && (parkStatus != PRK_PARKING)) return false; // fail, moving to home not allowed if PRK_PARKED
    if (lastError != ERR_NONE) return false;                                // fail, cannot move if there are errors
    if (movingTo) return false;                      // fail, moving to home not allowed during a move
    if (guideDirAxis1 || guideDirAxis2) return false;                       // fail, moving to home not allowed while guiding
    cli();
       
    targetAxis1.part.m = homeStepAxis1;
    targetAxis1.part.f = 0;
    targetAxis2.part.m = homeStepAxis2;
    targetAxis2.part.f = 0;
    startAxis1 = posAxis1;
    startAxis2 = posAxis2;
    SetSiderealClockRate(siderealInterval);
    sei();
    // stop tracking
    lastSideralTracking = false;
    sideralTracking = false;
    movingTo = true;
    homeMount = true;
    DecayModeGoto();
    return true;
}

// sets telescope home position; user manually moves to Hour Angle 90 and Declination 90 (CWD position),

// then the first gotoEqu will set the pier side and turn on tracking
boolean setHome()
{
    if (movingTo) return false;  // fail, forcing home not allowed during a move
    bool lastSideralTracking = sideralTracking;
    sideralTracking = false;
    // default values for state variables
    pierSide = PIER_EAST;
    dirAxis2 = 1;
    initLat();
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
    if (isAltAZ())
      Align.init();

    GeoAlign.init();

    // reset meridian flip control
if (mountType==MOUNT_TYPE_GEM)
    meridianFlip = FLIP_ALWAYS;
else if (mountType==MOUNT_TYPE_FORK)
    meridianFlip = FLIP_NEVER;
else if (mountType == MOUNT_TYPE_FORK_ALT)
    meridianFlip = FLIP_NEVER;
else if (mountType == MOUNT_TYPE_ALTAZM)
    meridianFlip = FLIP_NEVER;

    // where we are
    homeMount = false;
    atHome = true;
    lastError = ERR_NONE;

    // reset tracking and rates
    timerRateAxis1 = SiderealRate;
    timerRateAxis2 = SiderealRate;

    // not parked, but don't wipe the park position if it's saved - we can still use it
    parkStatus = PRK_UNPARKED;
    EEPROM.write(EE_parkStatus, parkStatus);

    // the polar home position
    startAxis1 = homeStepAxis1;
    startAxis2 = homeStepAxis2;

    // clear pulse-guiding state
    guideDirAxis1 = 0;
    guideDurationAxis1 = 0;
    guideDurationLastAxis1 = 0;
    guideDirAxis2 = 0;
    guideDurationAxis2 = 0;
    guideDurationLastAxis2 = 0;
    enable_Axis(false);

    cli();
    targetAxis1.part.m = startAxis1;
    targetAxis1.part.f = 0;
    posAxis1 = startAxis1;
    targetAxis2.part.m = startAxis2;
    targetAxis2.part.f = 0;
    posAxis2 = startAxis2;
    blAxis1 = 0;
    blAxis2 = 0;
    sei();

    // initialize/disable the stepper drivers
    DecayModeTracking();
    sideralTracking = lastSideralTracking;
    return true;
}

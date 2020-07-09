// -----------------------------------------------------------------------------------
// functions related to Homing the mount
// moves telescope to the home position, then stops tracking
bool goHome()
{
  if ((parkStatus != PRK_UNPARKED) && (parkStatus != PRK_PARKING)) return false; // fail, moving to home not allowed if PRK_PARKED
  if (lastError != ERR_NONE) return false;                                // fail, cannot move if there are errors
  if (movingTo) return false;                      // fail, moving to home not allowed during a move
  if (guideA1.dir || guideA2.dir) return false;                       // fail, moving to home not allowed while guiding
  cli();

  staA1.target = geoA1.homeDef;
  staA2.target = geoA2.homeDef;
  staA1.start = staA1.pos;
  staA2.start = staA2.pos;
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

// resets telescope home position; user manually moves home position,
bool syncPolarHome()
{
  if (movingTo) return false;  // fail, forcing home not allowed during a move
  // default values for state variables
  staA2.dir = true;
  staA1.dir = true;
  newTargetRA = 0;
  newTargetDec = 0;
  newTargetAlt = 0;
  newTargetAzm = 0;
  lastError = ERR_NONE;
  // reset tracking and rates
  staA1.timerRate = SiderealRate;
  staA2.timerRate = SiderealRate;
  parkStatus = PRK_UNPARKED;
  XEEPROM.update(EE_parkStatus, parkStatus);
  // clear pulse-guiding state
  guideA1.dir = 0;
  guideA1.duration = 0;
  guideA1.durationLast = 0;
  guideA2.dir = 0;
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
  return true;
}
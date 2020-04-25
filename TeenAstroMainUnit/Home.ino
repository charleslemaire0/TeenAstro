// -----------------------------------------------------------------------------------
// functions related to Homing the mount
// moves telescope to the home position, then stops tracking
bool goHome()
{
  if ((parkStatus != PRK_UNPARKED) && (parkStatus != PRK_PARKING)) return false; // fail, moving to home not allowed if PRK_PARKED
  if (lastError != ERR_NONE) return false;                                // fail, cannot move if there are errors
  if (movingTo) return false;                      // fail, moving to home not allowed during a move
  if (guideDirAxis1 || guideDirAxis2) return false;                       // fail, moving to home not allowed while guiding
  cli();

  targetAxis1 = homeStepAxis1;
  targetAxis2 = homeStepAxis2;
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

// resets telescope home position; user manually moves home position,
bool syncPolarHome()
{
  if (movingTo) return false;  // fail, forcing home not allowed during a move
  // default values for state variables
  dirAxis2 = true;
  dirAxis1 = true;
  newTargetRA = 0;
  newTargetDec = 0;
  newTargetAlt = 0;
  newTargetAzm = 0;
  lastError = ERR_NONE;
  // reset tracking and rates
  timerRateAxis1 = SiderealRate;
  timerRateAxis2 = SiderealRate;
  parkStatus = PRK_UNPARKED;
  XEEPROM.update(EE_parkStatus, parkStatus);
  // clear pulse-guiding state
  guideDirAxis1 = 0;
  guideDurationAxis1 = 0;
  guideDurationLastAxis1 = 0;
  guideDirAxis2 = 0;
  guideDurationAxis2 = 0;
  guideDurationLastAxis2 = 0;
  // update starting coordinates to reflect NCP or SCP polar home position
  startAxis1 = homeStepAxis1;
  startAxis2 = homeStepAxis2;
  cli();
  targetAxis1 = startAxis1;
  posAxis1 = startAxis1;
  targetAxis2 = startAxis2;
  posAxis2 = startAxis2;
  sei();
  // initialize/disable the stepper drivers
  DecayModeTracking();
  sideralTracking = false;
  atHome = true;
  return true;
}
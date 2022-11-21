// -----------------------------------------------------------------------------------
// functions to move the mount to the a new position

// moves the mount
void moveTo()
{
  // HA goes from +90...0..-90
  //                W   .   E
  static long lastPosAxis2 = 0;
  volatile unsigned long distStartAxis1, distStartAxis2, distDestAxis1, distDestAxis2, d;
  staA1.updateDeltaStart();
  staA2.updateDeltaStart();
  distStartAxis1 = max(abs(staA1.deltaStart),1L);  // distance from start HA
  distStartAxis2 = max(abs(staA2.deltaStart),1L);  // distance from start Dec

Again:
  staA1.updateDeltaTarget();
  staA2.updateDeltaTarget();

  distDestAxis1 = max(abs(staA1.deltaTarget),1L);  // distance from dest HA
  distDestAxis2 = max(abs(staA2.deltaTarget),1L);  // distance from dest Dec
  // adjust rates near the horizon to help keep from exceeding the minAlt limit
  if (!isAltAZ())
  {
    cli();
    volatile long tempPosAxis2 = staA2.pos;
    sei();
    if (tempPosAxis2 != lastPosAxis2)
    {
      bool decreasing = tempPosAxis2 < lastPosAxis2;
      //  Correct according to pier side and latitude
      if ((GetPierSide() == PIER_WEST) == (*localSite.latitude() > 0))
          decreasing = !decreasing;

      // if Dec is decreasing, slow down Dec
      if (decreasing)
      {
        cli();
        unsigned long a = max((currentAlt - minAlt)*geoA2.stepsPerDegree, 1);
        if (a < distDestAxis2)
          distDestAxis2 = a;
        sei();
      }
      else
        // if Dec is increasing, slow down HA
      {
        cli();
        unsigned long a = max((currentAlt - minAlt)*geoA1.stepsPerDegree, 1);
        if (a < distDestAxis1)
          distDestAxis1 = a;
        sei();
      }
    }
    lastPosAxis2 = tempPosAxis2;
  }

  // quickly slow the motors and stop in 1 degree
  if (abortSlew)
  {
    staA1.breakMove();
    guideA1.dir = 'b';
    staA2.breakMove();
    guideA2.dir = 'b';
    if (parkStatus == PRK_PARKING)
    {
      sideralTracking = lastSideralTracking;
      parkStatus = PRK_UNPARKED;
      XEEPROM.write(EE_parkStatus, parkStatus);
    }
    else if (homeMount)
    {
      sideralTracking = lastSideralTracking;
      homeMount = false;
    }
    abortSlew = false;
    goto Again;
  }

  // First, for Right Ascension
  double computed_interval;
  d = distStartAxis1 < distDestAxis1 ? distStartAxis1 : distDestAxis1;

  computed_interval = speed2interval(staA1.speedfromDist(d), maxInterval1);
  cli();
  staA1.interval_Step_Cur = max(computed_interval, minInterval1);
  sei();

  // Now, for Declination
  d = distStartAxis2 < distDestAxis2 ? distStartAxis2 : distDestAxis2;
  computed_interval = speed2interval(staA2.speedfromDist(d), maxInterval2);
  cli();
  staA2.interval_Step_Cur = max(computed_interval, minInterval2);
  sei();
  
  if (staA1.atTarget(false) && staA2.atTarget(false))
  {
    movingTo = false;
    SetsiderealClockSpeed(siderealClockSpeed);
    cli();
    staA1.resetToSidereal();
    staA2.resetToSidereal();
    sei();
    DecayModeTracking();
    // other special gotos: for parking the mount and homeing the mount
    if (parkStatus == PRK_PARKING)
    {
      parkStatus = PRK_FAILED;
      for (int i = 0; i < 12; i++)  // give the drives a moment to settle in
      {
        updateDeltaTarget();
        if (staA1.deltaTarget == 0 && staA2.deltaTarget == 0)
        {
          if (parkClearBacklash())
          {
            parkStatus = PRK_PARKED;// success, we're parked 
            enable_Axis(false);// disable the stepper drivers
          }
          break;
        }
        delay(250);
      }
      XEEPROM.write(EE_parkStatus, parkStatus);
    }
    else if (homeMount)
    {
      parkClearBacklash();
      syncAtHome();
      homeMount = false;
      // disable the stepper drivers
      enable_Axis(false);
    }
  }
}

// if stepper drive can switch decay mode, set it here
void DecayModeTracking()
{
  if (DecayModeTrack) return;
  DecayModeTrack = true;
  motorA1.driver.setCurrent((unsigned int)motorA1.lowCurr);
  motorA2.driver.setCurrent((unsigned int)motorA2.lowCurr);
}

void DecayModeGoto()
{
  if (!DecayModeTrack) return;
  DecayModeTrack = false;
  motorA1.driver.setCurrent((unsigned int)motorA1.highCurr);
  motorA2.driver.setCurrent((unsigned int)motorA2.highCurr);
}


// -----------------------------------------------------------------------------------
// functions to move the mount to the a new position

// moves the mount
void moveTo()
{
  // HA goes from +90...0..-90
  //                W   .   E
  static long lastPosAxis2 = 0;
  volatile long distStartAxis1, distStartAxis2, distDestAxis1, distDestAxis2;
  cli();
  distStartAxis1 = abs(distStepAxis1(&staA1.start, &staA1.pos));  // distance from start HA
  distStartAxis2 = abs(distStepAxis2(&staA2.start, &staA2.pos));  // distance from start Dec
  sei();
  if (distStartAxis1 < 1) distStartAxis1 = 1;
  if (distStartAxis2 < 1) distStartAxis2 = 1;
Again:
  updateDeltaTarget();
  cli();
  long tempPosAxis2 = staA2.pos;
  sei();
  distDestAxis1 = abs(staA1.deltaTarget);  // distance from dest HA
  distDestAxis2 = abs(staA2.deltaTarget);  // distance from dest Dec
  // adjust rates near the horizon to help keep from exceeding the minAlt limit
  if (!isAltAZ())
  {
    if (tempPosAxis2 != lastPosAxis2)
    {
      bool decreasing = tempPosAxis2 < lastPosAxis2;
      if (GetPierSide() >= PIER_WEST)
        decreasing = !decreasing;
      // if Dec is decreasing, slow down Dec
      if (decreasing)
      {
        cli();
        long a = max((currentAlt - minAlt)*geoA2.stepsPerDegree, 1);
        if (a < distDestAxis2)
          distDestAxis2 = a;
        sei();
      }
      else
        // if Dec is increasing, slow down HA
      {
        cli();
        long a = max((currentAlt - minAlt)*geoA1.stepsPerDegree, 1);
        if (a < distDestAxis1)
          distDestAxis1 = a;
        sei();
      }
    }
    lastPosAxis2 = tempPosAxis2;
  }

  if (distDestAxis1 < 1) distDestAxis1 = 1;
  if (distDestAxis2 < 1) distDestAxis2 = 1;

  // quickly slow the motors and stop in 1 degree
  if (abortSlew)
  {
    // set the destination near where we are now
    cli();
    // recompute distances
    updateDeltaTarget();
    long a = pow(getV(staA1.timerRate), 2.) / (2. * staA1.acc);
    if (abs(staA1.deltaTarget) > a)
    {
      if (0 > staA1.deltaTarget)
        a = -a;
      cli()
        staA1.target = staA1.pos + a;
      sei();
    }
    guideA1.dir = 'b';
    a = pow(getV(staA2.timerRate), 2.) / (2. * staA2.acc);
    if (abs(staA2.deltaTarget) > a)
    {
      if (0 > staA2.deltaTarget) // overshoot
        a = -a;
      cli();
      staA2.target = staA2.pos + a;
      sei();
    }
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
  double temp;
  if (distStartAxis1 >= distDestAxis1)
  {
    temp = getRate(sqrt(distDestAxis1 * 2 * staA1.acc)); // slow down (temp gets bigger)
  }
  else
  {
    temp = getRate(sqrt(distStartAxis1 * 2 * staA1.acc)); // speed up (temp gets smaller)
  }
  if (temp < maxRate) temp = maxRate;                            // fastest rate
  if (temp > TakeupRate) temp = TakeupRate;                      // slowest rate
  cli(); staA1.timerRate = temp; sei();

  // Now, for Declination

  if (distStartAxis2 >= distDestAxis2)
  {
    temp = getRate(sqrt(distDestAxis2 * 2 * staA2.acc)); // slow down
  }
  else
  {
    temp = getRate(sqrt(distStartAxis2 * 2 * staA2.acc));// speed up
  }
  if (temp < maxRate) temp = maxRate;                            // fastest rate
  if (temp > TakeupRate) temp = TakeupRate;                      // slowest rate
  cli(); staA2.timerRate = temp; sei();

  double v1 = max(abs(staA1.az_delta / 15.), 1.);
  double v2 = max(abs(staA2.az_delta / 15.), 1.);

  if (atTargetAxis1(true, v1) && atTargetAxis2(true, v2))
  {
    movingTo = false;
    SetSiderealClockRate(siderealInterval);
    cli();
    staA1.timerRate = SiderealRate;
    staA2.timerRate = SiderealRate;
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
      syncPolarHome();
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
  motorA1.driver.setCurrent((unsigned int)motorA1.lowCurr * 10);
  motorA2.driver.setCurrent((unsigned int)motorA2.lowCurr * 10);
}

void DecayModeGoto()
{
  if (!DecayModeTrack) return;
  DecayModeTrack = false;
  motorA1.driver.setCurrent((unsigned int)motorA1.highCurr * 10);
  motorA2.driver.setCurrent((unsigned int)motorA2.highCurr * 10);
}


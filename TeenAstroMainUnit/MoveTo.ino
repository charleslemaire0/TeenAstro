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
  distStartAxis1 = max(abs(staA1.deltaStart), 1L);  // distance from start HA
  distStartAxis2 = max(abs(staA2.deltaStart), 1L);  // distance from start Dec

Again:
  staA1.updateDeltaTarget();
  staA2.updateDeltaTarget();

  distDestAxis1 = max(abs(staA1.deltaTarget), 1L);  // distance from dest HA
  distDestAxis2 = max(abs(staA2.deltaTarget), 1L);  // distance from dest Dec
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
      if ((GetPoleSide() == POLE_OVER) == localSite.northHemisphere())
        decreasing = !decreasing;

      // if Dec is decreasing, slow down Dec
      if (decreasing)
      {
        cli();
        unsigned long a = max((currentAlt - minAlt) * geoA2.stepsPerDegree, 1);
        if (a < distDestAxis2)
          distDestAxis2 = a;
        sei();
      }
      else
        // if Dec is increasing, slow down HA
      {
        cli();
        unsigned long a = max((currentAlt - minAlt) * geoA1.stepsPerDegree, 1);
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
    staA1.breakMoveLowRate();
    guideA1.brake();
    staA2.breakMoveLowRate();
    guideA2.brake();
    if (parkStatus == PRK_PARKING)
    {
      sideralTracking = lastSideralTracking;
      parkStatus = PRK_UNPARKED;
      backlashStatus = DONE;
      XEEPROM.write(getMountAddress(EE_parkStatus), parkStatus);
    }
    else if (homeMount)
    {
      backlashStatus = DONE;
      sideralTracking = lastSideralTracking;
      homeMount = false;
    }
    abortSlew = false;
    goto Again;
  }

  // First, for Right Ascension
  d = distStartAxis1 < distDestAxis1 ? distStartAxis1 : distDestAxis1;
  cli();
  staA1.setIntervalfromDist(d, minInterval1, maxInterval1);
  sei();

  // Now, for Declination
  d = distStartAxis2 < distDestAxis2 ? distStartAxis2 : distDestAxis2;
  cli();
  staA2.setIntervalfromDist(d, minInterval2, maxInterval2);
  sei();


  bool atTarget = staA1.atTarget(false) && staA2.atTarget(false);
  if (parkStatus == PRK_PARKING || homeMount)
  {
    updateDeltaTarget();
    atTarget = staA1.deltaTarget == 0 && staA2.deltaTarget == 0;
  }
    
  if (atTarget)
  {
    movingTo = false;
    SetsiderealClockSpeed(siderealClockSpeed);
    cli();
    staA1.resetToSidereal();
    staA2.resetToSidereal();
    sei();
    if (homeMount)
    {
      finalizeHome();
    }
    else if(parkStatus == PRK_PARKING)
    {
      finalizePark();
    }
    DecayModeTracking();
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


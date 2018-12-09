// -----------------------------------------------------------------------------------
// functions to move the mount to the a new position

static long lastPosAxis2 = 0;



// moves the mount
void moveTo() {
  // HA goes from +90...0..-90
  //                W   .   E
  long distStartAxis1, distStartAxis2, distDestAxis1, distDestAxis2;
  cli();
  distStartAxis1 = abs(distStepAxis1(startAxis1, posAxis1));  // distance from start HA
  distStartAxis2 = abs(distStepAxis2(startAxis2, posAxis2));  // distance from start Dec
  sei();
  if (distStartAxis1 < 1) distStartAxis1 = 1;
  if (distStartAxis2 < 1) distStartAxis2 = 1;
Again:
  updateDeltaTarget();
  cli();
  long tempPosAxis2 = posAxis2;
  sei();
  distDestAxis1 = abs(deltaTargetAxis1);  // distance from dest HA
  distDestAxis2 = abs(deltaTargetAxis2);  // distance from dest Dec


  // adjust rates near the horizon to help keep from exceeding the minAlt limit
  if (mountType != MOUNT_TYPE_ALTAZM)
  {
    if ( tempPosAxis2 != lastPosAxis2) {
      bool decreasing = tempPosAxis2 < lastPosAxis2;
      if (pierSide >= PierSideWest)
        decreasing = !decreasing;
      // if Dec is decreasing, slow down Dec
      if (decreasing)
      {
        cli();
        long a = max((currentAlt - minAlt)*StepsPerDegreeAxis2, 1);
        if (a < distDestAxis2)
          distDestAxis2 = a;
        sei();
      }
      else
        // if Dec is increasing, slow down HA
      {
        cli();
        long a = max((currentAlt - minAlt)*StepsPerDegreeAxis1, 1);
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

    long a = getV(timerRateAxis1)*getV(timerRateAxis1) / (2. * AccAxis1);
    if (abs(deltaTargetAxis1) > a)
    {
      if (0 > deltaTargetAxis1)
        a = -a;
      cli()
      targetAxis1.part.m = posAxis1 + a;
      targetAxis1.part.f = 0;
      sei();
    }
    guideDirAxis1 = 'b';

    a = getV(timerRateAxis2)*getV(timerRateAxis2) / (2. * AccAxis2);
    if (abs(deltaTargetAxis2) > a)
    {
      if (0 > deltaTargetAxis2) // overshoot
        a = -a;
      cli();
      targetAxis2.part.m = posAxis2 + a;
      targetAxis2.part.f = 0;
      sei();
    }
    guideDirAxis2 = 'b';

    if (parkStatus == Parking)
    {
      lastTrackingState = abortTrackingState;
      parkStatus = NotParked;
      EEPROM.write(EE_parkStatus, parkStatus);
    }
    else if (homeMount)
    {
      lastTrackingState = abortTrackingState;
      homeMount = false;
    }

    abortSlew = false;
    goto Again;
  }

  // First, for Right Ascension
  double temp;
  if (distStartAxis1 >= distDestAxis1)
  {
    temp = getRate(sqrt(distDestAxis1 * 2 * AccAxis1)); // slow down (temp gets bigger)
  }
  else
  { 
    temp = getRate(sqrt(distStartAxis1 * 2 * AccAxis1)); // speed up (temp gets smaller)
  }
  if (temp < maxRate) temp = maxRate;                            // fastest rate
  if (temp > TakeupRate) temp = TakeupRate;                      // slowest rate
  cli(); timerRateAxis1 = temp; sei();

  // Now, for Declination
  
  if (distStartAxis2 >= distDestAxis2)
  {
    temp = getRate(sqrt(distDestAxis2 * 2 * AccAxis2)); // slow down
  }
  else
  {
    temp = getRate(sqrt(distStartAxis2 * 2 * AccAxis2));// speed up
  }
  if (temp < maxRate) temp = maxRate;                            // fastest rate
  if (temp > TakeupRate) temp = TakeupRate;                      // slowest rate
  cli(); timerRateAxis2 = temp; sei();

  if (mountType == MOUNT_TYPE_ALTAZM)
  {
    // In AltAz mode & at the end of slew & near the Zenith, disable tracking for a moment if we're getting close to the target
    if ((distDestAxis1 <= (long)StepsPerDegreeAxis1 * 2L) && (distDestAxis2 <= (long)StepsPerDegreeAxis2 * 2L)) {
      if ((long)targetAxis2.part.m > 80L * (long)StepsPerDegreeAxis2 ) {
        if (lastTrackingState == TrackingON) {
          lastTrackingState = TrackingOFF;
        }
      }
    }
  }
  updateDeltaTarget();

  distDestAxis1 = abs(deltaTargetAxis1);  // distance from dest HA
  distDestAxis2 = abs(deltaTargetAxis2);  // distance from dest Dec

  if ((distDestAxis1 <= 2) && (distDestAxis2 <= 2))
  {
    if (mountType == MOUNT_TYPE_ALTAZM)
    {
      // Near the Zenith disable tracking in AltAz mode for a moment if we're getting close to the target
      if (lastTrackingState == TrackingOFF)
        lastTrackingState = TrackingON;
    }
    // restore last tracking state
    trackingState = lastTrackingState;
    SetSiderealClockRate(siderealInterval);

    cli();
    timerRateAxis1 = SiderealRate;
    timerRateAxis2 = SiderealRate;
    sei();

    DecayModeTracking();

    // other special gotos: for parking the mount and homeing the mount
    if (parkStatus == Parking)
    {
      parkStatus = ParkFailed;

      for (int i = 0; i < 12; i++)  // give the drives a moment to settle in
      {
        updateDeltaTarget();
        if (deltaTargetAxis1 == 0 && deltaTargetAxis2 == 0)
        {
          if (parkClearBacklash())
          {
            parkStatus = Parked;// success, we're parked 
            enable_Axis(false);// disable the stepper drivers
          }
          break;
        }
        delay(250);
      }
      EEPROM.write(EE_parkStatus, parkStatus);

    }
    else if (homeMount) {
      parkClearBacklash();
      setHome();
      homeMount = false;
      atHome = true;

      // disable the stepper drivers
      enable_Axis(false);
    }
  }
}

// fast integer square root routine, Integer Square Roots by Jack W. Crenshaw
uint32_t isqrt32(uint32_t n) {
  register uint32_t root = 0, remainder, place = 0x40000000;
  remainder = n;

  while (place > remainder) place = place >> 2;
  while (place) {
    if (remainder >= root + place) {
      remainder = remainder - root - place;
      root = root + (place << 1);
    }
    root = root >> 1;
    place = place >> 2;
  }
  return root;
}

bool DecayModeTrack = false;

// if stepper drive can switch decay mode, set it here
void DecayModeTracking() {
  if (DecayModeTrack) return;
  DecayModeTrack = true;
  cli();
  motorAxis1.setCurrent((unsigned int)LowCurrAxis1*10);
  motorAxis2.setCurrent((unsigned int)LowCurrAxis2*10);
  sei();
}

void DecayModeGoto() {
  if (!DecayModeTrack) return;
  DecayModeTrack = false;
  cli();
  motorAxis1.setCurrent((unsigned int)HighCurrAxis1*10);
  motorAxis2.setCurrent((unsigned int)HighCurrAxis2*10);
  sei();
}

void pinModeOpen(int pin) {
#if defined(__arm__) && defined(TEENSYDUINO)
  pinMode(pin, OUTPUT_OPENDRAIN); digitalWrite(pin, HIGH);
#else
  pinMode(pin, INPUT);
#endif
}


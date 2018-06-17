// -----------------------------------------------------------------------------------
// functions to move the mount to the a new position

long lastPosAxis2 = 0;



// moves the mount
void moveTo() {
  // HA goes from +90...0..-90
  //                W   .   E
  // meridian flip, first phase.  only happens for GEM mounts
  if ((pierSide == PierSideFlipEW1) || (pierSide == PierSideFlipWE1))
  {

    // save destination
    cli();
    origTargetAxis1.fixed = targetAxis1.fixed;
    origTargetAxis2 = (long)targetAxis2.part.m;

    timerRateAxis1 = SiderealRate;
    timerRateAxis2 = SiderealRate;
    sei();

    // first phase, decide if we should move to 60 deg. HA (4 hours) to get away from the horizon limits or just go straight to the home position
    cli();

    targetAxis1.part.m = celestialPoleStepAxis1;
    targetAxis1.part.f = 0;
    targetAxis2.part.m = celestialPoleStepAxis2;
    targetAxis2.part.f = 0;

    sei();
    pierSide++;
  }

  long distStartAxis1, distStartAxis2, distDestAxis1, distDestAxis2;

  cli();
  distStartAxis1 = abs(posAxis1 - startAxis1);  // distance from start HA
  distStartAxis2 = abs(posAxis2 - startAxis2);  // distance from start Dec
  sei();
  if (distStartAxis1 < 1) distStartAxis1 = 1;
  if (distStartAxis2 < 1) distStartAxis2 = 1;
Again:
  cli();
  long tempPosAxis2 = posAxis2;
  distDestAxis1 = abs(posAxis1 - (long)targetAxis1.part.m);  // distance from dest HA
  distDestAxis2 = abs(tempPosAxis2 - (long)targetAxis2.part.m);  // distance from dest Dec
  sei();

  // adjust rates near the horizon to help keep from exceeding the minAlt limit
  if (mountType != MOUNT_TYPE_ALTAZM)
  {
    if (distDestAxis1 > (DegreesForAcceleration*StepsPerDegreeAxis1) / 16.0 && tempPosAxis2 != lastPosAxis2) {
      bool decreasing = tempPosAxis2 < lastPosAxis2;
      if (pierSide >= PierSideWest)
        decreasing = !decreasing;
      // if Dec is decreasing, slow down Dec
      if (decreasing)
      {
        cli();
        long a = max((currentAlt - minAlt - DegreesForRapidStop)*StepsPerDegreeAxis2 / 4, 1);
        if (a < distDestAxis2)
          distDestAxis2 = a;
        sei();
      }
      else
        // if Dec is increasing, slow down HA
      {
        cli();
        long a = max((currentAlt - minAlt - DegreesForRapidStop)*StepsPerDegreeAxis1 / 4, 1);
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
    // aborts any meridian flip
    if ((pierSide == PierSideFlipWE1) || (pierSide == PierSideFlipWE2) || (pierSide == PierSideFlipWE3)) pierSide = PierSideWest;
    if ((pierSide == PierSideFlipEW1) || (pierSide == PierSideFlipEW2) || (pierSide == PierSideFlipEW3)) pierSide = PierSideEast;

    // set the destination near where we are now
    cli();

    long a = (long)(StepsPerDegreeAxis1*DegreesForRapidStop);
    if (distDestAxis1 > a)
    {
      if (posAxis1 > (long)targetAxis1.part.m)
        a = -a;
      targetAxis1.part.m = posAxis1 + a;
      targetAxis1.part.f = 0;
    }

    a = (long)(StepsPerDegreeAxis2*DegreesForRapidStop);
    if (distDestAxis2 > a)
    {
      if (posAxis2 > (long)targetAxis2.part.m) // overshoot
        a = -a;
      targetAxis2.part.m = posAxis2 + a;
      targetAxis2.part.f = 0;
    }
    sei();

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
  long temp;
  if (distStartAxis1 > distDestAxis1)
  {
    temp = (StepsForRateChangeAxis1 / isqrt32(distDestAxis1));   // slow down (temp gets bigger)
  }
  else
  {
    temp = (StepsForRateChangeAxis1 / isqrt32(distStartAxis1));  // speed up (temp gets smaller)
  }
  if (temp < maxRate) temp = maxRate;                            // fastest rate
  if (temp > TakeupRate) temp = TakeupRate;                      // slowest rate
  cli(); timerRateAxis1 = temp; sei();

  // Now, for Declination
  if (distStartAxis2 > distDestAxis2)
  {
    temp = (StepsForRateChangeAxis2 / isqrt32(distDestAxis2));   // slow down
  }
  else
  {
    temp = (StepsForRateChangeAxis2 / isqrt32(distStartAxis2));  // speed up
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

  if ((distDestAxis1 <= 2) && (distDestAxis2 <= 2))
  {
    if (mountType == MOUNT_TYPE_ALTAZM)
    {
      // Near the Zenith disable tracking in AltAz mode for a moment if we're getting close to the target
      if (lastTrackingState == TrackingOFF)
        lastTrackingState = TrackingON;
    }

    if ((pierSide == PierSideFlipEW2) || (pierSide == PierSideFlipWE2))
    {
      // make sure we're at the home position just before flipping sides of the mount
      startAxis1 = posAxis1;
      startAxis2 = posAxis2;
      cli();
      if (celestialPoleStepAxis1 == 0)
      {
        // for fork mounts
        if (pierSide == PierSideFlipEW2)
          targetAxis1.part.m = halfRotAxis1;
        else
          targetAxis1.part.m = -halfRotAxis1;
        targetAxis1.part.f = 0;
      }
      else
      {
        // for eq mounts
        if (pierSide == PierSideFlipWE2)
          targetAxis1.part.m = -celestialPoleStepAxis1;
        else
          targetAxis1.part.m = celestialPoleStepAxis1;

        targetAxis1.part.f = 0;
      }

      targetAxis1.part.m = celestialPoleStepAxis1;
      targetAxis1.part.f = 0;
      targetAxis2.part.m = celestialPoleStepAxis2;
      targetAxis2.part.f = 0;
     
      pierSide++;
      sei();
    }
    else if ((pierSide == PierSideFlipEW3) || (pierSide == PierSideFlipWE3))
    {

      // the blAxis2 gets "reversed" when we Meridian flip, since the NORTH/SOUTH movements are reversed
      /*cli(); blAxis2 = backlashAxis2 - blAxis2; sei();*/
      cli();
      startAxis1 = posAxis1;
      targetAxis1.fixed = origTargetAxis1.fixed;
      startAxis2 = posAxis2;
      targetAxis2.part.m = origTargetAxis2;
      targetAxis2.part.f = 0;
      sei();
    }
    else
    {
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
        cli();
        int axis1 = posAxis1;
        int axis2 = posAxis2;
        sei();
        for (int i = 0; i < 12; i++)  // give the drives a moment to settle in
        {
          if ((axis1 == (long)targetAxis1.part.m) && (axis2 == (long)targetAxis2.part.m))
          {
            if (parkClearBacklash())
            {
              parkStatus = Parked;// success, we're parked 
              enable_Axis(false);// disable the stepper drivers
            }
            break;
          }
          delay(250);
          cli();
          axis1 = posAxis1;
          axis2 = posAxis2;
          sei();     
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
  tmc26XStepper1->setCurrent((unsigned int)LowCurrAxis1*10);
  tmc26XStepper2->setCurrent((unsigned int)LowCurrAxis2*10);
#if defined(DECAY_MODE_OPEN)
  pinModeOpen(Axis1_Mode);
  pinModeOpen(Axis2_Mode);
#elif defined(DECAY_MODE_LOW)
  pinMode(Axis1_Mode, OUTPUT); digitalWrite(Axis1_Mode, LOW);
  pinMode(Axis2_Mode, OUTPUT); digitalWrite(Axis1_Mode, LOW);
#elif defined(DECAY_MODE_HIGH)
  pinMode(Axis1_Mode, OUTPUT); digitalWrite(Axis1_Mode, HIGH);
  pinMode(Axis2_Mode, OUTPUT); digitalWrite(Axis2_Mode, HIGH);
#elif defined(MODE_SWITCH_BEFORE_SLEW_ON)
#ifdef AXIS1_MODE
  if ((AXIS1_MODE & 0b001000) == 0) { pinMode(Axis1_M0, OUTPUT); digitalWrite(Axis1_M0, (AXIS1_MODE & 1)); }
  else { pinModeOpen(Axis1_M0); }
  if ((AXIS1_MODE & 0b010000) == 0) { pinMode(Axis1_M1, OUTPUT); digitalWrite(Axis1_M1, (AXIS1_MODE >> 1 & 1)); }
  else { pinModeOpen(Axis1_M1); }
  if ((AXIS1_MODE & 0b100000) == 0) { pinMode(Axis1_M2, OUTPUT); digitalWrite(Axis1_M2, (AXIS1_MODE >> 2 & 1)); }
  else { pinModeOpen(Axis1_M2); }
#endif
#ifdef AXIS2_MODE
  if ((AXIS2_MODE & 0b001000) == 0) { pinMode(Axis2_M0, OUTPUT); digitalWrite(Axis2_M0, (AXIS2_MODE & 1)); }
  else { pinModeOpen(Axis2_M0); }
  if ((AXIS2_MODE & 0b010000) == 0) { pinMode(Axis2_M1, OUTPUT); digitalWrite(Axis2_M1, (AXIS2_MODE >> 1 & 1)); }
  else { pinModeOpen(Axis2_M1); }
  if ((AXIS2_MODE & 0b100000) == 0) { pinMode(Axis2_M2, OUTPUT); digitalWrite(Axis2_M2, (AXIS2_MODE >> 2 & 1)); }
  else { pinModeOpen(Axis2_M2); }
#endif
#elif defined(MODE_SWITCH_BEFORE_SLEW_SPI)
  bool nintpol = ((AXIS1_MODE & 0b0010000) != 0);
  bool stealth = ((AXIS1_MODE & 0b0100000) != 0);
  bool lowpwr = ((AXIS1_MODE & 0b1000000) != 0);
  //       SS      ,SCK     ,MISO     ,MOSI
  spiStart(Axis1_M2, Axis1_M1, Axis1_Aux, Axis1_M0);
  TMC2130_setup(!nintpol, stealth, AXIS1_MODE & 0b001111, lowpwr);  // default 256x interpolation ON, stealthChop OFF (spreadCycle), micro-steps
  spiEnd();
  nintpol = ((AXIS2_MODE & 0b0010000) != 0);
  stealth = ((AXIS2_MODE & 0b0100000) != 0);
  lowpwr = ((AXIS2_MODE & 0b1000000) != 0);
  spiStart(Axis2_M2, Axis2_M1, Axis2_Aux, Axis2_M0);
  TMC2130_setup(!nintpol, stealth, AXIS2_MODE & 0b001111, lowpwr);
  spiEnd();

  // allow stealth chop current regulation to ramp up to the initial motor current before moving
  if ((((AXIS1_MODE & 0b0100000) != 0) || ((AXIS2_MODE & 0b0100000) != 0)) & (atHome)) delay(100);
#endif

#if defined(AXIS1_MODE) && defined(AXIS1_MODE_GOTO)
  stepAxis1 = 1;
  stepAxis2 = 1;
#endif
#ifdef MODE_SWITCH_SLEEP_ON 
  delay(3);
#endif
  sei();
}

void DecayModeGoto() {
  if (!DecayModeTrack) return;

  DecayModeTrack = false;
  cli();
  tmc26XStepper1->setCurrent((unsigned int)HighCurrAxis1*10);
  tmc26XStepper2->setCurrent((unsigned int)HighCurrAxis2*10);
#if defined(DECAY_MODE_GOTO_OPEN)
  pinModeOpen(Axis1_Mode);
  pinModeOpen(Axis2_Mode);
#elif defined(DECAY_MODE_GOTO_LOW)
  pinMode(Axis1_Mode, OUTPUT); digitalWrite(Axis1_Mode, LOW);
  pinMode(Axis2_Mode, OUTPUT); digitalWrite(Axis1_Mode, LOW);
#elif defined(DECAY_MODE_GOTO_HIGH)
  pinMode(Axis1_Mode, OUTPUT); digitalWrite(Axis1_Mode, HIGH);
  pinMode(Axis2_Mode, OUTPUT); digitalWrite(Axis2_Mode, HIGH);
#elif defined(MODE_SWITCH_BEFORE_SLEW_ON)
#ifdef AXIS1_MODE_GOTO
  if ((AXIS1_MODE_GOTO & 0b001000) == 0) { pinMode(Axis1_M0, OUTPUT); digitalWrite(Axis1_M0, (AXIS1_MODE_GOTO & 1)); }
  else { pinModeOpen(Axis1_M0); }
  if ((AXIS1_MODE_GOTO & 0b010000) == 0) { pinMode(Axis1_M1, OUTPUT); digitalWrite(Axis1_M1, (AXIS1_MODE_GOTO >> 1 & 1)); }
  else { pinModeOpen(Axis1_M1); }
  if ((AXIS1_MODE_GOTO & 0b100000) == 0) { pinMode(Axis1_M2, OUTPUT); digitalWrite(Axis1_M2, (AXIS1_MODE_GOTO >> 2 & 1)); }
  else { pinModeOpen(Axis1_M2); }
#endif
#ifdef AXIS2_MODE_GOTO
  if ((AXIS2_MODE_GOTO & 0b001000) == 0) { pinMode(Axis2_M0, OUTPUT); digitalWrite(Axis2_M0, (AXIS2_MODE_GOTO & 1)); }
  else { pinModeOpen(Axis2_M0); }
  if ((AXIS2_MODE_GOTO & 0b010000) == 0) { pinMode(Axis2_M1, OUTPUT); digitalWrite(Axis2_M1, (AXIS2_MODE_GOTO >> 1 & 1)); }
  else { pinModeOpen(Axis2_M1); }
  if ((AXIS2_MODE_GOTO & 0b100000) == 0) { pinMode(Axis2_M2, OUTPUT); digitalWrite(Axis2_M2, (AXIS2_MODE_GOTO >> 2 & 1)); }
  else { pinModeOpen(Axis2_M2); }
#endif
#elif defined(MODE_SWITCH_BEFORE_SLEW_SPI)
  bool nintpol = ((AXIS1_MODE_GOTO & 0b0010000) != 0);
  bool stealth = ((AXIS1_MODE_GOTO & 0b0100000) != 0);
  bool lowpwr = ((AXIS1_MODE_GOTO & 0b1000000) != 0);
  //       CS      ,SCK     ,MISO     ,MOSI
  spiStart(Axis1_M2, Axis1_M1, Axis1_Aux, Axis1_M0);
  TMC2130_setup(!nintpol, stealth, AXIS1_MODE_GOTO & 0b001111, lowpwr);  // default 256x interpolation ON, stealthChop OFF (spreadCycle), micro-steps
  spiEnd();
  nintpol = ((AXIS2_MODE_GOTO & 0b0010000) != 0);
  stealth = ((AXIS2_MODE_GOTO & 0b0100000) != 0);
  lowpwr = ((AXIS2_MODE_GOTO & 0b1000000) != 0);
  spiStart(Axis2_M2, Axis2_M1, Axis2_Aux, Axis2_M0);
  TMC2130_setup(!nintpol, stealth, AXIS2_MODE_GOTO & 0b001111, lowpwr);
  spiEnd();
#endif

#if defined(AXIS1_MODE) && defined(AXIS1_MODE_GOTO)
  stepAxis1 = AXIS1_STEP_GOTO;
  stepAxis2 = AXIS2_STEP_GOTO;
#endif
#ifdef MODE_SWITCH_SLEEP_ON
  delay(3);
#endif
  sei();
}

void pinModeOpen(int pin) {
#if defined(__arm__) && defined(TEENSYDUINO)
  pinMode(pin, OUTPUT_OPENDRAIN); digitalWrite(pin, HIGH);
#else
  pinMode(pin, INPUT);
#endif
}


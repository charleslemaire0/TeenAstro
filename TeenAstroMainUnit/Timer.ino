// -----------------------------------------------------------------------------------
// Timers and interrupt handling

#define ISR(f)  void f (void)
void                TIMER1_COMPA_vect(void);
volatile boolean    clearAxis1 = true;
volatile boolean    takeStepAxis1 = false;
volatile boolean    clearAxis2 = true;
volatile boolean    takeStepAxis2 = false;
IntervalTimer       itimer1;

double getV(double rate) //Speed in step per second
{
  return 1000000.0 / (rate / 16.);
}


double getRate(double V)
{
  return max(16. * 1000000.0 / V, maxRate);
}


//--------------------------------------------------------------------------------------------------

// set timer1 to rate (in microseconds*16)
void Timer1SetRate(double rate)
{
  itimer1.begin(TIMER1_COMPA_vect, rate * 0.0625);
}

// set the master sidereal clock rate, also forces rate update for RA/Dec timer rates so that PPS adjustments take hold immediately
volatile double isrTimerRateAxis1 = 0;
volatile double isrTimerRateAxis2 = 0;
volatile double runtimerRateAxis1 = 0;
volatile double runTimerRateAxis2 = 0;

void SetSiderealClockRate(double Interval)
{
  Timer1SetRate(Interval / 100);
  isrTimerRateAxis1 = 0;
  isrTimerRateAxis2 = 0;
}

// set timer3 to rate (in microseconds*16)
volatile uint32_t   nextAxis1Rate = 100000UL;
void Timer3SetRate(double rate)
{
  cli();
  nextAxis1Rate = (F_BUS / 1000000) * (rate * 0.0625) * 0.5 - 1;
  sei();
}

// set timer4 to rate (in microseconds*16)
volatile uint32_t   nextAxis2Rate = 100000UL;
void Timer4SetRate(double rate)
{
  cli();
  nextAxis2Rate = (F_BUS / 1000000) * (rate * 0.0625) * 0.5 - 1;
  sei();
}

//--------------------------------------------------------------------------------------------------
// Timer1 handles sidereal time and programming the drive rates
volatile boolean    wasInbacklashAxis1 = false;
volatile boolean    wasInbacklashAxis2 = false;

volatile boolean    gotoRateAxis1 = false;
volatile boolean    gotoRateAxis2 = false;
volatile byte       cnt = 0;

volatile double     guideTimerRateAxis1A = 0;
volatile double     guideTimerRateAxis2A = 0;

void updateDeltaTarget()
{
  cli();
  deltaTargetAxis1 = distStepAxis1(posAxis1, targetAxis1);
  deltaTargetAxis2 = distStepAxis2(posAxis2, targetAxis2);
  sei();
}

ISR(TIMER1_COMPA_vect)
{
  // run 1/3 of the time at 3x the rate, unless a goto is happening
  rtk.m_lst++;

  if (!movingTo)
  {
    double maxguideTimerRate = 4;
    // automatic rate calculation HA
    {
      // guide rate acceleration/deceleration and control
      updateDeltaTarget();

      double  x = deltaTargetAxis1;
      bool other_axis_done = fabs(deltaTargetAxis2) < BreakDistAxis2;

      if (!inbacklashAxis1 && guideDirAxis1)
      {
        if ((fabs(guideTimerRateAxis1) < maxguideTimerRate) &&
          (fabs(guideTimerRateAxis1A) < maxguideTimerRate))
        {
          // break mode
          if (guideDirAxis1 == 'b')
          {
            guideTimerRateAxis1 = trackingTimerRateAxis1;
            if (guideTimerRateAxis1 >= 0)
              guideTimerRateAxis1 = 1.0;
            else
              guideTimerRateAxis1 = -1.0;
          }

          // slow speed guiding, no acceleration
          guideTimerRateAxis1A = guideTimerRateAxis1;
        }
        else
        {
          // use acceleration
          DecayModeGoto();
          double z = getRate(sqrt(fabs(x) * 2 * AccAxis1));
          guideTimerRateAxis1A = (1.0 / ((StepsPerDegreeAxis1 * (z / 1000000.0))) * 3600.0);
          if (guideTimerRateAxis1A < maxguideTimerRate) guideTimerRateAxis1A = maxguideTimerRate;
        }

        // stop guiding
        if (guideDirAxis1 == 'b')
        {
          if (fabs(x) < BreakDistAxis1)
          {
            guideDirAxis1 = 0;
            guideTimerRateAxis1 = 0;
            guideTimerRateAxis1A = 0;
            if (other_axis_done)
              DecayModeTracking();
          }
        }
      }
      double timerRateAxis1A = trackingTimerRateAxis1;
      double timerRateAxis1B = fabs(guideTimerRateAxis1A + timerRateAxis1A);
      double calculatedTimerRateAxis1;
      // round up to run the motor timers just a tiny bit slow, then adjust below if we start to fall behind during sidereal tracking
      if (timerRateAxis1B > 0.5)
        calculatedTimerRateAxis1 = SiderealRate / timerRateAxis1B;
      else
        calculatedTimerRateAxis1 = (double)SiderealRate * 2.0;


      // remember our "running" rate and only update the actual rate when it changes
      if (runtimerRateAxis1 != calculatedTimerRateAxis1)
      {
        timerRateAxis1 = calculatedTimerRateAxis1;
        runtimerRateAxis1 = calculatedTimerRateAxis1;
      }
    }
    // automatic rate calculation Dec
    {
      // guide rate acceleration/deceleration
      updateDeltaTarget();

      double x = fabs(deltaTargetAxis2);
      bool other_axis_done = fabs(deltaTargetAxis1) < BreakDistAxis1;

      if (!inbacklashAxis2 && guideDirAxis2)
      {
        if ((fabs(guideTimerRateAxis2) < maxguideTimerRate) &&
          (fabs(guideTimerRateAxis2A) < maxguideTimerRate))
        {
          // break mode
          if (guideDirAxis2 == 'b')
          {
            guideTimerRateAxis2 = trackingTimerRateAxis2;
            if (guideTimerRateAxis2 >= 0)
              guideTimerRateAxis2 = 1.0;
            else
              guideTimerRateAxis2 = -1.0;
          }

          // slow speed guiding, no acceleration
          guideTimerRateAxis2A = guideTimerRateAxis2;
        }
        else
        {
          // use acceleration
          DecayModeGoto();
          double z = getRate(sqrt(fabs(x) * 2 * AccAxis2));
          guideTimerRateAxis2A = (1.0 / (((double)StepsPerDegreeAxis2 * (z / 1000000.0))) * 3600.0);
          if (guideTimerRateAxis2A < maxguideTimerRate) guideTimerRateAxis2A = maxguideTimerRate;
        }

        // stop guiding
        if (guideDirAxis2 == 'b')
        {
          if (x < BreakDistAxis2)
          {
            guideDirAxis2 = 0;
            guideTimerRateAxis2 = 0;
            guideTimerRateAxis2A = 0;
            if (other_axis_done)
              DecayModeTracking();
          }
        }
      }
      double timerRateAxis2A = trackingTimerRateAxis2;
      double timerRateAxis2B = fabs(guideTimerRateAxis2A + timerRateAxis2A);
      double calculatedTimerRateAxis2;
      // round up to run the motor timers just a tiny bit slow, then adjust below if we start to fall behind during sidereal tracking
     // calculatedTimerRateAxis2 = (double)SiderealRate / timerRateAxis2B;
      if (timerRateAxis2B > 0.5)
        calculatedTimerRateAxis2 = (double)SiderealRate / timerRateAxis2B;
      else
        calculatedTimerRateAxis2 = (double)SiderealRate * 2.0;

      // remember our "running" rate and only update the actual rate when it changes
      if (runTimerRateAxis2 != calculatedTimerRateAxis2)
      {
        timerRateAxis2 = calculatedTimerRateAxis2;
        runTimerRateAxis2 = calculatedTimerRateAxis2;
      }
    }

    if (!guideDirAxis1 && !guideDirAxis2)
    {
      GuidingState = GuidingOFF;
    }
  }

  double thisTimerRateAxis1 = timerRateAxis1;
  double thisTimerRateAxis2 = timerRateAxis2 * timerRateRatio;

  timerRateAxis2 = max(timerRateAxis2, maxRate);
  thisTimerRateAxis2 = timerRateAxis2;

  // override rate during backlash compensation
  if (inbacklashAxis1)
  {
    thisTimerRateAxis1 = timerRateBacklashAxis1;
    wasInbacklashAxis1 = true;
  }

  // override rate during backlash compensation
  if (inbacklashAxis2)
  {
    thisTimerRateAxis2 = timerRateBacklashAxis2;
    wasInbacklashAxis2 = true;
  }
  if (sideralTracking && !movingTo)
  {
    // travel through the backlash is done, but we weren't following the target while it was happening!
    // so now get us back to near where we need to be
    updateDeltaTarget();
    if (!inbacklashAxis1 && wasInbacklashAxis1 && !guideDirAxis1)
    {
      cli();
      if (abs(deltaTargetAxis1) > 2)
        thisTimerRateAxis1 = TakeupRate;
      else
        wasInbacklashAxis1 = false;
      sei();
    }
    if (!inbacklashAxis2 && wasInbacklashAxis2 && !guideDirAxis2)
    {
      cli();
      if (abs(deltaTargetAxis2) > 2)
        thisTimerRateAxis2 = TakeupRate;
      else
        wasInbacklashAxis2 = false;
      sei();
    }
  }
  // set the rates
  if (thisTimerRateAxis1 != isrTimerRateAxis1)
  {
    Timer3SetRate(thisTimerRateAxis1);
    isrTimerRateAxis1 = thisTimerRateAxis1;
  }
  if (thisTimerRateAxis2 != isrTimerRateAxis2)
  {
    Timer4SetRate(thisTimerRateAxis2);
    isrTimerRateAxis2 = thisTimerRateAxis2;
  }
}
ISR(TIMER3_COMPA_vect)
{
  digitalWriteFast(Axis1StepPin, LOW);
  if (clearAxis1)
  {
    takeStepAxis1 = false;
    updateDeltaTarget();
    if (deltaTargetAxis1 != 0 || inbacklashAxis1)
    {                       
      // Move the RA stepper to the target
      dirAxis1 = 0 < deltaTargetAxis1;
      // Direction control
      if (ReverseAxis1^Axis1Reverse)
      {
        if (HADir == dirAxis1)
          digitalWriteFast(Axis1DirPin, LOW);
        else
          digitalWriteFast(Axis1DirPin, HIGH);
      }
      else
      {
        if (HADir == dirAxis1)
          digitalWriteFast(Axis1DirPin, HIGH);
        else
          digitalWriteFast(Axis1DirPin, LOW);
      }

      // telescope moves WEST with the sky, blAxis1 is the amount of EAST backlash
      if (dirAxis1)
      {
        if (blAxis1 < StepsBacklashAxis1)
        {
          blAxis1 += stepAxis1;
          inbacklashAxis1 = true;
        }
        else
        {
          inbacklashAxis1 = false;
          posAxis1 += stepAxis1;
        }
      }
      else
      {
        if (blAxis1 > 0)
        {
          blAxis1 -= stepAxis1;
          inbacklashAxis1 = true;
        }
        else
        {
          inbacklashAxis1 = false;
          posAxis1 -= stepAxis1;
        }
      }
      takeStepAxis1 = true;
    }

    clearAxis1 = false;
  }
  else
  {
    if (takeStepAxis1)
    {
      digitalWriteFast(Axis1StepPin, HIGH);
    }
    clearAxis1 = true;
    PIT_LDVAL1 = nextAxis1Rate * stepAxis1;
  }
}
ISR(TIMER4_COMPA_vect)
{
  digitalWriteFast(Axis2StepPin, LOW);
  // on the much faster Teensy and Tiva TM4C run this ISR at twice the normal rate and pull the step pin low every other call
  if (clearAxis2)
  {
    takeStepAxis2 = false;
    updateDeltaTarget();
    if (deltaTargetAxis2 != 0 || inbacklashAxis2)
    {                       
      // move the Dec stepper to the target
      // telescope normally starts on the EAST side of the pier looking at the WEST sky
      dirAxis2 = 0 < deltaTargetAxis2;
      // Direction control
      if (ReverseAxis2^Axis2Reverse)
      {
        if (dirAxis2)
          digitalWriteFast(Axis2DirPin, LOW);
        else
          digitalWriteFast(Axis2DirPin, HIGH);
      }
      else
      {
        if (dirAxis2)
          digitalWriteFast(Axis2DirPin, HIGH);
        else
          digitalWriteFast(Axis2DirPin, LOW);
      }

      // telescope moving toward celestial pole in the sky, blAxis2 is the amount of opposite backlash
      if (dirAxis2)
      {
        if (blAxis2 < StepsBacklashAxis2)
        {
          blAxis2 += stepAxis2;
          inbacklashAxis2 = true;
        }
        else
        {
          inbacklashAxis2 = false;
          posAxis2 += stepAxis2;
        }
      }
      else
      {
        if (blAxis2 > 0)
        {
          blAxis2 -= stepAxis2;
          inbacklashAxis2 = true;
        }
        else
        {
          inbacklashAxis2 = false;
          posAxis2 -= stepAxis2;
        }
      }
      takeStepAxis2 = true;
    }
    clearAxis2 = false;
  }
  else
  {
    if (takeStepAxis2)
    {
      digitalWriteFast(Axis2StepPin, HIGH);
    }
    clearAxis2 = true;
    PIT_LDVAL2 = nextAxis2Rate * stepAxis2;
  }
}


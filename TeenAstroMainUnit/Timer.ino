// -----------------------------------------------------------------------------------
// Timers and interrupt handling
#define stepAxis   1

#define ISR(f)  void f (void)
void TIMER1_COMPA_vect(void);
void TIMER3_COMPA_vect(void);
void TIMER4_COMPA_vect(void);

static IntervalTimer  itimer1;
static IntervalTimer  itimer3;
static IntervalTimer  itimer4;

static volatile double isrTimerRateAxis1 = 0;
static volatile double isrTimerRateAxis2 = 0;

double getV(double rate) //Speed in step per second
{
  return 1000000.0 / (rate / 16.);
}
double getRate(double V)
{
  return max(16. * 1000000.0 / V, maxRate);
}


// set the master sidereal clock rate, also forces rate update for RA/Dec timer rates so that PPS adjustments take hold immediately
void SetSiderealClockRate(double Interval)
{
  Timer1SetRate(Interval / 100);
  isrTimerRateAxis1 = 0;
  isrTimerRateAxis2 = 0;
}
void beginTimers()
{
  // set the system timer for millis() to the second highest priority
  SCB_SHPR3 = (32 << 24) | (SCB_SHPR3 & 0x00FFFFFF);
  itimer3.begin(TIMER3_COMPA_vect, (float)128 * 0.0625);
  itimer4.begin(TIMER4_COMPA_vect, (float)128 * 0.0625);
  // set the 1/100 second sidereal clock timer to run at the second highest priority
  NVIC_SET_PRIORITY(IRQ_PIT_CH0, 32);

  // set the motor timers to run at the highest priority
  NVIC_SET_PRIORITY(IRQ_PIT_CH1, 0);
  NVIC_SET_PRIORITY(IRQ_PIT_CH2, 0);
}
// set timer1 to rate (in microseconds*16)
static void Timer1SetRate(double rate)
{
  itimer1.begin(TIMER1_COMPA_vect, rate * 0.0625);
}

// set timer3 to rate (in microseconds*16)
static volatile uint32_t   nextAxis1Rate = 100000UL;


static void Timer3SetRate(double rate)
{
  cli();
  nextAxis1Rate = (F_BUS / 1000000) * (rate * 0.0625) * 0.5 - 1;
  sei();
}

// set timer4 to rate (in microseconds*16)
static volatile uint32_t   nextAxis2Rate = 100000UL;
static void Timer4SetRate(double rate)
{
  cli();
  nextAxis2Rate = (F_BUS / 1000000) * (rate * 0.0625) * 0.5 - 1;
  sei();
}


ISR(TIMER1_COMPA_vect)
{
  static volatile bool   wasInbacklashAxis1   = false;
  static volatile bool   wasInbacklashAxis2   = false;
  static volatile double guideTimerRateAxisA1 = 0;
  static volatile double guideTimerRateAxisA2 = 0;
  static volatile double runtimerRateAxis1    = 0;
  static volatile double runTimerRateAxis2    = 0;
  // run 1/3 of the time at 3x the rate, unless a goto is happening
  rtk.m_lst++;

  if (!movingTo)
  {
    double maxguideTimerRate = 4;
    // automatic rate calculation HA
    {
      // guide rate acceleration/deceleration and control
      updateDeltaTarget();

      double  x = staA1.deltaTarget;

      if (!backlashA1.correcting  && guideA1.dir)
      {
        if ((fabs(guideA1.timerRate) < maxguideTimerRate) &&
          (fabs(guideTimerRateAxisA1) < maxguideTimerRate))
        {
          // break mode
          if (guideA1.dir == 'b')
          {
            guideA1.timerRate = staA1.trackingTimerRate;
            if (guideA1.timerRate >= 0)
              guideA1.timerRate = 1.0;
            else
              guideA1.timerRate = -1.0;
          }

          // slow speed guiding, no acceleration
          guideTimerRateAxisA1 = guideA1.timerRate;
        }
        else
        {
          // use acceleration
          DecayModeGoto();
          double z = getRate(sqrt(fabs(x) * 2 * staA1.acc));
          guideTimerRateAxisA1 = 3600.0 / (geoA1.stepsPerDegree * z / 1000000.0);
          if (guideTimerRateAxisA1 < maxguideTimerRate) guideTimerRateAxisA1 = maxguideTimerRate;
        }

        // stop guiding
        if (guideA1.dir == 'b')
        {
          if (atTargetAxis1())
          {
            guideA1.dir = 0;
            guideA1.timerRate = 0;
            guideTimerRateAxisA1 = 0;
            if (atTargetAxis2())
              DecayModeTracking();
          }
        }
      }
      double timerRateAxis1A = staA1.trackingTimerRate;
      double timerRateAxis1B = fabs(guideTimerRateAxisA1 + timerRateAxis1A);
      double calculatedTimerRateAxis1;
      // round up to run the motor timers just a tiny bit slow, then adjust below if we start to fall behind during sidereal tracking
      if (timerRateAxis1B > 0.5)
        calculatedTimerRateAxis1 = SiderealRate / timerRateAxis1B;
      else
        calculatedTimerRateAxis1 = SiderealRate * 2.0;


      // remember our "running" rate and only update the actual rate when it changes
      if (runtimerRateAxis1 != calculatedTimerRateAxis1)
      {
        staA1.timerRate = calculatedTimerRateAxis1;
        runtimerRateAxis1 = calculatedTimerRateAxis1;
      }
    }
    // automatic rate calculation Dec
    {
      // guide rate acceleration/deceleration
      updateDeltaTarget();

      double x = abs(staA2.deltaTarget);

      if (!backlashA2.correcting  && guideA2.dir)
      {
        if ((fabs(guideA2.timerRate) < maxguideTimerRate) &&
          (fabs(guideTimerRateAxisA2) < maxguideTimerRate))
        {
          // break mode
          if (guideA2.dir == 'b')
          {
            guideA2.timerRate = staA2.trackingTimerRate;
            if (guideA2.timerRate >= 0)
              guideA2.timerRate = 1.0;
            else
              guideA2.timerRate = -1.0;
          }

          // slow speed guiding, no acceleration
          guideTimerRateAxisA2 = guideA2.timerRate;
        }
        else
        {
          // use acceleration
          DecayModeGoto();
          double z = getRate(sqrt(fabs(x) * 2 * staA2.acc));
          guideTimerRateAxisA2 = 3600.0 / (geoA2.stepsPerDegree * z / 1000000.0) ;
          if (guideTimerRateAxisA2 < maxguideTimerRate) guideTimerRateAxisA2 = maxguideTimerRate;
        }

        // stop guiding
        if (guideA2.dir == 'b')
        {
          if (atTargetAxis2())
          {
            guideA2.dir = 0;
            guideA2.timerRate = 0;
            guideTimerRateAxisA2 = 0;
            if (atTargetAxis1())
              DecayModeTracking();
          }
        }
      }
      double timerRateAxis2A = staA2.trackingTimerRate;
      double timerRateAxis2B = fabs(guideTimerRateAxisA2 + timerRateAxis2A);
      double calculatedTimerRateAxis2;
      // round up to run the motor timers just a tiny bit slow, then adjust below if we start to fall behind during sidereal tracking
     // calculatedTimerRateAxis2 = (double)SiderealRate / timerRateAxis2B;
      if (timerRateAxis2B > 0.5)
        calculatedTimerRateAxis2 = SiderealRate / timerRateAxis2B;
      else
        calculatedTimerRateAxis2 = 2.0 * SiderealRate;

      // remember our "running" rate and only update the actual rate when it changes
      if (runTimerRateAxis2 != calculatedTimerRateAxis2)
      {
        staA2.timerRate = calculatedTimerRateAxis2;
        runTimerRateAxis2 = calculatedTimerRateAxis2;
      }
    }

    if (!guideA1.dir && !guideA2.dir)
    {
      GuidingState = GuidingOFF;
    }
  }

  double thisTimerRateAxis1 = staA1.timerRate;
  double thisTimerRateAxis2 = staA2.timerRate * timerRateRatio;

  staA2.timerRate = max(staA2.timerRate, maxRate);
  thisTimerRateAxis2 = staA2.timerRate;

  // override rate during backlash compensation
  if (backlashA1.correcting)
  {
    thisTimerRateAxis1 = backlashA1.timerRate;
    wasInbacklashAxis1 = true;
  }

  // override rate during backlash compensation
  if (backlashA2.correcting)
  {
    thisTimerRateAxis2 = backlashA2.timerRate;
    wasInbacklashAxis2 = true;
  }
  if (sideralTracking && !movingTo)
  {
    // travel through the backlash is done, but we weren't following the target while it was happening!
    // so now get us back to near where we need to be
    if (!backlashA1.correcting  && wasInbacklashAxis1 && !guideA1.dir)
    {
      cli();
      if (!atTargetAxis1(true))
        thisTimerRateAxis1 = TakeupRate;
      else
        wasInbacklashAxis1 = false;
      sei();
    }
    if (!backlashA2.correcting  && wasInbacklashAxis2 && !guideA2.dir)
    {
      cli();
      if (!atTargetAxis2(true))
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
  static volatile bool clearAxis1 = true;
  static volatile bool takeStepAxis1 = false;
  digitalWriteFast(Axis1StepPin, LOW);
  if (clearAxis1)
  {
    takeStepAxis1 = false;
    staA1.updateDeltaTarget();
    if (staA1.deltaTarget != 0 || backlashA1.correcting)
    {                       
      // Move the RA stepper to the target
      staA1.dir = 0 < staA1.deltaTarget;
      // Direction control
      if (motorA1.reverse^Axis1Reverse)
      {
        digitalWriteFast(Axis1DirPin, HADir != staA1.dir);
      }
      else
      {
        digitalWriteFast(Axis1DirPin, HADir == staA1.dir);
      }

      // telescope moves WEST with the sky, blAxis1 is the amount of EAST backlash
      if (staA1.dir)
      {
        if (backlashA1.movedSteps < backlashA1.inSteps)
        {
          backlashA1.movedSteps += stepAxis;
          backlashA1.correcting = true;
        }
        else
        {
          backlashA1.correcting = false;
          staA1.pos += stepAxis;
        }
      }
      else
      {
        if (backlashA1.movedSteps > 0)
        {
          backlashA1.movedSteps -= stepAxis;
          backlashA1.correcting = true;
        }
        else
        {
          backlashA1.correcting = false;
          staA1.pos -= stepAxis;
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
    PIT_LDVAL1 = nextAxis1Rate * stepAxis;
  }
}
ISR(TIMER4_COMPA_vect)
{
  static volatile bool clearAxis2 = true;
  static volatile bool takeStepAxis2 = false;
  digitalWriteFast(Axis2StepPin, LOW);
  // on the much faster Teensy and Tiva TM4C run this ISR at twice the normal rate and pull the step pin low every other call
  if (clearAxis2)
  {
    takeStepAxis2 = false;
    staA2.updateDeltaTarget();
    if (staA2.deltaTarget != 0 || backlashA2.correcting)
    {                       
      // move the Dec stepper to the target
      // telescope normally starts on the EAST side of the pier looking at the WEST sky
      staA2.dir = 0 < staA2.deltaTarget;
      // Direction control
      if (motorA2.reverse^Axis2Reverse)
      {
        digitalWriteFast(Axis2DirPin, !staA2.dir);
      }
      else
      {
        digitalWriteFast(Axis2DirPin, staA2.dir);
      }

      // telescope moving toward celestial pole in the sky, blAxis2 is the amount of opposite backlash
      if (staA2.dir)
      {
        if (backlashA2.movedSteps < backlashA2.inSteps)
        {
          backlashA2.movedSteps += stepAxis;
          backlashA2.correcting = true;
        }
        else
        {
          backlashA2.correcting = false;
          staA2.pos += stepAxis;
        }
      }
      else
      {
        if (backlashA2.movedSteps > 0)
        {
          backlashA2.movedSteps -= stepAxis;
          backlashA2.correcting = true;
        }
        else
        {
          backlashA2.correcting = false;
          staA2.pos -= stepAxis;
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
    PIT_LDVAL2 = nextAxis2Rate * stepAxis;
  }
}


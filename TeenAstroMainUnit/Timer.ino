// -----------------------------------------------------------------------------------
// Timers and interrupt handling
#define clockRatio 0.01

#define ISR(f)  void f (void)
void TIMER1_COMPA_vect(void);
void TIMER3_COMPA_vect(void);
void TIMER4_COMPA_vect(void);

static IntervalTimer  itimer1;
static IntervalTimer  itimer3;
static IntervalTimer  itimer4;

static volatile double isrIntervalAxis1 = 0;
static volatile double isrIntervalAxis2 = 0;

speed interval2speed(interval i) //Speed in step per second
{
  return masterClockSpeed / i;
}
interval speed2interval(speed V, interval maxInterval)
{
  if (V == 0)
  {
    return maxInterval;
  }
  return min(masterClockSpeed / V, maxInterval);
}


// set the master sidereal clock rate, also forces rate update for Axis1 & Axis2 
void SetsiderealClockSpeed(double cs)
{
  Timer1SetInterval(cs * clockRatio);
  isrIntervalAxis1 = 0;
  isrIntervalAxis2 = 0;
}

#if defined(ARDUINO_TEENSY40) || defined(ARDUINO_TEENSY_MICROMOD) 

void beginTimers()
{
  itimer3.begin(TIMER3_COMPA_vect, (float)100);
  itimer4.begin(TIMER4_COMPA_vect, (float)100);
  itimer3.priority(0);
  itimer4.priority(0);
  itimer1.priority(32);
}
#else
void beginTimers()
{
  // set the system timer for millis() to the second highest priority
  SCB_SHPR3 = (32 << 24) | (SCB_SHPR3 & 0x00FFFFFF);
  itimer3.begin(TIMER3_COMPA_vect, 100);
  itimer4.begin(TIMER4_COMPA_vect, 100);
  // set the 1/100 second sidereal clock timer to run at the second highest priority
  NVIC_SET_PRIORITY(IRQ_PIT_CH0, 32);

  // set the motor timers to run at the highest priority
  NVIC_SET_PRIORITY(IRQ_PIT_CH1, 0);
  NVIC_SET_PRIORITY(IRQ_PIT_CH2, 0);
}

#endif
// set timer1 to interval (in microseconds)
static void Timer1SetInterval(interval i)
{
  itimer1.begin(TIMER1_COMPA_vect, i);
}


static void Timer3SetInterval(interval i)
{
  itimer3.update(i * 0.5);
}

// set timer4 to interval (in microseconds)
static void Timer4SetInterval(interval i)
{
  itimer4.update(i * 0.5);
}

static void UpdateIntervalTrackingGuiding(GuideAxis* guideA, StatusAxis* staA,
  GeoAxis* geoA, StatusAxis* staA_other,
  volatile double& tmp_guideRateA,
  const double minInterval, const double maxInterval)
{
  volatile double max_guideRate = staA->takeupRate;
  static volatile double sign;
  // guide rate acceleration/deceleration and control
  if (!staA->backlash_correcting && guideA->isBusy())
  {
    if ((guideA->absRate < max_guideRate) &&
      (fabs(tmp_guideRateA) < max_guideRate))
    {
      // slow speed guiding, no acceleration
      tmp_guideRateA = guideA->getRate();
    }
    else
    {
      DecayModeGoto();
      // for acceleration, we know run this routine this a fix amount of time "clockRatio" of a sideral second
      sign = guideA->isDirBW() ? -1 : 1;
      if (guideA->isBraking())
      {
        tmp_guideRateA -= 2 * staA->acc * clockRatio * sign / geoA->stepsPerSecond;
        if (sign == 1)
        {
          tmp_guideRateA = max(tmp_guideRateA, max_guideRate);
        }
        else
        {
          tmp_guideRateA = min(tmp_guideRateA, -max_guideRate);
        }
      }
      else
      {
        tmp_guideRateA += 2 * staA->acc * clockRatio * sign / geoA->stepsPerSecond;
      }
      if (tmp_guideRateA < 0)
      {
        tmp_guideRateA = min(-max_guideRate, tmp_guideRateA);
        tmp_guideRateA = max(guideA->getRate(), tmp_guideRateA);
      }
      else
      {
        tmp_guideRateA = max(max_guideRate, tmp_guideRateA);
        tmp_guideRateA = min(guideA->getRate(), tmp_guideRateA);
      }
    }
    // stop guiding
    if (guideA->isBraking())
    {
      if (max_guideRate < abs(tmp_guideRateA))
      {
        staA->breakMoveHighRate();
      }
      else
      {
        staA->breakMoveLowRate();
      }
      if (staA->atTarget(false))
      {
        guideA->setIdle();
        tmp_guideRateA = 0;
        staA_other->updateDeltaTarget();
        if (staA_other->atTarget(false))
          DecayModeTracking();
      }
    }
  }
  volatile double sumRateA = sideralTracking ? fabs(tmp_guideRateA + staA->CurrentTrackingRate) : fabs(tmp_guideRateA);
  staA->setIntervalfromRate(sumRateA, minInterval, maxInterval);
}

static void UpdateIntervalTrackingGuiding1()
{
  static volatile double tmp_guideRateA1 = 0;
  UpdateIntervalTrackingGuiding(&guideA1, &staA1, &geoA1, &staA2, tmp_guideRateA1, minInterval1, maxInterval1);
}

static void UpdateIntervalTrackingGuiding2()
{
  static volatile double tmp_guideRateA2 = 0;
  UpdateIntervalTrackingGuiding(&guideA2, &staA2, &geoA2, &staA1, tmp_guideRateA2, minInterval2, maxInterval2);
}

static void BacklashComp(GuideAxis* guideA, StatusAxis* staA,
  volatile double& thisIntervalAxis, volatile bool& wasInbacklashAxis)
{
  // override rate during backlash compensation
  if (staA->backlash_correcting)
  {
    thisIntervalAxis = staA->backlash_interval_Step;
    wasInbacklashAxis = true;
  }
  if (sideralTracking && !movingTo)
  {
    // travel through the backlash is done, but we weren't following the target while it was happening!
    // so now get us back to near where we need to be
    if (!staA->backlash_correcting && wasInbacklashAxis && !guideA->isBusy())
    {
      if (!staA->atTarget(true))
      {
        cli();
        thisIntervalAxis = staA->takeupInterval;
        sei();
      }
      else
      {
        cli();
        wasInbacklashAxis = false;
        sei();
      }
    }
  }
}

static void BacklashAndApplyInterval1()
{
  static volatile bool   wasInbacklashAxis1 = false;
  volatile double thisIntervalAxis1 = staA1.interval_Step_Cur;
  BacklashComp(&guideA1, &staA1, thisIntervalAxis1, wasInbacklashAxis1);
  if (thisIntervalAxis1 != isrIntervalAxis1)
  {
    Timer3SetInterval(thisIntervalAxis1);
    isrIntervalAxis1 = thisIntervalAxis1;
  }
}

static void BacklashAndApplyInterval2()
{
  static volatile bool   wasInbacklashAxis2 = false;
  volatile double thisIntervalAxis2 = staA2.interval_Step_Cur;
  BacklashComp(&guideA2, &staA2, thisIntervalAxis2, wasInbacklashAxis2);
  if (thisIntervalAxis2 != isrIntervalAxis2)
  {
    Timer4SetInterval(thisIntervalAxis2);
    isrIntervalAxis2 = thisIntervalAxis2;
  }
}

ISR(TIMER1_COMPA_vect)
{ 
  rtk.m_lst++;
  // in this mode the target is always a bit faster than the scope because we move first the target!!
  if (!movingTo)
  {
    // guide rate acceleration/deceleration
    UpdateIntervalTrackingGuiding1();
    UpdateIntervalTrackingGuiding2();
    if (!guideA1.isBusy() && !guideA2.isBusy())
    {
      GuidingState = Guiding::GuidingOFF;
    }
  }
  BacklashAndApplyInterval1();
  BacklashAndApplyInterval2();
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
    if (staA1.deltaTarget != 0 || staA1.backlash_correcting)
    {
      // Move the RA stepper to the target
      staA1.dir = 0 < staA1.deltaTarget;
      // Direction control
      if (motorA1.reverse ^ Axis1Reverse)
      {
        digitalWriteFast(Axis1DirPin, !staA1.dir);
      }
      else
      {
        digitalWriteFast(Axis1DirPin, staA1.dir);
      }

      // telescope moves WEST with the sky, blAxis1 is the amount of EAST backlash
      staA1.move(!isGuidingStar());
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
  }
}

ISR(TIMER4_COMPA_vect)
{
  static volatile bool clearAxis2 = true;
  static volatile bool takeStepAxis2 = false;
  digitalWriteFast(Axis2StepPin, LOW);
  if (clearAxis2)
  {
    takeStepAxis2 = false;
    staA2.updateDeltaTarget();
    if (staA2.deltaTarget != 0 || staA2.backlash_correcting)
    {
      // move the Dec stepper to the target
      // telescope normally starts on the EAST side of the pier looking at the WEST sky
      staA2.dir = 0 < staA2.deltaTarget;
      // Direction control
      if (motorA2.reverse ^ Axis2Reverse)
      {
        digitalWriteFast(Axis2DirPin, !staA2.dir);
      }
      else
      {
        digitalWriteFast(Axis2DirPin, staA2.dir);
      }

      // telescope moving toward celestial pole in the sky, blAxis2 is the amount of opposite backlash
      staA2.move(!isGuidingStar());
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
  }
}
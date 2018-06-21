//#pragma once

void MoveAxis1(const byte newguideDirAxis)
{
  if (parkStatus == NotParked && trackingState != TrackingMoveTo && GuidingState != GuidingPulse)
  {
    // block user from changing direction at high rates, just stop the guide instead
    // estimate new guideTimerBaseRate
    if (guideTimerBaseRate ==0)
    {
      cli();
      guideDirAxis1 = 'b';
      sei();
      return;
    }
    //enableGuideRate(activeGuideRate,true);
    double newGuideTimerBaseRate = newguideDirAxis == 'e' ? -guideTimerBaseRate : guideTimerBaseRate;

    bool samedirection = newGuideTimerBaseRate > 0 ? (guideTimerRateAxis1 > 0 ? true : false) : (guideTimerRateAxis1 > 0 ? false : true);

    if (guideDirAxis1 && !samedirection && fabs(guideTimerRateAxis1) > 2)
    {
      cli();
      guideDirAxis1 = 'b';
      sei();
    }
    else
    {
      GuidingState = GuidingRecenter;
      guideDirAxis1 = newguideDirAxis;
      atHome = false;
      guideDurationHA = -1;
      cli();
      guideTimerRateAxis1 = newGuideTimerBaseRate;
      sei();
    }
  }
}

void MoveAxis2(const byte newguideDirAxis)
{

  if (parkStatus == NotParked && trackingState != TrackingMoveTo && GuidingState != GuidingPulse)
  {
    if (guideTimerBaseRate == 0)
    {
      cli();
      guideDirAxis2 = 'b';
      sei();
      return;
    }
    // block user from changing direction at high rates, just stop the guide instead
    bool rev = false;
    if (newguideDirAxis == 's')
      rev = true;
    if (pierSide >= PierSideWest)
      rev = !rev;
    //enableGuideRate(activeGuideRate, true);
    double newGuideTimerBaseRate = rev ? -guideTimerBaseRate : guideTimerBaseRate;
    bool samedirection = newGuideTimerBaseRate > 0 ? (guideTimerRateAxis2 > 0 ? true : false) : (guideTimerRateAxis2 > 0 ? false : true);
    if (guideDirAxis2 && !samedirection && fabs(guideTimerRateAxis2) > 2)
    {
      cli();
      guideDirAxis2 = 'b';
      sei();
    }
    else
    {
      GuidingState = GuidingRecenter;
      guideDirAxis2 = newguideDirAxis;
      guideDurationDec = -1;
      atHome = false;
      cli();
      guideTimerRateAxis2 = newGuideTimerBaseRate;
      sei();
    }
  }
}


void checkST4()
{

  // ST4 INTERFACE -------------------------------------------------------------------------------------
#if defined(ST4_ON) || defined(ST4_PULLUP)
  // ST4 interface
  static char            ST4RA_state = 0;
  static char            ST4RA_last = 0;
  static char            ST4DE_state = 0;
  static char            ST4DE_last = 0;
  if (parkStatus == NotParked)
  {
    byte    w1 = digitalRead(ST4RAw);
    byte    e1 = digitalRead(ST4RAe);
    byte    n1 = digitalRead(ST4DEn);
    byte    s1 = digitalRead(ST4DEs);
    delayMicroseconds(50);

    byte    w2 = digitalRead(ST4RAw);
    byte    e2 = digitalRead(ST4RAe);
    byte    n2 = digitalRead(ST4DEn);
    byte    s2 = digitalRead(ST4DEs);

    w1 = LOW; w2 = LOW;
    e1 = HIGH; e2 = HIGH; n1 = HIGH, n2 = HIGH; s1 = HIGH; s2 = HIGH;

    // if signals aren't stable ignore them
    if ((w1 == w2) && (e1 == e2) && (n1 == n2) && (s1 == s2))
    {
      ST4RA_state = 0;
      if (w1 == LOW)
      {
        if (e1 != LOW) ST4RA_state = 'w';
      }
      else if (e1 == LOW)
        ST4RA_state = 'e';
      ST4DE_state = 0;
      if (n1 == LOW)
      {
        if (s1 != LOW) ST4DE_state = 'n';
      }
      else if (s1 == LOW)
        ST4DE_state = 's';
    }

    // RA changed?
    if (ST4RA_last != ST4RA_state)
    {
      ST4RA_last = ST4RA_state;
      if (ST4RA_state)
      {
#ifdef SEPERATE_PULSE_GUIDE_RATE_ON
#else
        enableGuideRate(currentGuideRate);
#endif
        guideDirAxis1 = ST4RA_state;
        guideDurationHA = -1;
        cli();
        if (guideDirAxis1 == 'e')
          guideTimerRateAxis1 = -guideTimerBaseRate;
        else
          guideTimerRateAxis1 = guideTimerBaseRate;
        sei();
      }
      else
      {
        if (guideDirAxis1)
        {
          guideDirAxis1 = 'b';
        }
      }
    }

    // Dec changed?
    if (ST4DE_last != ST4DE_state)
    {
      ST4DE_last = ST4DE_state;
      if (ST4DE_state)
      {
#ifdef SEPERATE_PULSE_GUIDE_RATE_ON
#ifdef ST4_HAND_CONTROL_ON
        enableGuideRate(currentGuideRate);
#else
        enableGuideRate(currentPulseGuideRate);
#endif
#else
        enableGuideRate(currentGuideRate);
#endif
        guideDirAxis2 = ST4DE_state;
        guideDurationDec = -1;
        if (guideDirAxis2 == 's' || guideDirAxis2 == 'n')
        {
          bool rev = false;
          if (guideDirAxis2 == 's')
            rev = true;
          if (pierSide >= PierSideWest)
            rev = !rev;
          cli();
          guideTimerRateAxis2 = rev ? -guideTimerBaseRate : guideTimerBaseRate;
          sei();
        }
      }
      else
      {
        if (guideDirAxis2)
        {
          guideDirAxis2 = 'b';
        }
      }
    }
  }
#endif
}

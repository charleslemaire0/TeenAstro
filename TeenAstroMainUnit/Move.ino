//#pragma once
static void MoveAxis(GuideAxis* guideA, StatusAxis* staA, const bool BW, const Guiding Mode)
{
  bool canMove = parkStatus == PRK_UNPARKED;
  canMove &= (Mode == GuidingRecenter || lastError == ERRT_NONE);
  canMove &= !movingTo;
  canMove &= (GuidingState == GuidingOFF || GuidingState == Mode);
  if (canMove)
  {
    // block user from changing direction at high rates, just stop the guide instead
    // estimate new guideTimerBaseRate
    if (guideA->absRate == 0 && guideA->isBusy())
    {
      cli();
      guideA->brake();
      sei();
      return;
    }

    bool samedirection = (BW == guideA->isDirBW());
    if (guideA->isBusy() && !samedirection && guideA->absRate > 2)
    {
      StopAxis(guideA, staA);
    }
    else
    {
      Mode == GuidingST4 ? enableST4GuideRate() : enableGuideRate(activeGuideRate);
      GuidingState = Mode;
      BW ? guideA->moveBW() : guideA->moveFW();
      atHome = false;
      guideA->duration = -1;
    }
  }
}

static void MoveAxisAtRate(GuideAxis* guideA, StatusAxis* staA, const double newrate)
{
  bool canMove = parkStatus == PRK_UNPARKED;
  canMove &= lastError == ERRT_NONE;
  canMove &= !movingTo;
  canMove &= (GuidingState == GuidingOFF || GuidingState == GuidingAtRate);
  if (canMove)
  {
    if (newrate == 0)
    {
      StopAxis(guideA, staA);
      return;
    }
    if (GuidingState != GuidingAtRate)
    {
      lastSideralTracking = sideralTracking;
      sideralTracking = false;
    }
    bool samedirection = ((newrate > 0) == (guideA->getRate() >= 0));
    if (guideA->isBusy() && !samedirection && guideA->absRate > 2)
    {
      StopAxis(guideA, staA);
    }
    else
    {
      guideA->enableAtRate(abs(newrate));
      GuidingState = Guiding::GuidingAtRate;
      newrate > 0 ? guideA->moveFW() : guideA->moveBW();
      atHome = false;
      guideA->duration = -1;
    }
  }
}

static void StopAxis(GuideAxis* guideA, StatusAxis* staA)
{
  if (!guideA->isMoving())
    return;
  staA->updateDeltaTarget();
  long a = pow(interval2speed(staA->interval_Step_Cur), 2.) / (4. * staA->acc);
  if (abs(staA->deltaTarget) > a)
  {
    if (0 > staA->deltaTarget)
      a = -a;
    cli();
    staA->target = staA->pos + a;
    sei();
  }
  guideA->brake();
}


void MoveAxis1(const bool BW, const Guiding Mode)
{
  MoveAxis(&guideA1, &staA1, BW, Mode);
}

void MoveAxisAtRate1(const double newrate)
{
  MoveAxisAtRate(&guideA1, &staA1, newrate);
}

void StopAxis1()
{
  StopAxis(&guideA1, &staA1);
}

void MoveAxis2(const bool BW, const Guiding Mode)
{
  MoveAxis(&guideA2, &staA2, BW, Mode);
}

void MoveAxisAtRate2(const double newrate)
{
  MoveAxisAtRate(&guideA2, &staA2, newrate);
}

void StopAxis2()
{
  StopAxis(&guideA2, &staA2);
}



void CheckEndOfMoveAxisAtRate()
{
  if (lastGuidingState == GuidingAtRate && GuidingState == GuidingOFF)
  {
    if (lastSideralTracking)
    {
      lastSetTrakingEnable = millis();
      sideralTracking = true;
      computeTrackingRate(true);
    }
    resetGuideRate();
  }
  lastGuidingState = GuidingState;
}

void CheckSpiral()
{
  static int iteration = 0;
  if (!doSpiral)
  {
    if (iteration != 0)
      iteration = 0;
    return;
  }
  int duration = iteration / 2 + 1;

  if (iteration == 20 || lastError != ERRT_NONE)
  {
    StopAxis1();
    StopAxis2();
    iteration = 0;
    doSpiral = false;
    return;
  }

  if (iteration % 2 == 0)
  {
    if (guideA1.isBusy())
      return;
    iteration % 4 < 2 ? guideA2.moveFW() : guideA2.moveBW();
    guideA2.durationLast = micros();
    guideA2.duration = (long)duration * 3000000L;
    cli();
    GuidingState = Guiding::GuidingPulse;
    sei();
  }
  else
  {
    if (guideA2.isBusy())
      return;
    iteration % 4 < 2 ? guideA1.moveBW() : guideA1.moveFW();
    guideA1.durationLast = micros();
    guideA1.duration = (long)duration * 3000000L;
    cli();
    GuidingState = Guiding::GuidingPulse;
    sei();
  }
  iteration++;
}

void checkST4()
{
  //Simulated ST4 with inactive signals
  byte w1 = HIGH, w2 = HIGH, e1 = HIGH, e2 = HIGH, n1 = HIGH, n2 = HIGH, s1 = HIGH, s2 = HIGH;
  static char ST4RA_state = 0;
  static char ST4RA_last = 0;
  static char ST4DE_state = 0;
  static char ST4DE_last = 0;
  // ST4 port is active only if there is no mount Error
  if (lastError == ERRT_NONE)
  {
    w1 = digitalRead(ST4RAw);
    e1 = digitalRead(ST4RAe);
    n1 = digitalRead(ST4DEn);
    s1 = digitalRead(ST4DEs);
    delayMicroseconds(5);
    w2 = digitalRead(ST4RAw);
    e2 = digitalRead(ST4RAe);
    n2 = digitalRead(ST4DEn);
    s2 = digitalRead(ST4DEs);
  }

  if ((w1 == w2) && (e1 == e2) && (n1 == n2) && (s1 == s2))
  {
    ST4RA_state = 0;
    if (w1 == LOW)
    {
      if (e1 != LOW) ST4RA_state = '+';
    }
    else if (e1 == LOW)
      ST4RA_state = '-';
    ST4DE_state = 0;
    if (n1 == LOW)
    {
      if (s1 != LOW)
      {
        if (GetPierSide() >= PIER_WEST)
        {
          ST4DE_state = '-';
        }
        else
        {
          ST4DE_state = '+';
        }
      }
    }
    else if (s1 == LOW)
    {
      if (GetPierSide() >= PIER_WEST)
      {
        ST4DE_state = '+';
      }
      else
      {
        ST4DE_state = '-';
      }
    }

  }

  // RA changed?
  if (ST4RA_last != ST4RA_state)
  {
    ST4RA_last = ST4RA_state;
    if (ST4RA_state)
    {
      MoveAxis1(ST4RA_state == '-', Guiding::GuidingST4);
    }
    else
    {
      StopAxis1();
    }
  }

  // Dec changed?
  if (ST4DE_last != ST4DE_state)
  {
    ST4DE_last = ST4DE_state;
    if (ST4DE_state)
    {
      MoveAxis2(ST4DE_state == '-', Guiding::GuidingST4);
    }
    else
    {
      StopAxis2();
    }
  }

}

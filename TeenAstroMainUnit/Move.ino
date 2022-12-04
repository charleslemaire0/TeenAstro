//#pragma once
void MoveAxis1(const bool BW, const Guiding Mode)
{
  bool canMove = parkStatus == PRK_UNPARKED;
  canMove &= (Mode == GuidingRecenter || lastError == ERRT_NONE);
  canMove &= !movingTo;
  canMove &= (GuidingState == GuidingOFF || GuidingState == Mode);
  if (canMove)
  {
    // block user from changing direction at high rates, just stop the guide instead
    // estimate new guideTimerBaseRate
    if (guideA1.absRate == 0)
    {
      cli();
      guideA1.brake();
      sei();
      return;
    }
    Mode == GuidingST4 ? enableST4GuideRate() : enableGuideRate(activeGuideRate);
    Mode == GuidingST4 ? enableST4GuideRate() : enableGuideRate(activeGuideRate);
    double newGuideTimerBaseRate = BW ? -guideA1.absRate : guideA1.absRate;
    bool samedirection = newGuideTimerBaseRate > 0 ? (guideA1.atRate > 0 ? true : false) : (guideA1.atRate > 0 ? false : true);
    if (guideA1.isBusy() && !samedirection && fabs(guideA1.atRate) > 2)
    {
      StopAxis1();
    }
    else
    {
      GuidingState = Mode;
      BW ? guideA1.moveBW() : guideA1.moveFW();
      atHome = false;
      guideA1.duration = -1;
      cli();
      guideA1.atRate = newGuideTimerBaseRate;
      sei();
    }
  }
}

void MoveAxis1AtRate(const double newrate)
{
  bool canMove = parkStatus == PRK_UNPARKED;
  canMove &= lastError == ERRT_NONE;
  canMove &= !movingTo;
  canMove &= (GuidingState == GuidingOFF || GuidingState == GuidingAtRate);
  if (canMove)
  {
    if (newrate == 0)
    {
      StopAxis1();
      return;
    }
    if (GuidingState != GuidingAtRate)
    {
      lastSideralTracking = sideralTracking;
      sideralTracking = false;
    }
    guideA1.enableAtRate(abs(newrate));
    bool samedirection = (newrate > 0) == (guideA1.atRate > 0);
    if (guideA1.isBusy() && !samedirection && fabs(guideA1.atRate) > 2)
    {
      StopAxis1();
    }
    else
    {
      GuidingState = GuidingAtRate;
      newrate > 0 ?  guideA1.moveFW() : guideA1.moveBW();
      atHome = false;
      guideA1.duration = -1;
      cli();
      guideA1.atRate = newrate;
      sei();
    }
  }
}

void StopAxis1()
{

  if (guideA1.isBraking()||!guideA1.isBusy())
    return;
  staA1.updateDeltaTarget();
  long a = pow(interval2speed(staA1.interval_Step_Cur), 2.) / (4. * staA1.acc);
  if (abs(staA1.deltaTarget) > a)
  {
    if (0 > staA1.deltaTarget)
      a = -a;
    cli();
    staA1.target = staA1.pos + a;
    sei();
  }
  guideA1.brake();
}

void MoveAxis2(const bool BW, const Guiding Mode)
{
  bool canMove = parkStatus == PRK_UNPARKED;
  canMove &= (Mode == Guiding::GuidingRecenter || lastError == ERRT_NONE);
  canMove &= !movingTo;
  canMove &= (GuidingState == Guiding::GuidingOFF || GuidingState == Mode);
  
  if (canMove)
  {
    if (guideA2.absRate == 0)
    {
      cli();
      guideA2.brake();
      sei();
      return;
    }
    // block user from changing direction at high rates, just stop the guide instead
    Mode == GuidingST4 ? enableST4GuideRate() : enableGuideRate(activeGuideRate);
    double newGuideTimerBaseRate = BW ? -guideA2.absRate : guideA2.absRate;
    bool samedirection = newGuideTimerBaseRate > 0 ? (guideA2.atRate > 0 ? true : false) : (guideA2.atRate > 0 ? false : true);
    if (guideA2.isBusy() && !samedirection && fabs(guideA2.atRate) > 2)
    {
      StopAxis2();
    }
    else
    {
      GuidingState = Mode;
      BW ? guideA2.moveBW() : guideA2.moveFW();
      guideA2.duration = -1;
      atHome = false;
      cli();
      guideA2.atRate = newGuideTimerBaseRate;
      sei();
    }
  }
}

void MoveAxis2AtRate(const double newrate)
{
  bool canMove = parkStatus == PRK_UNPARKED;
  canMove &= !movingTo;
  canMove &= (GuidingState == GuidingOFF || GuidingState == GuidingAtRate);
  canMove &= lastError == ERRT_NONE;
  if (canMove)
  {
    if (newrate == 0)
    {
      StopAxis2();
      return;
    }
    if (GuidingState != GuidingAtRate)
    {
      lastSideralTracking = sideralTracking;
      sideralTracking = false;
    }
    guideA2.enableAtRate(abs(newrate));
    bool samedirection = (newrate > 0) == (guideA2.atRate > 0);
    if (guideA2.isBusy() && !samedirection && fabs(guideA2.atRate) > 2)
    {
      StopAxis2();
    }
    else
    {
      GuidingState = Guiding::GuidingAtRate;
      newrate > 0 ? guideA2.moveFW() : guideA2.moveBW();
      atHome = false;
      guideA2.duration = -1;
      cli();
      guideA2.atRate = newrate;
      sei();
    }
  }
}

void StopAxis2()
{
  if (guideA2.isBraking() || !guideA2.isBusy())
    return;
  long a = pow(interval2speed(staA2.interval_Step_Cur), 2.) / (4. * staA2.acc);
  staA2.updateDeltaTarget();
  if (abs(staA2.deltaTarget) > a)
  {
    if (0 > staA2.deltaTarget)
      a = -a;
    cli();
    staA2.target = staA2.pos + a;
    sei();
  }
  guideA2.brake();
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
    iteration % 4 < 2 ? guideA1.moveFW() : guideA1.moveBW();
    guideA2.durationLast = micros();
    guideA2.duration = (long)duration * 3000000L;
    cli();
    GuidingState = Guiding::GuidingPulse;
    guideA2.atRate = guideA2.isBW() ? -guideA2.absRate : guideA2.absRate;
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
    guideA1.atRate = guideA1.isBW() ? -guideA1.absRate : guideA1.absRate;
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

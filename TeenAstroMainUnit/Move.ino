//#pragma once
void MoveAxis1(const byte newguideDirAxis, const Guiding Mode)
{

  bool canMove = parkStatus == PRK_UNPARKED;
  canMove &= (Mode == GuidingRecenter || lastError == ERR_NONE);
  canMove &= !movingTo;
  canMove &= (GuidingState == GuidingOFF || GuidingState == Mode);
  if (canMove)
  {
    // block user from changing direction at high rates, just stop the guide instead
    // estimate new guideTimerBaseRate
    if (guideTimerBaseRate == 0)
    {
      cli();
      guideA1.dir = 'b';
      sei();
      return;
    }
    Mode == GuidingST4 ? enableST4GuideRate() : enableGuideRate(activeGuideRate, false);
    double newGuideTimerBaseRate = newguideDirAxis == 'e' ? -guideTimerBaseRate : guideTimerBaseRate;
    bool samedirection = newGuideTimerBaseRate > 0 ? (guideA1.timerRate > 0 ? true : false) : (guideA1.timerRate > 0 ? false : true);
    if (guideA1.dir && !samedirection && fabs(guideA1.timerRate) > 2)
    {
      StopAxis1();
    }
    else
    {
      GuidingState = Mode;
      guideA1.dir = newguideDirAxis;
      atHome = false;
      guideA1.duration = -1;
      cli();
      guideA1.timerRate = newGuideTimerBaseRate;
      sei();
    }
  }
}

void StopAxis1()
{
  if (guideA1.dir == 'b')
    return;
  staA1.updateDeltaTarget();
  long a = pow(getV(staA1.timerRate), 2.) / (2. * staA1.acc);
  if (abs(staA1.deltaTarget) > a)
  {
    if (0 > staA1.deltaTarget)
      a = -a;
    cli();
    staA1.target = staA1.pos + a;
    sei();
  }
  guideA1.dir = 'b';
}

void MoveAxis2(const byte newguideDirAxis, const Guiding Mode)
{
  bool canMove = parkStatus == PRK_UNPARKED;
  canMove &= (Mode == GuidingRecenter || lastError == ERR_NONE);
  canMove &= !movingTo;
  canMove &= (GuidingState == GuidingOFF || GuidingState == Mode);

  if (canMove)
  {
    if (guideTimerBaseRate == 0)
    {
      cli();
      guideA2.dir = 'b';
      sei();
      return;
    }
    // block user from changing direction at high rates, just stop the guide instead
    bool rev = false;
    if (newguideDirAxis == 's')
      rev = true;
    if (GetPierSide() >= PIER_WEST)
      rev = !rev;
    Mode == GuidingST4 ? enableST4GuideRate() : enableGuideRate(activeGuideRate, false);
    double newGuideTimerBaseRate = rev ? -guideTimerBaseRate : guideTimerBaseRate;
    bool samedirection = newGuideTimerBaseRate > 0 ? (guideA2.timerRate > 0 ? true : false) : (guideA2.timerRate > 0 ? false : true);
    if (guideA2.dir && !samedirection && fabs(guideA2.timerRate) > 2)
    {
      StopAxis2();
    }
    else
    {
      GuidingState = Mode;
      guideA2.dir = newguideDirAxis;
      guideA2.duration = -1;
      atHome = false;
      cli();
      guideA2.timerRate = newGuideTimerBaseRate;
      sei();
    }
  }
}

void StopAxis2()
{
  long a = pow(getV(staA2.timerRate), 2.) / (2. * staA2.acc);
  staA2.updateDeltaTarget();
  if (abs(staA2.deltaTarget) > a)
  {
    if (0 > staA2.deltaTarget)
      a = -a;
    cli();
    staA2.target = staA2.pos + a;
    sei();
  }
  guideA2.dir = 'b';
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

  if (iteration == 20 || lastError != ERR_NONE)
  {
    StopAxis1();
    StopAxis2();
    iteration = 0;
    doSpiral = false;
    return;
  }

  if (iteration % 2 == 0)
  {
    if (guideA1.dir)
      return;
    guideA2.dir = iteration % 4 < 2 ? 'n' : 's';
    guideA2.durationLast = micros();
    guideA2.duration = (long)duration * 3000000L;
    if (guideA2.dir == 's' || guideA2.dir == 'n')
    {
      bool rev = false;
      if (guideA2.dir == 's')
        rev = true;
      if (GetPierSide() >= PIER_WEST)
        rev = !rev;
      cli();
      GuidingState = GuidingPulse;
      guideA2.timerRate = rev ? -guideTimerBaseRate : guideTimerBaseRate;
      sei();
    }
  }
  else
  {
    if (guideA2.dir)
      return;
    guideA1.dir = iteration % 4 < 2 ? 'e' : 'w';
    guideA1.durationLast = micros();
    guideA1.duration = (long)duration * 3000000L;
    cli();
    GuidingState = GuidingPulse;
    if (guideA1.dir == 'e')
      guideA1.timerRate = -guideTimerBaseRate;
    else
      guideA1.timerRate = guideTimerBaseRate;
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
  if (lastError == ERR_NONE)
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
      MoveAxis1(ST4RA_state, GuidingST4);
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
      MoveAxis2(ST4DE_state, GuidingST4);
    }
    else
    {
      StopAxis2();
    }
  }

}

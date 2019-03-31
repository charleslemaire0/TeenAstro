//#pragma once
void MoveAxis1(const byte newguideDirAxis, const Guiding Mode)
{
 
  bool canMove = parkStatus == PRK_UNPARKED;
  canMove &= !movingTo;
  canMove &= (GuidingState == GuidingOFF || GuidingState == Mode);
  if (canMove)
  {
    // block user from changing direction at high rates, just stop the guide instead
    // estimate new guideTimerBaseRate
    if (guideTimerBaseRate == 0)
    {
      cli();
      guideDirAxis1 = 'b';
      sei();
      return;
    }
    Mode == GuidingST4 ? enableST4GuideRate() : enableGuideRate(activeGuideRate, false);
    double newGuideTimerBaseRate = newguideDirAxis == 'e' ? -guideTimerBaseRate : guideTimerBaseRate;
    bool samedirection = newGuideTimerBaseRate > 0 ? (guideTimerRateAxis1 > 0 ? true : false) : (guideTimerRateAxis1 > 0 ? false : true);
    if (guideDirAxis1 && !samedirection && fabs(guideTimerRateAxis1) > 2)
    {
      StopAxis1();
    }
    else
    {
      GuidingState = Mode;
      guideDirAxis1 = newguideDirAxis;
      atHome = false;
      guideDurationHA = -1;
      cli();
      guideTimerRateAxis1 = newGuideTimerBaseRate;
      sei();
    }
  }
}

void StopAxis1()
{
  if (guideDirAxis1 == 'b')
    return;
  updateDeltaTarget();
  long a = getV(timerRateAxis1)*getV(timerRateAxis1) / (2 * AccAxis1);
  if (fabs(deltaTargetAxis1) > a)
  {
    if (0 > deltaTargetAxis1)
      a = -a;
    cli();
    targetAxis1.part.m = posAxis1 + a;
    targetAxis1.part.f = 0;
    sei();
  }
  guideDirAxis1 = 'b';
}

void MoveAxis2(const byte newguideDirAxis,const Guiding Mode)
{
  bool canMove = parkStatus == PRK_UNPARKED;
  canMove &= !movingTo;
  canMove &= (GuidingState == GuidingOFF || GuidingState == Mode);

  if (canMove)
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
    if (pierSide >= PIER_WEST)
      rev = !rev;
    Mode == GuidingST4 ? enableST4GuideRate() : enableGuideRate(activeGuideRate, false);
    double newGuideTimerBaseRate = rev ? -guideTimerBaseRate : guideTimerBaseRate;
    bool samedirection = newGuideTimerBaseRate > 0 ? (guideTimerRateAxis2 > 0 ? true : false) : (guideTimerRateAxis2 > 0 ? false : true);
    if (guideDirAxis2 && !samedirection && fabs(guideTimerRateAxis2) > 2)
    {
      StopAxis2();
    }
    else
    {
      GuidingState = Mode;
      guideDirAxis2 = newguideDirAxis;
      guideDurationDec = -1;
      atHome = false;
      cli();
      guideTimerRateAxis2 = newGuideTimerBaseRate;
      sei();
    }
  }
}

void StopAxis2()
{
  long a = getV(timerRateAxis2)*getV(timerRateAxis2) / (2 * AccAxis2);
  updateDeltaTarget();
  if (fabs(deltaTargetAxis2) > a)
  {
    if (0 > deltaTargetAxis2)
      a = -a;
    cli();
    targetAxis2.part.m = posAxis2 + a;
    targetAxis2.part.f = 0;
    sei();
  }
  guideDirAxis2 = 'b';
}

void checkST4()
{
  // ST4 INTERFACE -------------------------------------------------------------------------------------
  // ST4 interface
  static char            ST4RA_state = 0;
  static char            ST4RA_last = 0;
  static char            ST4DE_state = 0;
  static char            ST4DE_last = 0;

  byte    w1 = digitalRead(ST4RAw);
  byte    e1 = digitalRead(ST4RAe);
  byte    n1 = digitalRead(ST4DEn);
  byte    s1 = digitalRead(ST4DEs);
  delayMicroseconds(5);
  byte    w2 = digitalRead(ST4RAw);
  byte    e2 = digitalRead(ST4RAe);
  byte    n2 = digitalRead(ST4DEn);
  byte    s2 = digitalRead(ST4DEs);

  //w1 = LOW; w2 = LOW;
  //e1 = HIGH; e2 = HIGH; n1 = HIGH, n2 = HIGH; s1 = HIGH; s2 = HIGH;

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

// Guide ---------------------------------------------------------------------------------------------

// Guide, commands to move the mount in any direction at a series of fixed rates

void FinishpulseGuiding()
{
  return;
}


void apply_GuidingA1()
{
  bool rev = guideA1.dir == 'e';
  cli();
  rev ? staA1.target -= guideA1.amount : staA1.target += guideA1.amount;
  sei();
}
void apply_GuidingA2()
{
  bool rev = false;
  if (guideA2.dir == 's')
    rev = true;
  if (GetPierSide() >= PIER_WEST)
    rev = !rev;
  cli();
  rev ? staA2.target -= guideA2.amount : staA2.target += guideA2.amount;
  sei();
}
bool StopIfMountError()
{
  bool error = lastError != ERRT_NONE;
  if (error)
  {
    guideA1.duration = -1;
    guideA2.duration = -1;
    cli();
    if (guideA1.dir) guideA1.dir = 'b';
    if (guideA2.dir) guideA2.dir = 'b';
    sei();
  }
  return error;
}
void PerformPulseGuiding()
{
  if (StopIfMountError())
    return;
  if (guideA2.duration <= 0 && guideA1.duration <= 0 )
  {
    cli();
    if (guideA1.dir) guideA1.dir = 'b';
    if (guideA2.dir) guideA2.dir = 'b';
    sei();
    return;
  }
  if (guideA1.dir == 'w' || guideA1.dir == 'e')
  {
    if (guideA1.duration > 0)
    {
      if (!backlashA1.correcting)
      { 
        apply_GuidingA1();
        // for pulse guiding, count down the mS and stop when timed out
        guideA1.duration -= (long)(micros() - guideA1.durationLast);
        guideA1.durationLast = micros();        
      }
      else
      {
        guideA1.durationLast = micros();
      }
    }
    else
    {
      cli();
      guideA1.dir = 'b';
      sei();
    }
  }
  else
  {
    guideA1.duration = -1;
  }
  if (guideA2.dir == 's' || guideA2.dir == 'n')
  {
    if (guideA2.duration > 0 )
    {
      if (!backlashA2.correcting)
      {
        apply_GuidingA2();
        // for pulse guiding, count down the mS and stop when timed out
        guideA2.duration -= (long)(micros() - guideA2.durationLast);
        guideA2.durationLast = micros();
        
      }
      else
      {
        guideA2.durationLast = micros();
      }
    }
    else
    {
      cli();
      guideA2.dir = 'b';
      sei();
    } 
  }
  else
  {
    guideA2.duration = -1;
  }

}

void PerfomST4Guiding()
{
  if (StopIfMountError())
    return;
  if (guideA1.dir == 'w' || guideA1.dir == 'e')
  {
    if (!backlashA1.correcting)
    {
      apply_GuidingA1();
    }
  }
  if (guideA2.dir == 's' || guideA2.dir == 'n')
  {
    if (!backlashA2.correcting)
    {
      apply_GuidingA2();
    }
  }
}

void PerfomGuidingRecenter()
{
  if (guideA1.dir == 'w' || guideA1.dir == 'e')
  {
    if (!backlashA1.correcting)
    {
      apply_GuidingA1();
    }
  }
  if (guideA2.dir == 's' || guideA2.dir == 'n')
  {
    if (!backlashA2.correcting)
    {
      apply_GuidingA2();
    }
  }
}

void PerformGuidingAtRate()
{
  if (StopIfMountError())
    return;
  if (guideA1.dir == '+' || guideA1.dir == '-')
  {
    if (!backlashA1.correcting)
    {
      bool rev = guideA1.dir == '-';
      cli();
      rev ? staA1.target -= guideA1.amount : staA1.target += guideA1.amount;
      sei();
    }
  }
  if (guideA2.dir == '+' || guideA2.dir == '-')
  {
    if (!backlashA2.correcting)
    {
      bool rev = guideA2.dir == '-';
      cli();
      rev ? staA2.target -= guideA2.amount : staA2.target += guideA2.amount;
      sei();
    }
  }
}

void Guide()
{
  // 1/100 second sidereal timer, controls issue of steps at the selected RA and/or Dec rate(s)

  if (GuidingState == GuidingOFF)
  {
    return;
  }

  if (rtk.updateguideSiderealTimer())
  {
    if (GuidingState == GuidingPulse)
    {
      PerformPulseGuiding();
    }
    else if (GuidingState == GuidingST4)
    {
      PerfomST4Guiding();
    }
    else if (GuidingState == GuidingRecenter)
    {
      PerfomGuidingRecenter();
    }
    else if (GuidingState == GuidingAtRate)
    {
      PerformGuidingAtRate();
    }
  }
}


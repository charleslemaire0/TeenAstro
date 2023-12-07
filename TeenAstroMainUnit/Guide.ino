// Guide ---------------------------------------------------------------------------------------------

// Guide, commands to move the mount in any direction at a series of fixed rates


void apply_GuidingA1()
{
  cli();
  staA1.target += guideA1.getAmount();
  sei();
}
void apply_GuidingA2()
{
  cli();
  staA2.target += guideA2.getAmount();
  sei();
}
void StopGuiding()
{
  guideA1.duration = 0UL;
  guideA2.duration = 0UL;
  cli();
  guideA1.brake();
  guideA2.brake();
  sei();
}
bool StopIfMountError()
{
  bool error = lastError != ERRT_NONE;
  if (error)
  {
    StopGuiding();
  }
  return error;
}

void PerformPulseGuiding()
{
  if (StopIfMountError())
    return;
  if (guideA2.duration == 0 && guideA1.duration == 0 )
  {
    cli();
    guideA1.brake();
    guideA2.brake();
    sei();
    return;
  }
  if (guideA1.isMoving())
  {
    if (guideA1.duration > 0)
    {
      if (!staA1.backlash_correcting)
      { 
        apply_GuidingA1();
        // for pulse guiding, count down the mS and stop when timed out
        unsigned long elapsedtime = micros() - guideA1.durationLast;
        if (elapsedtime > guideA1.duration)
        {
          guideA1.duration = 0;
        }
        else
        {
          guideA1.duration -= elapsedtime;
        }
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
      guideA1.brake();
      sei();
    }
  }
  else
  {
    guideA1.duration = 0UL;
  }
  if (guideA2.isMoving())
  {
    if (guideA2.duration > 0 )
    {
      if (!staA2.backlash_correcting)
      {
        apply_GuidingA2();
        // for pulse guiding, count down the mS and stop when timed out
        unsigned long elapsedtime = micros() - guideA2.durationLast;
        if (elapsedtime > guideA2.duration)
        {
          guideA2.duration = 0;
        }
        else
        {
          guideA2.duration -= elapsedtime;
        }
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
      guideA2.brake();
      sei();
    } 
  }
  else
  {
    guideA2.duration = 0UL;
  }

}

void PerfomST4Guiding()
{
  if (StopIfMountError())
    return;
  if (guideA1.isMoving())
  {
    if (!staA1.backlash_correcting)
    {
      apply_GuidingA1();
    }
  }
  if (guideA2.isMoving())
  {
    if (!staA2.backlash_correcting)
    {
      apply_GuidingA2();
    }
  }
}

void PerfomGuidingRecenter()
{
  if (guideA1.isMoving())
  {
    if (!staA1.backlash_correcting)
    {
      apply_GuidingA1();
    }
  }
  if (guideA2.isMoving())
  {
    if (!staA2.backlash_correcting)
    {
      apply_GuidingA2();
    }
  }
}

void PerformGuidingAtRate()
{
  if (StopIfMountError())
    return;
  if (guideA1.isMoving())
  {
    if (!staA1.backlash_correcting)
    {
      cli();
      staA1.target += guideA1.getAmount();
      sei();
    }
  }
  if (guideA2.isMoving())
  {
    if (!staA2.backlash_correcting)
    {
      cli();
      staA2.target += guideA2.getAmount();
      sei();
    }
  }
}

bool isGuidingStar()
{
  return GuidingState == GuidingPulse || GuidingState == GuidingST4;
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


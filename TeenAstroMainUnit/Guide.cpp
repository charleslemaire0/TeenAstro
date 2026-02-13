// Guide ---------------------------------------------------------------------------------------------
// Guide, commands to move the mount in any direction at a series of fixed rates
#include "Global.h"

void apply_GuidingA1()
{
  cli();
  mount.staA1.target += mount.guideA1.getAmount();
  sei();
}
void apply_GuidingA2()
{
  cli();
  mount.staA2.target += mount.guideA2.getAmount();
  sei();
}
void StopGuiding()
{
  mount.guideA1.duration = 0UL;
  mount.guideA2.duration = 0UL;
  cli();
  mount.guideA1.brake();
  mount.guideA2.brake();
  sei();
}
bool StopIfMountError()
{
  bool error = mount.lastError != ERRT_NONE;
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
  if (mount.guideA2.duration == 0 && mount.guideA1.duration == 0 )
  {
    cli();
    mount.guideA1.brake();
    mount.guideA2.brake();
    sei();
    return;
  }
  if (mount.guideA1.isMoving())
  {
    if (mount.guideA1.duration > 0)
    {
      if (!mount.staA1.backlash_correcting)
      { 
        apply_GuidingA1();
        // for pulse guiding, count down the mS and stop when timed out
        unsigned long elapsedtime = micros() - mount.guideA1.durationLast;
        if (elapsedtime > mount.guideA1.duration)
        {
          mount.guideA1.duration = 0;
        }
        else
        {
          mount.guideA1.duration -= elapsedtime;
        }
        mount.guideA1.durationLast = micros();        
      }
      else
      {
        mount.guideA1.durationLast = micros();
      }
    }
    else
    {
      cli();
      mount.guideA1.brake();
      sei();
    }
  }
  else
  {
    mount.guideA1.duration = 0UL;
  }
  if (mount.guideA2.isMoving())
  {
    if (mount.guideA2.duration > 0 )
    {
      if (!mount.staA2.backlash_correcting)
      {
        apply_GuidingA2();
        // for pulse guiding, count down the mS and stop when timed out
        unsigned long elapsedtime = micros() - mount.guideA2.durationLast;
        if (elapsedtime > mount.guideA2.duration)
        {
          mount.guideA2.duration = 0;
        }
        else
        {
          mount.guideA2.duration -= elapsedtime;
        }
        mount.guideA2.durationLast = micros();
        
      }
      else
      {
        mount.guideA2.durationLast = micros();
      }
    }
    else
    {
      cli();
      mount.guideA2.brake();
      sei();
    } 
  }
  else
  {
    mount.guideA2.duration = 0UL;
  }

}

void PerfomST4Guiding()
{
  if (StopIfMountError())
    return;
  if (mount.guideA1.isMoving())
  {
    if (!mount.staA1.backlash_correcting)
    {
      apply_GuidingA1();
    }
  }
  if (mount.guideA2.isMoving())
  {
    if (!mount.staA2.backlash_correcting)
    {
      apply_GuidingA2();
    }
  }
}

void PerfomGuidingRecenter()
{
  if (mount.guideA1.isMoving())
  {
    if (!mount.staA1.backlash_correcting)
    {
      apply_GuidingA1();
    }
  }
  if (mount.guideA2.isMoving())
  {
    if (!mount.staA2.backlash_correcting)
    {
      apply_GuidingA2();
    }
  }
}

void PerformGuidingAtRate()
{
  if (StopIfMountError())
    return;
  if (mount.guideA1.isMoving())
  {
    if (!mount.staA1.backlash_correcting)
    {
      cli();
      mount.staA1.target += mount.guideA1.getAmount();
      sei();
    }
  }
  if (mount.guideA2.isMoving())
  {
    if (!mount.staA2.backlash_correcting)
    {
      cli();
      mount.staA2.target += mount.guideA2.getAmount();
      sei();
    }
  }
}

bool isGuidingStar()
{
  return mount.GuidingState == GuidingPulse || mount.GuidingState == GuidingST4;
}

void Guide()
{
  // 1/100 second sidereal timer, controls issue of steps at the selected RA and/or Dec rate(s)

  if (mount.GuidingState == GuidingOFF)
  {
    return;
  }

  if (rtk.updateguideSiderealTimer())
  {
    if (mount.GuidingState == GuidingPulse)
    {
      PerformPulseGuiding();
    }
    else if (mount.GuidingState == GuidingST4)
    {
      PerfomST4Guiding();
    }
    else if (mount.GuidingState == GuidingRecenter)
    {
      PerfomGuidingRecenter();
    }
    else if (mount.GuidingState == GuidingAtRate)
    {
      PerformGuidingAtRate();
    }
  }
}

// Guide ---------------------------------------------------------------------------------------------

// Guide, commands to move the mount in any direction at a series of fixed rates
void Guide()
{
  // 1/100 second sidereal timer, controls issue of steps at the selected RA and/or Dec rate(s)

  if (GuidingState == GuidingOFF)
  {
    return;
  }
  if (lastError != ERR_NONE)
  {
    guideA1.duration = -1;
    guideA2.duration = -1;
  }
  if (rtk.updateguideSiderealTimer())
  {

    if (guideA2.duration <= 0 && guideA1.duration <= 0 && GuidingState == GuidingPulse)
    {
      cli();
      if (guideA1.dir) guideA1.dir = 'b';
      if (guideA2.dir) guideA2.dir = 'b';
      sei();
      return;
    }
    if (guideA1.dir == 'w' || guideA1.dir == 'e')
    {
      if (guideA1.duration > 0 || GuidingState == GuidingRecenter || GuidingState == GuidingST4)
      {
        if (!bl_Axis1.correcting)
        {
          bool rev = guideA1.dir == 'e';
          cli();
          rev ? targetAxis1 -= guideA1.amount : targetAxis1 += guideA1.amount;
          sei();
          if (GuidingState == GuidingPulse)
          {
            // for pulse guiding, count down the mS and stop when timed out
            guideA1.duration -= (long)(micros() - guideA1.durationLast);
            guideA1.durationLast = micros();
          }
        }
        else
        {
          // don't count time if in backlash
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
      if (guideA2.duration > 0 || GuidingState == GuidingRecenter || GuidingState == GuidingST4)
      {
        if (!bl_Axis2.correcting)
        {
          bool rev = false;
          if (guideA2.dir == 's')
            rev = true;
          if (GetPierSide() >= PIER_WEST)
            rev = !rev;
          cli();
          rev ? targetAxis2 -= guideA2.amount : targetAxis2 += guideA2.amount;
          sei();
          if (GuidingState == GuidingPulse)
          {
            // for pulse guiding, count down the mS and stop when timed out
            guideA2.duration -= (long)(micros() - guideA2.durationLast);
            guideA2.durationLast = micros();
          }
        }
        else
        {
          // don't count time if in backlash
          guideA2.durationLast = micros();
        }
      }
      else
      {
        cli();
        guideA2.dir = 'b';
        sei();
      }   // break
    }
    else
    {
      guideA2.duration = -1;
    }
  }
}


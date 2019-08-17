// Guide ---------------------------------------------------------------------------------------------

// Guide, commands to move the mount in any direction at a series of fixed rates
void Guide()
{
  // 1/100 second sidereal timer, controls issue of steps at the selected RA and/or Dec rate(s)
  if (GuidingState == GuidingOFF)
  {
    return;
  }
  if (rtk.updateguideSiderealTimer())
  {

    if (guideDurationAxis2 <= 0 && guideDurationAxis1 <= 0 && GuidingState == GuidingPulse)
    {
      cli();
      if (guideDirAxis1) guideDirAxis1 = 'b';
      if (guideDirAxis2) guideDirAxis2 = 'b';
      sei();
      return;
    }
    if (guideDirAxis1 == 'w' || guideDirAxis1 == 'e')
    {
      if (guideDurationAxis1 > 0 || GuidingState == GuidingRecenter ||GuidingState == GuidingST4)
      {
        if (!inbacklashAxis1)
        {
          // guideAxis1 keeps track of how many steps we've moved for PEC recording
          if (guideDirAxis1 == 'e')
            guideAxis1.fixed = -amountGuideAxis1.fixed;
          else if (guideDirAxis1 == 'w')
            guideAxis1.fixed = amountGuideAxis1.fixed;
          cli();
          targetAxis1.fixed += guideAxis1.fixed;
          sei();
     ;
          if (GuidingState == GuidingPulse)
          {
            // for pulse guiding, count down the mS and stop when timed out
            guideDurationAxis1 -= (long)(micros() - guideDurationLastAxis1);
            guideDurationLastAxis1 = micros();
          }
        }
        else
        {
          // don't count time if in backlash
          guideDurationLastAxis1 = micros();
        }
      }
      else
      {
        cli();
        guideDirAxis1 = 'b';
        sei();
      }
    }
    else
    {
      guideDurationAxis1 = -1;
    }
    if (guideDirAxis2 == 's' || guideDirAxis2 == 'n')
    {
      if (guideDurationAxis2 > 0 || GuidingState == GuidingRecenter || GuidingState == GuidingST4)
      {
        if (!inbacklashAxis2)
        {
          bool rev = false;
          if (guideDirAxis2 == 's')
            rev = true;
          if (GetPierSide() >= PIER_WEST)
            rev = !rev;
          cli();
          rev ? targetAxis2.fixed -= amountGuideAxis2.fixed : targetAxis2.fixed += amountGuideAxis2.fixed;
          sei();
          if (GuidingState == GuidingPulse)
          {
            // for pulse guiding, count down the mS and stop when timed out
            guideDurationAxis2 -= (long)(micros() - guideDurationLastAxis2);
            guideDurationLastAxis2 = micros();
          }

        }
        else
        {
          // don't count time if in backlash
          guideDurationLastAxis2 = micros();
        }
      }
      else
      {
        cli();
        guideDirAxis2 = 'b';
        sei();
      }   // break
    }
    else
    {
      guideDurationAxis2 = -1;
    }

  }
}


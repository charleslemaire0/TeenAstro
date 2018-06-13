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

    if (guideDurationDec <= 0 && guideDurationHA <= 0 && GuidingState == GuidingPulse)
    {
      cli();
      if (guideDirAxis1) guideDirAxis1 = 'b';
      if (guideDirAxis2) guideDirAxis2 = 'b';
      sei();
      return;
    }
    if (guideDirAxis1 == 'w' || guideDirAxis1 == 'e')
    {
      if (guideDurationHA > 0 || GuidingState == GuidingRecenter)
      {
        if (!inbacklashAxis1)
        {
          // guideHA keeps track of how many steps we've moved for PEC recording
          if (guideDirAxis1 == 'e')
            guideHA.fixed = -amountGuideHA.fixed;
          else if (guideDirAxis1 == 'w')
            guideHA.fixed = amountGuideHA.fixed;
          cli();
          targetAxis1.fixed += guideHA.fixed;
          sei();
     ;
          if (GuidingState == GuidingPulse)
          {
            // for pulse guiding, count down the mS and stop when timed out
            guideDurationHA -= (long)(micros() - guideDurationLastHA);
            guideDurationLastHA = micros();
            Serial.println(guideDurationHA);
          }
        }
        else
        {
          // don't count time if in backlash
          guideDurationLastHA = micros();
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
      guideDurationHA = -1;
    }
    if (guideDirAxis2 == 's' || guideDirAxis2 == 'n')
    {
      if (guideDurationDec > 0 || GuidingState == GuidingRecenter)
      {
        if (!inbacklashAxis2)
        {
          bool rev = false;
          if (guideDirAxis2 == 's')
            rev = true;
          if (pierSide >= PierSideWest)
            rev = !rev;
          cli();
          rev ? targetAxis2.fixed -= amountGuideDec.fixed : targetAxis2.fixed += amountGuideDec.fixed;
          sei();
          if (GuidingState == GuidingPulse)
          {
            // for pulse guiding, count down the mS and stop when timed out
            guideDurationDec -= (long)(micros() - guideDurationLastDec);
            guideDurationLastDec = micros();
          }

        }
        else
        {
          // don't count time if in backlash
          guideDurationLastDec = micros();
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
      guideDurationDec = -1;
    }

  }
}


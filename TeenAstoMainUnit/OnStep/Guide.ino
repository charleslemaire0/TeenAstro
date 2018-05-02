// Guide ---------------------------------------------------------------------------------------------

// Guide, commands to move the mount in any direction at a series of fixed rates
void Guide()
{
  // 1/100 second sidereal timer, controls issue of steps at the selected RA and/or Dec rate(s)

  if (rtk.updateguideSiderealTimer())
  {
    if (guideDirAxis1)
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

        // for pulse guiding, count down the mS and stop when timed out
        if (guideDurationHA > 0)
        {
          guideDurationHA -= (long)(micros() - guideDurationLastHA);
          guideDurationLastHA = micros();
          if (guideDurationHA <= 0)
          {
            guideDirAxis1 = 'b';
          }   // break
        }
      }
      else
      {
        // don't count time if in backlash
        guideDurationLastHA = micros();
      }
    }

    if (guideDirAxis2 == 's' || guideDirAxis2 == 'n')
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

        // for pulse guiding, count down the mS and stop when timed out
        if (guideDurationDec > 0)
        {
          guideDurationDec -= (long)(micros() - guideDurationLastDec);
          guideDurationLastDec = micros();
          if (guideDurationDec <= 0)
          {
            guideDirAxis2 = 'b';
          }   // break
        }
      }
      else
      {
        // don't count time if in backlash
        guideDurationLastDec = micros();
      }
    }
  }
}

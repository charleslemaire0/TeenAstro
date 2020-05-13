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
    guideDurationAxis1 = -1;
    guideDurationAxis2 = -1;
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
      if (guideDurationAxis1 > 0 || GuidingState == GuidingRecenter || GuidingState == GuidingST4)
      {
        if (!bl_Axis1.correcting)
        {
          bool rev = guideDirAxis1 == 'e';
          cli();
          rev ? targetAxis1 -= amountGuideAxis1 : targetAxis1 += amountGuideAxis1;
          sei();
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
        if (!bl_Axis2.correcting)
        {
          bool rev = false;
          if (guideDirAxis2 == 's')
            rev = true;
          if (GetPierSide() >= PIER_WEST)
            rev = !rev;
          cli();
          rev ? targetAxis2 -= amountGuideAxis2 : targetAxis2 += amountGuideAxis2;
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


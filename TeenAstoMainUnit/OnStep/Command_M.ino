//   M - Telescope Movement Commands

void Command_M(bool &supress_frame)
{
  switch (command[1])
  {
  case 'A':
    //  :MA#   Goto the target Alt and Az
    //         Returns:
    //         0=Goto is Possible
    //         1=Object below horizon
    //         2=No object selected
    //         4=Position unreachable
    //         6=Outside limits
    i = goToHor(&newTargetAlt, &newTargetAzm);
    reply[0] = i + '0';
    reply[1] = 0;
    quietReply = true;
    supress_frame = true;
    break;

  case 'g':
  {
    //  :Mgdnnnn# Pulse guide command
        //          Returns: Nothing

    if ((atoi2((char *)&parameter[1], &i)) &&
      ((i > 0) && (i <= 16399)) && trackingState == TrackingON && GuidingState != GuidingRecenter )
    {
     
      if ((parameter[0] == 'e') || (parameter[0] == 'w'))
      {
#ifdef SEPERATE_PULSE_GUIDE_RATE_ON
        enableGuideRate(0,false);
#else
        enableGuideRate(0);
#endif
        guideDirAxis1 = parameter[0];
        guideDurationLastHA = micros();
        guideDurationHA = (long)i * 1000L;
        cli();
        GuidingState = GuidingPulse;
        if (guideDirAxis1 == 'e')
          guideTimerRateAxis1 = -guideTimerBaseRate;
        else
          guideTimerRateAxis1 = guideTimerBaseRate;
        sei();
        quietReply = true;
      }
      else if ((parameter[0] == 'n') || (parameter[0] == 's'))
      {
#ifdef SEPERATE_PULSE_GUIDE_RATE_ON
        enableGuideRate(0,false);
#else
        enableGuideRate(currentGuideRate);
#endif
        guideDirAxis2 = parameter[0];
        guideDurationLastDec = micros();
        guideDurationDec = (long)i * 1000L;
        if (guideDirAxis2 == 's' || guideDirAxis2 == 'n')
        {
          bool rev = false;
          if (guideDirAxis2 == 's')
            rev = true;
          if (pierSide >= PierSideWest)
            rev = !rev;
          cli();
          GuidingState = GuidingPulse;
          guideTimerRateAxis2 = rev ? -guideTimerBaseRate : guideTimerBaseRate;
          sei();
        }
        quietReply = true;
      }
      else
        commandError = true;
    }
    else
      commandError = true;
    break;

  }
  case 'e':
  case 'w':
    //  :Me# & :Mw#      Move Telescope East or West at current slew rate
    //         Returns: Nothing
  {
    MoveAxis1(command[1]);
    quietReply = true;
  }
  break;


  case 'n':
  case 's':
    //  :Mn# & :Ms#      Move Telescope North or South at current slew rate
    //         Returns: Nothing
  {
    MoveAxis2(command[1]);
    quietReply = true;
  }
  break;


  case 'P':

    //  :MP#   Goto the Current Position for Polar Align
        //         Returns:
        //         0=Goto is Possible
        //         1=Object below horizon    Outside limits, below the Horizon limit
        //         2=No object selected      Failure to resolve coordinates
        //         4=Position unreachable    Not unparked
        //         5=Busy                    Goto already active
        //         6=Outside limits          Outside limits, above the Zenith limit
  {
    double  r, d;
    getEqu(&r, &d, false);
    GeoAlign.altCor = 0.0;
    GeoAlign.azmCor = 0.0;
    i = goToEqu(r, d);
    reply[0] = i + '0';
    reply[1] = 0;
    quietReply = true;
    supress_frame = true;
  }
  break;

  case 'S':
  {
    //  :MS#   Goto the Target Object
        //         Returns:
        //         0=Goto is Possible
        //         1=Object below horizon    Outside limits, below the Horizon limit
        //         2=No object selected      Failure to resolve coordinates
        //         4=Position unreachable    Not unparked
        //         5=Busy                    Goto already active
        //         6=Outside limits          Outside limits, above the Zenith limit

    i = goToEqu(newTargetRA, newTargetDec);
    reply[0] = i + '0';
    reply[1] = 0;
    quietReply = true;
    supress_frame = true;
    break;
  default:
    commandError = true;
    break;
  }
  }
}
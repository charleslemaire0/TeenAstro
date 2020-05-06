//   M - Telescope Movement Commands
#include "ValueToString.h"
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
    i = goToHor(&newTargetAzm, &newTargetAlt, GetPierSide());
    reply[0] = i + '0';
    reply[1] = 0;
    quietReply = true;
    supress_frame = true;
    break;
  case 'F':
  {
    // Flip Mount
    //         Returns:
    //         0=Goto is Possible
    //         1=Object below horizon
    //         2=No object selected
    //         4=Position unreachable
    //         6=Outside limits
    if (mountType == MOUNT_TYPE_GEM)
    {
      getEqu(&f, &f1, localSite.cosLat(), localSite.sinLat(), false);
      newTargetRA = f;
      newTargetDec = f1;
      PierSide preferedPierSide = (GetPierSide() == PIER_EAST) ? PIER_WEST : PIER_EAST;
      double  newTargetHA = haRange(rtk.LST() * 15.0 - newTargetRA);
      i = goToEqu(newTargetHA, newTargetDec, preferedPierSide, localSite.cosLat(), localSite.sinLat());
      reply[0] = i + '0';
      reply[1] = 0;
      quietReply = true;
      supress_frame = true;
    }
    break;
  }
  case 'g':
  {
    //  :Mgdnnnn# Pulse guide command
        //          Returns: Nothing
    if ((atoi2((char *)&parameter[1], &i)) &&
      ((i > 0) && (i <= 16399)) && sideralTracking && !movingTo && lastError == ERR_NONE &&
        (GuidingState != GuidingRecenter || GuidingState != GuidingST4))
    {
      if ((parameter[0] == 'e') || (parameter[0] == 'w'))
      {
        enableGuideRate(0, false);
        guideDirAxis1 = parameter[0];
        guideDurationLastAxis1 = micros();
        guideDurationAxis1 = (long)i * 1000L;
        cli();
        GuidingState = GuidingPulse;
        if (guideDirAxis1 == 'e')
          guideTimerRateAxis1 = -guideTimerBaseRate;
        else
          guideTimerRateAxis1 = guideTimerBaseRate;
        sei();
        //reply[0] = '1';
        //reply[1] = 0;
      }
      else if ((parameter[0] == 'n') || (parameter[0] == 's'))
      {

        enableGuideRate(0, false);
        guideDirAxis2 = parameter[0];
        guideDurationLastAxis2 = micros();
        guideDurationAxis2 = (long)i * 1000L;
        if (guideDirAxis2 == 's' || guideDirAxis2 == 'n')
        {
          bool rev = false;
          if (guideDirAxis2 == 's')
            rev = true;
          if (GetPierSide() >= PIER_WEST)
            rev = !rev;
          cli();
          GuidingState = GuidingPulse;
          guideTimerRateAxis2 = rev ? -guideTimerBaseRate : guideTimerBaseRate;
          sei();
        }
        //reply[0] = '1';
        //reply[1] = 0;
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
    MoveAxis1(command[1], GuidingRecenter);
    quietReply = true;
  }
  break;
  case 'n':
  case 's':
    //  :Mn# & :Ms#      Move Telescope North or South at current slew rate
    //         Returns: Nothing
  {
    MoveAxis2(command[1], GuidingRecenter);
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
  //{
  //  double  r, d;
  //  getEqu(&r, &d, false);
  //  GeoAlign.altCor = 0.0;
  //  GeoAlign.azmCor = 0.0;
  //  i = goToEqu(r, d, pierSide);
  //  reply[0] = i + '0';
  //  reply[1] = 0;
  //  quietReply = true;
  //  supress_frame = true;
  //}
    break;

  case 'S': //:MS#   Goto the Target Object
  {
        //:MS#   Goto the Target Object
        //         Returns:
        //         0=Goto is Possible
        //         1=Object below horizon    Outside limits, below the Horizon limit
        //         2=No object selected      Failure to resolve coordinates
        //         4=Position unreachable    Not unparked
        //         5=Busy                    Goto already active
        //         6=Outside limits          Outside limits, above the Zenith limit
        //         7=Guiding
        //         8=has a an Error
    double  newTargetHA = haRange(rtk.LST() * 15.0 - newTargetRA);
    i = goToEqu(newTargetHA, newTargetDec, GetPierSide(), localSite.cosLat(), localSite.sinLat());
    if (i == 0)
    {
      sideralTracking = true;
      lastSetTrakingEnable = millis();
      atHome = false;
    }
    reply[0] = i + '0';
    reply[1] = 0;
    quietReply = true;
    supress_frame = true;
    break;
  }
  case 'U':
  {
    //  :MU#   Goto the User Defined Target Object
    //         Returns:
    //         0=Goto is Possible
    //         1=Object below horizon    Outside limits, below the Horizon limit
    //         2=No object selected      Failure to resolve coordinates
    //         4=Position unreachable    Not unparked
    //         5=Busy                    Goto already active
    //         6=Outside limits          Outside limits, above the Zenith limit
    //         7=Guiding
    //         8=has a an Error
    PierSide targetPierSide = GetPierSide();
    newTargetRA = (double)XEEPROM.readFloat(EE_RA);
    newTargetDec = (double)XEEPROM.readFloat(EE_DEC);
    double newTargetHA = haRange(rtk.LST() * 15.0 - newTargetRA);
    i = goToEqu(newTargetHA, newTargetDec, targetPierSide, localSite.cosLat(), localSite.sinLat());
    if (i == 0)
    {
      sideralTracking = true;
      lastSetTrakingEnable = millis();
      atHome = false;
    }
    reply[0] = i + '0';
    reply[1] = 0;
    quietReply = true;
    supress_frame = true;
    break;
  }
  case '?':
  {
    //  :M?#   Predict side of Pier for the Target Object
    double objectRa, objectHa, objectDec;
    char rastr[12];
    char decstr[12];
    strncpy(rastr, parameter, 8 * sizeof(char));
    rastr[8] = 0;
    strncpy(decstr, &parameter[8], 9 * sizeof(char));
    decstr[9] = 0;
    if (!hmsToDouble(&objectRa, rastr, highPrecision))
    {
      commandError = true;
      return;
    }
    if (!dmsToDouble(&objectDec, decstr, true, highPrecision))
    {
      commandError = true;
      return;
    }
    objectHa = haRange(rtk.LST() * 15.0 - objectRa * 15);

    byte side = predictSideOfPier(objectHa, objectDec, GetPierSide());
    if (side == 0) reply[0] = '?';
    else if (side == PIER_EAST) reply[0] = 'E';
    else if (side == PIER_WEST) reply[0] = 'W';
    reply[1] = 0;
    quietReply = true;
    break;
  }
  case '@':
  {
    //  :M@#   Start Spiral Search
    //         Return 1
    if (movingTo || GuidingState != GuidingOFF)
      commandError = true;
    else
      doSpiral = true;
    break;
  }
  default:
    commandError = true;
    break;
  }
}
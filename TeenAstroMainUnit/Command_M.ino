#include "ValueToString.h"
#include "Command.h"
//   M - Telescope Movement Commands
void Command_M()
{
  int i;
  double f, f1;
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
    }
    break;
  }
  case 'g':
  {
    //  :Mgdnnnn# Pulse guide command
    //  Returns: Nothing

    if ((atoi2((char *)&command[3], &i)) &&
      ((i > 0) && (i <= 16399)) && sideralTracking && !movingTo && lastError == ERR_NONE &&
        (GuidingState != GuidingRecenter || GuidingState != GuidingST4))
    {
      if ((command[2] == 'e') || (command[2] == 'w'))
      {
        enableGuideRate(0, false);
        guideA1.dir = command[2];
        guideA1.durationLast = micros();
        guideA1.duration = (long)i * 1000L;
        cli();
        GuidingState = GuidingPulse;
        if (guideA1.dir == 'e')
          guideA1.timerRate = -guideTimerBaseRate;
        else
          guideA1.timerRate = guideTimerBaseRate;
        sei();
        //reply[0] = '1';
        //reply[1] = 0;
      }
      else if ((command[2] == 'n') || (command[2] == 's'))
      {

        enableGuideRate(0, false);
        guideA2.dir = command[2];
        guideA2.durationLast = micros();
        guideA2.duration = (long)i * 1000L;
        if (guideA2.dir == 's' || guideA2.dir == 'n')
        {
          bool rev = false;
          if (guideA2.dir == 's')
            rev = true;
          if (GetPierSide() >= PIER_WEST)
            rev = !rev;
          cli();
          GuidingState = GuidingPulse;
          guideA2.timerRate = rev ? -guideTimerBaseRate : guideTimerBaseRate;
          sei();
        }
        //reply[0] = '1';
        //reply[1] = 0;
      }
    }
    break;
  }
  case 'e':
  case 'w':
    //  :Me# & :Mw#      Move Telescope East or West at current slew rate
    //  Returns: Nothing
  {
    MoveAxis1(command[1], GuidingRecenter);
  }
  break;
  case 'n':
  case 's':
    //  :Mn# & :Ms#      Move Telescope North or South at current slew rate
    //  Returns: Nothing
  {
    MoveAxis2(command[1], GuidingRecenter);
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
    break;
  }
  case '?':
  {
    //  :M?#   Predict side of Pier for the Target Object
    // reply ?# or E# or W#
    // reply !# if failed
    double objectRa, objectHa, objectDec;
    char rastr[12];
    char decstr[12];
    strncpy(rastr, &command[2], 8 * sizeof(char));
    rastr[8] = 0;
    strncpy(decstr, &command[10], 9 * sizeof(char));
    decstr[9] = 0;
    if (!hmsToDouble(&objectRa, rastr, highPrecision))
    {
      strcpy(reply, "!#");
      return;
    }
    if (!dmsToDouble(&objectDec, decstr, true, highPrecision))
    {
      strcpy(reply, "!#");
      return;
    }
    objectHa = haRange(rtk.LST() * 15.0 - objectRa * 15);

    byte side = predictSideOfPier(objectHa, objectDec, GetPierSide());
    strcpy(reply, "?#");
    if (side == 0) reply[0] = '?';
    else if (side == PIER_EAST) reply[0] = 'E';
    else if (side == PIER_WEST) reply[0] = 'W';
    break;
  }
  case '@':
  {
    //  :M@#   Start Spiral Search
    //         Return 0 if failed, i if success
    if (movingTo || GuidingState != GuidingOFF)
      strcpy(reply, "0");
    else
    {
      strcpy(reply, "1");
      doSpiral = true;
    }
    break;
  }
  default:
    strcpy(reply, "0");
    break;
  }
}
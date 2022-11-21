#include "ValueToString.h"
#include "Command.h"
//   M - Telescope Movement Commands
void Command_M()
{
  int i;
  double f;
  bool ok = false;
  char* conv_end;
  switch (command[1])
  {
    //  :M1svv.vvv# Move Axis1 at rate (signed value!) in time the sideral speed
    //  :M2svv.vvv# Move Axis2 at rate (signed value!) in time the sideral speed
  case '1':
  case '2':
    f = strtod(&command[2], &conv_end);
    ok = (&command[2] != conv_end) && abs(f) <= guideRates[4];
    ok &= !movingTo && lastError == ERRT_NONE;
    ok &= (GuidingState == GuidingOFF || GuidingState == GuidingAtRate);
    if (ok)
    {
      if (command[1] == '1')
      {
        MoveAxis1AtRate(f);
      }
      else
      {
        MoveAxis2AtRate(f);
      }
      strcpy(reply, "1");
    }
    else
      strcpy(reply, "0");
    break;
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
    //       Returns an ERRGOTO
    if (mountType == MOUNT_TYPE_GEM)
    {
      i = Flip();
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
      ((i > 0) && (i <= 16399)) && sideralTracking && !movingTo && lastError == ERRT_NONE &&
        (GuidingState != GuidingRecenter || GuidingState != GuidingST4))
    {
      if ((command[2] == 'e') || (command[2] == 'w'))
      {
        enableST4GuideRate();
        guideA1.dir = command[2];
        guideA1.durationLast = micros();
        guideA1.duration = (long)i * 1000L;
        cli();
        GuidingState = GuidingPulse;
        if (guideA1.dir == 'e')
          guideA1.atRate = -guideA1.absRate;
        else
          guideA1.atRate = guideA1.absRate;
        sei();
        //reply[0] = '1';
        //reply[1] = 0;
      }
      else if ((command[2] == 'n') || (command[2] == 's'))
      {
        enableST4GuideRate();
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
          guideA2.atRate = rev ? -guideA2.absRate : guideA2.absRate;
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
        //       Returns an ERRGOTO
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

  case 'S':
  {
    //:MS#   Goto the Target Object
    //       Returns an ERRGOTO
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
    //         Returns an ERRGOTO
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
    double Ra, Ha, Dec, azm, alt = 0, axis1angle,axis2angle;
    long axis1step, axis2step;
    PierSide predictedSide = PIER_NOTVALID;
    char rastr[12];
    char decstr[12];

    strncpy(rastr, &command[2], 8 * sizeof(char));
    rastr[8] = 0;
    strncpy(decstr, &command[10], 9 * sizeof(char));
    decstr[9] = 0;
    if (!hmsToDouble(&Ra, rastr, highPrecision))
    {
      strcpy(reply, "!#");
      break;
    }
    if (!dmsToDouble(&Dec, decstr, true, highPrecision))
    {
      strcpy(reply, "!#");
      break;
    }
    Ha = haRange(rtk.LST() * 15.0 - Ra * 15);
    EquToHor(Ha, Dec, doesRefraction.forGoto,&azm, &alt, localSite.cosLat(), localSite.sinLat());
    if (alt < minAlt || alt > maxAlt)
    {
      strcpy(reply, "!#");
      break;
    }
    alignment.toInstrumentalDeg(axis1angle, axis2angle, azm, alt);
    bool ok = predictTarget(axis1angle, axis2angle, GetPierSide(), axis1step, axis2step,predictedSide);
    if (!ok)
    {
      strcpy(reply, "!#");
      break;
    }
    else if (predictedSide == PIER_EAST) strcpy(reply, "E#");
    else if (predictedSide == PIER_WEST) strcpy(reply, "W#");
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
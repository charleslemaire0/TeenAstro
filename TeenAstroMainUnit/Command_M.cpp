/**
 * Move commands: M (slew at rate, goto Alt/Az, flip, pulse guide).
 */
#include "Command.h"
#include "ValueToString.h"

// -----------------------------------------------------------------------------
//   M - Move / slew  :M1# :M2# :MA# :MF# :Mg# etc.
// -----------------------------------------------------------------------------
void Command_M() {
  switch (command[1]) {
  case '1':
  case '2':
  {
    char* conv_end;
    double f = strtod(&command[2], &conv_end);
    bool ok = (&command[2] != conv_end);
    if (!ok)
    {
      strcpy(reply, "i");
    }
    else if (abs(f) > mount.guideRates[4])
    {
      strcpy(reply, "h");
    }
    else if (mount.movingTo)
    {
      strcpy(reply, "s");
    }
    else if (mount.lastError != ErrorsTraking::ERRT_NONE)
    {
      strcpy(reply, "e");
    }
    else if (!(mount.GuidingState == Guiding::GuidingOFF || mount.GuidingState == Guiding::GuidingAtRate))
    {
      strcpy(reply, "g");
    }
    else
    {
      if (command[1] == '1')
      {
        MoveAxisAtRate1(f);
      }
      else
      {
        MoveAxisAtRate2(f);
      }
      replyShortTrue();
    }
  }
    break;
  case 'A':
    //  :MA#   Goto the target Alt and Az
    //         Returns:
    //         0=Goto is Possible
    //         1=Object below horizon
    //         2=No object selected
    //         4=Position unreachable
    //         6=Outside limits
  {
    Coord_HO HO_T(0, mount.newTargetAlt * DEG_TO_RAD, mount.newTargetAzm * DEG_TO_RAD, true);
    byte i = goToHor(HO_T, GetPoleSide());
    reply[0] = i + '0';
    reply[1] = 0;
  }
  break;
  case 'F':
  {
    // :MF# Flip Mount
    //       Returns an ERRGOTO
    if (mount.mountType == MOUNT_TYPE_GEM)
    {
      int i = Flip();
      reply[0] = i + '0';
      reply[1] = 0;
    }
    break;
  }
  case 'g':
  {
    //  :Mgdnnnn# Pulse guide command
    //  Returns: Nothing
    int i;
    if ((atoi2((char *)&command[3], &i)) &&
      ((i > 0) && (i <= 30000)) && !mount.movingTo && mount.lastError == ERRT_NONE &&
        (mount.GuidingState != GuidingRecenter || mount.GuidingState != GuidingST4))
    {
      if ((command[2] == 'e') || (command[2] == 'w'))
      {
        enableST4GuideRate();
        if (command[2] == 'e')
        {
          mount.guideA1.moveBW();
        }
        else if (command[2] == 'w')
        {
          mount.guideA1.moveFW();
        }
        mount.guideA1.durationLast = micros();
        mount.guideA1.duration = (unsigned long)(i * 1000UL);
        cli();
        mount.GuidingState = Guiding::GuidingPulse;
        sei();
        //reply[0] = '1';
        //reply[1] = 0;
      }
      else if ((command[2] == 'n') || (command[2] == 's'))
      {
        enableST4GuideRate();
        if (GetPoleSide() == POLE_UNDER)
        {
          if (command[2] == 'n')
          {
            mount.guideA2.moveFW();
          }
          else if (command[2] == 's')
          {
            mount.guideA2.moveBW();
          }
        }
        else
        {
          if (command[2] == 'n')
          {
            mount.guideA2.moveBW();
          }
          else if (command[2] == 's')
          {
            mount.guideA2.moveFW();
          }
        }
        mount.guideA2.durationLast = micros();
        mount.guideA2.duration = (unsigned long)(i * 1000UL);
        cli();
        mount.GuidingState = Guiding::GuidingPulse;
        sei();
        
        //reply[0] = '1';
        //reply[1] = 0;
      }
    }
    replyNothing();
    break;
  }
    //  :Me# & :Mw#      Move Telescope East or West at current slew rate
    //  Returns: Nothing
  case 'e':
    MoveAxis1(true, Guiding::GuidingRecenter);
    break;
  case 'w':
    MoveAxis1(false, Guiding::GuidingRecenter);
    break;
  break;
  //  :Mn# & :Ms#      Move Telescope North or South at current slew rate
  //  Returns: Nothing
  case 'n':

    if (GetPoleSide() >= POLE_OVER)
      MoveAxis2(true, Guiding::GuidingRecenter);
    else
      MoveAxis2(false, Guiding::GuidingRecenter);
    break;
  case 's':
    if (GetPoleSide() >= POLE_OVER)
      MoveAxis2(false, Guiding::GuidingRecenter);
    else
     MoveAxis2(true, Guiding::GuidingRecenter);
  break;

  case 'P':
    //  :MP#   Goto the Current Position for Polar Align
        //       Returns an ERRGOTO
  //{
  //  double  r, d;
  //  getEqu(&r, &d, false);
  //  GeoAlign.altCor = 0.0;
  //  GeoAlign.azmCor = 0.0;
  //  i = goToEqu(r, d, PoleSide);
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
    double  newTargetHA = haRange(rtk.LST() * 15.0 - mount.newTargetRA);
    Coord_EQ EQ_T(0, mount.newTargetDec* DEG_TO_RAD, newTargetHA* DEG_TO_RAD);
    byte i = goToEqu(EQ_T, GetPoleSide(), *localSite.latitude() * DEG_TO_RAD);
    if (i == 0)
    {
      StartSideralTracking();
    }
    reply[0] = i + '0';
    reply[1] = 0;
    break;
  }
  case 'U':
  {
    //  :MU#   Goto the User Defined Target Object
    //         Returns an ERRGOTO
    PoleSide targetPoleSide = GetPoleSide();
    mount.newTargetRA = (double)XEEPROM.readFloat(getMountAddress(EE_RA));
    mount.newTargetDec = (double)XEEPROM.readFloat(getMountAddress(EE_DEC));
    double newTargetHA = haRange(rtk.LST() * 15.0 - mount.newTargetRA);
    Coord_EQ EQ_T(0, mount.newTargetDec* DEG_TO_RAD, newTargetHA* DEG_TO_RAD);
    byte i = goToEqu(EQ_T, targetPoleSide, *localSite.latitude() * DEG_TO_RAD);
    if (i == 0)
    {
      StartSideralTracking();
    }
    reply[0] = i + '0';
    reply[1] = 0;
    break;
  }
  case '?':
  {
    // :M?#   Predict side of Pier for the Target Object
    // reply ? or E or W
    // reply ! if failed
    double Ra, Ha, Dec, alt = 0, axis1angle,axis2angle;
    long axis1step, axis2step;
    PoleSide predictedSide = POLE_NOTVALID;
    char rastr[12];
    char decstr[12];

    strncpy(rastr, &command[2], 8 * sizeof(char));
    rastr[8] = 0;
    strncpy(decstr, &command[10], 9 * sizeof(char));
    decstr[9] = 0;

    if (!hmsToDouble(&Ra, rastr, highPrecision))
    {
      strcpy(reply, "!");
      break;
    }
    if (!dmsToDouble(&Dec, decstr, true, highPrecision))
    {
      strcpy(reply, "!");
      break;
    }
    Ha = haRange(rtk.LST() * 15.0 - Ra * 15);

    Coord_EQ EQ_T(0, Dec * DEG_TO_RAD , Ha * DEG_TO_RAD);
    Coord_HO HO_T = EQ_T.To_Coord_HO(*localSite.latitude() * DEG_TO_RAD, RefrOptForGoto());
    if (HO_T.Alt() < mount.minAlt * DEG_TO_RAD || alt > mount.maxAlt * DEG_TO_RAD)
    {
      strcpy(reply, "!");
      break;
    }
    Coord_IN instr_T = HO_T.To_Coord_IN(alignment.Tinv);
    axis1angle = instr_T.Axis1() * RAD_TO_DEG;
    axis2angle = instr_T.Axis2() * RAD_TO_DEG;
    bool ok = predictTarget(axis1angle, axis2angle, GetPoleSide(), axis1step, axis2step,predictedSide);
    if (!ok)
    {
      strcpy(reply, "?");
      break;
    }
    else if (predictedSide == POLE_UNDER) strcpy(reply, "E");
    else if (predictedSide == POLE_OVER) strcpy(reply, "W");
    break;
  }
  case '@':
  {
    //  :M@V#   Start Spiral Search V in arcminutes
    //         Return 0 if failed, i if success
    if (mount.movingTo || mount.GuidingState != Guiding::GuidingOFF)
      replyShortFalse();
    else
    {
      mount.SpiralFOV = (double)strtol(&command[2], NULL, 10) / 60.0;
      replyShortTrue();
      mount.atHome = false;
      mount.doSpiral = true;
    }
    break;
  }
  default:
    replyNothing();
    break;
  }
}
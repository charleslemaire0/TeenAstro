/**
 * Move commands: M (slew at rate, goto Alt/Az, flip, pulse guide).
 * One file per letter (plan). :Me# :Mw# :Mn# :Ms# :MS# :MA# LX200 standard; :MF# :Mg# etc. TeenAstro specific.
 */
#include "Command.h"
#include "ValueToString.h"

// -----------------------------------------------------------------------------
//   M - Move / slew  :M1# :M2# :MA# :MF# :Mg# etc.
// -----------------------------------------------------------------------------
void Command_M() {
  switch (commandState.command[1]) {
  case '1':
  case '2':
  {
    char* conv_end;
    double f = strtod(&commandState.command[2], &conv_end);
    bool ok = (&commandState.command[2] != conv_end);
    if (!ok)
    {
      strcpy(commandState.reply, "i");
    }
    else if (abs(f) > mount.guiding.guideRates[4])
    {
      strcpy(commandState.reply, "h");
    }
    else if (mount.tracking.movingTo)
    {
      strcpy(commandState.reply, "s");
    }
    else if (mount.errors.lastError != ErrorsTraking::ERRT_NONE)
    {
      strcpy(commandState.reply, "e");
    }
    else if (!(mount.guiding.GuidingState == Guiding::GuidingOFF || mount.guiding.GuidingState == Guiding::GuidingAtRate))
    {
      strcpy(commandState.reply, "g");
    }
    else
    {
      if (commandState.command[1] == '1')
      {
        mount.moveAxisAtRate1(f);
      }
      else
      {
        mount.moveAxisAtRate2(f);
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
    Coord_HO HO_T(0, mount.targetCurrent.newTargetAlt * DEG_TO_RAD, mount.targetCurrent.newTargetAzm * DEG_TO_RAD, true);
    byte i = mount.goToHor(HO_T, mount.getPoleSide());
    commandState.reply[0] = i + '0';
    commandState.reply[1] = 0;
  }
  break;
  case 'F':
  {
    // :MF# Flip Mount
    //       Returns an ERRGOTO
    if (mount.config.identity.mountType == MOUNT_TYPE_GEM)
    {
      int i = mount.flip();
      commandState.reply[0] = i + '0';
      commandState.reply[1] = 0;
    }
    break;
  }
  case 'g':
  {
    //  :Mgdnnnn# Pulse guide command
    //  Returns: Nothing
    int i;
    if ((atoi2((char *)&commandState.command[3], &i)) &&
      ((i > 0) && (i <= 30000)) && !mount.tracking.movingTo && mount.errors.lastError == ERRT_NONE &&
        (mount.guiding.GuidingState != GuidingRecenter || mount.guiding.GuidingState != GuidingST4))
    {
      if ((commandState.command[2] == 'e') || (commandState.command[2] == 'w'))
      {
        mount.enableST4GuideRate();
        mount.guiding.guideA1.speedMultiplier = (i > 50) ? ((double)i / 50.0) : 1.0;
        if (commandState.command[2] == 'e')
        {
          mount.guiding.guideA1.moveBW();
        }
        else if (commandState.command[2] == 'w')
        {
          mount.guiding.guideA1.moveFW();
        }
        mount.guiding.guideA1.durationLast = micros();
        mount.guiding.guideA1.duration = (unsigned long)(i * 1000UL);
        cli();
        mount.guiding.GuidingState = Guiding::GuidingPulse;
        sei();
        //commandState.reply[0] = '1';
        //commandState.reply[1] = 0;
      }
      else if ((commandState.command[2] == 'n') || (commandState.command[2] == 's'))
      {
        mount.enableST4GuideRate();
        mount.guiding.guideA2.speedMultiplier = (i > 50) ? ((double)i / 50.0) : 1.0;
        if (mount.getPoleSide() == POLE_UNDER)
        {
          if (commandState.command[2] == 'n')
          {
            mount.guiding.guideA2.moveFW();
          }
          else if (commandState.command[2] == 's')
          {
            mount.guiding.guideA2.moveBW();
          }
        }
        else
        {
          if (commandState.command[2] == 'n')
          {
            mount.guiding.guideA2.moveBW();
          }
          else if (commandState.command[2] == 's')
          {
            mount.guiding.guideA2.moveFW();
          }
        }
        mount.guiding.guideA2.durationLast = micros();
        mount.guiding.guideA2.duration = (unsigned long)(i * 1000UL);
        cli();
        mount.guiding.GuidingState = Guiding::GuidingPulse;
        sei();
        
        //commandState.reply[0] = '1';
        //commandState.reply[1] = 0;
      }
    }
    replyNothing();
    break;
  }
    //  :Me# & :Mw#      Move Telescope East or West at current slew rate
    //  Returns: Nothing
  case 'e':
    mount.moveAxis1(true, Guiding::GuidingRecenter);
    break;
  case 'w':
    mount.moveAxis1(false, Guiding::GuidingRecenter);
    break;
  break;
  //  :Mn# & :Ms#      Move Telescope North or South at current slew rate
  //  Returns: Nothing
  case 'n':

    if (mount.getPoleSide() >= POLE_OVER)
      mount.moveAxis2(true, Guiding::GuidingRecenter);
    else
      mount.moveAxis2(false, Guiding::GuidingRecenter);
    break;
  case 's':
    if (mount.getPoleSide() >= POLE_OVER)
      mount.moveAxis2(false, Guiding::GuidingRecenter);
    else
     mount.moveAxis2(true, Guiding::GuidingRecenter);
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
  //  commandState.reply[0] = i + '0';
  //  commandState.reply[1] = 0;
  //  quietReply = true;
  //  supress_frame = true;
  //}
    break;

  case 'S':
  {
    //:MS#   Goto the Target Object
    //       Returns an ERRGOTO
    double  newTargetHA = haRange(rtk.LST() * 15.0 - mount.targetCurrent.newTargetRA);
    Coord_EQ EQ_T(0, mount.targetCurrent.newTargetDec* DEG_TO_RAD, newTargetHA* DEG_TO_RAD);
    byte i = mount.goToEqu(EQ_T, mount.getPoleSide(), *localSite.latitude() * DEG_TO_RAD);
    if (i == 0)
    {
      mount.startSideralTracking();
    }
    commandState.reply[0] = i + '0';
    commandState.reply[1] = 0;
    break;
  }
  case 'U':
  {
    //  :MU#   Goto the User Defined Target Object
    //         Returns an ERRGOTO
    PoleSide targetPoleSide = mount.getPoleSide();
    mount.targetCurrent.newTargetRA = (double)XEEPROM.readFloat(getMountAddress(EE_RA));
    mount.targetCurrent.newTargetDec = (double)XEEPROM.readFloat(getMountAddress(EE_DEC));
    double newTargetHA = haRange(rtk.LST() * 15.0 - mount.targetCurrent.newTargetRA);
    Coord_EQ EQ_T(0, mount.targetCurrent.newTargetDec* DEG_TO_RAD, newTargetHA* DEG_TO_RAD);
    byte i = mount.goToEqu(EQ_T, targetPoleSide, *localSite.latitude() * DEG_TO_RAD);
    if (i == 0)
    {
      mount.startSideralTracking();
    }
    commandState.reply[0] = i + '0';
    commandState.reply[1] = 0;
    break;
  }
  case '?':
  {
    // :M?#   Predict side of Pier for the Target Object
    // reply ? or E or W
    // reply ! if failed
    double Ra, Ha, Dec, alt = 0.0, axis1angle, axis2angle;
    long axis1step, axis2step;
    PoleSide predictedSide = POLE_NOTVALID;
    char rastr[12];
    char decstr[12];

    strncpy(rastr, &commandState.command[2], 8 * sizeof(char));
    rastr[8] = 0;
    strncpy(decstr, &commandState.command[10], 9 * sizeof(char));
    decstr[9] = 0;

    if (!hmsToDouble(&Ra, rastr, commandState.highPrecision))
    {
      strcpy(commandState.reply, "!");
      break;
    }
    if (!dmsToDouble(&Dec, decstr, true, commandState.highPrecision))
    {
      strcpy(commandState.reply, "!");
      break;
    }
    Ha = haRange(rtk.LST() * 15.0 - Ra * 15);

    Coord_EQ EQ_T(0, Dec * DEG_TO_RAD , Ha * DEG_TO_RAD);
    Coord_HO HO_T = EQ_T.To_Coord_HO(*localSite.latitude() * DEG_TO_RAD, mount.refrOptForGoto());
    if (HO_T.Alt() < mount.limits.minAlt * DEG_TO_RAD || alt > mount.limits.maxAlt * DEG_TO_RAD)
    {
      strcpy(commandState.reply, "!");
      break;
    }
    Coord_IN instr_T = HO_T.To_Coord_IN(mount.alignment.conv.Tinv);
    axis1angle = instr_T.Axis1() * RAD_TO_DEG;
    axis2angle = instr_T.Axis2() * RAD_TO_DEG;
    bool ok = mount.predictTarget(axis1angle, axis2angle, mount.getPoleSide(), axis1step, axis2step,predictedSide);
    if (!ok)
    {
      strcpy(commandState.reply, "?");
      break;
    }
    else if (predictedSide == POLE_UNDER) strcpy(commandState.reply, "E");
    else if (predictedSide == POLE_OVER) strcpy(commandState.reply, "W");
    break;
  }
  case '@':
  {
    //  :M@V#   Start Spiral Search V in arcminutes
    //         Return 0 if failed, i if success
    if (mount.tracking.movingTo || mount.guiding.GuidingState != Guiding::GuidingOFF)
      replyShortFalse();
    else
    {
      mount.tracking.SpiralFOV = (double)strtol(&commandState.command[2], NULL, 10) / 60.0;
      replyShortTrue();
      mount.parkHome.atHome = false;
      mount.tracking.doSpiral = true;
    }
    break;
  }
  default:
    replyNothing();
    break;
  }
}
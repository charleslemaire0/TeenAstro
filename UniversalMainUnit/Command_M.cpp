#include "Global.h"
//   M - Telescope Movement Commands

void Command_M()
{
  int i;
  double speed;
  bool ok = false;
  char* conv_end;
  switch (command[1])
  {
    //  :M1svv.vvv# Move Axis1 at rate (signed value!) in time the sideral speed
    //  :M2svv.vvv# Move Axis2 at rate (signed value!) in time the sideral speed
  case '1':
  case '2':
    speed = strtod(&command[2], &conv_end);
    ok = (&command[2] != conv_end) && abs(speed) <= guideRates[4];
    ok &= !isSlewing() && lastError() == ERRT_NONE;
    ok &= (GuidingState == GuidingOFF || GuidingState == GuidingAtRate);
    if (ok)
    {
      byte dir; 
      if (command[1] == '1')
      {
        dir = (speed >= 0) ? 'w' : 'e'; 
        MoveAxis1AtRate(fabs(speed), dir);
      }
      else
      {
        dir = (speed >= 0) ? 'n' : 's'; 
        MoveAxis2AtRate(fabs(speed), dir);
      }
      replyShortTrue();
    }
    else
      replyLongUnknown();
    break;
  case 'A':
    //  :MA#   Goto the target Alt and Az
    //         Returns:
    //         0=Goto is Possible
    //         1=Object below horizon
    //         2=No object selected
    //         4=Position unreachable
    //         6=Outside limits
    HorCoords hc;
    hc.az =  newTargetAzm;
    hc.alt =  newTargetAlt;
    i = mount.mP->goToHor(&hc);
    reply[0] = i + '0';
    reply[1] = 0;
    break;
  case 'F': 
  {
    // Flip Mount
    //       Returns an ERRGOTO
    if (mount.mP->type == MOUNT_TYPE_GEM)
    {
      i = mount.mP->Flip();
      reply[0] = i + '0';
      reply[1] = 0;
    }
    break;
  }
  case 'g':
  {
    //  :Mgdnnnn# Pulse guide command
    //  Returns: Nothing
    if ((atoi2((char *)&command[3], &i)) && ((i > 0) && (i <= 16399)))
    {
      char dir = command[2];
      startGuiding(dir, i); 
    }
  }
  break;
  case 'e':
  case 'w':
    //  :Me# & :Mw#      Move Telescope East or West at current slew rate
    //  Returns: Nothing
  {
    MoveAxis1(command[1]);
  }
  break;
  case 'n':
  case 's':
    //  :Mn# & :Ms#      Move Telescope North or South at current slew rate
    //  Returns: Nothing
  {
    MoveAxis2(command[1]);
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
    EqCoords eq;
    eq.ha = haRange(rtk.LST() * 15.0 - newTargetRA);
    eq.dec = newTargetDec;
    i = mount.mP->goToEqu(&eq);
    if (i == 0)
    {
      startTracking();
    }
    reply[0] = i + '0';
    reply[1] = 0;
    break;
  }
  case 'U':
  {
    //  :MU#   Goto the User Defined Target Object
    //         Returns an ERRGOTO
    EqCoords eq;
    newTargetRA = (double)XEEPROM.readFloat(getMountAddress(EE_RA));
    newTargetDec = (double)XEEPROM.readFloat(getMountAddress(EE_DEC));
    eq.ha = haRange(rtk.LST() * 15.0 - newTargetRA);
    eq.dec = newTargetDec;
    i = mount.mP->goToEqu(&eq);
    if (i == 0)
    {
      startTracking();
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
    double Ra, Ha, Dec, azm, alt = 0;
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
    if (alt < limits.minAlt || alt > limits.maxAlt)
    {
      strcpy(reply, "!#");
      break;
    }
    EqCoords eqCoords;
    eqCoords.ha = Ha;
    eqCoords.dec = Dec;
    bool res = mount.mP->getTargetPierSide (&eqCoords, &predictedSide);
    if (!res)
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
    //  :M@V#   Start Spiral Search V in arcminutes
    //         Return 0 if failed, 1 if success
    if (isSlewing() || GuidingState != GuidingOFF)
      replyShortFalse();
    else
    {
      double fov = (double)strtol(&command[2], NULL, 10) / 60.0;
      startSpiral(fov);
      replyShortTrue();
    }
    break;
  }
  default:
    replyNothing();
    break;
  }
}
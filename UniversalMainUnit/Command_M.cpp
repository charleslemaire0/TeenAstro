#include "Global.h"
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
    ok &= !isSlewing() && lastError == ERRT_NONE;
    ok &= (GuidingState == GuidingOFF || GuidingState == GuidingAtRate);
    if (ok)
    {
      double speed = f*geoA1.stepsPerSecond;
      byte dir; 
      if (command[1] == '1')
      {
        dir = (speed >= 0) ? 'w' : 'e'; 
        mount.mP->MoveAxis1AtRate(speed, dir);
      }
      else
      {
        dir = (speed >= 0) ? 'n' : 's'; 
        mount.mP->MoveAxis2AtRate(speed, dir);
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
    HorCoords hc;
    hc.az =  newTargetAzm;
    hc.alt =  newTargetAlt;
    i = mount.mP->goToHor(&hc);
    reply[0] = i + '0';
    reply[1] = 0;
    break;
  case 'F': // move this to control task
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
      mount.mP->startGuiding(dir, i); 
    }
  }
  break;
  case 'e':
  case 'w':
    //  :Me# & :Mw#      Move Telescope East or West at current slew rate
    //  Returns: Nothing
  {
    mount.mP->MoveAxis1(command[1]);
  }
  break;
  case 'n':
  case 's':
    //  :Mn# & :Ms#      Move Telescope North or South at current slew rate
    //  Returns: Nothing
  {
    mount.mP->MoveAxis2(command[1]);
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
      mount.mP->startTracking();
//      siderealTracking = true;
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
    newTargetRA = (double)XEEPROM.readFloat(EE_RA);
    newTargetDec = (double)XEEPROM.readFloat(EE_DEC);
    eq.ha = haRange(rtk.LST() * 15.0 - newTargetRA);
    eq.dec = newTargetDec;
    i = mount.mP->goToEqu(&eq);
    if (i == 0)
    {
      mount.mP->startTracking();
//      siderealTracking = true;
//      lastSetTrackingEnable = millis();
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
    //  :M@#   Start Spiral Search
    //         Return 0 if failed, i if success
    if (isSlewing() || GuidingState != GuidingOFF)
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
#include "Global.h"

void Command_dollar()
{
  switch (command[1])
  {
  case '$':
    for (int i = 0; i < XEEPROM.length(); i++)
    {
      XEEPROM.write(i, 0);
    }
    break;
  case '!':
    reboot_unit = true;
    replyShortTrue();
    break;
  case 'X':
    initMotors(true);
    replyShortTrue();
    break;
  default:
    replyNothing();
    break;
  }
}
void Command_ACK()
{
  switch (mount.mP->type)
  {
  case MOUNT_TYPE_ALTAZM:
  case MOUNT_TYPE_FORK_ALT:
    strcpy(reply, "A");
    break;
  case MOUNT_TYPE_FORK:
    strcpy(reply, "P");
    break;
  case MOUNT_TYPE_GEM:
    strcpy(reply, "G");
    break;
  case MOUNT_UNDEFINED:
  default:
    strcpy(reply, "L");
    break;
  }
}

//----------------------------------------------------------------------------------
//   A - Alignment Commands
//  :A0#
//  :A2#
//  :AC#
//  :AW#
//  :AA#  Resets alignment as AC# AND activates alignment on next 3 syncs!  (<-> sync modded accordingly)
void Command_A()
{
  switch (command[1])
  {
  case '0':
    mount.mP->initModel(true);
    vTaskDelay(10);
    replyShortTrue();
    break;
  case '2':
  {
    EqCoords eq;
    eq.ha = haRange(rtk.LST() * 15.0 - newTargetRA);
    eq.dec = newTargetDec;

    mount.mP->pm.addStar(&eq);

    replyShortTrue();
    break;
  }
  case 'E':
  {
    double val = mount.mP->pm.getError()* RAD_TO_DEG;
    doubleToDms(reply, &val, false, true, true);
    strcat(reply, "#");
  }
  break;
  case 'C':
  case 'A':
    mount.mP->initModel(true);
    syncAtHome();
    autoAlignmentBySync = command[1] == 'A';
    replyShortTrue();
    break;
  case 'W':
    mount.mP->pm.save();
    replyShortTrue();
    break;
  default:
    replyNothing();
  }
}


//----------------------------------------------------------------------------------
//   B - Reticule/Accessory Control
//  :B+#   Increase reticule Brightness
//         Returns: Nothing
//  :B-#   Decrease Reticule Brightness
//         Returns: Nothing
void Command_B()
{
  if (command[1] != '+' && command[1] != '-')
    return;
#ifdef RETICULE_LED_PINS
  if (reticuleBrightness > 255)
    reticuleBrightness = 255;
  if (reticuleBrightness < 31)
    reticuleBrightness = 31;

  if (command[1] == '-')
    reticuleBrightness /= 1.4;
  if (command[1] == '+')
    reticuleBrightness *= 1.4;

  if (reticuleBrightness > 255)
    reticuleBrightness = 255;
  if (reticuleBrightness < 31)
    reticuleBrightness = 31;

  analogWrite(RETICULE_LED_PINS, reticuleBrightness);
#endif
  replyNothing();
}

//   C - Sync Control
//  :CA#   Synchonize the telescope with the current Azimuth and Altitude coordinates
//         Returns: "N/A#" on success, "En#" on failure where n is the error code per the :MS# command
//  :CM#   Synchonize the telescope with the current database object (as above)
//         Returns: "N/A#" on success, "En#" on failure where n is the error code per the :MS# command
//  :CS#   Synchonize the telescope with the current right ascension and declination coordinates
//         Returns: "N/A#" on success, "En#" on failure where n is the error code per the :MS# command
//  :CU#   Synchonize the telescope with the User defined object
//         Returns: "N/A#" on success, "En#" on failure where n is the error code per the :MS# command
void Command_C()
{
  if ((parkStatus() == PRK_UNPARKED) &&
      !isSlewing() &&
      (command[1] == 'A' || command[1] == 'M' || command[1] == 'S' || command[1] == 'U'))
  {
    PierSide targetPierSide = mount.mP->GetPierSide();
    if (newTargetPierSide != PIER_NOTVALID)
    {
      targetPierSide = newTargetPierSide;
      newTargetPierSide = PIER_NOTVALID;
    }
    switch (command[1])
    {
    case 'M':
    case 'S':
    {
      double newTargetHA;
#if 0      
      if (autoAlignmentBySync)
      {
        newTargetHA = haRange(rtk.LST() * 15.0 - newTargetRA);
        double Azm, Alt;
        EquToHor(newTargetHA, newTargetDec, doesRefraction.forGoto, &Azm, &Alt, localSite.cosLat(), localSite.sinLat());
        if (mount.mP->alignment.getRefs() == 0)
        {
          mount.mP->syncAzAlt(Azm, Alt, mount.mP->GetPierSide());
        }
        double Axis1 = motorA1.getCurrentPos() / geoA1.stepsPerDegree;
        double Axis2 = motorA2.getCurrentPos() / geoA2.stepsPerDegree;
        mount.mP->alignment.addReferenceDeg(Azm, Alt, Axis1, Axis2);
        if (mount.mP->alignment.isReady())
          {
            mount.mP->hasStarAlignment(true);
            motorA1.setTargetPos(motorA1.getCurrentPos());
            motorA2.setTargetPos(motorA2.getCurrentPos());
            autoAlignmentBySync = false;
          }
      }      
      else
#endif      
      {
        newTargetHA = haRange(rtk.LST() * 15.0 - newTargetRA);
        mount.mP->syncEqu(newTargetHA, newTargetDec, targetPierSide, localSite.cosLat(), localSite.sinLat());
      }
      if (command[1] == 'M')
      {
        strcpy(reply, "N/A#");
      }
    }
    break;
    case 'U':
    {
      // :CU# sync with the User Defined RA DEC
      newTargetRA = (double)XEEPROM.readFloat(getMountAddress(EE_RA));
      newTargetDec = (double)XEEPROM.readFloat(getMountAddress(EE_DEC));
      double newTargetHA = haRange(rtk.LST() * 15.0 - newTargetRA);
      mount.mP->syncEqu(newTargetHA, newTargetDec, targetPierSide, localSite.cosLat(), localSite.sinLat());
      strcpy(reply, "N/A#");
      break;
    }
    case 'A':
    {
      // Guess pier side from Azimuth
      if (newTargetAzm > 0 && newTargetAzm < 180)
        targetPierSide = PIER_WEST;
      else
        targetPierSide = PIER_EAST;

      mount.mP->syncAzAlt(newTargetAzm, newTargetAlt, targetPierSide);
      strcpy(reply, "N/A#");
    }
    break;
    }
  }
}

//----------------------------------------------------------------------------------
//   D - Distance Bars
//  :D#    returns an "\0x7f#" if the mount is moving, otherwise returns "#".
void Command_D()
{
  if (command[1] != 0)
  {
    return;
  }
    
  if (isSlewing())
  {
    reply[0] = (char)127;
    reply[1] = 0;
  }
  else
  {
    reply[0] = 0;
  }
  strcat(reply, "#");
}

//----------------------------------------------------------------------------------
//  h - Home Position Commands
void Command_h()
{
  switch (command[1])
  {
  case 'F':
    //  :hF#   Reset telescope at the home position.  This position is required for a Cold Start.
    //         Point to the celestial pole with the counterweight pointing downwards (CWD position).
    //         Return: Nothing
    syncAtHome();
    break;
  case 'C':
    //  :hC#   Moves telescope to the home position
    //          Return: 0 on failure
    //                  1 on success
    if (!goHome())
      replyShortFalse();
    else
      replyShortTrue();
    break;
  case 'B':
    //  :hB#   Set the home position
    //          Return: 0 on failure
    //                  1 on success
    if (!setHome()) replyLongUnknown();
    else replyShortTrue();
    break;
  case 'b':
    //  :hb#   Reset the home position
    //          Return: 0 on failure
    //                  1 on success
    homeSaved = false;
    XEEPROM.write(getMountAddress(EE_homeSaved), false);
    initHome();
    replyShortTrue();
    break;
  case 'O':
    // : hO#   Reset telescope at the Park position if Park position is stored.
    //          Return: 0 on failure
    //                  1 on success
    if (!syncAtPark())
      replyShortFalse();
    else
      replyShortTrue();
    break;
  case 'P':
    // : hP#   Goto the Park Position
    //          Return: 0 on failure
    //                  1 on success
    if (park())
      replyShortFalse();
    else
      replyShortTrue();
    break;
  case 'Q':
    //  :hQ#   Set the park position
    //          Return: 0 on failure
    //                  1 on success
    if (!setPark())
      replyShortFalse();
    else
      replyShortTrue();
    break;
  case 'R':
    //  :hR#   Restore parked telescope to operation
    //          Return: 0 on failure
    //                  1 on success
    unpark();
    replyShortTrue();
    break;
  case 'S':
    //  :hS#   return true if the park position is saved
    //          Return: 0 or 1
    parkSaved ? replyShortTrue() : replyShortFalse();
    break;


  default:
    replyLongUnknown();
    break;
  }
}

//----------------------------------------------------------------------------------
//   Q - Halt Movement Commands
void Command_Q()
{
  switch (command[1])
  {
  case 0:
    //  :Q#    Halt all slews, stops goto
    //         has no effect when tracking, but stops spiral
    //         Returns: Nothing
    if (isTracking())
    {
      if (getEvent(EV_SPIRAL))
      {
        stopSpiral();
      }
    }
    else
      stopMoving();
    break;
  case 'e':
  case 'w':
    //  :Qe# & Qw#   Halt east/westward Slews
    //         Returns: Nothing
        StopAxis1();
    break;
  case 'n':
  case 's':
    //  :Qn# & Qs#   Halt north/southward Slews
    //         Returns: Nothing
        StopAxis2();
    break;
  default:
    replyLongUnknown();
    break;
  }
}

//----------------------------------------------------------------------------------
//   R - Slew Rate Commands
void Command_R()
{
  //  :RG#   Set Slew rate to Guiding Rate (slowest) user defined
  //  :RC#   Set Slew rate to Centering rate (2nd slowest) 4X
  //  :RM#   Set Slew rate to Find Rate (2nd Fastest) 32X
  //  :RS#   Set Slew rate to max (fastest) ?X (MaxRate)
  //  :Rn#   Set Slew rate to n, where n=0..9
  //         Returns: Nothing

  int i = 5;
  switch (command[1])
  {
  case 'G':
    i = RG;
    break;
  case 'C':
    i = RC;
    break;
  case 'M':
    i = RM;
    break;
  case 'S':
    i = RS;
    break;
  case '0':
  case '1':
  case '2':
  case '3':
  case '4':
    i = command[1] - '0';
    break;
  default:
    replyLongUnknown();
    return;
  }
  if (!isSlewing() && GuidingState == GuidingOFF)
  {
    enableGuideRate(i);
    XEEPROM.write(getMountAddress(EE_DefaultRate), i);    // also save it to EEPROM
  }
}



//   U - Precision Toggle
//  :U#    Toggle between low/hi precision positions
//         Low -  RA/Dec/etc. displays and accepts HH:MM.M sDD*MM
//         High - RA/Dec/etc. displays and accepts HH:MM:SS sDD*MM:SS
//         Returns Nothing
void Command_U()
{
  if (command[1] == 0)
  {
    highPrecision = !highPrecision;
    replyNothing();
  }
  else
    replyLongUnknown();
}

//   W - Site Select/Site get
//  :Wn#
//         Sets current site to n, 0..3 or queries site with ?
//         Returns: Nothing or current site ?#
void Command_W()
{
  switch (command[1])
  {
  case '0':
  case '1':
  case '2':
  case '3':
  {
    currentSite = command[1] - '0';
    XEEPROM.write(EE_currentSite, currentSite);
    localSite.ReadSiteDefinition(currentSite);
    rtk.resetLongitude(*localSite.longitude());
    initHome();
    mount.mP->initModel(true);
    syncAtHome();
    replyNothing();
    break;
  }
  case '?':
    sprintf(reply, "%d#", currentSite);
    break;
  default:
    replyNothing();
    break;
  }
}

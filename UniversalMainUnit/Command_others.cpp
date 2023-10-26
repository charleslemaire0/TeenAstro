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
    // telescope should be set in the polar home for a starting point
    mount.mP->initTransformation(true);
    syncAtHome();
    // enable the stepper drivers
    vTaskDelay(10);
    startTracking();
    lastSetTrackingEnable = millis();
    replyShortTrue();
    break;
  case '2':
  {
    Axes axes;
    HorCoords hc;
    Steps steps;
    double newTargetHA = haRange(rtk.LST() * 15.0 - newTargetRA);
    EquToHor(newTargetHA, newTargetDec, doesRefraction.forGoto, &hc.az, &hc.alt, localSite.cosLat(), localSite.sinLat());

    if (mount.mP->alignment.getRefs() == 0)
    {
      mount.mP->syncAzAlt(hc.az, hc.alt, mount.mP->GetPierSide());
    }

    steps.steps1 = motorA1.getCurrentPos();
    steps.steps2 = motorA2.getCurrentPos();
    mount.mP->stepsToAxes(&steps, &axes);

    mount.mP->alignment.addReferenceDeg(hc.az, hc.alt, axes.axis1, axes.axis2);
    if (mount.mP->alignment.getRefs() == 2)
    {
      mount.mP->alignment.calculateThirdReference();
      if (mount.mP->alignment.isReady())
      {
        mount.mP->hasStarAlignment(true);

//        motorA1.setTargetPos(motorA1.getCurrentPos());
//        motorA2.setTargetPos(motorA2.getCurrentPos());
      }
    }
    replyShortTrue();
    break;
  }
  case 'C':
  case 'A':
    mount.mP->initTransformation(true);
    syncAtHome();
    autoAlignmentBySync = command[1] == 'A';
    replyShortTrue();
    break;
  case 'W':
    saveAlignModel();
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
    replyShortFalse();
    return;
  }
    
  if (isSlewing())
  {
    reply[0] = (char)127;
    reply[1] = 0;
  }
  else
  {
    replyNothing();
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
    //         Returns: Nothing
    stopSpiral();
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

//----------------------------------------------------------------------------------
//   T - Tracking Commands
//  :T+#   Master sidereal clock faster by 0.1 Hertz (I use a fifth of the LX200 standard, stored in XEEPROM) Returns: Nothing
//  :T-#   Master sidereal clock slower by 0.1 Hertz (stored in XEEPROM) Returns: Nothing
//  :TS#   Track rate solar Returns: Nothing
//  :TL#   Track rate lunar Returns: Nothing
//  :TQ#   Track rate sidereal Returns: Nothing
//  :TT#   Track rate target Returns: Nothing
//  :TR#   Master sidereal clock reset (to calculated sidereal rate, stored in EEPROM) Returns: Nothing
//  :Te#   Tracking enable  (replies 0/1)
//  :Td#   Tracking disable (replies 0/1)

//  :T0#   Track compensation disable (replies 0/1)
//  :T1#   Track compensation only in RA (replies 0/1)
//  :T2#   Track compensation BOTH (replies 0/1)
void Command_T()
{

  switch (command[1])

  {
  case '+':
    siderealClockSpeed -= HzCf * (0.02);
    replyNothing();
    break;
  case '-':
    siderealClockSpeed += HzCf * (0.02);
    replyNothing();
    break;
  case 'S':
    // solar tracking rate 60Hz
    mount.mP->setTrackingSpeed(TrackingSolar);
    siderealMode = SIDM_SUN;
     replyNothing();
    break;
  case 'L':
    // lunar tracking rate 57.9Hz
    mount.mP->setTrackingSpeed(TrackingLunar);
    siderealMode = SIDM_MOON;
    replyNothing();
    break;
  case 'Q':
    // sidereal tracking rate
    mount.mP->setTrackingSpeed(TrackingStar);
    siderealMode = SIDM_STAR;
    replyNothing();
    break;
  case 'R':
    // reset master sidereal clock interval
    siderealClockSpeed = mastersiderealClockSpeed;
    mount.mP->setTrackingSpeed(TrackingStar);
    siderealMode = SIDM_STAR;
    replyNothing();
    break;
  case 'T':
    //set Target tracking rate
//    SetTrackingRate(1.0 - (double)storedTrackingRateRA / 10000.0, (double)storedTrackingRateDEC / 10000.0);
//    siderealMode = SIDM_TARGET;
    replyNothing();
    break;
  case 'e':
    if (parkStatus() == PRK_UNPARKED)
    {
//      lastSetTrackingEnable = millis();
//     atHome = false;
      startTracking();
//      computeTrackingRate(true);
      replyShortTrue();
    }
    else
      replyLongUnknown();
    break;
  case 'd':
    if (parkStatus() == PRK_UNPARKED)
    {
      stopTracking();
      replyShortTrue();
    }
    else
      replyLongUnknown();
    break;
#if 0
  case '0':
    // turn compensation off
    tc = TC_NONE;
    computeTrackingRate(true);
    XEEPROM.write(getMountAddress(EE_TC_Axis), 0);
    replyShortTrue();
    break;
  case '1':
    // turn compensation RA only
    tc = TC_RA;
    computeTrackingRate(true);
    XEEPROM.write(getMountAddress(EE_TC_Axis), 0);
    replyShortTrue();
    break;
  case '2':
    // turn compensation BOTH
    tc = TC_BOTH;
    computeTrackingRate(true);
    XEEPROM.write(getMountAddress(EE_TC_Axis), 2);
    replyShortTrue();
    break;
#endif
  default:
    replyLongUnknown();
    break;
  }

  // Only burn the new rate if changing the sidereal interval
  if (command[1] == '+' || command[1] == '-' || command[1] == 'R')
  {
    XEEPROM.writeLong(getMountAddress(EE_siderealClockSpeed), siderealClockSpeed);
    updateSidereal();
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
    mount.mP->initTransformation(true);
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

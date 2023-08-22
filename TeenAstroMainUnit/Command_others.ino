#include "Command.h"

void Command_dollar()
{
  switch (command[1])
  {
  case '$':
    for (int i = 0; i < XEEPROM.length(); i++)
    {
      XEEPROM.write(i, 0);
    }
  case '!':
    reboot_unit = true;
    replyShortTrue();
    break;
  case 'X':
    initmotor(true);
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
//  :A3#
//  :AC#
//  :AW#
//  :AA#  Resets alignment as AC# AND activates alignment on next 3 syncs!  (<-> sync modded accordingly)

void Command_A()
{
  switch (command[1])
  {
  case '0':
    // telescope should be set in the polar home for a starting point
    initTransformation(true);
    syncAtHome();
    // enable the stepper drivers
    enable_Axis(true);
    delay(10);
    // start tracking
    sideralTracking = true;
    lastSetTrakingEnable = millis();
    replyShortTrue();
    break;
  case '2':
  {
    bool ok = true;
    double newTargetHA = haRange(rtk.LST() * 15.0 - newTargetRA);
    double Lat = *localSite.latitude();
    Coord_EQ EQ_T(0, newTargetDec * DEG_TO_RAD, newTargetHA * DEG_TO_RAD);
    Coord_HO HO_T = EQ_T.To_Coord_HO( Lat * DEG_TO_RAD, RefrOptForGoto());
    if (alignment.getRefs() == 0)
    {
      syncAzAlt(&HO_T, GetPierSide());
    }
    Coord_IN IN_T = getInstr();
    alignment.addReference(HO_T.Az(), HO_T.Alt(), IN_T.Axis1(), IN_T.Axis2());
    if (alignment.isReady())
    {
      cli();
      staA1.target = staA1.pos;
      staA2.target = staA2.pos;
      sei();
      hasStarAlignment = true;
    }
    replyShortTrue();
    break;
  }
  case 'C':
  case 'A':
    initTransformation(true);
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
    break;
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
//  :CA#   Synchonize the telescope with the current Target Azimuth and Altitude coordinates
//         Returns: "N/A#" 
//  :CM#   Synchonize the telescope with the current database object (as above)
//         Returns: "N/A#"  
//  :CS#   Synchonize the telescope with the current Target right ascension and declination coordinates
//         Returns: "N/A#" 
//  :CU#   Synchonize the telescope with the User defined object
//         Returns: "N/A#"

void Command_C()
{
  if ((parkStatus == PRK_UNPARKED) &&
    !movingTo &&
    (command[1] == 'A' || command[1] == 'M' || command[1] == 'S' || command[1] == 'U'))
  {
    PierSide targetPierSide = GetPierSide();
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
      if (autoAlignmentBySync) {
        double newTargetHA = haRange(rtk.LST() * 15.0 - newTargetRA);
        Coord_EQ EQ_T(0, newTargetDec * DEG_TO_RAD, newTargetHA * DEG_TO_RAD);
        Coord_HO HO_T = EQ_T.To_Coord_HO(*localSite.latitude() * DEG_TO_RAD, RefrOptForGoto());

        if (alignment.getRefs() == 0)
        {
          syncAzAlt(&HO_T, GetPierSide());
        }
        Coord_IN IN_T = getInstr();
        alignment.addReference(HO_T.Az(), HO_T.Alt(), IN_T.Axis1(), IN_T.Axis2());
        if (alignment.isReady())
        {
          cli();
          staA1.target = staA1.pos;
          staA2.target = staA2.pos;
          sei();
          hasStarAlignment = true;
          autoAlignmentBySync = false;
        }
      }
      else
      {
        double newTargetHA = haRange(rtk.LST() * 15.0 - newTargetRA);
        Coord_EQ EQ_T(0, newTargetDec * DEG_TO_RAD, newTargetHA * DEG_TO_RAD);
        syncEqu(&EQ_T, targetPierSide, *localSite.latitude() * DEG_TO_RAD);
        syncEwithT();
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
      Coord_EQ EQ_T(0, newTargetDec * DEG_TO_RAD, newTargetHA * DEG_TO_RAD);
      syncEqu(&EQ_T, targetPierSide, *localSite.latitude() * DEG_TO_RAD);
      syncEwithT();
      strcpy(reply, "N/A#");
      break;
    }
    case 'A':
    {
      Coord_HO HO_T(0, newTargetAlt * DEG_TO_RAD, newTargetAzm * DEG_TO_RAD, true);
      syncAzAlt(&HO_T, targetPierSide);
      syncEwithT();
      strcpy(reply, "N/A#");
    }
    break;
    }
  }
}
//----------------------------------------------------------------------------------
//    E - encoder commands
//  :EAS#   Align Encoder Start Returns 1# or 0#
//  :EAE#   Align Encoder End Returns 1# or 0#
//  :EAQ#   Align Encoder Quit Returns 1# or 0#
//  :ECT#   Synchonize the telescope with the Encoders Returns 1# or 0#
//  :ECE#   Synchonize the Encoders with the telescope Returns 1# or 0#
//  :ECS#   Synchronise at the end of a pushto to Target Returns 1# or 0#
//  :ED#   Distance to target axis Returns long#
//  :EMS#   Set pushTo sidereal Target
//  :EMU#   Set pushTo sidereal User defined Target
//  :EMA#   Set pushTo altaz Target
//  :EMQ#   Set pushTo altaz Target

void Command_E()
{
  switch (command[1])
  {
  case 'A':
  {
    switch (command[2])
    {
    case 'S':
    {
      //  :EAS#  Align Encoder Start
      double A1, A2, A3;
      EncodeSyncMode = ES_OFF;
      syncEwithT();
      getInstrDeg(&A1, &A2, &A3);
      encoderA1.setRef(A1);
      encoderA2.setRef(A2);
      replyLongTrue();
    }
    break;
    case 'E':
    {
      //  :EAE#  Align Encoder End
      double A1, A2, A3;
      getInstrDeg(&A1, &A2, &A3);
      bool ok = encoderA1.calibrate(A1);
      ok &= encoderA1.calibrate(A2);
      ok ? replyLongTrue() : replyLongFalse();
    }
    break;
    case 'Q':
    {
      //  :EAQ#  Align Encoder Quit
      encoderA1.delRef();
      encoderA2.delRef();
      replyLongTrue();
    }
    break;
    default:
      replyNothing();
      break;
    }
  }
  break;
  case 'C':
  {
    switch (command[2])
    {
    case 'T':
    {
      //  :ECT#   Synchonize the telescope with the Encoders
      syncTwithE();
      replyLongTrue();
    }
    break;
    case 'E':
    {
      //  :ECE#   Synchonize the Encoders with the telescope
      syncEwithT();
      replyLongTrue();
    }
    break;
    case 'S':
    {
      //  :ECS#   Synchonize the Telescope and Encoder to Target
      switch (PushtoStatus)
      {
      case PT_RADEC:
      {
        double newTargetHA = haRange(rtk.LST() * 15.0 - newTargetRA);
        Coord_EQ EQ_T(0, newTargetDec * DEG_TO_RAD, newTargetHA * DEG_TO_RAD);
        syncEqu(&EQ_T, GetPierSide(), *localSite.latitude() * DEG_TO_RAD);
        syncEwithT();
        replyLongTrue();
      }
      break;
      case PT_ALTAZ:
      {
        Coord_HO HO_T(0, newTargetAlt * DEG_TO_RAD, newTargetAzm * DEG_TO_RAD, true);
        syncAzAlt(&HO_T, GetPierSide());
        syncEwithT();
        replyLongTrue();
      }
      break;
      default:
        replyLongTrue();
        break;
      }
    }
    break;
    default:
      replyLongUnknow();
      break;
    }
  }
  break;
  case 'D':
  {
    float delta1;
    float delta2;
    int e = 0;
    switch (PushtoStatus)
    {
    case PT_RADEC:
    {
      double newTargetHA = haRange(rtk.LST() * 15.0 - newTargetRA);
      Coord_EQ EQ_T(0, newTargetDec * DEG_TO_RAD, newTargetHA * DEG_TO_RAD);
      e = PushToEqu(EQ_T, GetPierSide(), *localSite.latitude() * DEG_TO_RAD, &delta1, &delta2);
      sprintf(reply, "%d,%+06d,%+06d#", e, (int)(60 * delta1), (int)(60 * delta2));
    }
    break;
    case PT_ALTAZ:
    {
      Coord_HO HO_T(0, newTargetAlt * DEG_TO_RAD, newTargetAzm * DEG_TO_RAD, true);
      e = PushToHor(HO_T, GetPierSide(), &delta1, &delta2);
      sprintf(reply, "%d,%+06d,%+06d#", e, (int)(60 * delta1), (int)(60 * delta2));
    }
    break;
    default:
      sprintf(reply, "%d,%+06d,%+06d#", 0, 0, 0);
      break;
    }
  }
  break;
  case 'M':
  {
    float delta1;
    float delta2;
    int e = 0;
    if (command[2] == 'S')
    {
      double newTargetHA = haRange(rtk.LST() * 15.0 - newTargetRA);
      Coord_EQ EQ_T(0, newTargetDec* DEG_TO_RAD, newTargetHA* DEG_TO_RAD);
      e = PushToEqu(EQ_T, GetPierSide(), *localSite.latitude() * DEG_TO_RAD, &delta1, &delta2);
      sprintf(reply, "%d", e);
      PushtoStatus = PT_RADEC;
    }
    else if (command[2] == 'A')
    {
      Coord_HO HO_T(0, newTargetAlt* DEG_TO_RAD, newTargetAzm* DEG_TO_RAD, true);
      e = PushToHor(HO_T, GetPierSide(), &delta1, &delta2);
      sprintf(reply, "%d", e);
      PushtoStatus = PT_ALTAZ;
    }
    else if (command[2] == 'U')
    {

      newTargetRA = (double)XEEPROM.readFloat(getMountAddress(EE_RA));
      newTargetDec = (double)XEEPROM.readFloat(getMountAddress(EE_DEC));
      double newTargetHA = haRange(rtk.LST() * 15.0 - newTargetRA);
      Coord_EQ EQ_T(0, newTargetDec * DEG_TO_RAD, newTargetHA * DEG_TO_RAD);
      e = PushToEqu(EQ_T, GetPierSide(), *localSite.latitude() * DEG_TO_RAD, &delta1, &delta2);
      sprintf(reply, "%d", e);
      PushtoStatus = PT_RADEC;
    }
    else if (command[2] == 'Q')
    {
      PushtoStatus = PT_OFF;
      replyShortTrue();
    }
    else
    {
      replyNothing();
    }
  }
  break;
  default:
    replyNothing();
    break;
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

  if (movingTo)
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
    if (!setHome()) replyShortFalse();
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
    replyNothing();
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
    doSpiral = false;
    if ((parkStatus == PRK_UNPARKED) || (parkStatus == PRK_PARKING))
    {
      if (movingTo)
      {
        abortSlew = true;
      }
      else
      {
        StopAxis1();
        StopAxis2();
      }
    }
    break;
  case 'e':
  case 'w':
    //  :Qe# & Qw#   Halt east/westward Slews
    //         Returns: Nothing
  {
    if ((parkStatus == PRK_UNPARKED) && !movingTo)
    {
      StopAxis1();
    }
  }
  break;
  case 'n':
  case 's':
    //  :Qn# & Qs#   Halt north/southward Slews
    //         Returns: Nothing
  {
    if ((parkStatus == PRK_UNPARKED) && !movingTo)
    {
      StopAxis2();
    }
  }
  break;
  default:
    replyNothing();
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
    replyNothing();
    return;
  }
  if (!movingTo && GuidingState == GuidingOFF)
  {
    recenterGuideRate = i;
    enableGuideRate(recenterGuideRate);
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
    SetTrackingRate(TrackingSolar, 0);
    sideralMode = SIDM_SUN;
    replyNothing();
    break;
  case 'L':
    // lunar tracking rate 57.9Hz
    SetTrackingRate(TrackingLunar, 0);
    sideralMode = SIDM_MOON;
    replyNothing();
    break;
  case 'Q':
    // sidereal tracking rate
    SetTrackingRate(TrackingStar, 0);
    sideralMode = SIDM_STAR;
    replyNothing();
    break;
  case 'R':
    // reset master sidereal clock interval
    siderealClockSpeed = mastersiderealClockSpeed;
    SetTrackingRate(TrackingStar, 0);
    sideralMode = SIDM_STAR;
    replyNothing();
    break;
  case 'T':
    //set Target tracking rate
    SetTrackingRate(1.0 - (double)storedTrakingRateRA / 10000.0, (double)storedTrakingRateDEC / 10000.0);
    sideralMode = SIDM_TARGET;
    replyNothing();
    break;
  case 'e':
    if (parkStatus == PRK_UNPARKED)
    {
      lastSetTrakingEnable = millis();
      atHome = false;
      sideralTracking = true;
      computeTrackingRate(true);
      replyShortTrue();
    }
    else
      replyShortFalse();
    break;
  case 'd':
    if (parkStatus == PRK_UNPARKED)
    {
      sideralTracking = false;
      replyShortTrue();
    }
    else
      replyShortFalse();
    break;
  case '0':
    // turn compensation off
    tc = TC_NONE;
    computeTrackingRate(true);
    XEEPROM.update(getMountAddress(EE_TC_Axis), 0);
    replyShortTrue();
    break;
  case '1':
    // turn compensation RA only
    tc = TC_RA;
    computeTrackingRate(true);
    XEEPROM.update(getMountAddress(EE_TC_Axis), 0);
    replyShortTrue();
    break;
  case '2':
    // turn compensation BOTH
    tc = TC_BOTH;
    computeTrackingRate(true);
    XEEPROM.update(getMountAddress(EE_TC_Axis), 2);
    replyShortTrue();
    break;
  default:
    replyNothing();
    break;
  }

  // Only burn the new rate if changing the sidereal interval
  if (command[1] == '+' || command[1] == '-' || command[1] == 'R')
  {
    XEEPROM.writeLong(getMountAddress(EE_siderealClockSpeed), siderealClockSpeed * 16);
    updateSideral();
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
    replyNothing();
}

//   W - Site Select/Site get
//  :Wn#
//         Sets current site to n, 0..2 or queries site with ?
//         Returns: Nothing or current site ?#
void Command_W()
{
  switch (command[1])
  {
  case '0':
  case '1':
  case '2':
  {
    uint8_t currentSite = command[1] - '0';
    XEEPROM.write(getMountAddress(EE_currentSite), currentSite);
    localSite.ReadSiteDefinition(currentSite);
    rtk.resetLongitude(*localSite.longitude());
    initCelestialPole();
    initHome();
    initTransformation(true);
    syncAtHome();
    replyNothing();
    break;
  }
  case '?':
    sprintf(reply, "%d#", localSite.siteIndex());
    break;
  default:
    replyNothing();
    break;
  }
}

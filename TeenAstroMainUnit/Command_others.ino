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
    reboot();
    break;
  case 'X':
    initmotor(true);
    replyOk();
    break;
  default:
    replyFailed();
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
    replyOk();
    break;
  case '2':
  {
    double newTargetHA = haRange(rtk.LST() * 15.0 - newTargetRA);
    double Azm, Alt;
    EquToHor(newTargetHA, newTargetDec, doesRefraction.forGoto, &Azm, &Alt, localSite.cosLat(), localSite.sinLat());

    if (alignment.getRefs() == 0)
    {
      syncAzAlt(Azm, Alt, GetPierSide());
    }

    cli();
    double Axis1 = staA1.pos / geoA1.stepsPerDegree;
    double Axis2 = staA2.pos / geoA2.stepsPerDegree;
    sei();

    alignment.addReferenceDeg(Azm, Alt, Axis1, Axis2);
    if (alignment.getRefs() == 2)
    {
      alignment.calculateThirdReference();
      if (alignment.isReady())
      {
        hasStarAlignment = true;
        cli();
        staA1.target = staA1.pos;
        staA2.target = staA2.pos;
        sei();
      }
    }
    replyOk();
    break;
  }
  case '3':
  {
    double newTargetHA = haRange(rtk.LST() * 15.0 - newTargetRA);
    double Azm, Alt;
    EquToHor(newTargetHA, newTargetDec, doesRefraction.forGoto, &Azm, &Alt, localSite.cosLat(), localSite.sinLat());
    if (alignment.getRefs() == 0)
    {
      syncAzAlt(Azm, Alt, GetPierSide());
    }
    cli();
    double Axis1 = staA1.pos / geoA1.stepsPerDegree;
    double Axis2 = staA2.pos / geoA2.stepsPerDegree;
    sei();
    alignment.addReferenceDeg(Azm, Alt, Axis1, Axis2);
    if (alignment.isReady())
    {
      hasStarAlignment = true;
      cli();
      staA1.target = staA1.pos;
      staA2.target = staA2.pos;
      sei();
    }
    replyOk();
    break;
  }
  case 'C':
  case 'A':
    initTransformation(true);
    syncAtHome();
    autoAlignmentBySync = command[1] == 'A';
    replyOk();
    break;
  case 'W':
    saveAlignModel();
    replyOk();
    break;
  default:
    replyFailed();
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
      double newTargetHA;
      if (autoAlignmentBySync) {
        newTargetHA = haRange(rtk.LST() * 15.0 - newTargetRA);
        double Azm, Alt;
        EquToHor(newTargetHA, newTargetDec, doesRefraction.forGoto, &Azm, &Alt, localSite.cosLat(), localSite.sinLat());
        if (alignment.getRefs() == 0)
        {
          syncAzAlt(Azm, Alt, GetPierSide());
        }
        cli();
        double Axis1 = staA1.pos / geoA1.stepsPerDegree;
        double Axis2 = staA2.pos / geoA2.stepsPerDegree;
        sei();
        alignment.addReferenceDeg(Azm, Alt, Axis1, Axis2);
        if (alignment.isReady())
        {
          hasStarAlignment = true;
          cli();
          staA1.target = staA1.pos;
          staA2.target = staA2.pos;
          sei();
          autoAlignmentBySync = false;
        }
      }
      else
      {
        newTargetHA = haRange(rtk.LST() * 15.0 - newTargetRA);
        syncEqu(newTargetHA, newTargetDec, targetPierSide, localSite.cosLat(), localSite.sinLat());
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
      syncEqu(newTargetHA, newTargetDec, targetPierSide, localSite.cosLat(), localSite.sinLat());
      syncEwithT();
      strcpy(reply, "N/A#");
      break;
    }
    case 'A':
      syncAzAlt(newTargetAzm, newTargetAlt, targetPierSide);
      syncEwithT();
      strcpy(reply, "N/A#");
      break;
    }
  }
}
//----------------------------------------------------------------------------------
//    E - encoder commands
//    all commands Returns 1# or 0#
//  :EAS#   Align Encoder Start
//  :EAE#   Align Encoder End
//  :EAQ#   Align Encoder Quit
//  :ECT#   Synchonize the telescope with the Encoders
//  :ECE#   Synchonize the Encoders with the telescope
//  :ECS#   Synchronise at the end of a pushto to Target
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
#if HasEncoder
      //  :EAS#  Align Encoder Start
      double A1, A2;
      EncodeSyncMode = ES_OFF;
      syncEwithT();
      getInstrDeg(&A1, &A2);
      encoderA1.setRef(A1);
      encoderA2.setRef(A2);
      replyOk();
#endif // HASEncoder
    }
    break;
    case 'E':
    {
#if HasEncoder
      //  :EAE#  Align Encoder End
      double A1, A2;
      getInstrDeg(&A1, &A2);
      bool ok = encoderA1.calibrate(A1);
      ok &= encoderA1.calibrate(A2);
      ok ? replyOk() : replyFailed();
#endif // HASEncoder
    }
    break;
    case 'Q':
    {
#if HasEncoder
      //  :EAQ#  Align Encoder Quit
      encoderA1.delRef();
      encoderA2.delRef();
      replyOk();
#endif // HASEncoder
    }
    break;
    default:
      replyFailed();
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
      replyOk();
    }
    break;
    case 'E':
    {
      //  :ECE#   Synchonize the Encoders with the telescope
      syncEwithT();
      replyOk();
    }
    break;
    case 'S':
    {
      //  :ECE#   Synchonize the Telescope and Encoder to Target
      switch (PushtoStatus)
      {
      case PT_RADEC:
      {
        double newTargetHA = haRange(rtk.LST() * 15.0 - newTargetRA);
        syncEqu(newTargetHA, newTargetDec, GetPierSide(), localSite.cosLat(), localSite.sinLat());
        syncEwithT();
        replyOk();
      }
      break;
      case PT_ALTAZ:
      {
        syncAzAlt(newTargetAzm, newTargetAlt, GetPierSide());
        syncEwithT();
        replyOk();
      }
      break;
      default:
        replyFailed();
        break;
      }
    }
    break;
    default:
      replyFailed();
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
      e = PushToEqu(newTargetRA, newTargetDec, GetPierSide(), localSite.cosLat(), localSite.sinLat(), &delta1, &delta2);
      sprintf(reply, "%d,%+06d,%+06d#", e, (int)(60 * delta1), (int)(60 * delta2));
      break;
    case PT_ALTAZ:
      e = PushToHor(&newTargetAzm, &newTargetAlt, GetPierSide(), &delta1, &delta2);
      sprintf(reply, "%d,%+06d,%+06d#", e, (int)(60 * delta1), (int)(60 * delta2));
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
      e = PushToEqu(newTargetRA, newTargetDec, GetPierSide(), localSite.cosLat(), localSite.sinLat(), &delta1, &delta2);
      sprintf(reply, "%d", e);
      PushtoStatus = PT_RADEC;
    }
    else if (command[2] == 'A')
    {
      e = PushToHor(&newTargetAzm, &newTargetAlt, GetPierSide(), &delta1, &delta2);
      sprintf(reply, "%d", e);
      PushtoStatus = PT_ALTAZ;
    }
    else if (command[2] == 'U')
    {
      newTargetRA = (double)XEEPROM.readFloat(getMountAddress(EE_RA));
      newTargetDec = (double)XEEPROM.readFloat(getMountAddress(EE_DEC));
      e = PushToEqu(newTargetRA, newTargetDec, GetPierSide(), localSite.cosLat(), localSite.sinLat(), &delta1, &delta2);
      sprintf(reply, "%d", e);
      PushtoStatus = PT_RADEC;
    }
    else if (command[2] == 'Q')
    {
      PushtoStatus = PT_OFF;
      replyOk();
    }
    else
    {
      replyFailed();
    }

  }
  break;
  default:
    replyFailed();
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
    replyFailed();
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
      replyFailed();
    else
      replyOk();
    break;
  case 'B':
    //  :hB#   Set the home position
    //          Return: 0 on failure
    //                  1 on success
    if (!setHome()) replyFailed();
    else replyOk();
    break;
  case 'b':
    //  :hb#   Reset the home position
    //          Return: 0 on failure
    //                  1 on success
    homeSaved = false;
    XEEPROM.write(getMountAddress(EE_homeSaved), false);
    initHome();
    replyOk();
    break;
  case 'O':
    // : hO#   Reset telescope at the Park position if Park position is stored.
    //          Return: 0 on failure
    //                  1 on success
    if (!syncAtPark())
      replyFailed();
    else
      replyOk();
    break;
  case 'P':
    // : hP#   Goto the Park Position
    //          Return: 0 on failure
    //                  1 on success
    if (park())
      replyFailed();
    else
      replyOk();
    break;
  case 'Q':
    //  :hQ#   Set the park position
    //          Return: 0 on failure
    //                  1 on success
    if (!setPark())
      replyFailed();
    else
      replyOk();
    break;
  case 'R':
    //  :hR#   Restore parked telescope to operation
    //          Return: 0 on failure
    //                  1 on success
    unpark();
    replyOk();
    break;


  default:
    replyFailed();
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
      else if (GuidingState == GuidingRecenter || GuidingState == GuidingST4 || GuidingState == GuidingPulse)
      {
        StopAxis1();
        StopAxis2();
      }
      else
      {
        guideA1.brake();
        guideA2.brake();
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
    replyFailed();
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
    replyFailed();
    return;
  }
  if (!movingTo && GuidingState == GuidingOFF)
  {
    enableGuideRate(i);
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
      replyOk();
    }
    else
      replyFailed();
    break;
  case 'd':
    if (parkStatus == PRK_UNPARKED)
    {
      sideralTracking = false;
      replyOk();
    }
    else
      replyFailed();
    break;
  case '0':
    // turn compensation off
    tc = TC_NONE;
    computeTrackingRate(true);
    XEEPROM.update(getMountAddress(EE_TC_Axis), 0);
    replyOk();
    break;
  case '1':
    // turn compensation RA only
    tc = TC_RA;
    computeTrackingRate(true);
    XEEPROM.update(getMountAddress(EE_TC_Axis), 0);
    replyOk();
    break;
  case '2':
    // turn compensation BOTH
    tc = TC_BOTH;
    computeTrackingRate(true);
    XEEPROM.update(getMountAddress(EE_TC_Axis), 2);
    replyOk();
    break;
  default:
    replyFailed();
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
    replyFailed();
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
    replyFailed();
    break;
  }
}

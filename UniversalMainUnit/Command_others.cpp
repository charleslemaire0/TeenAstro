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
  case '!':
    Serial.end();
    Serial1.end();
    Serial2.end();
//    Serial3.end();
    delay(1000);
#ifdef ARDUINO_TEENSY40 // In fact this code is suitable for Teensy 3.2 also
#define CPU_RESTART_ADDR (uint32_t *)0xE000ED0C
#define CPU_RESTART_VAL 0x5FA0004
#define CPU_RESTART (*CPU_RESTART_ADDR = CPU_RESTART_VAL);
    CPU_RESTART;
#endif
    break;
  case 'X':
    initMotors(true);
    strcpy(reply, "1");
    break;
  default:
    strcpy(reply, "0");
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
    delay(10);
    // start tracking
    mount.mP->startTracking();
//    siderealTracking = true;
    lastSetTrackingEnable = millis();
    strcpy(reply, "1");
    break;
  case '2':
  {
    double newTargetHA = haRange(rtk.LST() * 15.0 - newTargetRA);
    double Azm, Alt;
    EquToHor(newTargetHA, newTargetDec, doesRefraction.forGoto, &Azm, &Alt, localSite.cosLat(), localSite.sinLat());

    if (alignment.getRefs() == 0)
    {
      mount.mP->syncAzAlt(Azm, Alt, mount.mP->GetPierSide());
    }

    double Axis1 = motorA1.getCurrentPos() / geoA1.stepsPerDegree;
    double Axis2 = motorA2.getCurrentPos() / geoA2.stepsPerDegree;

    alignment.addReferenceDeg(Azm, Alt, Axis1, Axis2);
    if (alignment.getRefs() == 2)
    {
      alignment.calculateThirdReference();
      if (alignment.isReady())
      {
        hasStarAlignment = true;

        motorA1.setTargetPos(motorA1.getCurrentPos());
        motorA2.setTargetPos(motorA2.getCurrentPos());
      }
    }
    strcpy(reply, "1");
    break;
  }
  case '3':
  {
    double newTargetHA = haRange(rtk.LST() * 15.0 - newTargetRA);
    double Azm, Alt;
    EquToHor(newTargetHA, newTargetDec, doesRefraction.forGoto, &Azm, &Alt, localSite.cosLat(), localSite.sinLat());
    if (alignment.getRefs() == 0)
    {
      mount.mP->syncAzAlt(Azm, Alt, mount.mP->GetPierSide());
    }

    double Axis1 = motorA1.getCurrentPos() / geoA1.stepsPerDegree;
    double Axis2 = motorA2.getCurrentPos() / geoA2.stepsPerDegree;

    alignment.addReferenceDeg(Azm, Alt, Axis1, Axis2);
    if (alignment.isReady())
    {
      hasStarAlignment = true;
      motorA1.setTargetPos(motorA1.getCurrentPos());
      motorA2.setTargetPos(motorA2.getCurrentPos());
    }
    strcpy(reply, "1");
    break;
  }
  case 'C':
  case 'A':
    initTransformation(true);
    syncAtHome();
    autoAlignmentBySync = command[1] == 'A';
    strcpy(reply, "1");
    break;
  case 'W':
    saveAlignModel();
    strcpy(reply, "1");
    break;
  default:
    strcpy(reply, "0");
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
  reply[0] = 0;
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
  int i;
  if ((parkStatus == PRK_UNPARKED) &&
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
      double newTargetHA;
      if (autoAlignmentBySync){
        newTargetHA = haRange(rtk.LST() * 15.0 - newTargetRA);
        double Azm, Alt;
        EquToHor(newTargetHA, newTargetDec, doesRefraction.forGoto, &Azm, &Alt, localSite.cosLat(), localSite.sinLat());
        if (alignment.getRefs() == 0)
        {
          mount.mP->syncAzAlt(Azm, Alt, mount.mP->GetPierSide());
        }
        double Axis1 = motorA1.getCurrentPos() / geoA1.stepsPerDegree;
        double Axis2 = motorA2.getCurrentPos() / geoA2.stepsPerDegree;
        alignment.addReferenceDeg(Azm, Alt, Axis1, Axis2);
        if (alignment.isReady())
          {
            hasStarAlignment = true;
            motorA1.setTargetPos(motorA1.getCurrentPos());
            motorA2.setTargetPos(motorA2.getCurrentPos());
            autoAlignmentBySync = false;
          }
      }
      else
      {
        newTargetHA = haRange(rtk.LST() * 15.0 - newTargetRA);
        i = mount.mP->syncEqu(newTargetHA, newTargetDec, targetPierSide, localSite.cosLat(), localSite.sinLat());
      }
        break;
    case 'U':
    {
      // :CU# sync with the User Defined RA DEC
      newTargetRA = (double)XEEPROM.readFloat(EE_RA);
      newTargetDec = (double)XEEPROM.readFloat(EE_DEC);
      double newTargetHA = haRange(rtk.LST() * 15.0 - newTargetRA);
      i = mount.mP->syncEqu(newTargetHA, newTargetDec, targetPierSide, localSite.cosLat(), localSite.sinLat());
      break;
    }
    case 'A':
      i = mount.mP->syncAzAlt(newTargetAzm, newTargetAlt, targetPierSide);
      break;
    }
    i = 0;
    if (command[1] == 'M' || command[1] == 'A' || command[1] == 'U')
    {
      if (i == 0)
        strcpy(reply, "N/A#");
      if (i > 0)
      {
        reply[0] = 'E';
        reply[1] = '0' + i;
        reply[2] = '#';
      }
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
    strcpy(reply, "0");
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
      strcpy(reply, "0");
    else
      strcpy(reply, "1");
    break;
  case 'B':
    //  :hB#   Set the home position
    //          Return: 0 on failure
    //                  1 on success
    if (!setHome()) strcpy(reply, "0");
    else strcpy(reply, "1");
    break;
  case 'b':
    //  :hb#   Reset the home position
    //          Return: 0 on failure
    //                  1 on success
    homeSaved = false;
    XEEPROM.write(EE_homeSaved, false);
    initHome();
    strcpy(reply, "1");
    break;
  case 'O':
    // : hO#   Reset telescope at the Park position if Park position is stored.
    //          Return: 0 on failure
    //                  1 on success
    if (!syncAtPark())
      strcpy(reply, "0");
    else
      strcpy(reply, "1");
    break;
  case 'P':
    // : hP#   Goto the Park Position
    //          Return: 0 on failure
    //                  1 on success
    if (park())
      strcpy(reply, "0");
    else
      strcpy(reply, "1");
    break;
  case 'Q':
    //  :hQ#   Set the park position
    //          Return: 0 on failure
    //                  1 on success
    if (!setPark())
      strcpy(reply, "0");
    else
      strcpy(reply, "1");
    break;
  case 'R':
    //  :hR#   Restore parked telescope to operation
    //          Return: 0 on failure
    //                  1 on success
    unpark();
    strcpy(reply, "1");
    break;


  default:
    strcpy(reply, "0");
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
    abortSlew();
    break;
  case 'e':
  case 'w':
    //  :Qe# & Qw#   Halt east/westward Slews
    //         Returns: Nothing
        mount.mP->StopAxis1();
    break;
  case 'n':
  case 's':
    //  :Qn# & Qs#   Halt north/southward Slews
    //         Returns: Nothing
        mount.mP->StopAxis2();
    break;
  default:
    strcpy(reply, "0");
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
    strcpy(reply, "0");
    return;
  }
  if (!isSlewing() && GuidingState == GuidingOFF)
  {
    enableGuideRate(i, false);
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
    siderealClockRate -= HzCf * (0.02);
    reply[0] = 0;
    break;
  case '-':
    siderealClockRate += HzCf * (0.02);
    reply[0] = 0;
    break;
  case 'S':
    // solar tracking rate 60Hz
    mount.mP->setTrackingSpeed(TrackingSolar);
    reply[0] = 0;
    break;
  case 'L':
    // lunar tracking rate 57.9Hz
    mount.mP->setTrackingSpeed(TrackingLunar);
    reply[0] = 0;
    break;
  case 'Q':
    // sidereal tracking rate
    mount.mP->setTrackingSpeed(TrackingStar);
    reply[0] = 0;
    break;
  case 'R':
    // reset master sidereal clock interval
    siderealClockRate = masterSiderealClockRate;
    mount.mP->setTrackingSpeed(TrackingStar);
    reply[0] = 0;
    break;
  case 'T':
    //set Target tracking rate
//    SetTrackingRate(1.0 - (double)storedTrackingRateRA / 10000.0, (double)storedTrackingRateDEC / 10000.0);
//    siderealMode = SIDM_TARGET;
    reply[0] = 0;
    break;
  case 'e':
    if (parkStatus == PRK_UNPARKED)
    {
//      lastSetTrackingEnable = millis();
//     atHome = false;
//      siderealTracking = true;
      mount.mP->startTracking();
//      computeTrackingRate(true);
      strcpy(reply, "1");
    }
    else
      strcpy(reply, "0");
    break;
  case 'd':
    if (parkStatus == PRK_UNPARKED)
    {
//      siderealTracking = false;
      mount.mP->stopTracking();
      strcpy(reply, "1");
    }
    else
      strcpy(reply, "0");
    break;
#if 0
  case '0':
    // turn compensation off
    tc = TC_NONE;
    computeTrackingRate(true);
    XEEPROM.write(EE_TC_Axis, 0);
    strcpy(reply, "1");
    break;
  case '1':
    // turn compensation RA only
    tc = TC_RA;
    computeTrackingRate(true);
    XEEPROM.write(EE_TC_Axis, 0);
    strcpy(reply, "1");
    break;
  case '2':
    // turn compensation BOTH
    tc = TC_BOTH;
    computeTrackingRate(true);
    XEEPROM.write(EE_TC_Axis, 2);
    strcpy(reply, "1");
    break;
#endif
  default:
    strcpy(reply, "0");
    break;
  }

  // Only burn the new rate if changing the sidereal interval
  if (command[1] == '+' || command[1] == '-' || command[1] == 'R')
  {
    XEEPROM.writeLong(EE_siderealClockRate, siderealClockRate);
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
    reply[0] = 0;
  }
  else
    strcpy(reply, "0");
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
    XEEPROM.write(EE_currentSite, currentSite);
    localSite.ReadSiteDefinition(currentSite);
    rtk.resetLongitude(*localSite.longitude());
    initHome();
    initTransformation(true);
    syncAtHome();
    reply[0] = 0;
    break;
  }
  case '?':
    sprintf(reply, "%d#", localSite.siteIndex());
    break;
  default:
    strcpy(reply, "0");
    break;
  }
}

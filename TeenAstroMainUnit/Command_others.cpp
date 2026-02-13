/**
 * Command handlers: $ (reset), ACK, A (alignment), B (reticule), C (sync),
 * D (distance), E (encoder), h (home/park), Q (halt), R (rate), U (precision), W (site).
 */
#include "Command.h"
#include "ValueToString.h"

// -----------------------------------------------------------------------------
//   $ - Reset / reboot / reinit
//   :$$#  Clean EEPROM
//   :$!#  Reboot main unit
//   :$X#  Reinit encoder and motors
// -----------------------------------------------------------------------------
void Command_dollar() {
  switch (command[1]) {
  case '$':
    for (int i = 0; i < XEEPROM.length(); i++)
      XEEPROM.write(i, 0);
    replyShortTrue();
    break;
  case '!':
    mount.reboot_unit = true;
    replyShortTrue();
    break;
  case 'X':
    initencoder();
    initmotor(true);
    replyShortTrue();
    break;
  default:
    replyNothing();
    break;
  }
}

// -----------------------------------------------------------------------------
//   <ACK> - Mount type (LX200)
// -----------------------------------------------------------------------------
void Command_ACK()
{
  switch (mount.mountType)
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
// -----------------------------------------------------------------------------
//   A - Alignment
//   :A0# :A*# :A2# :AE# :AC# :AA# :AW#
// -----------------------------------------------------------------------------
void Command_A() {
  switch (command[1]) {
  case '0':
    // telescope should be set in the polar home for a starting point
    initTransformation(true);
    syncAtHome();
    // enable the stepper drivers
    enable_Axis(true);
    delay(10);
    // start tracking
    if (mount.enableMotor)
    {
      StartSideralTracking();
    }
    replyShortTrue();
    break;
  case '*': {
    // Telescope at target position
    initTransformation(true);
    enable_Axis(true);
    delay(10);
    // start tracking
    if (mount.enableMotor)
    {
      StartSideralTracking();
    }
    PoleSide targetPoleSide = GetPoleSide();
    if (mount.newTargetPoleSide != POLE_NOTVALID)
    {
      targetPoleSide = mount.newTargetPoleSide;
      mount.newTargetPoleSide = POLE_NOTVALID;
    }
    double newTargetHA = haRange(rtk.LST() * 15.0 - mount.newTargetRA);
    double Lat = *localSite.latitude();
    Coord_EQ EQ_T(0, mount.newTargetDec * DEG_TO_RAD, newTargetHA * DEG_TO_RAD);
    Coord_HO HO_T = EQ_T.To_Coord_HO(Lat * DEG_TO_RAD, RefrOptForGoto());

    syncAzAlt(&HO_T, targetPoleSide);

    Coord_IN IN_T = getInstr();
    alignment.addReference(HO_T.direct_Az_S(), HO_T.Alt(), IN_T.Axis1_direct(), IN_T.Axis2());
    replyShortTrue();
    break;
  }

  case '2':
  {
    double newTargetHA = haRange(rtk.LST() * 15.0 - mount.newTargetRA);
    double Lat = *localSite.latitude();
    Coord_EQ EQ_T(0, mount.newTargetDec * DEG_TO_RAD, newTargetHA * DEG_TO_RAD);
    Coord_HO HO_T = EQ_T.To_Coord_HO( Lat * DEG_TO_RAD, RefrOptForGoto());
    if (alignment.getRefs() == 0)
    {
      syncAzAlt(&HO_T, GetPoleSide());
    }
    Coord_IN IN_T = getInstr();
    alignment.addReference(HO_T.direct_Az_S(), HO_T.Alt(), IN_T.Axis1_direct(), IN_T.Axis2());
    if (alignment.isReady())
    {
      alignment.minimizeAxis2();
      alignment.minimizeAxis1(mount.mountType == MOUNT_TYPE_GEM ? (Lat >= 0 ? M_PI_2 : -M_PI_2) : 0);
      syncAzAlt(&HO_T, GetPoleSide());
      hasStarAlignment = true;
    }
    mount.PushtoStatus = PT_OFF;
    replyShortTrue();
    break;
  }
  case 'E':
  {
    double val = alignment.getError() * RAD_TO_DEG;
    doubleToDms(reply, &val, false, true, true);
    //sprintf(reply, "%+05.4f", val);
    strcat(reply, "#");
  }
  break;
  case 'C':
  case 'A':
    initTransformation(true);
    syncAtHome();
    mount.autoAlignmentBySync = command[1] == 'A';
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


// -----------------------------------------------------------------------------
//   B - Reticule brightness  :B+# :B-#
// -----------------------------------------------------------------------------
void Command_B() {
  if (command[1] != '+' && command[1] != '-')
    return;
#ifdef RETICULE_LED_PINS
  if (mount.reticuleBrightness > 255)
    mount.reticuleBrightness = 255;
  if (mount.reticuleBrightness < 31)
    mount.reticuleBrightness = 31;

  if (command[1] == '-')
    mount.reticuleBrightness /= 1.4;
  if (command[1] == '+')
    mount.reticuleBrightness *= 1.4;

  if (mount.reticuleBrightness > 255)
    mount.reticuleBrightness = 255;
  if (mount.reticuleBrightness < 31)
    mount.reticuleBrightness = 31;

  analogWrite(RETICULE_LED_PINS, mount.reticuleBrightness);
#endif
  replyNothing();
}

// -----------------------------------------------------------------------------
//   C - Sync  :CA# :CM# :CS# :CU#
// -----------------------------------------------------------------------------
void Command_C() {
  if ((mount.parkStatus == PRK_UNPARKED) &&
    !TelescopeBusy() &&
    (command[1] == 'A' || command[1] == 'M' || command[1] == 'S' || command[1] == 'U'))
  {
    PoleSide targetPoleSide = GetPoleSide();
    if (mount.newTargetPoleSide != POLE_NOTVALID)
    {
      targetPoleSide = mount.newTargetPoleSide;
      mount.newTargetPoleSide = POLE_NOTVALID;
    }
    switch (command[1])
    {
    case 'M':
    case 'S':
    {
      if (mount.autoAlignmentBySync) {
        double newTargetHA = haRange(rtk.LST() * 15.0 - mount.newTargetRA);
        Coord_EQ EQ_T(0, mount.newTargetDec * DEG_TO_RAD, newTargetHA * DEG_TO_RAD);
        Coord_HO HO_T = EQ_T.To_Coord_HO(*localSite.latitude() * DEG_TO_RAD, RefrOptForGoto());

        if (alignment.getRefs() == 0)
        {
          syncAzAlt(&HO_T, targetPoleSide);
        }
        Coord_IN IN_T = getInstr();
        alignment.addReference(HO_T.direct_Az_S(), HO_T.Alt(), IN_T.Axis1_direct(), IN_T.Axis2());
        if (alignment.isReady())
        {
          alignment.minimizeAxis2();
          alignment.minimizeAxis1(mount.mountType == MOUNT_TYPE_GEM ? (*localSite.latitude() >= 0 ? M_PI_2 : -M_PI_2) : 0);
          syncAzAlt(&HO_T, GetPoleSide());
          hasStarAlignment = true;
          mount.autoAlignmentBySync = false;
        }
      }
      else
      {
        double newTargetHA = haRange(rtk.LST() * 15.0 - mount.newTargetRA);
        Coord_EQ EQ_T(0, mount.newTargetDec * DEG_TO_RAD, newTargetHA * DEG_TO_RAD);
        syncEqu(&EQ_T, targetPoleSide, *localSite.latitude() * DEG_TO_RAD);
      }
      if (command[1] == 'M')
      {
        strcpy(reply, "N/A#");
      }
      StartSideralTracking();
    }
    break;
    case 'U':
    {
      // :CU# sync with the User Defined RA DEC
      mount.newTargetRA = (double)XEEPROM.readFloat(getMountAddress(EE_RA));
      mount.newTargetDec = (double)XEEPROM.readFloat(getMountAddress(EE_DEC));
      double newTargetHA = haRange(rtk.LST() * 15.0 - mount.newTargetRA);
      Coord_EQ EQ_T(0, mount.newTargetDec * DEG_TO_RAD, newTargetHA * DEG_TO_RAD);
      syncEqu(&EQ_T, targetPoleSide, *localSite.latitude() * DEG_TO_RAD);
      strcpy(reply, "N/A#");
      break;
    }
    case 'A':
    {
      Coord_HO HO_T(0, mount.newTargetAlt * DEG_TO_RAD, mount.newTargetAzm * DEG_TO_RAD, true);
      syncAzAlt(&HO_T, targetPoleSide);
      strcpy(reply, "N/A#");
    }
    break;
    }
  }
}
// -----------------------------------------------------------------------------
//   E - Encoder / push-to  :EAS# :EAE# :EAQ# :ECT# :ECE# :ECS# :ED# :EMS# :EMU# :EMA# :EMQ#
// -----------------------------------------------------------------------------
void Command_E() {
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
      mount.EncodeSyncMode = ES_OFF;
      syncEwithT();
      getInstrDeg(&A1, &A2, &A3);
      mount.encoderA1.setRef(A1);
      mount.encoderA2.setRef(A2);
      replyLongTrue();
    }
    break;
    case 'E':
    {
      //  :EAE#  Align Encoder End
      double A1, A2, A3;
      getInstrDeg(&A1, &A2, &A3);
      bool ok = mount.encoderA1.calibrate(A1);
      ok &= mount.encoderA1.calibrate(A2);
      ok ? replyLongTrue() : replyLongFalse();
    }
    break;
    case 'Q':
    {
      //  :EAQ#  Align Encoder Quit
      mount.encoderA1.delRef();
      mount.encoderA2.delRef();
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
      switch (mount.PushtoStatus)
      {
      case PT_RADEC:
      {
        double newTargetHA = haRange(rtk.LST() * 15.0 - mount.newTargetRA);
        Coord_EQ EQ_T(0, mount.newTargetDec * DEG_TO_RAD, newTargetHA * DEG_TO_RAD);
        syncEqu(&EQ_T, GetPoleSide(), *localSite.latitude() * DEG_TO_RAD);
        replyLongTrue();
      }
      break;
      case PT_ALTAZ:
      {
        Coord_HO HO_T(0, mount.newTargetAlt * DEG_TO_RAD, mount.newTargetAzm * DEG_TO_RAD, true);
        syncAzAlt(&HO_T, GetPoleSide());
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
    switch (mount.PushtoStatus)
    {
    case PT_RADEC:
    {
      double newTargetHA = haRange(rtk.LST() * 15.0 - mount.newTargetRA);
      Coord_EQ EQ_T(0, mount.newTargetDec * DEG_TO_RAD, newTargetHA * DEG_TO_RAD);
      e = PushToEqu(EQ_T, GetPoleSide(), *localSite.latitude() * DEG_TO_RAD, &delta1, &delta2);
      sprintf(reply, "%d,%+06d,%+06d#", e, (int)(60 * delta1), (int)(60 * delta2));
    }
    break;
    case PT_ALTAZ:
    {
      Coord_HO HO_T(0, mount.newTargetAlt * DEG_TO_RAD, mount.newTargetAzm * DEG_TO_RAD, true);
      e = PushToHor(HO_T, GetPoleSide(), &delta1, &delta2);
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
      double newTargetHA = haRange(rtk.LST() * 15.0 - mount.newTargetRA);
      Coord_EQ EQ_T(0, mount.newTargetDec* DEG_TO_RAD, newTargetHA* DEG_TO_RAD);
      e = PushToEqu(EQ_T, GetPoleSide(), *localSite.latitude() * DEG_TO_RAD, &delta1, &delta2);
      sprintf(reply, "%d", e);
      mount.PushtoStatus = PT_RADEC;
    }
    else if (command[2] == 'A')
    {
      Coord_HO HO_T(0, mount.newTargetAlt* DEG_TO_RAD, mount.newTargetAzm* DEG_TO_RAD, true);
      e = PushToHor(HO_T, GetPoleSide(), &delta1, &delta2);
      sprintf(reply, "%d", e);
      mount.PushtoStatus = PT_ALTAZ;
    }
    else if (command[2] == 'U')
    {

      mount.newTargetRA = (double)XEEPROM.readFloat(getMountAddress(EE_RA));
      mount.newTargetDec = (double)XEEPROM.readFloat(getMountAddress(EE_DEC));
      double newTargetHA = haRange(rtk.LST() * 15.0 - mount.newTargetRA);
      Coord_EQ EQ_T(0, mount.newTargetDec * DEG_TO_RAD, newTargetHA * DEG_TO_RAD);
      e = PushToEqu(EQ_T, GetPoleSide(), *localSite.latitude() * DEG_TO_RAD, &delta1, &delta2);
      sprintf(reply, "%d", e);
      mount.PushtoStatus = PT_RADEC;
    }
    else if (command[2] == 'Q')
    {
      mount.PushtoStatus = PT_OFF;
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


// -----------------------------------------------------------------------------
//   D - Distance bars  :D#  (0x7f# if moving, else #)
// -----------------------------------------------------------------------------
void Command_D() {
  if (command[1] != 0)
  {
    return;
  }
  if (mount.movingTo)
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

// -----------------------------------------------------------------------------
//   h - Home / park  :hF# :hC# :hB# :hb# :hO# :hP# :hQ# :hS# :hR#
// -----------------------------------------------------------------------------
void Command_h() {
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
    mount.homeSaved = false;
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
    if (park()==0)
      replyShortTrue();
    else
      replyShortFalse();
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
  case 'S':
    //  :hS#   Get if the park position is saved
    //          Return: 0 or 1
    mount.parkSaved ? replyShortTrue() : replyShortFalse();
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

// -----------------------------------------------------------------------------
//   Q - Halt  :Q# :Qe# :Qw# :Qn# :Qs#
// -----------------------------------------------------------------------------
void Command_Q() {
  switch (command[1])
  {
  case 0:
    //  :Q#    Halt all slews, stops goto
    //         Returns: Nothing
    mount.doSpiral = false;
    if (mount.parkStatus != PRK_PARKED)
    {
      if (mount.movingTo)
      {
        mount.abortSlew = true;
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
    if ((mount.parkStatus == PRK_UNPARKED) && !mount.movingTo)
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
    if ((mount.parkStatus == PRK_UNPARKED) && !mount.movingTo)
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

// -----------------------------------------------------------------------------
//   R - Slew rate  :RG# :RC# :RM# :RS# :R0#..:R4#
// -----------------------------------------------------------------------------
void Command_R() {
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
  if (!TelescopeBusy())
  {
    mount.recenterGuideRate = i;
    enableGuideRate(mount.recenterGuideRate);
  }
}



// -----------------------------------------------------------------------------
//   U - Precision toggle  :U#  (low/high precision)
// -----------------------------------------------------------------------------
void Command_U() {
  if (command[1] == 0) {
    highPrecision = !highPrecision;
  }
  replyNothing();
}

// -----------------------------------------------------------------------------
//   W - Site  :W0# :W1# :W2# :W?#
// -----------------------------------------------------------------------------
void Command_W() {
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
    initLimit();
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

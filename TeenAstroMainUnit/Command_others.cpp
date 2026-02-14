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
  switch (commandState.command[1]) {
  case '$':
    for (int i = 0; i < XEEPROM.length(); i++)
      XEEPROM.write(i, 0);
    replyShortTrue();
    break;
  case '!':
    mount.motorsEncoders.reboot_unit = true;
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
  switch (mount.config.identity.mountType)
  {
  case MOUNT_TYPE_ALTAZM:
  case MOUNT_TYPE_FORK_ALT:
    strcpy(commandState.reply, "A");
    break;
  case MOUNT_TYPE_FORK:
    strcpy(commandState.reply, "P");
    break;
  case MOUNT_TYPE_GEM:
    strcpy(commandState.reply, "G");
    break;
  case MOUNT_UNDEFINED:
  default:
    strcpy(commandState.reply, "L");
    break;
  }
}
// -----------------------------------------------------------------------------
//   A - Alignment
//   :A0# :A*# :A2# :AE# :AC# :AA# :AW#
// -----------------------------------------------------------------------------
void Command_A() {
  switch (commandState.command[1]) {
  case '0':
    // telescope should be set in the polar home for a starting point
    initTransformation(true);
    mount.syncAtHome();
    // enable the stepper drivers
    mount.axes.enable(true);
    delay(10);
    // start tracking
    if (mount.motorsEncoders.enableMotor)
    {
      mount.startSideralTracking();
    }
    replyShortTrue();
    break;
  case '*': {
    // Telescope at target position
    initTransformation(true);
    mount.axes.enable(true);
    delay(10);
    // start tracking
    if (mount.motorsEncoders.enableMotor)
    {
      mount.startSideralTracking();
    }
    PoleSide targetPoleSide = mount.getPoleSide();
    if (mount.targetCurrent.newTargetPoleSide != POLE_NOTVALID)
    {
      targetPoleSide = mount.targetCurrent.newTargetPoleSide;
      mount.targetCurrent.newTargetPoleSide = POLE_NOTVALID;
    }
    double newTargetHA = haRange(rtk.LST() * 15.0 - mount.targetCurrent.newTargetRA);
    double Lat = *localSite.latitude();
    Coord_EQ EQ_T(0, mount.targetCurrent.newTargetDec * DEG_TO_RAD, newTargetHA * DEG_TO_RAD);
    Coord_HO HO_T = EQ_T.To_Coord_HO(Lat * DEG_TO_RAD, mount.refrOptForGoto());

    mount.syncAzAlt(&HO_T, targetPoleSide);

    Coord_IN IN_T = mount.getInstr();
    mount.alignment.conv.addReference(HO_T.direct_Az_S(), HO_T.Alt(), IN_T.Axis1_direct(), IN_T.Axis2());
    replyShortTrue();
    break;
  }

  case '2':
  {
    double newTargetHA = haRange(rtk.LST() * 15.0 - mount.targetCurrent.newTargetRA);
    double Lat = *localSite.latitude();
    Coord_EQ EQ_T(0, mount.targetCurrent.newTargetDec * DEG_TO_RAD, newTargetHA * DEG_TO_RAD);
    Coord_HO HO_T = EQ_T.To_Coord_HO( Lat * DEG_TO_RAD, mount.refrOptForGoto());
    if (mount.alignment.conv.getRefs() == 0)
    {
      mount.syncAzAlt(&HO_T, mount.getPoleSide());
    }
    Coord_IN IN_T = mount.getInstr();
    mount.alignment.conv.addReference(HO_T.direct_Az_S(), HO_T.Alt(), IN_T.Axis1_direct(), IN_T.Axis2());
    if (mount.alignment.conv.isReady())
    {
      mount.alignment.conv.minimizeAxis2();
      mount.alignment.conv.minimizeAxis1(mount.config.identity.mountType == MOUNT_TYPE_GEM ? (Lat >= 0 ? M_PI_2 : -M_PI_2) : 0);
      mount.syncAzAlt(&HO_T, mount.getPoleSide());
      mount.alignment.hasValid = true;
    }
    mount.config.peripherals.PushtoStatus = PT_OFF;
    replyShortTrue();
    break;
  }
  case 'E':
  {
    double val = mount.alignment.conv.getError() * RAD_TO_DEG;
    doubleToDms(commandState.reply, &val, false, true, true);
    //sprintf(commandState.reply, "%+05.4f", val);
    strcat(commandState.reply, "#");
  }
  break;
  case 'C':
  case 'A':
    initTransformation(true);
    mount.syncAtHome();
    mount.alignment.autoAlignmentBySync = commandState.command[1] == 'A';
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
  if (commandState.command[1] != '+' && commandState.command[1] != '-')
    return;
#ifdef RETICULE_LED_PINS
  // Reticule: direct member access (see Mount access convention).
  if (mount.reticule.reticuleBrightness > 255)
    mount.reticule.reticuleBrightness = 255;
  if (mount.reticule.reticuleBrightness < 31)
    mount.reticule.reticuleBrightness = 31;

  if (commandState.command[1] == '-')
    mount.reticule.reticuleBrightness /= 1.4;
  if (commandState.command[1] == '+')
    mount.reticule.reticuleBrightness *= 1.4;

  if (mount.reticule.reticuleBrightness > 255)
    mount.reticule.reticuleBrightness = 255;
  if (mount.reticule.reticuleBrightness < 31)
    mount.reticule.reticuleBrightness = 31;

  analogWrite(RETICULE_LED_PINS, mount.reticule.reticuleBrightness);
#endif
  replyNothing();
}

// -----------------------------------------------------------------------------
//   C - Sync  :CA# :CM# :CS# :CU#
// -----------------------------------------------------------------------------
void Command_C() {
  if ((!mount.isParked()) &&
    !mount.isSlewing() &&
    (commandState.command[1] == 'A' || commandState.command[1] == 'M' || commandState.command[1] == 'S' || commandState.command[1] == 'U'))
  {
    PoleSide targetPoleSide = mount.getPoleSide();
    if (mount.targetCurrent.newTargetPoleSide != POLE_NOTVALID)
    {
      targetPoleSide = mount.targetCurrent.newTargetPoleSide;
      mount.targetCurrent.newTargetPoleSide = POLE_NOTVALID;
    }
    switch (commandState.command[1])
    {
    case 'M':
    case 'S':
    {
      if (mount.alignment.autoAlignmentBySync) {
        double newTargetHA = haRange(rtk.LST() * 15.0 - mount.targetCurrent.newTargetRA);
        Coord_EQ EQ_T(0, mount.targetCurrent.newTargetDec * DEG_TO_RAD, newTargetHA * DEG_TO_RAD);
        Coord_HO HO_T = EQ_T.To_Coord_HO(*localSite.latitude() * DEG_TO_RAD, mount.refrOptForGoto());

        if (mount.alignment.conv.getRefs() == 0)
        {
          mount.syncAzAlt(&HO_T, targetPoleSide);
        }
        Coord_IN IN_T = mount.getInstr();
        mount.alignment.conv.addReference(HO_T.direct_Az_S(), HO_T.Alt(), IN_T.Axis1_direct(), IN_T.Axis2());
        if (mount.alignment.conv.isReady())
        {
          mount.alignment.conv.minimizeAxis2();
          mount.alignment.conv.minimizeAxis1(mount.config.identity.mountType == MOUNT_TYPE_GEM ? (*localSite.latitude() >= 0 ? M_PI_2 : -M_PI_2) : 0);
          mount.syncAzAlt(&HO_T, mount.getPoleSide());
          mount.alignment.hasValid = true;
          mount.alignment.autoAlignmentBySync = false;
        }
      }
      else
      {
        double newTargetHA = haRange(rtk.LST() * 15.0 - mount.targetCurrent.newTargetRA);
        Coord_EQ EQ_T(0, mount.targetCurrent.newTargetDec * DEG_TO_RAD, newTargetHA * DEG_TO_RAD);
        mount.syncEqu(&EQ_T, targetPoleSide, *localSite.latitude() * DEG_TO_RAD);
      }
      if (commandState.command[1] == 'M')
      {
        strcpy(commandState.reply, "N/A#");
      }
      mount.startSideralTracking();
    }
    break;
    case 'U':
    {
      // :CU# sync with the User Defined RA DEC
      mount.targetCurrent.newTargetRA = (double)XEEPROM.readFloat(getMountAddress(EE_RA));
      mount.targetCurrent.newTargetDec = (double)XEEPROM.readFloat(getMountAddress(EE_DEC));
      double newTargetHA = haRange(rtk.LST() * 15.0 - mount.targetCurrent.newTargetRA);
      Coord_EQ EQ_T(0, mount.targetCurrent.newTargetDec * DEG_TO_RAD, newTargetHA * DEG_TO_RAD);
      mount.syncEqu(&EQ_T, targetPoleSide, *localSite.latitude() * DEG_TO_RAD);
      strcpy(commandState.reply, "N/A#");
      break;
    }
    case 'A':
    {
      Coord_HO HO_T(0, mount.targetCurrent.newTargetAlt * DEG_TO_RAD, mount.targetCurrent.newTargetAzm * DEG_TO_RAD, true);
      mount.syncAzAlt(&HO_T, targetPoleSide);
      strcpy(commandState.reply, "N/A#");
    }
    break;
    }
  }
}
// -----------------------------------------------------------------------------
//   E - Encoder / push-to  :EAS# :EAE# :EAQ# :ECT# :ECE# :ECS# :ED# :EMS# :EMU# :EMA# :EMQ#
// -----------------------------------------------------------------------------
void Command_E() {
  switch (commandState.command[1])
  {
  case 'A':
  {
    switch (commandState.command[2])
    {
    case 'S':
    {
      //  :EAS#  Align Encoder Start
      double A1, A2, A3;
      mount.motorsEncoders.EncodeSyncMode = ES_OFF;
      mount.syncEwithT();
      mount.getInstrDeg(&A1, &A2, &A3);
      mount.motorsEncoders.encoderA1.setRef(A1);
      mount.motorsEncoders.encoderA2.setRef(A2);
      replyLongTrue();
    }
    break;
    case 'E':
    {
      //  :EAE#  Align Encoder End
      double A1, A2, A3;
      mount.getInstrDeg(&A1, &A2, &A3);
      bool ok = mount.motorsEncoders.encoderA1.calibrate(A1);
      ok &= mount.motorsEncoders.encoderA1.calibrate(A2);
      ok ? replyLongTrue() : replyLongFalse();
    }
    break;
    case 'Q':
    {
      //  :EAQ#  Align Encoder Quit
      mount.motorsEncoders.encoderA1.delRef();
      mount.motorsEncoders.encoderA2.delRef();
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
    switch (commandState.command[2])
    {
    case 'T':
    {
      //  :ECT#   Synchonize the telescope with the Encoders
      mount.syncTwithE();
      replyLongTrue();
    }
    break;
    case 'E':
    {
      //  :ECE#   Synchonize the Encoders with the telescope
      mount.syncEwithT();
      replyLongTrue();
    }
    break;
    case 'S':
    {
      //  :ECS#   Synchonize the Telescope and Encoder to Target
      switch (mount.config.peripherals.PushtoStatus)
      {
      case PT_RADEC:
      {
        double newTargetHA = haRange(rtk.LST() * 15.0 - mount.targetCurrent.newTargetRA);
        Coord_EQ EQ_T(0, mount.targetCurrent.newTargetDec * DEG_TO_RAD, newTargetHA * DEG_TO_RAD);
        mount.syncEqu(&EQ_T, mount.getPoleSide(), *localSite.latitude() * DEG_TO_RAD);
        replyLongTrue();
      }
      break;
      case PT_ALTAZ:
      {
        Coord_HO HO_T(0, mount.targetCurrent.newTargetAlt * DEG_TO_RAD, mount.targetCurrent.newTargetAzm * DEG_TO_RAD, true);
        mount.syncAzAlt(&HO_T, mount.getPoleSide());
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
    switch (mount.config.peripherals.PushtoStatus)
    {
    case PT_RADEC:
    {
      double newTargetHA = haRange(rtk.LST() * 15.0 - mount.targetCurrent.newTargetRA);
      Coord_EQ EQ_T(0, mount.targetCurrent.newTargetDec * DEG_TO_RAD, newTargetHA * DEG_TO_RAD);
      e = PushToEqu(EQ_T, mount.getPoleSide(), *localSite.latitude() * DEG_TO_RAD, &delta1, &delta2);
      sprintf(commandState.reply, "%d,%+06d,%+06d#", e, (int)(60 * delta1), (int)(60 * delta2));
    }
    break;
    case PT_ALTAZ:
    {
      Coord_HO HO_T(0, mount.targetCurrent.newTargetAlt * DEG_TO_RAD, mount.targetCurrent.newTargetAzm * DEG_TO_RAD, true);
      e = PushToHor(HO_T, mount.getPoleSide(), &delta1, &delta2);
      sprintf(commandState.reply, "%d,%+06d,%+06d#", e, (int)(60 * delta1), (int)(60 * delta2));
    }
    break;
    default:
      sprintf(commandState.reply, "%d,%+06d,%+06d#", 0, 0, 0);
      break;
    }
  }
  break;
  case 'M':
  {
    float delta1;
    float delta2;
    int e = 0;
    if (commandState.command[2] == 'S')
    {
      double newTargetHA = haRange(rtk.LST() * 15.0 - mount.targetCurrent.newTargetRA);
      Coord_EQ EQ_T(0, mount.targetCurrent.newTargetDec* DEG_TO_RAD, newTargetHA* DEG_TO_RAD);
      e = PushToEqu(EQ_T, mount.getPoleSide(), *localSite.latitude() * DEG_TO_RAD, &delta1, &delta2);
      sprintf(commandState.reply, "%d", e);
      mount.config.peripherals.PushtoStatus = PT_RADEC;
    }
    else if (commandState.command[2] == 'A')
    {
      Coord_HO HO_T(0, mount.targetCurrent.newTargetAlt* DEG_TO_RAD, mount.targetCurrent.newTargetAzm* DEG_TO_RAD, true);
      e = PushToHor(HO_T, mount.getPoleSide(), &delta1, &delta2);
      sprintf(commandState.reply, "%d", e);
      mount.config.peripherals.PushtoStatus = PT_ALTAZ;
    }
    else if (commandState.command[2] == 'U')
    {

      mount.targetCurrent.newTargetRA = (double)XEEPROM.readFloat(getMountAddress(EE_RA));
      mount.targetCurrent.newTargetDec = (double)XEEPROM.readFloat(getMountAddress(EE_DEC));
      double newTargetHA = haRange(rtk.LST() * 15.0 - mount.targetCurrent.newTargetRA);
      Coord_EQ EQ_T(0, mount.targetCurrent.newTargetDec * DEG_TO_RAD, newTargetHA * DEG_TO_RAD);
      e = PushToEqu(EQ_T, mount.getPoleSide(), *localSite.latitude() * DEG_TO_RAD, &delta1, &delta2);
      sprintf(commandState.reply, "%d", e);
      mount.config.peripherals.PushtoStatus = PT_RADEC;
    }
    else if (commandState.command[2] == 'Q')
    {
      mount.config.peripherals.PushtoStatus = PT_OFF;
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
  if (commandState.command[1] != 0)
  {
    return;
  }
  if (mount.tracking.movingTo)
  {
    commandState.reply[0] = (char)127;
    commandState.reply[1] = 0;
  }
  else
  {
    commandState.reply[0] = 0;
  }
  strcat(commandState.reply, "#");
}

// -----------------------------------------------------------------------------
//   h - Home / park  :hF# :hC# :hB# :hb# :hO# :hP# :hQ# :hS# :hR#
// -----------------------------------------------------------------------------
void Command_h() {
  switch (commandState.command[1])
  {
  case 'F':
    //  :hF#   Reset telescope at the home position.  This position is required for a Cold Start.
    //         Point to the celestial pole with the counterweight pointing downwards (CWD position).
    //         Return: Nothing
    mount.syncAtHome();
    break;
  case 'C':
    //  :hC#   Moves telescope to the home position
    //          Return: 0 on failure
    //                  1 on success
    if (!mount.goHome())
      replyShortFalse();
    else
      replyShortTrue();
    break;
  case 'B':
    //  :hB#   Set the home position
    //          Return: 0 on failure
    //                  1 on success
    if (!mount.setHome()) replyShortFalse();
    else replyShortTrue();
    break;
  case 'b':
    //  :hb#   Reset the home position
    //          Return: 0 on failure
    //                  1 on success
    mount.parkHome.homeSaved = false;
    XEEPROM.write(getMountAddress(EE_homeSaved), false);
    mount.initHome();
    replyShortTrue();
    break;
  case 'O':
    // : hO#   Reset telescope at the Park position if Park position is stored.
    //          Return: 0 on failure
    //                  1 on success
    if (!mount.syncAtPark())
      replyShortFalse();
    else
      replyShortTrue();
    break;
  case 'P':
    // : hP#   Goto the Park Position
    //          Return: 0 on failure
    //                  1 on success
    if (mount.park()==0)
      replyShortTrue();
    else
      replyShortFalse();
    break;
  case 'Q':
    //  :hQ#   Set the park position
    //          Return: 0 on failure
    //                  1 on success
    if (!mount.setPark())
      replyShortFalse();
    else
      replyShortTrue();
    break;
  case 'S':
    //  :hS#   Get if the park position is saved
    //          Return: 0 or 1
    mount.parkHome.parkSaved ? replyShortTrue() : replyShortFalse();
    break;
  case 'R':
    //  :hR#   Restore parked telescope to operation
    //          Return: 0 on failure
    //                  1 on success
    mount.unpark();
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
  switch (commandState.command[1])
  {
  case 0:
    //  :Q#    Halt all slews, stops goto
    //         Returns: Nothing
    mount.tracking.doSpiral = false;
    if (!mount.isParked())
    {
      if (mount.tracking.movingTo)
      {
        mount.abortSlew();
      }
      else
      {
        mount.stopAxis1();
        mount.stopAxis2();
      }
    }
    break;
  case 'e':
  case 'w':
    //  :Qe# & Qw#   Halt east/westward Slews
    //         Returns: Nothing
  {
    if ((!mount.isParked()) && !mount.tracking.movingTo)
    {
      mount.stopAxis1();
    }
  }
  break;
  case 'n':
  case 's':
    //  :Qn# & Qs#   Halt north/southward Slews
    //         Returns: Nothing
  {
    if ((!mount.isParked()) && !mount.tracking.movingTo)
    {
      mount.stopAxis2();
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
  switch (commandState.command[1])
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
    i = commandState.command[1] - '0';
    break;
  default:
    replyNothing();
    return;
  }
  if (!mount.isSlewing())
  {
    mount.guiding.recenterGuideRate = i;
    mount.enableGuideRate(mount.guiding.recenterGuideRate);
  }
}



// -----------------------------------------------------------------------------
//   U - Precision toggle  :U#  (low/high precision)
// -----------------------------------------------------------------------------
void Command_U() {
  if (commandState.command[1] == 0) {
    commandState.highPrecision = !commandState.highPrecision;
  }
  replyNothing();
}

// -----------------------------------------------------------------------------
//   W - Site  :W0# :W1# :W2# :W?#
// -----------------------------------------------------------------------------
void Command_W() {
  switch (commandState.command[1])
  {
  case '0':
  case '1':
  case '2':
  {
    uint8_t currentSite = commandState.command[1] - '0';
    XEEPROM.write(getMountAddress(EE_currentSite), currentSite);
    localSite.ReadSiteDefinition(currentSite);
    rtk.resetLongitude(*localSite.longitude());
    initCelestialPole();
    mount.limits.initLimit();
    mount.initHome();
    initTransformation(true);
    mount.syncAtHome();
    replyNothing();
    break;
  }
  case '?':
    sprintf(commandState.reply, "%d#", localSite.siteIndex());
    break;
  default:
    replyNothing();
    break;
  }
}

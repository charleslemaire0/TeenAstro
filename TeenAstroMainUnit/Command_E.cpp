/**
 * E - Encoder / push-to. One file per letter (plan).
 * All :Ex# sub-commands are TeenAstro specific (not in Meade LX200).
 */
#include "Command.h"
#include "ValueToString.h"

// -----------------------------------------------------------------------------
//   E - Encoder / push-to  :EAS# :EAE# :EAQ# :ECT# :ECE# :ECS# :ED# :EMS# :EMU# :EMA# :EMQ#
// -----------------------------------------------------------------------------
void Command_E() {
  switch (commandState.command[1]) {
  case 'A': {
    switch (commandState.command[2]) {
    case 'S': {
      // :EAS#  Align Encoder Start  TeenAstro specific
      double A1, A2, A3;
      mount.motorsEncoders.EncodeSyncMode = ES_OFF;
      mount.syncEwithT();
      mount.getInstrDeg(&A1, &A2, &A3);
      mount.motorsEncoders.encoderA1.setRef(A1);
      mount.motorsEncoders.encoderA2.setRef(A2);
      replyLongTrue();
      break;
    }
    case 'E': {
      // :EAE#  Align Encoder End  TeenAstro specific
      double A1, A2, A3;
      bool ok;
      mount.getInstrDeg(&A1, &A2, &A3);
      ok = mount.motorsEncoders.encoderA1.calibrate(A1);
      ok &= mount.motorsEncoders.encoderA2.calibrate(A2);
      ok ? replyLongTrue() : replyLongFalse();
      break;
    }
    case 'Q':
      // :EAQ#  Align Encoder Quit  TeenAstro specific
      mount.motorsEncoders.encoderA1.delRef();
      mount.motorsEncoders.encoderA2.delRef();
      replyLongTrue();
      break;
    default:
      replyNothing();
      break;
    }
    break;
  }
  case 'C': {
    switch (commandState.command[2]) {
    case 'T':
      // :ECT#  Sync telescope with Encoders  TeenAstro specific
      mount.syncTwithE();
      replyLongTrue();
      break;
    case 'E':
      // :ECE#  Sync Encoders with telescope  TeenAstro specific
      mount.syncEwithT();
      replyLongTrue();
      break;
    case 'S':
      // :ECS#  Sync Telescope and Encoder to Target  TeenAstro specific
      switch (mount.config.peripherals.PushtoStatus) {
      case PT_RADEC: {
        double newTargetHA = haRange(rtk.LST() * 15.0 - mount.targetCurrent.newTargetRA);
        Coord_EQ EQ_T(0, mount.targetCurrent.newTargetDec * DEG_TO_RAD, newTargetHA * DEG_TO_RAD);
        mount.syncEqu(&EQ_T, mount.getPoleSide(), *localSite.latitude() * DEG_TO_RAD);
        replyLongTrue();
        break;
      }
      case PT_ALTAZ: {
        Coord_HO HO_T(0, mount.targetCurrent.newTargetAlt * DEG_TO_RAD, mount.targetCurrent.newTargetAzm * DEG_TO_RAD, true);
        mount.syncAzAlt(&HO_T, mount.getPoleSide());
        replyLongTrue();
        break;
      }
      default:
        replyLongTrue();
        break;
      }
      break;
    default:
      replyLongUnknow();
      break;
    }
    break;
  }
  case 'D': {
    // :ED#  TeenAstro specific (push-to delta)
    float delta1, delta2;
    int e = 0;
    switch (mount.config.peripherals.PushtoStatus) {
    case PT_RADEC: {
      double newTargetHA = haRange(rtk.LST() * 15.0 - mount.targetCurrent.newTargetRA);
      Coord_EQ EQ_T(0, mount.targetCurrent.newTargetDec * DEG_TO_RAD, newTargetHA * DEG_TO_RAD);
      e = PushToEqu(EQ_T, mount.getPoleSide(), *localSite.latitude() * DEG_TO_RAD, &delta1, &delta2);
      sprintf(commandState.reply, "%d,%+06d,%+06d#", e, (int)(60 * delta1), (int)(60 * delta2));
      break;
    }
    case PT_ALTAZ: {
      Coord_HO HO_T(0, mount.targetCurrent.newTargetAlt * DEG_TO_RAD, mount.targetCurrent.newTargetAzm * DEG_TO_RAD, true);
      e = PushToHor(HO_T, mount.getPoleSide(), &delta1, &delta2);
      sprintf(commandState.reply, "%d,%+06d,%+06d#", e, (int)(60 * delta1), (int)(60 * delta2));
      break;
    }
    default:
      sprintf(commandState.reply, "%d,%+06d,%+06d#", 0, 0, 0);
      break;
    }
    break;
  }
  case 'M': {
    // :EMS# :EMA# :EMU# :EMQ#  TeenAstro specific
    float delta1, delta2;
    int e = 0;
    if (commandState.command[2] == 'S') {
      double newTargetHA = haRange(rtk.LST() * 15.0 - mount.targetCurrent.newTargetRA);
      Coord_EQ EQ_T(0, mount.targetCurrent.newTargetDec * DEG_TO_RAD, newTargetHA * DEG_TO_RAD);
      e = PushToEqu(EQ_T, mount.getPoleSide(), *localSite.latitude() * DEG_TO_RAD, &delta1, &delta2);
      sprintf(commandState.reply, "%d", e);
      mount.config.peripherals.PushtoStatus = PT_RADEC;
    } else if (commandState.command[2] == 'A') {
      Coord_HO HO_T(0, mount.targetCurrent.newTargetAlt * DEG_TO_RAD, mount.targetCurrent.newTargetAzm * DEG_TO_RAD, true);
      e = PushToHor(HO_T, mount.getPoleSide(), &delta1, &delta2);
      sprintf(commandState.reply, "%d", e);
      mount.config.peripherals.PushtoStatus = PT_ALTAZ;
    } else if (commandState.command[2] == 'U') {
      mount.targetCurrent.newTargetRA = (double)XEEPROM.readFloat(getMountAddress(EE_RA));
      mount.targetCurrent.newTargetDec = (double)XEEPROM.readFloat(getMountAddress(EE_DEC));
      double newTargetHA = haRange(rtk.LST() * 15.0 - mount.targetCurrent.newTargetRA);
      Coord_EQ EQ_T(0, mount.targetCurrent.newTargetDec * DEG_TO_RAD, newTargetHA * DEG_TO_RAD);
      e = PushToEqu(EQ_T, mount.getPoleSide(), *localSite.latitude() * DEG_TO_RAD, &delta1, &delta2);
      sprintf(commandState.reply, "%d", e);
      mount.config.peripherals.PushtoStatus = PT_RADEC;
    } else if (commandState.command[2] == 'Q') {
      mount.config.peripherals.PushtoStatus = PT_OFF;
      replyShortTrue();
    } else {
      replyNothing();
    }
    break;
  }
  default:
    replyNothing();
    break;
  }
}

/**
 * A - Alignment. One file per letter (plan).
 */
#include "Command.h"
#include "ValueToString.h"

namespace {

void alignmentPolarFinalizeFromCurrentTarget()
{
  // Mechanical pole pass complete: discard the provisional two-star soft model
  // and rebuild RAM to the cold-boot baseline (synthetic refs for an ideal
  // polar mount, hasValid=false, EE_Tvalid cleared). Mount stays synced on the
  // recentered alignment star so :MS# pointing remains correct against the now
  // mechanically-true pole.
  double newTargetHA = haRange(rtk.LST() * 15.0 - mount.targetCurrent.newTargetRA);
  double Lat = *localSite.latitude();
  Coord_EQ EQ_T(0, mount.targetCurrent.newTargetDec * DEG_TO_RAD, newTargetHA * DEG_TO_RAD);
  Coord_HO HO_T = EQ_T.To_Coord_HO(Lat * DEG_TO_RAD, mount.refrOptForGoto());

  // Cold-baseline conv: clean(), seed synthetic ideal-polar refs, hasValid=false,
  // EE_Tvalid cleared in EEPROM. See initTransformation() in EEPROM.cpp.
  initTransformation(true);

  // Sync encoders so the current observing direction == alignment star.
  mount.syncAzAlt(&HO_T, mount.getPoleSide());

  mount.alignment.alignPolarThirdPending = false;
  mount.alignment.alignPhase   = ALIGN_IDLE;
  mount.alignment.alignStarNum = 0;
  mount.alignment.alignNumStarsSession = 2;
  mount.config.peripherals.PushtoStatus = PT_OFF;
}

} // namespace

// -----------------------------------------------------------------------------
//   A - Alignment  :A0# :A0,2# :A0,m# :A*# :A*,m# :A1# .. :A9# :AP# :AB# :AC# :AA# :AE# :AW#
// -----------------------------------------------------------------------------
void Command_A() {
  switch (commandState.command[1]) {
  case '0': {
    // :A0#  LX200 standard (alignment menu 0); :A0,2# two-star; :A0,m# mechanical pole (two-star + bolt pass)
    if (commandState.command[2] == ',' && commandState.command[3] == '3' && commandState.command[4] == 0) {
      replyNothing();
      break;
    }
    if (commandState.command[2] == ',' && commandState.command[4] != 0) {
      replyNothing();
      break;
    }
    if (commandState.command[2] == ',' && commandState.command[3] != '2' && commandState.command[3] != 'm') {
      replyNothing();
      break;
    }
    uint8_t numStarsSession = 2;
    if (commandState.command[2] == ',' && commandState.command[3] == 'm' && commandState.command[4] == 0)
      numStarsSession = 3;
    else if (commandState.command[2] == ',' && commandState.command[3] == '2' && commandState.command[4] == 0)
      numStarsSession = 2;
    initTransformation(true);
    mount.syncAtHome();
    mount.axes.enable(true);
    delay(10);
    if (mount.motorsEncoders.enableMotor)
      mount.startSideralTracking();
    mount.alignment.alignPhase   = ALIGN_SELECT;
    mount.alignment.alignStarNum = 1;
    mount.alignment.alignStarName[0] = '\0';
    mount.alignment.alignPolarThirdPending = false;
    mount.alignment.alignNumStarsSession = numStarsSession;
    replyShortTrue();
    break;
  }
  case '*': {
    // :A*#  LX200 standard (telescope at target); :A*,m# same + mechanical pole session (defer on 2nd star)
    if (commandState.command[2] == ',' && commandState.command[3] == '3' && commandState.command[4] == 0) {
      replyNothing();
      break;
    }
    if (commandState.command[2] == ',' && commandState.command[4] != 0) {
      replyNothing();
      break;
    }
    if (commandState.command[2] == ',' && commandState.command[3] != 'm') {
      replyNothing();
      break;
    }
    const bool mechanicalPole = (commandState.command[2] == ',' && commandState.command[3] == 'm' && commandState.command[4] == 0);
    initTransformation(true);
    mount.axes.enable(true);
    delay(10);
    if (mount.motorsEncoders.enableMotor)
      mount.startSideralTracking();
    PoleSide targetPoleSide = mount.getPoleSide();
    if (mount.targetCurrent.newTargetPoleSide != POLE_NOTVALID) {
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
    mount.alignment.alignPhase   = ALIGN_SELECT;
    mount.alignment.alignStarNum = 2;
    mount.alignment.alignNumStarsSession = mechanicalPole ? 3 : 2;
    mount.alignment.alignPolarThirdPending = false;
    replyShortTrue();
    break;
  }
  case '1':
  case '2':
  case '3':
  case '4':
  case '5':
  case '6':
  case '7':
  case '8':
  case '9': {
    uint8_t starIdx = commandState.command[1] - '0';
    double newTargetHA = haRange(rtk.LST() * 15.0 - mount.targetCurrent.newTargetRA);
    double Lat = *localSite.latitude();
    Coord_EQ EQ_T(0, mount.targetCurrent.newTargetDec * DEG_TO_RAD, newTargetHA * DEG_TO_RAD);
    Coord_HO HO_T = EQ_T.To_Coord_HO(Lat * DEG_TO_RAD, mount.refrOptForGoto());

    // :A3# does not finalize polar pass — use :AP#
    if (starIdx == 3 && mount.alignment.alignPolarThirdPending) {
      replyNothing();
      break;
    }

    // :A3# without pending session — reject (avoids corrupting conv)
    if (starIdx == 3 && !mount.alignment.alignPolarThirdPending) {
      replyNothing();
      break;
    }

    if (mount.alignment.conv.getRefs() == 0)
      mount.syncAzAlt(&HO_T, mount.getPoleSide());
    Coord_IN IN_T = mount.getInstr();
    mount.alignment.conv.addReference(HO_T.direct_Az_S(), HO_T.Alt(), IN_T.Axis1_direct(), IN_T.Axis2());
    if (mount.alignment.conv.isReady()) {
      const bool deferThird = !mount.isAltAZ() && mount.alignment.alignNumStarsSession >= 3 && starIdx == 2;
      if (deferThird) {
        mount.syncAzAlt(&HO_T, mount.getPoleSide());
        mount.alignment.alignPolarThirdPending = true;
        mount.alignment.alignPhase   = ALIGN_RECENTER;
        mount.alignment.alignStarNum = 3;
        mount.alignment.hasValid = false;
      } else {
        mount.alignment.conv.minimizeAxis2();
        mount.alignment.conv.minimizeAxis1(mount.config.identity.mountType == MOUNT_TYPE_GEM ? (Lat >= 0 ? M_PI_2 : -M_PI_2) : 0);
        mount.syncAzAlt(&HO_T, mount.getPoleSide());
        mount.alignment.hasValid = true;
        mount.alignment.alignPhase   = ALIGN_IDLE;
        mount.alignment.alignStarNum = 0;
        mount.alignment.alignPolarThirdPending = false;
      }
    } else {
      mount.alignment.alignPhase   = ALIGN_SELECT;
      mount.alignment.alignStarNum = starIdx + 1;
    }
    mount.config.peripherals.PushtoStatus = PT_OFF;
    replyShortTrue();
    break;
  }
  case 'P':
    // :AP#  Polar / mechanical bolt pass complete (after :A0,m# / :A*,m# defer); TeenAstro extension
    if (commandState.command[2] != 0) {
      replyNothing();
      break;
    }
    if (!mount.alignment.alignPolarThirdPending) {
      replyNothing();
      break;
    }
    alignmentPolarFinalizeFromCurrentTarget();
    replyShortTrue();
    break;
  case 'E': {
    // :AE#  LX200 standard (alignment error / AltAz)
    double val = mount.alignment.conv.getError() * RAD_TO_DEG;
    doubleToDms(commandState.reply, &val, false, true, true);
    strcat(commandState.reply, "#");
    break;
  }
  case 'C':
  case 'A':
    // :AC# :AA#  LX200 standard
    initTransformation(true);
    mount.syncAtHome();
    mount.alignment.autoAlignmentBySync = (commandState.command[1] == 'A');
    mount.alignment.alignPhase   = ALIGN_IDLE;
    mount.alignment.alignStarNum = 0;
    mount.alignment.alignPolarThirdPending = false;
    replyShortTrue();
    break;
  case 'B':
    // :AB#  Abort alignment in progress (clear refs without syncing at home)
    mount.alignment.conv.clean();
    mount.alignment.hasValid = false;
    mount.alignment.alignPhase   = ALIGN_IDLE;
    mount.alignment.alignStarNum = 0;
    mount.alignment.alignPolarThirdPending = false;
    mount.alignment.alignStarName[0] = '\0';
    XEEPROM.write(getMountAddress(EE_Tvalid), 0);
    replyShortTrue();
    break;
  case 'W':
    // :AW#  LX200 standard (save alignment)
    saveAlignModel();
    replyShortTrue();
    break;
  default:
    replyNothing();
    break;
  }
}

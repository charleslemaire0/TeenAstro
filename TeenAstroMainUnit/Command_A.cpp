/**
 * A - Alignment. One file per letter (plan).
 */
#include "Command.h"
#include "ValueToString.h"

// -----------------------------------------------------------------------------
//   A - Alignment  :A0# :A*# :A2# :AE# :AC# :AA# :AW#
// -----------------------------------------------------------------------------
void Command_A() {
  switch (commandState.command[1]) {
  case '0':
    // :A0#  LX200 standard (alignment menu 0)
    initTransformation(true);
    mount.syncAtHome();
    mount.axes.enable(true);
    delay(10);
    if (mount.motorsEncoders.enableMotor)
      mount.startSideralTracking();
    replyShortTrue();
    break;
  case '*': {
    // :A*#  LX200 standard (telescope at target)
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
    replyShortTrue();
    break;
  }
  case '2': {
    // :A2#  LX200 standard (alignment menu 2)
    double newTargetHA = haRange(rtk.LST() * 15.0 - mount.targetCurrent.newTargetRA);
    double Lat = *localSite.latitude();
    Coord_EQ EQ_T(0, mount.targetCurrent.newTargetDec * DEG_TO_RAD, newTargetHA * DEG_TO_RAD);
    Coord_HO HO_T = EQ_T.To_Coord_HO(Lat * DEG_TO_RAD, mount.refrOptForGoto());
    if (mount.alignment.conv.getRefs() == 0)
      mount.syncAzAlt(&HO_T, mount.getPoleSide());
    Coord_IN IN_T = mount.getInstr();
    mount.alignment.conv.addReference(HO_T.direct_Az_S(), HO_T.Alt(), IN_T.Axis1_direct(), IN_T.Axis2());
    if (mount.alignment.conv.isReady()) {
      mount.alignment.conv.minimizeAxis2();
      mount.alignment.conv.minimizeAxis1(mount.config.identity.mountType == MOUNT_TYPE_GEM ? (Lat >= 0 ? M_PI_2 : -M_PI_2) : 0);
      mount.syncAzAlt(&HO_T, mount.getPoleSide());
      mount.alignment.hasValid = true;
    }
    mount.config.peripherals.PushtoStatus = PT_OFF;
    replyShortTrue();
    break;
  }
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

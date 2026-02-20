/**
 * C - Sync. One file per letter (plan).
 */
#include "Command.h"

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
      // :CM# :CS#  LX200 standard
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
      // :CU# sync with User Defined RA DEC  TeenAstro specific
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
      // :CA#  LX200 standard (sync Alt/Az)
      Coord_HO HO_T(0, mount.targetCurrent.newTargetAlt * DEG_TO_RAD, mount.targetCurrent.newTargetAzm * DEG_TO_RAD, true);
      mount.syncAzAlt(&HO_T, targetPoleSide);
      strcpy(commandState.reply, "N/A#");
    }
    break;
    }
  }
}

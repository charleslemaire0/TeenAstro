/**
 * W - Site (waypoint). One file per letter (plan).
 */
#include "Command.h"

// -----------------------------------------------------------------------------
//   W - Site  :W0# :W1# :W2# :W?#  LX200 standard
// -----------------------------------------------------------------------------
void Command_W() {
  switch (commandState.command[1]) {
  case '0':
  case '1':
  case '2': {
    // :W0# :W1# :W2#  LX200 standard (set site)
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
    // :W?#  LX200 standard (get current site index)
    sprintf(commandState.reply, "%d#", localSite.siteIndex());
    break;
  default:
    replyNothing();
    break;
  }
}

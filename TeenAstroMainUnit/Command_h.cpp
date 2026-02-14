/**
 * h - Home / park. One file per letter (plan).
 * All :hx# are TeenAstro specific (Meade h is 16" home search only).
 */
#include "Command.h"

// -----------------------------------------------------------------------------
//   h - Home / park  :hF# :hC# :hB# :hb# :hO# :hP# :hQ# :hS# :hR#
// -----------------------------------------------------------------------------
void Command_h() {
  switch (commandState.command[1]) {
  case 'F':
    // :hF#  TeenAstro specific (reset at home)
    mount.syncAtHome();
    break;
  case 'C':
    // :hC#  TeenAstro specific (goto home)
    if (!mount.goHome())
      replyShortFalse();
    else
      replyShortTrue();
    break;
  case 'B':
    // :hB#  TeenAstro specific (set home)
    if (!mount.setHome()) replyShortFalse();
    else replyShortTrue();
    break;
  case 'b':
    // :hb#  TeenAstro specific (reset home)
    mount.parkHome.homeSaved = false;
    XEEPROM.write(getMountAddress(EE_homeSaved), false);
    mount.initHome();
    replyShortTrue();
    break;
  case 'O':
    // :hO#  TeenAstro specific (sync at park)
    if (!mount.syncAtPark())
      replyShortFalse();
    else
      replyShortTrue();
    break;
  case 'P':
    // :hP#  TeenAstro specific (goto park)
    if (mount.park() == 0)
      replyShortTrue();
    else
      replyShortFalse();
    break;
  case 'Q':
    // :hQ#  TeenAstro specific (set park)
    if (!mount.setPark())
      replyShortFalse();
    else
      replyShortTrue();
    break;
  case 'S':
    // :hS#  TeenAstro specific (park saved?)
    mount.parkHome.parkSaved ? replyShortTrue() : replyShortFalse();
    break;
  case 'R':
    // :hR#  TeenAstro specific (unpark)
    mount.unpark();
    replyShortTrue();
    break;
  default:
    replyNothing();
    break;
  }
}

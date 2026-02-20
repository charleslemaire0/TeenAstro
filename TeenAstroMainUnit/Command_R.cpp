/**
 * R - Slew rate. One file per letter (plan).
 */
#include "Command.h"

// -----------------------------------------------------------------------------
//   R - Slew rate  :RG# :RC# :RM# :RS# :R0#..:R4#  LX200 standard
// -----------------------------------------------------------------------------
void Command_R() {
  int i = 5;
  switch (commandState.command[1]) {
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
  if (!mount.isSlewing()) {
    mount.guiding.recenterGuideRate = i;
    mount.enableGuideRate(mount.guiding.recenterGuideRate);
  }
}

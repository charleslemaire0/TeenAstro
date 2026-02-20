/**
 * D - Distance bars. One file per letter (plan).
 */
#include "Command.h"

// -----------------------------------------------------------------------------
//   D - Distance bars  :D#  LX200 standard (0x7f# if moving, else #)
// -----------------------------------------------------------------------------
void Command_D() {
  if (commandState.command[1] != 0)
    return;
  if (mount.tracking.movingTo) {
    commandState.reply[0] = (char)127;
    commandState.reply[1] = 0;
  } else {
    commandState.reply[0] = 0;
  }
  strcat(commandState.reply, "#");
}

/**
 * U - Precision toggle. One file per letter (plan).
 */
#include "Command.h"

// -----------------------------------------------------------------------------
//   U - Precision toggle  :U#  LX200 standard (low/high precision)
// -----------------------------------------------------------------------------
void Command_U() {
  if (commandState.command[1] == 0) {
    commandState.highPrecision = !commandState.highPrecision;
  }
  replyNothing();
}

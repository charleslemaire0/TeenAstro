/**
 * <ACK> - Mount type (alignment query). One file per letter (plan).
 */
#include "Command.h"

// -----------------------------------------------------------------------------
//   <ACK> - Mount type  LX200 standard
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

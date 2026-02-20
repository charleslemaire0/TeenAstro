/**
 * Q - Halt. One file per letter (plan).
 */
#include "Command.h"

//   Q - Halt  :Q# :Qe# :Qw# :Qn# :Qs#  LX200 standard
void Command_Q() {
  switch (commandState.command[1]) {
  case 0:
    mount.tracking.doSpiral = false;
    if (!mount.isParked()) {
      if (mount.tracking.movingTo)
        mount.abortSlew();
      else {
        mount.stopAxis1();
        mount.stopAxis2();
      }
    }
    break;
  case 'e':
  case 'w':
    if ((!mount.isParked()) && !mount.tracking.movingTo)
      mount.stopAxis1();
    break;
  case 'n':
  case 's':
    if ((!mount.isParked()) && !mount.tracking.movingTo)
      mount.stopAxis2();
    break;
  default:
    replyNothing();
    break;
  }
}

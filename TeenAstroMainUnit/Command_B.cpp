/**
 * B - Reticule brightness. One file per letter (plan).
 */
#include "Command.h"

// -----------------------------------------------------------------------------
//   B - Reticule  :B+# :B-#  LX200 standard
// -----------------------------------------------------------------------------
void Command_B() {
  if (commandState.command[1] != '+' && commandState.command[1] != '-')
    return;
#ifdef RETICULE_LED_PINS
  if (mount.reticule.reticuleBrightness > 255)
    mount.reticule.reticuleBrightness = 255;
  if (mount.reticule.reticuleBrightness < 31)
    mount.reticule.reticuleBrightness = 31;
  if (commandState.command[1] == '-')
    mount.reticule.reticuleBrightness /= 1.4;
  if (commandState.command[1] == '+')
    mount.reticule.reticuleBrightness *= 1.4;
  if (mount.reticule.reticuleBrightness > 255)
    mount.reticule.reticuleBrightness = 255;
  if (mount.reticule.reticuleBrightness < 31)
    mount.reticule.reticuleBrightness = 31;
  analogWrite(RETICULE_LED_PINS, mount.reticule.reticuleBrightness);
#endif
  replyNothing();
}

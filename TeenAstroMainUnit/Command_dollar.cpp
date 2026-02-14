/**
 * $ - Reset / reboot / reinit. One file per letter (plan).
 */
#include "Command.h"

// -----------------------------------------------------------------------------
//   $ - Reset / reboot / reinit
// -----------------------------------------------------------------------------
void Command_dollar() {
  switch (commandState.command[1]) {
  case '$':
    // :$$#  Clean EEPROM  TeenAstro specific
    for (int i = 0; i < XEEPROM.length(); i++)
      XEEPROM.write(i, 0);
    replyShortTrue();
    break;
  case '!':
    // :$!#  Reboot main unit  TeenAstro specific
    mount.motorsEncoders.reboot_unit = true;
    replyShortTrue();
    break;
  case 'X':
    // :$X#  Reinit encoder and motors  TeenAstro specific
    initencoder();
    initmotor(true);
    replyShortTrue();
    break;
  default:
    replyNothing();
    break;
  }
}

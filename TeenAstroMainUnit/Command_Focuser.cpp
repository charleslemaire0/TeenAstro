/**
 * Focuser commands: F (forward focuser protocol to Focus_Serial).
 * One file per letter (plan). :F+# :F-# :FQ# :FF# :FS# LX200 standard.
 */
#include "Command.h"

// -----------------------------------------------------------------------------
//   F - Focuser  :F+# :F-# :F?# etc. (passthrough to focuser hardware)
// -----------------------------------------------------------------------------
void Command_F() {
  if (!mount.config.peripherals.hasFocuser) {
    if (commandState.command[1] == '?') replyLongFalse();
    else replyNothing();
    return;
  }
  bool focuserNoResponse = false;
  bool focuserShortResponse = false;
  char command_out[30] = ":";
  Focus_Serial.flush();
  while (Focus_Serial.available() > 0) Focus_Serial.read();
  strcat(command_out, commandState.command);
  strcat(command_out, "#");

  switch (commandState.command[1])
  {
  case '+':
  case '-':
  case 'g':
  case 'G':
  case 'P':
  case 'Q':
  case 's':
  case 'S':
  case '!':
  case '$':
    focuserNoResponse = true;
    break;
  case 'x':
  case '?':
  case '~':
  case 'M':
  case 'V':
  case 'A':
  case 'a':
    focuserNoResponse = false;
    focuserShortResponse = false;
    break;
  case 'O':
  case 'o':
  case 'I':
  case 'i':
  case 'W':
  case '0':
  case '1':
  case '2':
  case '3':
  case '4':
  case '5':
  case '6':
  case '7':
  case '8':
  case 'c':
  case 'C':
  case 'm':
  case 'r':
    focuserNoResponse = false;
    focuserShortResponse = true;
    break;
  default:
  {
    replyNothing();
    return;
    break;
  }
  }

  Focus_Serial.print(command_out);
  Focus_Serial.flush();

  if (focuserNoResponse)
  {
    replyNothing();
    return;
  }

  unsigned long deadline = millis() + 500;
  int pos = 0;

  while (millis() < deadline)
  {
    if (Focus_Serial.available() <= 0)
    {
      delayMicroseconds(200);
      continue;
    }
    char b = Focus_Serial.read();
    commandState.reply[pos] = b;

    if (focuserShortResponse)
    {
      commandState.reply[pos + 1] = 0;
      if (b != '1')
        replyShortFalse();
      return;
    }

    if (b == '#')
    {
      commandState.reply[pos + 1] = 0;
      return;
    }

    pos++;
    if (pos >= REPLY_BUFFER_LEN - 1)
    {
      replyShortFalse();
      return;
    }
    commandState.reply[pos] = 0;
  }
  replyShortFalse();
}

#include "Command.h"


// process commands
void processCommands()
{
  Command process_command = COMMAND_NONE;

  S_USB.update();
  S_SHC.update();

  process_command = COMMAND_NONE;
  S_USB.getCmdPar(command, process_command);
  S_SHC.getCmdPar(command, process_command);

  if (process_command == COMMAND_NONE)
  {
    return;
  }

  // Handles empty and one char replies
  clearReply();

  switch (command[0])
  {
  case '$':
    Command_dollar();
    break;
  case 'A':
    Command_A();
    break;
  case 'B':
    Command_B();
    break;
  case 'C':
    Command_C();
    break;
  case 'D':
    Command_D();
    break;
  case 'E':
    Command_E();
    break;
  case 'F':
    Command_F();
    break;
  case 'G':
    Command_G();
    break;
  case 'g':
    Command_GNSS();
    break;
  case 'h':
    Command_h();
    break;
  case 'M':
    Command_M();
    break;
  case 'Q':
    Command_Q();
    break;
  case 'R':
    Command_R();
    break;
  case 'S':
    Command_S(process_command);
    break;
  case 'T':
    Command_T();
    break;
  case 'W':
    Command_W();
    break;
  default:
    replyNothing();
    break;
  }

  if (strlen(reply) > 0)
  {
    S_USB.reply(reply, process_command);
    S_SHC.reply(reply, process_command);
  }
  if (reboot_unit)
  {
    reboot();
  }
}

void replyShortTrue()
{
  strcpy(reply, "1");
}

void replyLongTrue()
{
  strcpy(reply, "1#");
}

void replyShortFalse()
{
  strcpy(reply, "0");
}

void replyLongFalse()
{
  strcpy(reply, "0#");
}

void replyLongUnknow()
{
  strcpy(reply, "#");
}

void replyValueSetShort(bool set)
{
  set ? replyShortTrue() : replyShortFalse();
}

void replyNothing()
{
  reply[0] = 0;
}

void clearReply()
{
  for (int i = 0; i < 50; i++)
  {
    reply[i] = 0;
  }
}
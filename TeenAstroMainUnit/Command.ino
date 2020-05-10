#include "Command.h"
// scratch-pad variables
double          f, f1, f2, f3;
int             i, i1, i2, i3, i4, i5;

//enum Command { COMMAND_NONE, COMMAND_SERIAL, COMMAND_SERIAL1, COMMAND_ETHERNET };

// process commands
void processCommands()
{
  Command process_command = COMMAND_NONE;

  S_USB.update();
  S_SHC.update();

  process_command = COMMAND_NONE;
  S_USB.getCmdPar(command, parameter, process_command);
  S_SHC.getCmdPar(command, parameter, process_command);

  if (process_command == COMMAND_NONE)
  {
    return;
  }

  // Handles empty and one char replies
  reply[0] = 0;
  reply[1] = 0;

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
    strcpy(reply, "0");
    break;
  }

  if (strlen(reply) > 0)
  {
    S_USB.reply(reply, process_command);
    S_SHC.reply(reply, process_command);
  }

}
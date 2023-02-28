#include "Global.h"


void processCommandsTask(void *arg)
{
  while(1)
  { 
    processCommands();
    vTaskDelay(CMD_TASK_PERIOD  / portTICK_PERIOD_MS);
  }
}


// process commands
void processCommands()
{
  Command process_command = COMMAND_NONE;

#ifndef __arm__
  S_USB.update();
#endif  
  S_SHC.update();
  process_command = COMMAND_NONE;
  S_USB.getCmdPar(command, process_command);
  S_SHC.getCmdPar(command, process_command);
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
#if 0    
  case 'F':
    Command_F();
    break;
#endif
  case 'G':
    Command_G();
    break;
#if 0
  case 'g':
    Command_GNSS();
    break;
#endif
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
#ifndef __arm__
    S_USB.reply(reply, process_command);
#endif    
    S_SHC.reply(reply, process_command);
  }

}
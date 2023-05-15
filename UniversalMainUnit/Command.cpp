#include "Global.h"

#if 0
// useful for tracing commands with debugger

#define BUFFER_SIZE 2000
char cmdBuffer[BUFFER_SIZE];
static unsigned long cmdPtr = 0;
#endif

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

  S_USB.update();
  S_SHC.update();
  process_command = COMMAND_NONE;
  S_USB.getCmdPar(command, process_command);
  S_SHC.getCmdPar(command, process_command);
  if (process_command == COMMAND_NONE)
  {
    return;
  }

#if 0
  strcpy(&cmdBuffer[cmdPtr], command);
  cmdPtr += strlen(command) + 1;
  if (cmdPtr >= BUFFER_SIZE)
    cmdPtr = 0;
#endif

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
    HAL_reboot();
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

void replyLongUnknown()
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
#include "Global.h"
//----------------------------------------------------------------------------------
//   F - Focuser
void Command_F()
{
  if (command[1] == '?') 
    replyLongFalse();
  else 
    replyNothing();
}

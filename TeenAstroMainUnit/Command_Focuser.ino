#include "Command.h"
//----------------------------------------------------------------------------------
//   F - Focuser
void Command_F()
{
  if (!hasFocuser )
  {
    if (command[1] == '?') strcpy(reply, "0#");
    else strcpy(reply, "0");
    return;
  }
  bool focuserNoResponse = false;
  bool focuserShortResponse = false;
  char command_out[30] = ":";
  Serial2.flush();
  while (Serial2.available() > 0) Serial2.read();
  strcat(command_out, command);
  strcat(command_out, "#");

  switch (command[1])
  {
  case '+':
  case '-':
  case 'g':
  case 'G':
  case 'P':
  case 'Q':
  case 's':
  case 'S':
    focuserNoResponse = true;
    break;
  case 'x':
  case '?':
  case '~':
  case 'M':
  case 'V':
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
    strcpy(reply, "0");
    return;
    break;
  }
  }

  Serial2.print(command_out);
  Serial2.flush();

  if (!focuserNoResponse)
  {
    unsigned long start = millis();
    int pos = 0;
    char b = 0;
    while (millis() - start < 40)
    {
      if (Serial2.available() > 0)
      {
        b = Serial2.read();
        if (b == '#' && !focuserShortResponse)
        {
          reply[pos] = b;
          reply[pos+1] = 0;
          return;
        }
        reply[pos] = b;
        pos++;
        if (pos > 49)
        {
          strcpy(reply, "0");
          return;
        }
        reply[pos] = 0;
        if (focuserShortResponse)
        {
          if (b != '1')
            strcpy(reply, "0");
          return;
        }
      }

    }
    strcpy(reply, "0");
  }
  else
  {
    reply[0] = 0;
  }
}
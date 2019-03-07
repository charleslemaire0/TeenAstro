
//----------------------------------------------------------------------------------
//   F - Focuser
void Command_F()
{
  quietReply = true; // the focuser is responding
  boolean focuserNoResponse = false;
  boolean focuserShortResponse = false;
  char command_out[30];
  Serial2.flush();
  while (Serial2.available() > 0) Serial2.read();

  switch (command[1])
  {
  case '+':
  case '-':
  case 'P':
  case 'Q':
  case 'O':
  case 'o':
  case 'I':
  case 'i':
  case '?':
  case '~':
  case 'M':
  case 'V':
    memcpy(command_out, ":F_#", sizeof(":F_#"));
    command_out[2] = command[1];
    break;
  case 'g':
  case 'x':
    command_out[0] = ':';
    command_out[1] = 'F';
    command_out[2] = command[1];
    command_out[3] = parameter[0];
    command_out[4] = '#';
    command_out[5] = 0;
    break;
  case 's':
  {
    command_out[0] = ':';
    command_out[1] = 'F';
    command_out[2] = command[1];
    command_out[3] = parameter[0];
    command_out[4] = ' ';
    int pos = 5;
    for (unsigned int k = 0; k + 1 < strlen(parameter); k++)
    {
      command_out[pos] = parameter[k + 1];
      pos++;
    }
    command_out[pos] = '#';
    command_out[pos + 1] = 0;
  }
  break;
  case 'S':
  case 'G':
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
  {
    command_out[0] = ':';
    command_out[1] = 'F';
    command_out[2] = command[1];
    int k = strlen(parameter);
    int p = 3;
    if (k > 0)
    {
      command_out[p] = ' ';
      p++;
      memcpy(&command_out[p], &parameter[0], k * sizeof(char));
    }
    command_out[p + k] = '#';
    command_out[p + k + 1] = 0;

    break;
  }

  break;
  default:
  {
    break;
  }
  }

  switch (command[1])
  {
  case '+':
  case '-':
  case 'P':
  case 'Q':
  case 'G':
  case 'g':
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
    focuserNoResponse = false;
    focuserShortResponse = true;
    break;
  default:
  {
    commandError = true;
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
          quietReply = true;
          return;
        }
        reply[pos] = b;
        pos++;
        if (pos > 49)
        {
          commandError = true;
          return;
        }
        reply[pos] = 0;
        if (focuserShortResponse)
        {
          quietReply = false;
          if (b != '1')
            commandError = true;
          return;
        }
      }

    }
    commandError = true;
  }
}
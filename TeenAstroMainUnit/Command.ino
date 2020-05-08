#include "Command.h"
// -----------------------------------------------------------------------------------
// Command processing
// these turn on and off checksum error correction on the serial ports, default=OFF
bool         serial_zero_ready = false;
bool         serial_one_ready = false;

// scratch-pad variables
double          f, f1, f2, f3;
int             i, i1, i2, i3, i4, i5;

//enum Command { COMMAND_NONE, COMMAND_SERIAL, COMMAND_SERIAL1, COMMAND_ETHERNET };

// process commands
void processCommands()
{
  Command process_command = COMMAND_NONE;
  bool supress_frame = false;
  if ((Serial.available() > 0) && (!serial_zero_ready))
  {
    serial_zero_ready = buildCommand_serial_zero(Serial.read());
  }
  if ((Serial1.available() > 0) && (!serial_one_ready))
  {
    serial_one_ready = buildCommand_serial_one(Serial1.read());
  }

  process_command = COMMAND_NONE;
  if (serial_zero_ready)
  {
    strcpy(command, command_serial_zero);
    strcpy(parameter, parameter_serial_zero);
    serial_zero_ready = false;
    clearCommand_serial_zero();
    process_command = COMMAND_SERIAL;
  }
  else if (serial_one_ready)
  {
    strcpy(command, command_serial_one);
    strcpy(parameter, parameter_serial_one);
    serial_one_ready = false;
    clearCommand_serial_one();
    process_command = COMMAND_SERIAL1;
  }
  else
  {
    return;
  }
  if (process_command)
  {
    // Command is two chars followed by an optional parameter...
    commandError = false;

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
      Command_M(supress_frame);
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
      commandError = true;
      break;
    }

    if (!quietReply)
    {
      if (commandError)
        reply[0] = '0';
      else
        reply[0] = '1';
      reply[1] = 0;
      supress_frame = true;
    }


    if (strlen(reply) > 0)
    {
      if (process_command == COMMAND_SERIAL)
      {
        if (!supress_frame) strcat(reply, "#");
        Serial.print(reply);
      }

      if (process_command == COMMAND_SERIAL1)
      {
        if (!supress_frame) strcat(reply, "#");
        Serial1.print(reply);
      }
    }

    quietReply = false;
  }
}

// Build up a command
bool buildCommand_serial_zero(char c)
{
  // (chr)6 is a special status command for the LX200 protocol
  if ((c == (char)6) && (bufferPtr_serial_zero == 0))
  {
    mountType == isAltAZ() ? Serial.print("A") : Serial.print("P");
  }

  // ignore spaces/lf/cr, dropping spaces is another tweek to allow compatibility with LX200 protocol
  if ((c != (char)32) && (c != (char)10) && (c != (char)13) && (c != (char)6))
  {
    command_serial_zero[bufferPtr_serial_zero] = c;
    bufferPtr_serial_zero++;
    command_serial_zero[bufferPtr_serial_zero] = (char)0;
    if (bufferPtr_serial_zero > 22)
    {
      bufferPtr_serial_zero = 22;
    }   // limit maximum command length to avoid overflow, c2+p16+cc2+eol2+eos1=23 bytes max ranging from 0..22
  }

  if (c == '#')
  {
    // validate the command frame, normal command
    if ((bufferPtr_serial_zero > 1) && (command_serial_zero[0] == ':') &&
      (command_serial_zero[bufferPtr_serial_zero - 1] == '#'))
    {
      command_serial_zero[bufferPtr_serial_zero - 1] = 0;
    }
    else
    {
      clearCommand_serial_zero();
      return false;
    }



    // break up the command into a two char command and the remaining parameter
    // the parameter can be up to 16 chars in length
    memmove(parameter_serial_zero, (char *)&command_serial_zero[3], 17);

    // the command is either one or two chars in length
    command_serial_zero[3] = 0;
    memmove(command_serial_zero, (char *)&command_serial_zero[1], 3);

    return true;
  }
  else
  {
    return false;
  }
}

// clear commands
bool clearCommand_serial_zero()
{
  bufferPtr_serial_zero = 0;
  command_serial_zero[bufferPtr_serial_zero] = (char)0;
  return true;
}

// Build up a command
bool buildCommand_serial_one(char c)
{
  // (chr)6 is a special status command for the LX200 protocol
  if ((c == (char)6) && (bufferPtr_serial_one == 0))
  {
    mountType == isAltAZ() ? Serial1.print("A") : Serial1.print("P");
  }

  // ignore spaces/lf/cr, dropping spaces is another tweek to allow compatibility with LX200 protocol
  if ((c != (char)32) && (c != (char)10) && (c != (char)13) && (c != (char)6))
  {
    command_serial_one[bufferPtr_serial_one] = c;
    bufferPtr_serial_one++;
    command_serial_one[bufferPtr_serial_one] = (char)0;
    if (bufferPtr_serial_one > 22)
    {
      bufferPtr_serial_one = 22;
    }   // limit maximum command length to avoid overflow, c2+p16+cc2+eol2+eos1=23 bytes max ranging from 0..22
  }

  if (c == '#')
  {
    // validate the command frame, normal command
    if ((bufferPtr_serial_one > 1) && (command_serial_one[0] == ':') &&
      (command_serial_one[bufferPtr_serial_one - 1] == '#'))
    {
      command_serial_one[bufferPtr_serial_one - 1] = 0;
    }
    else
    {
      clearCommand_serial_one();
      return false;
    }
    // break up the command into a two char command and the remaining parameter
    // the parameter can be up to 16 chars in length
    memmove(parameter_serial_one, (char *)&command_serial_one[3], 17);

    // the command is either one or two chars in length
    command_serial_one[3] = 0;
    memmove(command_serial_one, (char *)&command_serial_one[1], 3);

    return true;
  }
  else
  {
    return false;
  }
}

// clear commands
bool clearCommand_serial_one()
{
  bufferPtr_serial_one = 0;
  command_serial_one[bufferPtr_serial_one] = (char)0;
  return true;
}


#include "Command.h"
// -----------------------------------------------------------------------------------
// Command processing
// these turn on and off checksum error correction on the serial ports, default=OFF
#define CHKSUM0_OFF // default _OFF: as required for OnStep ASCOM driver
#define CHKSUM1_OFF // default _OFF: as required for OnStep Controller2 Android App (and others)
boolean         serial_zero_ready = false;
boolean         serial_one_ready = false;
#if defined(W5100_ON)
boolean         ethernet_ready = false;
#endif

// scratch-pad variables
double          f, f1, f2, f3;
int             i, i1, i2;
byte            b;
unsigned long   _coord_t = 0;
double          _dec, _ra;

//enum Command { COMMAND_NONE, COMMAND_SERIAL, COMMAND_SERIAL1, COMMAND_ETHERNET };

// process commands
void processCommands()
{
  Command process_command = COMMAND_NONE;

  boolean supress_frame = false;


  if ((Serial_available() > 0) && (!serial_zero_ready))
  {
    serial_zero_ready = buildCommand_serial_zero(Serial_read());
  }

  if ((Serial1_available() > 0) && (!serial_one_ready))
  {
    serial_one_ready = buildCommand_serial_one(Serial1_read());
  }

#if defined(W5100_ON)
  if ((Ethernet_available() > 0) && (!ethernet_ready))
  {
    ethernet_ready = buildCommand_ethernet(Ethernet_read());
  }

  if (Serial_transmit() || Serial1_transmit() || Ethernet_transmit())
    return;
#else
  if (Serial_transmit() || Serial1_transmit()) return;
#endif
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

#if defined(W5100_ON)
  else if (ethernet_ready)
  {
    strcpy(command, command_ethernet);
    strcpy(parameter, parameter_ethernet);
    ethernet_ready = false;
    clearCommand_ethernet();
    process_command = COMMAND_ETHERNET;
  }
#endif
  else
  {
#if defined(W5100_ON)
#if (defined(__arm__) && defined(TEENSYDUINO))
    Ethernet_www();
#else
    if (!Ethernet_cmd_busy()) Ethernet_www();
#endif
#endif
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
    case '%':
      Command_pct();
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
    case 'G':
      Command_G();
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
#ifdef CHKSUM0_ON
        // calculate the checksum
        char    HEXS[3] = "";
        byte    cks = 0;
        for (int cksCount0 = 0; cksCount0 < strlen(reply); cksCount0++)
        {
          cks += reply[cksCount0];
        }

        sprintf(HEXS, "%02X", cks);
        strcat(reply, HEXS);
#endif
        if (!supress_frame) strcat(reply, "#");
        Serial_print(reply);
      }

      if (process_command == COMMAND_SERIAL1)
      {


#ifdef CHKSUM1_ON
        // calculate the checksum
        char    HEXS[3] = "";
        byte    cks = 0;
        for (int cksCount0 = 0; cksCount0 < strlen(reply); cksCount0++)
        {
          cks += reply[cksCount0];
        }

        sprintf(HEXS, "%02X", cks);
        strcat(reply, HEXS);
#endif
        if (!supress_frame) strcat(reply, "#");
        Serial1_print(reply);
      }

#if defined(W5100_ON)
      if (process_command == COMMAND_ETHERNET)
      {
#ifdef CHKSUM0_ON
        // calculate the checksum
        char    HEXS[3] = "";
        byte    cks = 0;
        for (int cksCount0 = 0; cksCount0 < strlen(reply); cksCount0++)
        {
          cks += reply[cksCount0];
        }

        sprintf(HEXS, "%02X", cks);
        strcat(reply, HEXS);
#endif
        if (!supress_frame) strcat(reply, "#");
        Ethernet_print(reply);
      }
#endif
    }

    quietReply = false;
  }
}

// Build up a command
boolean buildCommand_serial_zero(char c)
{
  // (chr)6 is a special status command for the LX200 protocol
  if ((c == (char)6) && (bufferPtr_serial_zero == 0))
  {
    mountType == MOUNT_TYPE_ALTAZM ? Serial_print("A") : Serial_print("P");
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

#ifdef CHKSUM0_ON
    // checksum the data, for example ":11111126".  I don't include the command frame in the checksum.  The error response is a checksumed null string "00#\r\n" which means re-transmit.
    byte    len = strlen(command_serial_zero);
    byte    cks = 0;
    for (int cksCount0 = 1; cksCount0 < len - 2; cksCount0++)
    {
      cks += command_serial_zero[cksCount0];
    }

    char    chkSum[3];
    sprintf(chkSum, "%02X", cks);
    if (!((chkSum[0] == command_serial_zero[len - 2]) &&
      (chkSum[1] == command_serial_zero[len - 1])))
    {
      clearCommand_serial_zero();
      Serial_print("00#");
      return false;
    }

    --len;
    command_serial_zero[--len] = 0;
#endif

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
boolean clearCommand_serial_zero()
{
  bufferPtr_serial_zero = 0;
  command_serial_zero[bufferPtr_serial_zero] = (char)0;
  return true;
}

// Build up a command
boolean buildCommand_serial_one(char c)
{
  // (chr)6 is a special status command for the LX200 protocol
  if ((c == (char)6) && (bufferPtr_serial_one == 0))
  {
    mountType == MOUNT_TYPE_ALTAZM ? Serial1_print("A") : Serial1_print("P");
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

#ifdef CHKSUM1_ON
    // checksum the data, as above.
    byte    len = strlen(command_serial_one);
    byte    cks = 0;
    for (int cksCount0 = 1; cksCount0 < len - 2; cksCount0++)
    {
      cks = cks + command_serial_one[cksCount0];
    }

    char    chkSum[3];
    sprintf(chkSum, "%02X", cks);
    if (!((chkSum[0] == command_serial_one[len - 2]) &&
      (chkSum[1] == command_serial_one[len - 1])))
    {
      clearCommand_serial_one();
      Serial1_print("00#");
      return false;
    }

    --len;
    command_serial_one[--len] = 0;
#endif

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
boolean clearCommand_serial_one()
{
  bufferPtr_serial_one = 0;
  command_serial_one[bufferPtr_serial_one] = (char)0;
  return true;
}


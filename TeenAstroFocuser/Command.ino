// 
// 
// 
#include "Global.h"
#include "ConfigStepper.h"
#include "Command.h"

#define INPUT_SIZE 30

void Command_move(int sign, double& t)
{
  unsigned int nextposition = position + sign;
  if (inlimit(nextposition))
  {
    Go(storage.minSpeed, sign, t);
  }
  else
  {
    currSpeed = 0;
    t = 0;
  }
}
void Command_stop(int sign)
{
  Stop(sign);
}

void SerCom::Get_Command()
{
  m_hasReceivedCommand = false;
  if (ser.available() > 3)
  {
    unsigned long start = millis();
    int pos = 0;
    char b = 0;
    char input[INPUT_SIZE];
    while (millis() - start < 50 && !m_hasReceivedCommand)
    {
      if (ser.available() > 0)
      {
        b = ser.read();
        input[pos] = b;
        pos++;
        if (pos == INPUT_SIZE)
          pos = INPUT_SIZE - 1;
        input[pos] = 0;
        if (b == '#')
        {
          m_hasReceivedCommand = true;
          while (ser.available() > 0)
            b = ser.read();
          break;
        }
      }
    }
    int size = strlen(input);
    if (size < 4 || input[0] != ':' || input[1] != 'F')
    {
      m_hasReceivedCommand = false;
    }
    // Read each command pair
    m_command = input[2];
    char* separator = strchr(input, ' ');
    m_valuedefined = false;
    if (separator != 0)
    {
      separator++;
      m_valuedefined = true;
      m_value = atoi(separator);
    }

  }
}

void SerCom::Command_Check(void)
{
  if (!m_hasReceivedCommand)
    return;
  // Get next command from ser (add 1 for final 0)

  switch (m_command)
  {
  case AzCmd_Help:
    sayHello();
    ser.println("$ Commands");
    ser.println("$ H Help, + start Focus in, - Start Focus out, * Stop Focus in, : Stop Focus in, Q stop, G Goto, P Park, S Sync, W Write, ?");
    ser.println("$ Settings");
    ser.println("$ 0 startP, 1 maxP, 2 minS , 3 maxS, 4 cmdAcc, 5 mAcc, 6 mDec");
    ser.flush();
    break;
  case AzCmd_Version:
    sayHello();
    break;
  case FocCmd_Halt:
    halt = true;
    mdirOUT = LOW;
    mdirIN = LOW;
    break;
  case FocCmd_Goto:
    if (m_valuedefined)
    {
      halt = false;
      MoveTo(m_value);
    }
    break;
  case FocCmd_Park:
    halt = false;
    MoveTo(storage.startPosition);
    break;
  case FocCmd_Sync:      // "reset" position
    setvalue(m_valuedefined, m_value, 0, storage.maxPosition, position);
    writePos();
    break;
  case FocCmd_Write:
    saveConfig();
    ser.print("1");
    ser.flush();
    break;
  case FocCmd_startPosition:
    setvalue(m_valuedefined, m_value, 0u, 65535u, storage.startPosition);
    break;
  case FocCmd_maxPosition:
    setvalue(m_valuedefined, m_value, 0u, 65535u, storage.maxPosition);
    break;
  case FocCmd_maxSpeed:
    setvalue(m_valuedefined, (uint8_t)m_value, 1u, 100u, storage.maxSpeed);
    break;
  case FocCmd_minSpeed:
    setvalue(m_valuedefined, (uint8_t)m_value, 1u, 100u, storage.minSpeed);
    break;
  case FocCmd_cmdAcc:
    setvalue(m_valuedefined, (uint8_t)m_value, 1u, 100u, storage.cmdAcc);
    break;
  case FocCmd_manualAcc:
    setvalue(m_valuedefined, (uint8_t)m_value, 1u, 100u, storage.manAcc);
    break;
  case FocCmd_manualDec:
    setvalue(m_valuedefined, (uint8_t)m_value, 1u, 100u, storage.manDec);
    break;
  case FocCmd_Inv:
    setbool(m_valuedefined, m_value, storage.reverse);
    break;
  case CmdDumpState: // "?" dump state including details
    dumpState();
    break;
  case CmdDumpConfig:
    dumpConfig();
    break;
  case Char_CR:  // ignore cr
  case Char_Spc:
  case Char_254:
    break;
  default:
    break;
  }
}

void SerCom::MoveRequest(void)
{
  if (!m_hasReceivedCommand)
    return ;
  switch (m_command)
  {
  case CmdDumpState: // "?" dump state including details
    dumpState();
    m_hasReceivedCommand = false;
    break;
  case FocCmd_in:
    mdirIN = HIGH;
    m_hasReceivedCommand = false;
    break;
  case FocCmd_in_stop:
    mdirIN = LOW;
    m_hasReceivedCommand = false;
    break;
  case FocCmd_out:
    mdirOUT = HIGH;
    m_hasReceivedCommand = false;
    break;
  case FocCmd_out_stop:
    mdirOUT = LOW;
    m_hasReceivedCommand = false;
    break;
  default:
    break;
  }
}

void SerCom::HaltRequest(void)
{
  Get_Command();
  if (!m_hasReceivedCommand)
    return;
  switch (m_command)
  {
  case CmdDumpState: // "?" dump state including details
    dumpState();
    m_hasReceivedCommand = false;
    break;
  case FocCmd_in:
  case FocCmd_in_stop:
  case FocCmd_out:
  case FocCmd_out_stop:
  case FocCmd_Halt:
    mdirOUT = LOW;
    mdirIN = LOW;
    halt = true;
    m_hasReceivedCommand = false;
    break;
  default:
    return;
  }
}

void SerCom::setvalue(bool valuedefined, unsigned int value, uint8_t min, uint8_t max, uint8_t &adress)
{
  if (valuedefined && value >= min && value <= max)
  {
    adress = value;
    ser.print("1");
  }
  else
    ser.print("0");
  ser.flush();
}

void SerCom::setvalue(bool valuedefined, unsigned int value, unsigned int min, unsigned int max, unsigned int &adress)
{
  if (valuedefined && value >= min && value <= max)
  {
    adress = value;
    ser.print("1");
  }
  else
    ser.print("0");
  ser.flush();
}

void SerCom::setbool(bool valuedefined, unsigned int value, bool  &adress)
{
  if (valuedefined && (value == 0 || value == 1))
  {
    adress = value;
    ser.print("1");
  }
  else
    ser.print("0");
  ser.flush();
}

void SerCom::dumpConfig()
{
  char buf[50];
  sprintf(buf, "~%05u %05u %03u %03u %03u %03u %03u %1u#",
    storage.startPosition,
    storage.maxPosition,
    storage.minSpeed,
    storage.maxSpeed,
    storage.cmdAcc,
    storage.manAcc,
    storage.manDec,
    storage.reverse);
  ser.print(buf);
  ser.flush();
}

void SerCom::dumpState()
{
  char buf[20];
  sprintf(buf, "?%05u %03u#", position, currSpeed);
  ser.print(buf);
  ser.flush();
}

void SerCom::updateGoto(void)
{
  HaltRequest();
}

void SerCom::sayHello(void)
{
  ser.print("$ ");
  ser.print(PROJECT);
  ser.print(" ");
  ser.print(BOARDINFO);
  ser.print(" ");
  ser.print(Version);
  ser.print("#");
  ser.flush();
}

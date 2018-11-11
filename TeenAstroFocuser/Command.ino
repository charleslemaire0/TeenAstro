// 
// 
// 

#include "Command.h"

#define INPUT_SIZE 30
void update_stepper()
{
  stepper.run();
}



void Command_stop()
{
  Stop();
}

void Command_Run()
{
  Run();
}

bool SerCom::Get_Command()
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
  return m_hasReceivedCommand;
}

void SerCom::Command_Check(void)
{
  if (!m_hasReceivedCommand)
    return;
  // Get next command from ser (add 1 for final 0)
  m_hasReceivedCommand = false;
  switch (m_command)
  {
  case AzCmd_Help:
    sayHello();
    ser.println("$ Commands");
    ser.println("$ H Help");
    ser.println("$ + start Focus in, - Start Focus out,  all without reply");
    ser.println("$ O start Focus in, I Start Focus out, o Stop Focus in, i Stop Focus in,  all with reply");
    ser.println("Q stop, G Goto, P Park, S Sync, W Write, ?");
    ser.println("$ Settings");
    ser.println("$ 0 startP, 1 maxP, 2 minS , 3 maxS, 4 cmdAcc, 5 mAcc, 6 mDec, 7 mRev, 8 mInp");
    ser.flush();
    break;
  case AzCmd_Version:
    sayHello();
    break;
  case FocCmd_Halt:
    halt = true;
    stepper.setAcceleration(storage.manDec*100);
    stepper.stop();
    mdirOUT = LOW;
    mdirIN = LOW;
    break;
  case FocCmd_Goto:
    if (m_valuedefined)
    {
      halt = false;
      stepper.setAcceleration(storage.cmdAcc*100);
      MoveTo((unsigned long)m_value * storage.resolution);
    }
    break;
  case FocCmd_Park:
    halt = false;
    stepper.setAcceleration(storage.cmdAcc*100);
    MoveTo(storage.startPosition);
    break;
  case FocCmd_Sync:      // "reset" position
    unsigned long position;
    setvalue(m_valuedefined, (unsigned long)m_value * storage.resolution, 0, storage.maxPosition, position);
    stepper.setCurrentPosition((long)position);
    writePos();
    break;
  case FocCmd_Write:
    saveConfig();
    ser.print("1");
    ser.flush();
    break;
  case FocCmd_startPosition:
    setvalue(m_valuedefined, (unsigned long)m_value * storage.resolution, 0UL, 2000000000UL, storage.startPosition);
    break;
  case FocCmd_maxPosition:
    setvalue(m_valuedefined, (unsigned long)m_value * storage.resolution, 0UL, 2000000000UL, storage.maxPosition);
    break;
  case FocCmd_highSpeed:
    setvalue(m_valuedefined, m_value, 1u, 999u, storage.highSpeed);
    stepper.setMaxSpeed(storage.highSpeed*pow(2,storage.micro));
    break;
  case FocCmd_lowSpeed:
    setvalue(m_valuedefined, m_value, 1u, 999u, storage.lowSpeed);
    break;
  case FocCmd_cmdAcc:
    setvalue(m_valuedefined, (uint8_t)m_value, 1u, 200u, storage.cmdAcc);
    break;
  case FocCmd_manualAcc:
    setvalue(m_valuedefined, (uint8_t)m_value, 1u, 100u, storage.manAcc);
    break;
  case FocCmd_manualDec:
    setvalue(m_valuedefined, (uint8_t)m_value, 1u, 100u, storage.manDec);
    break;
  case FocCmd_Inv:
    setbool(m_valuedefined, m_value, storage.reverse);
    stepper.setPinsInverted(storage.reverse, false, false);
    break;
  case FocCmd_inpulse:
    setvalue(m_valuedefined, m_value, 1u, 512u, storage.resolution);
    checkvalue();
    break;
  case FocCmd_current:
    setvalue(m_valuedefined, m_value, 10u, 100u, storage.curr);
    driver.rms_current(storage.curr);
    break;
  case FocCmd_micro:
    setvalue(m_valuedefined, m_value, 2u, 7, storage.micro);
    driver.microsteps(pow(2,storage.curr));
    break;
  case CmdDumpState: // "?" dump state including details
    dumpState();
    break;
  case CmdDumpConfig:
    dumpConfig();
    break;
  case CmdDumpConfigMotor:
    dumpConfigMotor();
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
  case FocCmd_in_wor:
    mdirIN = HIGH;
    m_hasReceivedCommand = false;
    break;
  case FocCmd_out_wor:
    mdirOUT = HIGH;
    m_hasReceivedCommand = false;
    break;
  case FocCmd_in:
    mdirIN = HIGH;
    m_hasReceivedCommand = false;
    ser.print("1");
    ser.flush();
    break;
  case FocCmd_in_stop:
    mdirIN = LOW;
    m_hasReceivedCommand = false;
    ser.print("1");
    ser.flush();
    break;
  case FocCmd_out:
    mdirOUT = HIGH;
    m_hasReceivedCommand = false;
    ser.print("1");
    ser.flush();
    break;
  case FocCmd_out_stop:
    mdirOUT = LOW;
    m_hasReceivedCommand = false;
    ser.print("1");
    ser.flush();
    break;
  case FocCmd_Halt:
    mdirOUT = LOW;
    mdirIN = LOW;
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
  case FocCmd_in_wor:
  case FocCmd_out_wor:
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

void SerCom::setvalue(bool valuedefined, unsigned long value, unsigned long min, unsigned long max, unsigned long &adress)
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
  sprintf(buf, "~%05u %05u %03u %03u %03u %03u %03u#",
    (unsigned int)(storage.startPosition / storage.resolution),
    (unsigned int)(storage.maxPosition / storage.resolution),
    storage.lowSpeed,
    storage.highSpeed,
    storage.cmdAcc,
    storage.manAcc,
    storage.manDec
   );
  ser.print(buf);
  ser.flush();
}

void SerCom::dumpConfigMotor()
{
  char buf[50];
  sprintf(buf, "~%1u %1u  %03u %03u#",
    (unsigned int)(storage.reverse),
    (unsigned int)(storage.micro),
    (unsigned int)(storage.resolution),
    (unsigned int)(storage.curr));
  ser.print(buf);
  ser.flush();
}

void SerCom::dumpState()
{
  char buf[20];

  unsigned int pos = (unsigned int)(stepper.currentPosition() / storage.resolution);
  if (pos > 65535)
  {
    stepper.setCurrentPosition( 65535 * storage.resolution);
  }
  sprintf(buf, "?%05u %03u#", (unsigned int)(stepper.currentPosition() /storage.resolution), (unsigned int)abs(stepper.speed() / pow(2, storage.micro)));
  ser.print(buf);
}

void SerCom::updateGoto(void)
{
  return;
  if (millis() - m_lastupdate > 10)
  {
    //HaltRequest();
    m_lastupdate = millis();
  }
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

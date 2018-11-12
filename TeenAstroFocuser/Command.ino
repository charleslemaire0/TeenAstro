// 
// 
// 

#include "Command.h"

void modeGoto()
{
  stepper.setAcceleration(100.*cmdAcc->get());
  stepper.setMaxSpeed(highSpeed->get()*pow(2, micro->get()));
}

void modeMan()
{
  stepper.setAcceleration(100.*manAcc->get());
  stepper.setMaxSpeed(lowSpeed->get()*pow(2, micro->get()));
}


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
  if (ser.peek() == ':' && ser.available() > 3)
  {
    unsigned long start = millis();
    int pos = 0;
    char b = 0;
    char input[INPUT_SIZE];
    while (!m_hasReceivedCommand)
    {
      if (ser.available() > 0)
      {
        
        b = ser.read();
        input[pos] = b;
        if (input[pos] == '#')
        {
          m_hasReceivedCommand = true;         
          while (ser.available() > 0)
            b = ser.read();
          break;
        }
        if (pos == INPUT_SIZE-1)
        {
          m_hasReceivedCommand = false;
          while (ser.available() > 0)
            b = ser.read();
          break;
        }
        pos++;
        input[pos] = 0;
      }
    }
    int size = strlen(input);
    if (size < 4 || input[1] != 'F')
    {
      m_hasReceivedCommand = false;
    }
    // Read each command pair
    m_command = input[2];
    m_parameter = input[3];
    m_valuedefined = false;
    if (m_parameter == ' ')
    {
      m_valuedefined = true;
      m_value = atoi(&input[4]);
    }
    else
    {
      memcpy(m_input, input, sizeof(m_input));
    }
  }
  else 
  {
    while (ser.available() > 0)
      ser.read();
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
    ser.println("$ 0 startP, 1 maxP, 2 lowS , 3 highS, 4 cmdAcc, 5 mAcc, 6 mDec, 7 mRev, 8 mInp");
    ser.flush();
    break;
  case AzCmd_Version:
    sayHello();
    break;
  case FocCmd_Halt:
    halt = true;
    stepper.setAcceleration(100.*manDec->get());
    stepper.stop();
    mdirOUT = LOW;
    mdirIN = LOW;
    break;
  case FocCmd_Goto:
    switch (m_parameter)
    {
    case ' ':
    {
      if (m_valuedefined)
      {
        halt = false;
        modeGoto();
        MoveTo((unsigned long)m_value * resolution->get());
      }
      break;
    }
    case '0':
    case '1':
    case '2':
    case '3':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
    {
      int val = m_parameter - '0';
      if (PositionList[val]->isvalid())
      {
        halt = false;
        modeGoto();
        MoveTo(PositionList[val]->getPosition());
      }
      break;
    }
    default:
      break;
    }

    break;
  case FocCmd_Park:
    halt = false;
    modeGoto();
    MoveTo(startPosition->get());
    break;
  case FocCmd_Sync:      // "reset" position
    switch (m_parameter)
    {
    case ' ':
    {
      unsigned long position;
      if (setvalue(m_valuedefined, (unsigned long)m_value * resolution->get(), 0, maxPosition->get(), position))
      {
        stepper.setCurrentPosition((long)position);
        writePos();
      }
      break;
    }
    case '0':
    case '1':
    case '2':
    case '3':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
    {
      int val = m_parameter - '0';
      m_value = atoi(&m_input[4]);
      PositionList[val]->set(&m_input[10], (unsigned long)m_value* resolution->get());
      break;
    }
    default:
      break;
    }
    break;
  case FocCmd_startPosition:
    setvalue(m_valuedefined, (unsigned long)m_value * resolution->get(), startPosition);
    break;
  case FocCmd_maxPosition:
    setvalue(m_valuedefined, (unsigned long)m_value * resolution->get(), maxPosition);
    break;
  case FocCmd_highSpeed:
    if (setvalue(m_valuedefined, m_value, highSpeed))
    {
      stepper.setMaxSpeed(highSpeed->get()*pow(2, micro->get()));
    }
 
    break;
  case FocCmd_lowSpeed:
    setvalue(m_valuedefined, m_value, lowSpeed);
    break;
  case FocCmd_cmdAcc:
    setvalue(m_valuedefined, (uint8_t)m_value, cmdAcc);
    break;
  case FocCmd_manualAcc:
    setvalue(m_valuedefined, (uint8_t)m_value, manAcc);
    break;
  case FocCmd_manualDec:
    setvalue(m_valuedefined, (uint8_t)m_value, manDec);
    break;
  case FocCmd_Inv:
    setvalue(m_valuedefined, m_value, reverse);
    stepper.setPinsInverted(reverse->get(), false, false);
    break;
  case FocCmd_inpulse:
    setvalue(m_valuedefined, m_value, resolution);
    break;
  case FocCmd_current:
    if (setvalue(m_valuedefined, m_value, curr))
    {
      driver.rms_current(curr->get());
    }
    break;
  case FocCmd_micro:
    if (setvalue(m_valuedefined, m_value, micro))
    {
      driver.microsteps(pow(2, micro->get()));
    }
    break;
  case CmdDumpState: // "?" dump state including details
    dumpState();
    break;
  case CmdDumpConfig:
    switch (m_parameter)
    {
    case '#':
      dumpConfig();
      break;
    case '0':
    case '1':
    case '2':
    case '3':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
    {
      int val = m_parameter - '0';
      dumpParameterPosition(PositionList[val]);
      break;
    }
    default:
      break;
    }
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

bool SerCom::setvalue(bool valuedefined, unsigned long value, unsigned long min, unsigned long max, unsigned long &adress)
{
  bool ok = false;
  if (valuedefined && value >= min && value <= max)
  {
    adress = value;
    ser.print("1");
    ok = true;
  }
  else
    ser.print("0");
  ser.flush();
  return ok;
}

bool SerCom::setvalue(bool valuedefined, unsigned int value, Parameteruint8_t *adress)
{
  bool ok = false;
  if (!valuedefined)
    ser.print("0");
  else if (adress->set(value))
  {
    ser.print("1");
    ok = true;
  }
  else
    ser.print("0");
  ser.flush();
  return ok;
}

bool SerCom::setvalue(bool valuedefined, unsigned long value, Parameterulong *adress)
{
  bool ok = false;
  if (!valuedefined)
    ser.print("0");
  else if (adress->set(value))
  {
    ser.print("1");
    ok = true;
  }
  else
    ser.print("0");
  ser.flush();
  return ok;
}

bool SerCom::setvalue(bool valuedefined, unsigned int value, Parameteruint *adress)
{
  bool ok = false;
  if (!valuedefined)
    ser.print("0");
  else if (adress->set(value))
  {
    ser.print("1");
    ok = true;
  }
  else
    ser.print("0");
  ser.flush();
  return ok;
}


void SerCom::dumpConfig()
{
  char buf[50];
  sprintf(buf, "~%05u %05u %03u %03u %03u %03u %03u#",
    (unsigned int)(startPosition->get() / resolution->get()),
    (unsigned int)(maxPosition->get() / resolution->get()),
    lowSpeed->get(),
    highSpeed->get(),
    cmdAcc->get(),
    manAcc->get(),
    manDec->get()
   );
  ser.print(buf);
  ser.flush();
}

void SerCom::dumpConfigMotor()
{
  char buf[50];
  sprintf(buf, "M%u %u %03u %03u#",
    (unsigned int)(reverse->get()),
    (unsigned int)(micro->get()),
    (unsigned int)(resolution->get()),
    (unsigned int)(curr->get()));
  ser.print(buf);
  ser.flush();
}

void SerCom::dumpState()
{
  char buf[20];

  unsigned int pos = (unsigned int)(stepper.currentPosition() /resolution->get());
  if (pos > 65535)
  {
    stepper.setCurrentPosition( 65535 *resolution->get());
  }
  sprintf(buf, "?%05u %03u#", (unsigned int)(stepper.currentPosition() /resolution->get()), (unsigned int)abs(stepper.speed() / pow(2, micro->get())));
  ser.print(buf);
}

void SerCom::dumpParameterPosition(ParameterPosition* Pos)
{
  char buf[20];
  char id[11];
  unsigned long val;
  if (!Pos->isvalid())
  {
    ser.print("0#");
    ser.flush();
    return;
  }
  Pos->get(id, val);
  unsigned int pos = (unsigned int)(val / resolution->get());
  if (pos > 65535)
  {
    ser.print("0#");
    ser.flush();
    return;
  }
  sprintf(buf, "%05u %s#", pos, id);
  ser.print(buf);
  ser.flush();
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

// 
// 
// 

#include "Command.h"
#include <EEPROM.h>

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
 /* m_hasReceivedCommand = ser.findUntil(':','#');
  if (!m_hasReceivedCommand)
    return false;*/
  m_hasReceivedCommand = false;

  while (ser.available() > 0)
  {
    char b = ser.read();

    if (!m_hasreceivedstart)
    {
      if (b == ':')
      {
        m_hasreceivedstart = true;
        m_input[message_pos] = b;
        message_pos++;
      }
      continue;
    }
    else
    {
      m_input[message_pos] = b;
      if (b == '#')
      {
        m_hasReceivedCommand = true;
        m_hasreceivedstart = false;
        message_pos = 0;
        break;
      }
      message_pos++;
    }
    if (message_pos == INPUT_SIZE)
    {
      m_hasreceivedstart = false;
      message_pos = 0;
    }
  }  
  if (!m_hasReceivedCommand)
    return m_hasReceivedCommand;


  unsigned int size = strlen(m_input);
  if (size < 2 || m_input[1] != 'F' || m_input[0] != ':' )
  {
    m_hasReceivedCommand = false;
    return m_hasReceivedCommand;
  }
  // Read each command pair
  m_command = m_input[2];
  m_parameter = size < 4 ? 0 : m_input[3];
  m_valuedefined = false;
  if (m_parameter == ' ')
  {
    m_valuedefined = true;
    m_value = atoi(&m_input[4]);
  }
  m_hasReceivedCommand = true;
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

  case FocCmd_Reset:
    for (int i = 0; i < EEPROM.length(); i++)
    {
      EEPROM.write(i, 0);
    }
  case FocCmd_Reboot:
    Serial.end();
    Serial1.end();
    Serial2.end();
    delay(1000);
    _reboot_Teensyduino_();
    break;
  case FocCmd_Halt:
    halt = true;
    stepper.setAcceleration(100.*manDec->get());
    stepper.stop();
    break;
  case FocCmd_Goto:
      if (m_valuedefined)
      {
        halt = false;
        modeGoto();
        MoveTo((unsigned long)m_value * resolution->get());
      }
      break;
  case FocCmd_goto:
    switch (m_parameter)
    {
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
  {
    unsigned long position;
    if (setvalue(m_valuedefined, (unsigned long)m_value * resolution->get(), 0, maxPosition->get(), position))
    {
      stepper.setCurrentPosition((long)position);
      writePos();
    }
    break;
  }
  case FocCmd_set:    
    switch (m_parameter)
    {
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
    {
      int val = m_parameter - '0';
      m_value = atoi(&m_input[5]);
      PositionList[val]->set(&m_input[11], (unsigned long)m_value* resolution->get());
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
      teenAstroStepper.setCurrent(10*curr->get());
    }
    break;
  case FocCmd_micro:
    if (setvalue(m_valuedefined, m_value, micro))
    {
      teenAstroStepper.setMicrostep( micro->get());
    }
    break;
  case CmdDumpState: // "?" dump state including details
    dumpState();
    break;
  case CmdDumpConfig:
    dumpConfig();
    break;
  case CmdDumpConfigPos:
    switch (m_parameter)
    {
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
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
  default:
    break;
  }
}

bool SerCom::MoveRequest(void)
{
  if (!m_hasReceivedCommand)
    return false;
  switch (m_command)
  {
  case FocCmd_in_wor:
    modeMan();
    stepper.moveTo(0);
    m_hasReceivedCommand = false;
    break;
  case FocCmd_out_wor:
    modeMan();
    stepper.moveTo(0);
    m_hasReceivedCommand = false;
    break;
  case FocCmd_in:
    modeMan();
    stepper.moveTo(0);
    m_hasReceivedCommand = false;
    ser.print("1");
    ser.flush();
    break;
  case FocCmd_in_stop:
    stepper.setAcceleration(100.*manDec->get());
    stepper.stop();
    m_hasReceivedCommand = false;
    ser.print("1");
    ser.flush();
    break;
  case FocCmd_out:
    modeMan();
    stepper.moveTo(maxPosition->get());
    m_hasReceivedCommand = false;
    ser.print("1");
    ser.flush();
    break;
  case FocCmd_out_stop:
    stepper.setAcceleration(100.*manDec->get());
    stepper.stop();
    m_hasReceivedCommand = false;
    ser.print("1");
    ser.flush();
    break;
  case FocCmd_Halt:
    stepper.setAcceleration(100.*manDec->get());
    stepper.stop();
    m_hasReceivedCommand = false;
    break;
  default:
    break;
  }
  if (!m_hasReceivedCommand)
    return true;
  return false;
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
  if (!stepper.isRunning())
  {
    tempSensors.requestTemperaturesByIndex(0);
    lastTemp = max(min(tempSensors.getTempCByIndex(0), 99.9999), -99.9999);
    lastTemp = 0.00;
  }
  stepper.run();
  ser.print("?");
  stepper.run();
  unsigned int p = (unsigned int)(stepper.currentPosition() / resolution->get());
  printvalue(p, 5, 0, false);
  ser.print(" ");
  p = (unsigned int)abs(stepper.speed() / pow(2, micro->get()));
  printvalue(p, 3, 0, false);
  ser.print(" ");
  printvalue(lastTemp, 2, 2, true);
  ser.print("#");
}

void SerCom::printvalue(double val, int n, int d, bool plus)
{
  if (plus)
    val >= 0 ? ser.print("+") : ser.print("-");
  stepper.run();

  val = abs(val);
  int valint = val;
  int valit = pow10(n - 1);
  for (int k = n; k > 1; k--)
  {
    if (val < valit)
    {
      ser.print("0");
      stepper.run();
    }
    else
      break;
    valit /= 10;
  }
  ser.print(valint);
  stepper.run();
  if (d > 0)
  {
    ser.print(".");
    stepper.run();
    valint = (val - valint)*pow10(d);
    valit = pow10(d - 1);
    for (int k = d; k > 1; k--)
    {
      if (val < valit)
      {
        ser.print("0");
        stepper.run();
      }
      else
        break;
      valit /= 10;
    }
    ser.print(valint);
    stepper.run();
  }
}

void SerCom::dumpParameterPosition(ParameterPosition* Pos)
{
  char id[11] = {0};
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
  ser.print("P");
  printvalue(pos, 5, 0, false);
  ser.print(" ");
  ser.print(id);
  stepper.run();
  ser.print("#");
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

#include "Command.h"
#include <EEPROM.h>
#define INPUT_SIZE 30

//target
enum MoveMode
{
  RESET, GOTO, MAN
};
bool breakgoto = false;
MoveMode mode = RESET;
long target = 0;
long deltaTarget = 0;

constexpr unsigned PID_Interval = 10; // ms  
constexpr float P = 10;             // (P)roportional constant of the regulator needs to be adjusted (depends on speed and acceleration setting)

void pid()
{
  static unsigned lastTick = 0;
  if (deltaTarget == 0)
  {
    rotateController.stopAsync();
    lastTick = 0;
    return;
  }
  if (millis() - lastTick > PID_Interval)
  {
    float delta = (target - stepper.getPosition()) * (P / (PID_Interval*lowSpeed->get()*pow(2, micro->get())));  // This implements a simple P regulator (can be extended to a PID if necessary)
    float factor = std::max(-1.0f, std::min(1.0f, delta)); // limit to -1.0..1.0
    rotateController.overrideSpeed(factor);                // set new speed
    lastTick = 0;
  }
}

void modeGoto()
{
  if (mode == GOTO)
    return;
  stepper.setAcceleration(AccFact*cmdAcc->get());
  stepper.setMaxSpeed(highSpeed->get()*pow(2, micro->get()));
  mode = GOTO;
}

void modeMan()
{
  if (mode == MAN)
    return;
  stepper.setAcceleration(AccFact*manAcc->get());
  stepper.setMaxSpeed(lowSpeed->get()*pow(2, micro->get()));
  rotateController.rotateAsync(stepper);
  mode = MAN;
}

bool SerCom::Do()
{
  if (Get_Command())
  {
    if (HaltRequest())
    {
      //Serial.println("Stop");
      return true;
    }
    if (GetRequest())
      return true;
    if (!controller.isRunning())
    {
      if (MoveRequest())
      {
        //Serial.println("Manual Move");
        return true;
      }
    }
    if (!controller.isRunning() && !rotateController.isRunning())
    {
      if (SetRequest())
      {
        //Serial.println("Command Set");
        return true;
      }
    }
  }
  return false;
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
  if (size < 2 || m_input[1] != 'F' || m_input[0] != ':')
  {
    m_hasReceivedCommand = false;
    return m_hasReceivedCommand;
  }
  // Read each command pair
  m_command = m_input[2];
  m_parameter = size < 4 ? 0 : m_input[3];
  m_valuedefined = false;
  if (m_parameter == ',')
  {
    m_valuedefined = true;
    m_value = atoi(&m_input[4]);
  }
  m_hasReceivedCommand = true;
  m_input[0] = 0;
  return m_hasReceivedCommand;
}

bool SerCom::GetRequest(void)
{
  if (!m_hasReceivedCommand)
    return false;
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
    m_hasReceivedCommand = false;
    break;
  case AzCmd_Version:
    sayHello();
    m_hasReceivedCommand = false;
    break;
  case CmdDumpState: // "?" dump state including details
    dumpState();
    m_hasReceivedCommand = false;
    break;
  case CmdDumpConfig:
    dumpConfig();
    m_hasReceivedCommand = false;
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
      m_hasReceivedCommand = false;
      break;
    }
    default:
      m_hasReceivedCommand = false;
      break;
    }
    m_hasReceivedCommand = false;
    break;
  case CmdDumpConfigMotor:
    dumpConfigMotor();
    m_hasReceivedCommand = false;
    break;
  default:
    break;
  }
  if (!m_hasReceivedCommand)
    return true;
  return false;
}

bool SerCom::SetRequest(void)
{
  if (!m_hasReceivedCommand)
    return false;
  switch (m_command)
  {
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
  case FocCmd_Goto:
    if (m_valuedefined)
    {
      MoveTo((unsigned long)m_value * resolution->get());
    }
    m_hasReceivedCommand = false;
    break;
  case FocCmd_goto:
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
      if (PositionList[val]->isvalid())
      {
        MoveTo(PositionList[val]->getPosition());
      }
      m_hasReceivedCommand = false;
      break;
    }
    default:
      m_hasReceivedCommand = false;
      break;
    }
    m_hasReceivedCommand = false;
    break;
  case FocCmd_Park:
    MoveTo(startPosition->get());
    m_hasReceivedCommand = false;
    break;
  case FocCmd_Sync:      // "reset" position
  {
    unsigned long position;
    if (setvalue(m_valuedefined, (unsigned long)m_value * resolution->get(), 0, maxPosition->get(), position))
    {
      stepper.setPosition((long)position);
      writePos();
    }
    m_hasReceivedCommand = false;
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
      m_hasReceivedCommand = false;
      break;
    }
    default:
      m_hasReceivedCommand = false;
      break;
    }
    m_hasReceivedCommand = false;
    break;
  case FocCmd_startPosition:
    setvalue(m_valuedefined, (unsigned long)m_value * resolution->get(), startPosition);
    m_hasReceivedCommand = false;
    break;
  case FocCmd_maxPosition:
    setvalue(m_valuedefined, (unsigned long)m_value * resolution->get(), maxPosition);
    m_hasReceivedCommand = false;
    break;
  case FocCmd_highSpeed:
    setvalue(m_valuedefined, m_value, highSpeed);
    mode = RESET;
    m_hasReceivedCommand = false;
    break;
  case FocCmd_lowSpeed:
    setvalue(m_valuedefined, m_value, lowSpeed);
    mode = RESET;
    m_hasReceivedCommand = false;
    break;
  case FocCmd_cmdAcc:
    setvalue(m_valuedefined, (uint8_t)m_value, cmdAcc);
    mode = RESET;
    m_hasReceivedCommand = false;
    break;
  case FocCmd_manualAcc:
    setvalue(m_valuedefined, (uint8_t)m_value, manAcc);
    mode = RESET;
    m_hasReceivedCommand = false;
    break;
  case FocCmd_manualDec:
    setvalue(m_valuedefined, (uint8_t)m_value, manDec);
    mode = RESET;
    m_hasReceivedCommand = false;
    break;
  case FocCmd_Inv:
    setvalue(m_valuedefined, m_value, reverse);
    stepper.setInverseRotation(reverse->get());
    m_hasReceivedCommand = false;
    break;
  case FocCmd_inpulse:
    setvalue(m_valuedefined, m_value, resolution);
    m_hasReceivedCommand = false;
    break;
  case FocCmd_current:
    if (setvalue(m_valuedefined, m_value, curr))
    {
      teenAstroStepper.setCurrent(10 * curr->get());
    }
    m_hasReceivedCommand = false;
    break;
  case FocCmd_micro:
    if (setvalue(m_valuedefined, m_value, micro))
    {
      teenAstroStepper.setMicrostep(micro->get());
    }
    m_hasReceivedCommand = false;
    break;
  case FocCmd_steprot:
    setvalue(m_valuedefined, m_value, steprot);
    m_hasReceivedCommand = false;
    break;
  default:
    break;
  }
  if (!m_hasReceivedCommand)
    return true;
  return false;
}

bool SerCom::HaltRequest(void)
{
  if (!m_hasReceivedCommand)
    return false;
  switch (m_command)
  {
  case FocCmd_Halt:
    if (rotateController.isRunning())
    {
      deltaTarget = 0;
    }
    if (controller.isRunning() && !breakgoto)
    {
      breakgoto = true;
      controller.stopAsync();
    }
    m_hasReceivedCommand = false;
    break;
    //break Goto if goto is running and a Manual move is request
  case FocCmd_in_wor:
  case FocCmd_out_wor:
  case FocCmd_in:
  case FocCmd_in_stop:
  case FocCmd_out:
  case FocCmd_out_stop:
    if (controller.isRunning() && !breakgoto)
    {
      breakgoto = true;
      controller.stopAsync();
      m_hasReceivedCommand = false;
    }
  default:
    break;
  }
  if (!m_hasReceivedCommand)
    return true;
  return false;
}

bool SerCom::MoveRequest(void)
{
  if (!m_hasReceivedCommand)
    return false;
  switch (m_command)
  {
  case FocCmd_in_wor:
    m_hasReceivedCommand = false;
    modeMan();
    deltaTarget = -1;
    target = 0;
    break;
  case FocCmd_out_wor:
    m_hasReceivedCommand = false;
    modeMan();
    deltaTarget = 1;
    target = maxPosition->get();
    break;
  case FocCmd_in:
    m_hasReceivedCommand = false;
    modeMan();
    deltaTarget = -1;
    target = 0;
    ser.print("1");
    ser.flush();
    break;
  case FocCmd_out:
    m_hasReceivedCommand = false;
    modeMan();
    deltaTarget = 1;
    target = maxPosition->get();
    ser.print("1");
    ser.flush();
    break;
  case FocCmd_in_stop:
    m_hasReceivedCommand = false;
    deltaTarget = 0;
    ser.print("1");
    ser.flush();
    break;
  case FocCmd_out_stop:
    m_hasReceivedCommand = false;
    deltaTarget = 0;
    ser.print("1");
    ser.flush();
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
  sprintf(buf, "M%u %u %03u %03u %03u#",
    (unsigned int)(reverse->get()),
          (unsigned int)(micro->get()),
          (unsigned int)(resolution->get()),
          (unsigned int)(curr->get()),
          (unsigned int)(steprot->get()));
  ser.print(buf);
  ser.flush();
}

void SerCom::dumpState()
{
  unsigned int pos = (unsigned int)(stepper.getPosition() / resolution->get());
  if (pos > 65535U)
  {
    stepper.setPosition(65535U * resolution->get());
  }
  ser.print("?");
  unsigned int p = (unsigned int)(stepper.getPosition() / resolution->get());
  printvalue(p, 5, 0, false);
  ser.print(" ");
  p = (controller.isRunning() || rotateController.isRunning()) ?
    (unsigned int)abs(rotateController.getCurrentSpeed() / pow(2, micro->get())) : 0;
  printvalue(p, 3, 0, false);
  ser.print(" ");
  printvalue(lastTemp, 2, 2, true);
  ser.print("#");
  ser.flush();
}

void SerCom::printvalue(double val, int n, int d, bool plus)
{
  if (plus)
    val >= 0 ? ser.print("+") : ser.print("-");

  val = abs(val);
  int valint = val;
  int valit = pow10(n - 1);
  for (int k = n; k > 1; k--)
  {
    if (val < valit)
    {
      ser.print("0");
    }
    else
      break;
    valit /= 10;
  }
  ser.print(valint);
  if (d > 0)
  {
    ser.print(".");
    valint = (val - valint)*pow10(d);
    valit = pow10(d - 1);
    for (int k = d; k > 1; k--)
    {
      if (val < valit)
      {
        ser.print("0");
      }
      else
        break;
      valit /= 10;
    }
    ser.print(valint);
  }
}

void SerCom::dumpParameterPosition(ParameterPosition* Pos)
{
  char id[11] = { 0 };
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
  ser.print(FirmwareVersion);
  ser.print("#");
  ser.flush();
}

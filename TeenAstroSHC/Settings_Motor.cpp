#include "SmartController.h"
#include "SHC_text.h"


void SmartHandController::menuMotor(const uint8_t axis)
{
  static uint8_t s_sel = 1;
  uint8_t tmp_sel = s_sel;
  while (tmp_sel)
  {
    const char *string_list_Motor = T_SHOWSETTINGS "\n" T_ROTATION "\n" T_GEAR "\n" T_STEPSPERROT "\n"
      T_MICROSTEP "\n" T_BACKLASH "\n" T_BACKLASHSPEED  "\n" T_LOWCURR "\n" T_HIGHCURR "\n" "Silent" ;
    tmp_sel = display->UserInterfaceSelectionList(&buttonPad, axis == 1 ? T_MOTOR " 1" : T_MOTOR " 2", tmp_sel, string_list_Motor);
    s_sel = tmp_sel > 0 ? tmp_sel : s_sel;
    switch (tmp_sel)
    {
    case 1:
      DisplayMotorSettings(axis);
      break;
    case 2:
      menuSetReverse(axis);
      break;
    case 3:
      menuSetTotGear(axis);
      break;
    case 4:
      menuSetStepPerRot(axis);
      break;
    case 5:
      menuSetMicro(axis);
      break;
    case 6:
      menuSetBacklash(axis);
      break;
    case 7:
      menuSetBacklashRate(axis);
      break;
    case 8:
      menuSetCurrent(axis,0);
      break;
    case 9:
      menuSetCurrent(axis,1);
      break;
    case 10:
      menuSetSilentStep(axis);
      break;
    default:
      break;
    }
  }
}

void SmartHandController::DisplayMotorSettings(const uint8_t &axis)
{
  if (!ta_MountStatus.hasConfig())
  {
    DisplayMessage(T_LX200COMMAND, T_FAILED, 500);
    return;
  }
  int ax = axis - 1;
  char line1[32] = "";
  char line2[32] = "";
  char line3[32] = "";
  char line4[32] = "";
  sprintf(line1, T_MOTORSETTINGS, axis);
  ta_MountStatus.getCfgReverse(ax) ? sprintf(line3, T_REVERSEDROTATION) : sprintf(line3, T_DIRECTROTATION);
  sprintf(line4, T_RATIO": %u", (unsigned int)(ta_MountStatus.getCfgGear(ax) / 1000.0f));
  DisplayLongMessage(line1, NULL, line3, line4, -1);

  line2[0] = line3[0] = line4[0] = 0;
  sprintf(line2, "%u " T_STEPSPERROT, (unsigned int)ta_MountStatus.getCfgStepRot(ax));
  sprintf(line3, T_MICROSTEP ": %u", (unsigned int)pow(2, ta_MountStatus.getCfgMicro(ax)));
  sprintf(line4, T_BACKLASH": %u sec.", (unsigned int)ta_MountStatus.getCfgBacklash(ax));
  DisplayLongMessage(line1, line2, line3, line4, -1);

  line2[0] = line3[0] = line4[0] = 0;
  sprintf(line3, T_LOWCURR " %u mA", ta_MountStatus.getCfgLowCurr(ax));
  sprintf(line4, T_HIGHCURR " %u mA", ta_MountStatus.getCfgHighCurr(ax));
  DisplayLongMessage(line1, NULL, line3, line4, -1);
}

bool SmartHandController::menuSetReverse(const uint8_t &axis)
{
  if (!ta_MountStatus.hasConfig()) { DisplayMessage(T_LX200COMMAND, T_FAILED, 500); return false; }
  bool reverse = ta_MountStatus.getCfgReverse(axis - 1);
  char text[20];
  const char* string_list = T_DIRECT "\n" T_REVERSE;
  sprintf(text, T_ROTATION " M%u", axis);
  uint8_t choice = display->UserInterfaceSelectionList(&buttonPad, text, (uint8_t)reverse + 1, string_list);
  if (choice)
  {
    reverse = (bool)(choice - 1);
    bool ok = DisplayMessageLX200(m_client->writeReverse(axis, reverse), false);
    if (ok) ta_MountStatus.updateAllConfig(true);
    return ok;
  }
  return true;
}

bool SmartHandController::menuSetBacklash(const uint8_t &axis)
{
  if (!ta_MountStatus.hasConfig()) { DisplayMessage(T_LX200COMMAND, T_FAILED, 500); return false; }
  float backlash = (float)ta_MountStatus.getCfgBacklash(axis - 1);
  char text[20];
  sprintf(text, T_BACKLASH " M%u", axis);
  if (display->UserInterfaceInputValueFloat(&buttonPad, text, "", &backlash, 0, 999, 4, 0, " " T_INSECONDS))
  {
    bool ok = DisplayMessageLX200(m_client->writeBacklash(axis, backlash), false);
    if (ok) ta_MountStatus.updateAllConfig(true);
    return ok;
  }
  return true;
}

bool SmartHandController::menuSetBacklashRate(const uint8_t& axis)
{
  if (!ta_MountStatus.hasConfig()) { DisplayMessage(T_LX200COMMAND, T_FAILED, 500); return false; }
  float rate = (float)ta_MountStatus.getCfgBacklashRate(axis - 1);
  char text[20];
  sprintf(text, T_BACKLASHSPEED " M%u", axis);
  if (display->UserInterfaceInputValueFloat(&buttonPad, text, "", &rate, 16, 64, 2, 0, ""))
  {
    bool ok = DisplayMessageLX200(m_client->writeBacklashRate(axis, rate), false);
    if (ok) ta_MountStatus.updateAllConfig(true);
    return ok;
  }
  return true;
}

bool SmartHandController::menuSetTotGear(const uint8_t &axis)
{
  if (!ta_MountStatus.hasConfig()) { DisplayMessage(T_LX200COMMAND, T_FAILED, 500); return false; }
  float totGear = ta_MountStatus.getCfgGear(axis - 1) / 1000.0f;
  char text[20];
  sprintf(text, T_GEAR " M%u", axis);
  if (display->UserInterfaceInputValueFloat(&buttonPad, text, "", &totGear, 1, 60000, 8, 3, ""))
  {
    bool ok = DisplayMessageLX200(m_client->writeTotGear(axis, totGear), false);
    if (ok) ta_MountStatus.updateAllConfig(true);
    return ok;
  }
  return true;
}

bool SmartHandController::menuSetStepPerRot(const uint8_t &axis)
{
  if (!ta_MountStatus.hasConfig()) { DisplayMessage(T_LX200COMMAND, T_FAILED, 500); return false; }
  float stepPerRot = (float)ta_MountStatus.getCfgStepRot(axis - 1);
  char text[20];
  sprintf(text, T_STEPPER " M%u", axis);
  if (display->UserInterfaceInputValueFloatIncr(&buttonPad, text, "", &stepPerRot, 20, 400, 3, 0, 1, " " T_STEPS))
  {
    bool ok = DisplayMessageLX200(m_client->writeStepPerRot(axis, stepPerRot), false);
    if (ok) ta_MountStatus.updateAllConfig(true);
    return ok;
  }
  return true;
}

bool SmartHandController::menuSetMicro(const uint8_t &axis)
{
  if (!ta_MountStatus.hasConfig()) { DisplayMessage(T_LX200COMMAND, T_FAILED, 500); return false; }
  uint8_t microStep = ta_MountStatus.getCfgMicro(axis - 1);
  char text[20];
  const char* string_list_micro = "2\n4\n8\n16 (~256)\n32\n64\n128\n256";
  sprintf(text, T_STEPPER " M%u", axis);
  uint8_t choice = microStep - 1 + 1;
  choice = display->UserInterfaceSelectionList(&buttonPad, text, choice, string_list_micro);
  if (choice)
  {
    microStep = choice - 1 + 1;
    bool ok = DisplayMessageLX200(m_client->writeMicro(axis, microStep), false);
    if (ok) ta_MountStatus.updateAllConfig(true);
    return ok;
  }
  return true;
}

bool SmartHandController::menuSetSilentStep(const uint8_t &axis)
{
  if (!ta_MountStatus.hasConfig()) { DisplayMessage(T_LX200COMMAND, T_FAILED, 500); return false; }
  uint8_t silent = ta_MountStatus.getCfgSilent(axis - 1) ? 1 : 0;
  char text[20];
  const char* string_list_mode = T_OFF "\n" T_ON;
  sprintf(text, T_STEPPER " M%u", axis);
  uint8_t choice = silent + 1;
  choice = display->UserInterfaceSelectionList(&buttonPad, text, choice, string_list_mode);
  if (choice)
  {
    silent = choice - 1;
    bool ok = DisplayMessageLX200(m_client->writeSilentStep(axis, silent), false);
    if (ok) ta_MountStatus.updateAllConfig(true);
    return ok;
  }
  return true;
}

bool SmartHandController::menuSetCurrent(const uint8_t &axis, bool high)
{
  if (!ta_MountStatus.hasConfig()) { DisplayMessage(T_LX200COMMAND, T_FAILED, 500); return false; }
  unsigned int curr = high ? ta_MountStatus.getCfgHighCurr(axis - 1)
                           : ta_MountStatus.getCfgLowCurr(axis - 1);
  char text[20];
  sprintf(text, high ? T_HIGHCURR " M%u" : T_LOWCURR " M%u", axis);
  uint8_t curr8 = curr / 100;
  if (display->UserInterfaceInputValueInteger(&buttonPad, text, "", &curr8, 2, 28, 3, "00 mA " T_PEAK))
  {
    curr = (unsigned int)curr8 * 100;
    bool ok = high ? DisplayMessageLX200(m_client->writeHighCurr(axis, curr), false)
                   : DisplayMessageLX200(m_client->writeLowCurr(axis, curr), false);
    if (ok) ta_MountStatus.updateAllConfig(true);
    return ok;
  }
  return true;
}

bool SmartHandController::menuSetEncoderReverse(const uint8_t& axis)
{
  if (!ta_MountStatus.hasConfig()) { DisplayMessage(T_LX200COMMAND, T_FAILED, 500); return false; }
  bool reverse = ta_MountStatus.getCfgEncReverse(axis - 1);
  char text[20];
  const char* string_list = T_DIRECT "\n" T_REVERSE;
  sprintf(text, T_ROTATION " M%u", axis);
  uint8_t choice = display->UserInterfaceSelectionList(&buttonPad, text, (uint8_t)reverse + 1, string_list);
  if (choice)
  {
    reverse = (bool)(choice - 1);
    bool ok = DisplayMessageLX200(m_client->writeEncoderReverse(axis, reverse), false);
    if (ok) ta_MountStatus.updateAllConfig(true);
    return ok;
  }
  return true;
}

bool SmartHandController::menuSetEncoderPulsePerDegree(const uint8_t& axis)
{
  if (!ta_MountStatus.hasConfig()) { DisplayMessage(T_LX200COMMAND, T_FAILED, 500); return false; }
  // Cache stores ppd×100 (same as what readPulsePerDegree() returned).
  float ppd = axis == 1 ? (float)ta_MountStatus.getCfgPPD1()
                        : (float)ta_MountStatus.getCfgPPD2();
  char text[20];
  ppd /= 100;
  sprintf(text, T_PULSEPERDEGREE " E%u", axis);
  if (display->UserInterfaceInputValueFloatIncr(&buttonPad, text, "", &ppd, 0.5, 3600.00, 5, 2, 0.01, ""))
  {
    ppd *= 100;
    bool ok = DisplayMessageLX200(m_client->writePulsePerDegree(axis, ppd), false);
    if (ok) ta_MountStatus.updateAllConfig(true);
    return ok;
  }
  return true;
}

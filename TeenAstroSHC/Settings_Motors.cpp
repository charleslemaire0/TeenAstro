#include "SmartController.h"
#include "SHC_text.h"

void SmartHandController::menuMotors()
{
  ta_MountStatus.updateAllConfig(true);
  if (ta_MountStatus.motorsEnable())
  {
    const char* string_list = T_SHOWSETTINGS "\n" T_MOTOR " 1\n" T_MOTOR " 2\n"
      T_ACCELERATION "\n" T_SPEED "\n" T_TRACKING "\n"  T_SETTLETIME "\n" T_DISABLE;
    static uint8_t s_sel = 1;
    uint8_t tmp_sel;
    while (!exitMenu)
    {
      tmp_sel = display->UserInterfaceSelectionList(&buttonPad, T_MOTORS, s_sel, string_list);
      s_sel = tmp_sel > 0 ? tmp_sel : s_sel;
      switch (tmp_sel)
      {
      case 0:
        return;
      case 1:
        DisplayMountSettings();
        break;
      case 2:
        menuMotor(1);
        break;
      case 3:
        menuMotor(2);
        break;
      case 4:
        menuAcceleration();
        break;
      case 5:
        MenuRates();
        break;
      case 6:
        MenuTracking();
        break;
      case 7:
        menuSettleTime();
        break;
      case 8:
        if (display->UserInterfaceMessage(&buttonPad, T_DISABLE, T_MOTORS, "", T_NO "\n" T_YES) == 2)
        {
          if (m_client->enableMotors(false) == LX200_VALUESET)
          {
            DisplayMessage(T_TELESCOPE, T_REBOOT, 500);
            powerCycleRequired = true;
            exitMenu = true;
          }
          else
          {
            DisplayMessage(T_DISABLE, T_FAILED, 500);
          }
        }
        break;

      default:
        break;
      }
    }
  }
  else
  {
    const char* string_list = T_ENABLE;
    static uint8_t s_sel = 1;
    uint8_t tmp_sel;
    while (!exitMenu)
    {
      tmp_sel = display->UserInterfaceSelectionList(&buttonPad, T_MOTORS, s_sel, string_list);
      s_sel = tmp_sel > 0 ? tmp_sel : s_sel;
      switch (tmp_sel)
      {
      case 0:
        return;
      case 1:
        if (display->UserInterfaceMessage(&buttonPad, T_ENABLE, T_MOTORS, "", T_NO "\n" T_YES) == 2)
        {
          if (m_client->enableMotors(true) == LX200_VALUESET)
          {
            DisplayMessage(T_TELESCOPE, T_REBOOT, 500);
            powerCycleRequired = true;
            exitMenu = true;
          }
          else
          {
            DisplayMessage(T_ENABLE, T_FAILED, 500);
          }
        }
        break;
      default:
        break;
      }
    }
  }
}

void SmartHandController::DisplayMountSettings()
{
  DisplayAccMaxRateSettings();
  DisplayMotorSettings(1);
  DisplayMotorSettings(2);
}

void SmartHandController::menuAcceleration()
{
  if (!ta_MountStatus.hasConfig()) { DisplayMessage(T_LX200COMMAND, T_FAILED, 500); return; }
  char cmd[20];
  float acc = ta_MountStatus.getCfgAcceleration();
  if (display->UserInterfaceInputValueFloat(&buttonPad, T_ACCELERATION, "", &acc, 0.1, 25, 4, 1, " deg."))
  {
    sprintf(cmd, ":SXRA,%04d#", (int)(acc * 10.));
    if (DisplayMessageLX200(m_client->set(cmd), false))
      ta_MountStatus.updateAllConfig(true);
  }
}

void SmartHandController::MenuRates()
{
  static uint8_t s_sel = 1;
  uint8_t tmp_sel;
  while (!exitMenu)
  {
    const char* string_list = T_MAX "\n" T_FAST "\n" T_MEDIUM " \n" T_SLOW " \n" T_GUIDESPEED "\n"  T_DEFAULTSPEED;
    tmp_sel = display->UserInterfaceSelectionList(&buttonPad, T_SPEED, s_sel, string_list);
    s_sel = tmp_sel > 0 ? tmp_sel : s_sel;
    switch (tmp_sel)
    {
    case 0:
      return;
    case 1:
      menuMaxRate();
      break;
    case 2:
      menuRate(3);
      break;
    case 3:
      menuRate(2);
      break;
    case 4:
      menuRate(1);
      break;
    case 5:
      menuGuideRate();
      break;
    case 6:
      MenuDefaultSpeed();
      break;
    default:
      break;
    }
  }
}

void SmartHandController::menuRate(int r)
{
  if (!ta_MountStatus.hasConfig()) { DisplayMessage(T_LX200COMMAND, T_FAILED, 500); return; }
  char cmd[20];
  float rate;
  switch (r)
  {
  case 3: rate = ta_MountStatus.getCfgFastRate();   break;
  case 2: rate = ta_MountStatus.getCfgMediumRate(); break;
  case 1: rate = ta_MountStatus.getCfgSlowRate();   break;
  default: rate = ta_MountStatus.getCfgGuideRate(); break;
  }
  char title[20];
  r == 3 ? strcpy(title, T_FASTSPEED) : (r == 2 ? strcpy(title, T_MEDIUMSPEED) : strcpy(title, T_SLOWSPEED));
  if (display->UserInterfaceInputValueFloat(&buttonPad, title, "", &rate, 1.f, 255.f, 4u, 0u, "x"))
  {
    sprintf(cmd, ":SXR%d,%03d#", r, (int)(rate));
    if (DisplayMessageLX200(m_client->set(cmd), false))
      ta_MountStatus.updateAllConfig(true);
  }
}

void SmartHandController::menuMaxRate()
{
  if (!ta_MountStatus.hasConfig()) { DisplayMessage(T_LX200COMMAND, T_FAILED, 500); return; }
  char cmd[20];
  float maxrate = (float)ta_MountStatus.getCfgMaxRate();
  if (display->UserInterfaceInputValueFloatIncr(&buttonPad, T_MAXSPEED, "", &maxrate, 60, 3600, 4, 0, 60, ""))
  {
    sprintf(cmd, ":SXRX,%04d#", (int)maxrate);
    if (DisplayMessageLX200(m_client->set(cmd), false))
      ta_MountStatus.updateAllConfig(true);
  }
}

void SmartHandController::MenuDefaultSpeed()
{
  if (!ta_MountStatus.hasConfig()) { DisplayMessage(T_LX200COMMAND, T_FAILED, 500); return; }
  static uint8_t s_sel = 1;
  char out[12];
  s_sel = ta_MountStatus.getCfgDefaultRate() + 1;
  s_sel = s_sel > 5 ? 5 : s_sel;
  const char* string_list = T_GUIDESPEED " \n" T_SLOW "\n" T_MEDIUM "\n" T_FAST "\n" T_MAX;
  uint8_t tmp_sel = display->UserInterfaceSelectionList(&buttonPad, T_DEFAULTSPEED, s_sel, string_list);
  if (tmp_sel)
  {
    sprintf(out, ":SXRD,%u#", tmp_sel - 1);
    if (DisplayMessageLX200(m_client->set(out), false))
      ta_MountStatus.updateAllConfig(true);
  }
}

void SmartHandController::MenuTracking()
{
  static uint8_t s_sel = 1;
  uint8_t tmp_sel;

  while (!exitMenu)
  {
    ta_MountStatus.updateMount();
    const char* string_list_tracking = ta_MountStatus.isAltAz() ?
      T_DRIFTSPEED "\n" T_REFRACTION : T_DRIFTSPEED "\n" T_REFRACTION  "\n" T_TRACKINGCORRECTION;
    tmp_sel = display->UserInterfaceSelectionList(&buttonPad, T_TRACKINGOPTIONS, s_sel, string_list_tracking);
    s_sel = tmp_sel > 0 ? tmp_sel : s_sel;
    switch (tmp_sel)
    {
    case 0:
      return;
    case 1:
      menuTrackRate();
      break;
    case 2:
      MenuTrackingRefraction();
      break;
    case 3:
      MenuTrackingCorrection();
      break;
    default:
      break;
    }
  }
}

void SmartHandController::MenuTrackingCorrection()
{
  while (!exitMenu)
  {
    ta_MountStatus.updateMount();
    TeenAstroMountStatus::RateCompensation comp = ta_MountStatus.getRateCompensation();
    uint8_t tmp_sel = (comp == TeenAstroMountStatus::RC_BOTH) ? 2u : 1u;
    const char* string_list_tracking = T_RIGHTASC "\n" T_BOTH;
    tmp_sel = display->UserInterfaceSelectionList(&buttonPad, T_TRACKINGCORRECTION, tmp_sel, string_list_tracking);
    switch (tmp_sel)
    {
    case 0:
      return;
    case 1:
      DisplayMessageLX200(m_client->setStepperMode(1), false);
      break;
    case 2:
      DisplayMessageLX200(m_client->setStepperMode(2), false);
      break;
    default:
      break;
    }
  }
}

void SmartHandController::menuSettleTime()
{
  if (!ta_MountStatus.hasConfig()) { DisplayMessage(T_LX200COMMAND, T_FAILED, 500); return; }
  char cmd[20];
  float val = (float)ta_MountStatus.getCfgSettleTime();
  if (display->UserInterfaceInputValueFloat(&buttonPad, T_SETTLETIME, "", &val, 0.0f, 20.f, 1u, 0u, " sec."))
  {
    sprintf(cmd, ":SXOS,%02d#", (int)(val));
    if (DisplayMessageLX200(m_client->set(cmd), false))
      ta_MountStatus.updateAllConfig(true);
  }
}

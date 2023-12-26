#include "SmartController.h"
#include "SHC_text.h"

void SmartHandController::menuMotors()
{
  if (ta_MountStatus.motorsEnable())
  {
    const char* string_list = T_SHOWSETTINGS "\n" T_MOTOR " 1\n" T_MOTOR " 2\n"
      T_ACCELERATION "\n" T_SPEED "\n" T_TRACKING "\n" T_DISABLE;
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
        if (display->UserInterfaceMessage(&buttonPad, T_DISABLE, T_MOTORS, "", T_NO "\n" T_YES) == 2)
        {
          if (SetLX200(":SXME,n#") == LX200_VALUESET)
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
          if (SetLX200(":SXME,y#") == LX200_VALUESET)
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
  char outAcc[20];
  char outStepsPerDegree[20];
  char cmd[20];
  if (DisplayMessageLX200(GetLX200(":GXRA#", outAcc, sizeof(outAcc))))
  {
    float acc = atof(&outAcc[0]);
    if (display->UserInterfaceInputValueFloat(&buttonPad, T_ACCELERATION, "", &acc, 0.1, 25, 4, 1, " deg."))
    {
      sprintf(cmd, ":SXRA,%04d#", (int)(acc * 10.));
      DisplayMessageLX200(SetLX200(cmd), false);
    }
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
  char outRate[20];
  char cmd[20];
  sprintf(cmd, ":GXR%d#", r);
  char title[20];
  r == 3 ? strcpy(title, T_FASTSPEED) : (r == 2 ? strcpy(title, T_MEDIUMSPEED) : strcpy(title, T_SLOWSPEED));
  if (DisplayMessageLX200(GetLX200(cmd, outRate, sizeof(outRate))))
  {
    float rate = atof(&outRate[0]);
    if (display->UserInterfaceInputValueFloat(&buttonPad, title, "", &rate, 1.f, 255.f, 4u, 0u, "x"))
    {
      sprintf(cmd, ":SXR%d,%03d#", r, (int)(rate));
      DisplayMessageLX200(SetLX200(cmd), false);
    }
  }
}

void SmartHandController::menuMaxRate()
{
  char outRate[20];
  char outStepsPerDegree[20];
  char cmd[20];
  if (DisplayMessageLX200(GetLX200(":GXRX#", outRate, sizeof(outRate))))
  {
    float maxrate = (float)strtol(&outRate[0], NULL, 10);
    if (display->UserInterfaceInputValueFloatIncr(&buttonPad, T_MAXSPEED, "", &maxrate, 60, 3600, 4, 0, 60, ""))
    {
      sprintf(cmd, ":SXRX,%04d#", (int)maxrate);
      DisplayMessageLX200(SetLX200(cmd), false);
    }
  }
}

void SmartHandController::MenuDefaultSpeed()
{
  static uint8_t s_sel = 1;
  char out[10];
  uint8_t tmp_sel;
  if (DisplayMessageLX200(GetLX200(":GXRD#", out, sizeof(out))))
  {
    s_sel = out[0] - '0' + 1;
    s_sel = s_sel > 5 ? 4 : s_sel;
    const char* string_list = T_GUIDESPEED " \n" T_SLOW "\n" T_MEDIUM "\n" T_FAST "\n" T_MAX;
    tmp_sel = display->UserInterfaceSelectionList(&buttonPad, T_DEFAULTSPEED, s_sel, string_list);
    if (tmp_sel)
    {
      sprintf(out, ":SXRD,%u#", tmp_sel - 1);
      DisplayMessageLX200(SetLX200(out), false);
    }
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
      T_DRIFTSPEED "\n" T_REFRACTION : T_DRIFTSPEED "\n" T_REFRACTION  "\n"  T_ALIGNMENT "\n" T_TRACKINGCORRECTION;
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
      MenuTrackingAlignment();
      break;
    case 4:
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
    uint8_t tmp_sel;
    TeenAstroMountStatus::RateCompensation comp = ta_MountStatus.getRateCompensation();
    switch (comp)
    {
    case TeenAstroMountStatus::RC_UNKNOWN:
    case TeenAstroMountStatus::RC_NONE:
      tmp_sel = 1;
      break;
    case TeenAstroMountStatus::RC_ALIGN_RA:
    case TeenAstroMountStatus::RC_FULL_RA:
      tmp_sel = 2;
      break;
    case TeenAstroMountStatus::RC_ALIGN_BOTH:
    case TeenAstroMountStatus::RC_FULL_BOTH:
      tmp_sel = 3;
      break;
    }
    const char* string_list_tracking = T_NONE "\n" T_RIGHTASC "\n" T_BOTH;
    tmp_sel = display->UserInterfaceSelectionList(&buttonPad, T_TRACKINGCORRECTION, tmp_sel, string_list_tracking);
    switch (tmp_sel)
    {
    case 0:
      return;
    case 1:
      DisplayMessageLX200(SetLX200(":T0#"), false);
      break;
    case 2:
      DisplayMessageLX200(SetLX200(":T1#"), false);
      break;
    case 3:
      DisplayMessageLX200(SetLX200(":T2#"), false);
      break;
    default:
      break;
    }
  }
}

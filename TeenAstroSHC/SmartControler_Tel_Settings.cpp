#include "SmartController.h"
#include "SHC_text.h"


void SmartHandController::menuTelSettings()
{
  buttonPad.setMenuMode();

  static uint8_t s_sel = 1;
  uint8_t tmp_sel;
  while (!exitMenu)
  {
    const char *string_list_TelSettings = T_HANDCONTROLLER "\n" T_TIME " & " T_SITE "\n" T_PARKANDHOME "\n" T_MOUNT "\n" T_LIMITS "\n" T_MAINUNITINFO "\nWifi";
    tmp_sel = display->UserInterfaceSelectionList(&buttonPad, T_TELESCOPESETTINGS, s_sel, string_list_TelSettings);
    s_sel = tmp_sel > 0 ? tmp_sel : s_sel;
    switch (tmp_sel)
    {
    case 0:
      exitMenu = true;
    case 1:
      menuSHCSettings();
      break;
      //case 2:
      //  menuAlignment();
      //  break;
    case 2:
      menuTimeAndSite();
      break;
    case 3:
      menuParkAndHome();
      break;
    case 4:
      menuMount();
      break;
    case 5:
      menuLimits();
      break;
    case 6:
      menuMainUnitInfo();
      break;
    case 7:
      menuWifi();
      break;
    default:
      break;
    }
  }
  buttonPad.setControlerMode();
}
void SmartHandController::menuMainUnitInfo()
{
  const char *string_list_MainUnitInfo = T_SHOWVERSION "\n" T_REBOOT "\n" T_RESETTOFACTORY;
  static uint8_t s_sel = 1;
  uint8_t tmp_sel = s_sel;
  while (tmp_sel)
  {
    tmp_sel = display->UserInterfaceSelectionList(&buttonPad, T_MAINUNITINFO, tmp_sel, string_list_MainUnitInfo);
    s_sel = tmp_sel > 0 ? tmp_sel : s_sel;
    switch (tmp_sel)
    {
    case 0:
      return;
    case 1:
      ta_MountStatus.updateV();
      if (ta_MountStatus.hasInfoV())
      {
        DisplayMessage(ta_MountStatus.getVN(), ta_MountStatus.getVD(), -1);
      }
      break;
    case 2:
      DisplayMessageLX200(SetLX200(":$!#"), false);
      delay(500);
      powerCycleRequired = true;
      return;
    case 3:
      if (display->UserInterfaceMessage(&buttonPad, "Reset", "To", "Factory?", "NO\nYES") == 2)
      {
        DisplayMessageLX200(SetLX200(":$$#"), false);
        delay(500);
        powerCycleRequired = true;
        return;
      }
      break;
    default:
      break;
    }
  }
}
void SmartHandController::menuParkAndHome()
{
  const char *string_list_ParkAndHome = T_SETPARK "\n" T_SETHOME "\n" T_RESETHOME;
  static uint8_t s_sel = 1;
  uint8_t tmp_sel = s_sel;
  while (tmp_sel)
  {
    tmp_sel = display->UserInterfaceSelectionList(&buttonPad, T_PARKANDHOME, tmp_sel, string_list_ParkAndHome);
    s_sel = tmp_sel > 0 ? tmp_sel : s_sel;
    switch (tmp_sel)
    {
    case 0:
      return;
    case 1:
      DisplayMessageLX200(SetLX200(":hQ#"), false);
      return;
    case 2:
      DisplayMessageLX200(SetLX200(":hB#"), false);
      return;
    case 3:
      DisplayMessageLX200(SetLX200(":hb#"), false);
      return;
    default:
      break;
    }
  }
}
//----------------------------------//
//             LIMITS               //
//----------------------------------//
void SmartHandController::menuLimits()
{
  const char *string_list_LimitsL2 = T_HORIZON "\n" T_OVERHEAD "\n" T_MERIDIANE "\n" T_MERIDIANW "\n" T_UNDERPOLE;
  static uint8_t s_sel = 1;
  uint8_t tmp_sel = s_sel;
  while (tmp_sel)
  {
    tmp_sel = display->UserInterfaceSelectionList(&buttonPad, T_LIMITS, s_sel, string_list_LimitsL2);
    s_sel = tmp_sel > 0 ? tmp_sel : s_sel;
    switch (tmp_sel)
    {
    case 1:
      menuHorizon();
      break;
    case 2:
      menuOverhead();
      break;
    case 3:
      menuMeridian(true);
      break;
    case 4:
      menuMeridian(false);
      break;
    case 5:
      menuUnderPole();
      break;
    default:
      break;
    }
  }
}
void SmartHandController::menuHorizon()
{
  char out[20];
  if (DisplayMessageLX200(GetLX200(":GXLH#", out, sizeof(out))))
  {
    float angle = (float)strtol(&out[0], NULL, 10);
    if (display->UserInterfaceInputValueFloat(&buttonPad, T_HORIZONLIMIT, "", &angle, -10, 20, 2, 0, " " T_DEGREE))
    {
      sprintf(out, ":SXLH,%+03d#", (int)angle);
      DisplayMessageLX200(SetLX200(out), false);
    }
  }
}
void SmartHandController::menuOverhead()
{
  char out[20];
  if (DisplayMessageLX200(GetLX200(":GXLO#", out, sizeof(out))))
  {
    float angle = (float)strtol(&out[0], NULL, 10);
    if (display->UserInterfaceInputValueFloat(&buttonPad, T_OVERHEADLIMIT, "", &angle, 60, 91, 2, 0, " " T_DEGREE))
    {
      sprintf(out, ":SXLO,%02d#", (int)angle);
      DisplayMessageLX200(SetLX200(out), false);
    }
  }
}
void SmartHandController::menuUnderPole()
{
  char out[20];
  char cmd[15];
  if (DisplayMessageLX200(GetLX200(":GXLU#", out, sizeof(out))))
  {
    float angle = (float)strtol(&out[0], NULL, 10) / 10;
    if (display->UserInterfaceInputValueFloat(&buttonPad, T_MAXHOURANGLE, "+-", &angle, 9, 12, 2, 1, " " T_HOURS))
    {
      sprintf(cmd, ":SXLU,%03d#", (int)(angle * 10));
      DisplayMessageLX200(SetLX200(cmd), false);
    }
  }
}
void SmartHandController::menuMeridian(bool east)
{
  char out[20];
  char cmd[15];
  sprintf(cmd, ":GXLX#");
  cmd[4] = east ? 'E' : 'W';

  if (DisplayMessageLX200(GetLX200(cmd, out, sizeof(out))))
  {
    float angle = (float)strtol(&out[0], NULL, 10) / 4.0;
    if (display->UserInterfaceInputValueFloat(&buttonPad, east ? T_MERIDIANLIMITE : T_MERIDIANLIMITW, "", &angle, -45, 45, 2, 0, " " T_DEGREE))
    {
      sprintf(cmd, ":SXLX,%+03d#", (int)(angle*4.0));
      cmd[4] = east ? 'E' : 'W';
      DisplayMessageLX200(SetLX200(cmd), false);
    }
  }
}


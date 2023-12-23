#include "SmartController.h"
#include "SHC_text.h"


//----------------------------------//
//             LIMITS               //
//----------------------------------//
void SmartHandController::menuLimits()
{
  const char* string_list_LimitsL2 = T_HORIZON "\n" T_OVERHEAD "\n" T_AXIS "\n" T_GERMANEQUATORIAL;
  static uint8_t s_sel = 1;
  uint8_t tmp_sel = s_sel;
  while (!exitMenu && tmp_sel)
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
      menuLimitAxis();
      break;
    case 4:
      menuLimitGEM();
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

#ifdef keepTrackingOnWhenFarFromPole
void SmartHandController::menuFarFromPole()
{
  char out[20];
  if (DisplayMessageLX200(GetLX200(":GXLS#", out, sizeof(out))))
  {
    float angle = (float)strtol(&out[0], NULL, 10);
    if (display->UserInterfaceInputValueFloat(&buttonPad, T_DISTANCE, "", &angle, 0, 181, 2, 0, " " T_DEGREE))
    {
      sprintf(out, ":SXLS,%03d#", (int)angle);
      DisplayMessageLX200(SetLX200(out), false);
    }
  }
}
#endif

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
      sprintf(cmd, ":SXLX,%+03d#", (int)(angle * 4.0));
      cmd[4] = east ? 'E' : 'W';
      DisplayMessageLX200(SetLX200(cmd), false);
    }
  }
}


void SmartHandController::menuLimitGEM()
{
#ifndef keepTrackingOnWhenFarFromPole
  const char* string_list_LimitsL3 = T_MERIDIANE "\n" T_MERIDIANW "\n" T_UNDERPOLE;
#else
  const char* string_list_LimitsL3 = T_MERIDIANE "\n" T_MERIDIANW "\n" T_UNDERPOLE  "\n" T_FARFROMPOLE;
#endif

  static uint8_t s_sel = 1;
  uint8_t tmp_sel = s_sel;
  while (tmp_sel)
  {
    tmp_sel = display->UserInterfaceSelectionList(&buttonPad, T_GERMANEQUATORIAL, s_sel, string_list_LimitsL3);
    s_sel = tmp_sel > 0 ? tmp_sel : s_sel;
    switch (tmp_sel)
    {
    case 1:
      menuMeridian(true);
      break;
    case 2:
      menuMeridian(false);
      break;
    case 3:
      menuUnderPole();
      break;

#ifdef keepTrackingOnWhenFarFromPole
    case 4:
      menuFarFromPole();
      break;
#endif

    default:
      break;
    }
  }
}

void SmartHandController::menuLimitAxis()
{
  const char* string_list_LimitsL3 = T_DISPLAYAXIS "\n" T_AXIS1MIN "\n" T_AXIS1MAX "\n" T_AXIS2MIN "\n" T_AXIS2MAX;
  static uint8_t s_sel = 1;
  uint8_t tmp_sel = s_sel;
  while (tmp_sel)
  {
    tmp_sel = display->UserInterfaceSelectionList(&buttonPad, T_AXIS, s_sel, string_list_LimitsL3);
    s_sel = tmp_sel > 0 ? tmp_sel : s_sel;
    switch (tmp_sel)
    {
    case 1:
      pages[P_AXIS_DEG].show =
        display->UserInterfaceMessage(&buttonPad, T_DISPLAY, T_AXIS, T_COORDINATES"?", T_YES "\n" T_NO) == 1;
      current_page = P_AXIS_DEG;
      exitMenu = true;
      return;
      break;
    case 2:
      menuAxis('A');
      break;
    case 3:
      menuAxis('B');
      break;
    case 4:
      menuAxis('C');
      break;
    case 5:
      menuAxis('D');
      break;
    default:
      break;
    }
  }

}

void SmartHandController::menuAxis(char mode)
{
  char out[20];
  char cmd[15];
  char menu[32];
  double fact = 10.;
  double minval = 0;
  double maxval = 360;
  sprintf(cmd, ":GXXX#");
  cmd[3] = 'l';
  switch (mode)
  {
  case 'A':
    sprintf(menu, T_AXIS1MIN);
    cmd[4] = 'l';
    cmd[4] = mode;
    if (!DisplayMessageLX200(GetLX200(cmd, out, sizeof(out))))
    {
      return;
    }
    minval = (float)strtol(&out[0], NULL, 10);
    cmd[3] = 'L';
    cmd[4] = 'B';
    if (!DisplayMessageLX200(GetLX200(cmd, out, sizeof(out))))
    {
      return;
    }
    maxval = (float)strtol(&out[0], NULL, 10) / fact;
    break;
  case 'B':
    sprintf(menu, T_AXIS1MAX);
    cmd[4] = 'l';
    cmd[4] = mode;
    if (!DisplayMessageLX200(GetLX200(cmd, out, sizeof(out))))
    {
      return;
    }
    maxval = (float)strtol(&out[0], NULL, 10);
    cmd[3] = 'L';
    cmd[4] = 'A';
    if (!DisplayMessageLX200(GetLX200(cmd, out, sizeof(out))))
    {
      return;
    }
    minval = (float)strtol(&out[0], NULL, 10) / fact;
    break;
  case 'C':
    sprintf(menu, T_AXIS2MIN);
    cmd[4] = 'l';
    cmd[4] = mode;
    if (!DisplayMessageLX200(GetLX200(cmd, out, sizeof(out))))
    {
      return;
    }
    minval = (float)strtol(&out[0], NULL, 10);
    cmd[3] = 'L';
    cmd[4] = 'D';
    if (!DisplayMessageLX200(GetLX200(cmd, out, sizeof(out))))
    {
      return;
    }
    maxval = (float)strtol(&out[0], NULL, 10) / fact;
    break;
  case 'D':
    sprintf(menu, T_AXIS2MAX);
    cmd[4] = 'l';
    cmd[4] = mode;
    if (!DisplayMessageLX200(GetLX200(cmd, out, sizeof(out))))
    {
      return;
    }
    maxval = (float)strtol(&out[0], NULL, 10);
    cmd[3] = 'L';
    cmd[4] = 'C';
    if (!DisplayMessageLX200(GetLX200(cmd, out, sizeof(out))))
    {
      return;
    }
    minval = (float)strtol(&out[0], NULL, 10) / fact;
    break;
  }
  cmd[3] = 'L';
  cmd[4] = mode;
  if (DisplayMessageLX200(GetLX200(cmd, out, sizeof(out))))
  {
    float val = (float)strtol(&out[0], NULL, 10) / fact;
    if (display->UserInterfaceInputValueFloat(&buttonPad, menu, "", &val, minval, maxval, 3, 1, " " T_DEGREE))
    {
      sprintf(cmd, ":SXLX,%+03d#", (int)(val * fact));
      cmd[4] = mode;
      DisplayMessageLX200(SetLX200(cmd), false);
    }
  }
}
#include "SmartController.h"
#include "SHC_text.h"


//----------------------------------//
//             LIMITS               //
//----------------------------------//
void SmartHandController::menuLimits()
{
  ta_MountStatus.updateAllConfig(true);
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
  if (!ta_MountStatus.hasConfig()) { DisplayMessage(T_LX200COMMAND, T_FAILED, 500); return; }
  char out[20];
  float angle = (float)ta_MountStatus.getCfgMinAlt();
  if (display->UserInterfaceInputValueFloat(&buttonPad, T_HORIZONLIMIT, "", &angle, -10, 20, 2, 0, " " T_DEGREE))
  {
    sprintf(out, ":SXLH,%+03d#", (int)angle);
    if (DisplayMessageLX200(m_client->set(out), false))
      ta_MountStatus.updateAllConfig(true);
  }
}

void SmartHandController::menuOverhead()
{
  if (!ta_MountStatus.hasConfig()) { DisplayMessage(T_LX200COMMAND, T_FAILED, 500); return; }
  char out[20];
  float angle = (float)ta_MountStatus.getCfgMaxAlt();
  if (display->UserInterfaceInputValueFloat(&buttonPad, T_OVERHEADLIMIT, "", &angle, 60, 91, 2, 0, " " T_DEGREE))
  {
    sprintf(out, ":SXLO,%02d#", (int)angle);
    if (DisplayMessageLX200(m_client->set(out), false))
      ta_MountStatus.updateAllConfig(true);
  }
}

void SmartHandController::menuUnderPole()
{
  if (!ta_MountStatus.hasConfig()) { DisplayMessage(T_LX200COMMAND, T_FAILED, 500); return; }
  char cmd[15];
  // getCfgUnderPole10() returns underPoleLimitGOTO × 10 (same as :GXLU# response).
  float angle = ta_MountStatus.getCfgUnderPole10() / 10.0f;
  if (display->UserInterfaceInputValueFloat(&buttonPad, T_MAXHOURANGLE, "+-", &angle, 9, 12, 2, 1, " " T_HOURS))
  {
    sprintf(cmd, ":SXLU,%03d#", (int)(angle * 10));
    if (DisplayMessageLX200(m_client->set(cmd), false))
      ta_MountStatus.updateAllConfig(true);
  }
}

#ifdef keepTrackingOnWhenFarFromPole
void SmartHandController::menuFarFromPole()
{
  if (!ta_MountStatus.hasConfig()) { DisplayMessage(T_LX200COMMAND, T_FAILED, 500); return; }
  char out[20];
  float angle = (float)ta_MountStatus.getCfgMinDistPole();
  if (display->UserInterfaceInputValueFloat(&buttonPad, T_DISTANCE, "", &angle, 0, 181, 2, 0, " " T_DEGREE))
  {
    sprintf(out, ":SXLS,%03d#", (int)angle);
    if (DisplayMessageLX200(m_client->set(out), false))
      ta_MountStatus.updateAllConfig(true);
  }
}
#endif

void SmartHandController::menuMeridian(bool east)
{
  if (!ta_MountStatus.hasConfig()) { DisplayMessage(T_LX200COMMAND, T_FAILED, 500); return; }
  char cmd[15];
  // getCfgMeridianE/W() return arcminutes (same as :GXLE#/:GXLW# responses).
  int16_t rawVal = east ? ta_MountStatus.getCfgMeridianE() : ta_MountStatus.getCfgMeridianW();
  float angle = rawVal / 4.0f;
  if (display->UserInterfaceInputValueFloat(&buttonPad, east ? T_MERIDIANLIMITE : T_MERIDIANLIMITW, "", &angle, -45, 45, 2, 0, " " T_DEGREE))
  {
    sprintf(cmd, ":SXLX,%+03d#", (int)(angle * 4.0));
    cmd[4] = east ? 'E' : 'W';
    if (DisplayMessageLX200(m_client->set(cmd), false))
      ta_MountStatus.updateAllConfig(true);
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
      pages[P_AXIS_DEG].show = true;
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
  if (!ta_MountStatus.hasConfig()) { DisplayMessage(T_LX200COMMAND, T_FAILED, 500); return; }

  char out[20];
  char cmd[15];
  char menu[32];
  const double fact = 10.;
  double minval = 0;
  double maxval = 360;
  double val    = 0;

  // Build the :GXl[mode]# command (mount-type limits — read-only range bounds).
  sprintf(cmd, ":GXlX#");
  cmd[4] = mode;

  switch (mode)
  {
  case 'A':
    sprintf(menu, T_AXIS1MIN);
    // Mount-type axis1 min → UI slider lower bound (raw int, NOT ÷ fact).
    if (!DisplayMessageLX200(m_client->get(cmd, out, sizeof(out)))) return;
    minval = (double)strtol(&out[0], NULL, 10);
    // Current axis1 max from cache → UI slider upper bound.
    maxval = ta_MountStatus.getCfgAxis1Max() / fact;
    // Current axis1 min from cache → the value being edited.
    val    = ta_MountStatus.getCfgAxis1Min() / fact;
    break;
  case 'B':
    sprintf(menu, T_AXIS1MAX);
    // Mount-type axis1 max → UI slider upper bound.
    if (!DisplayMessageLX200(m_client->get(cmd, out, sizeof(out)))) return;
    maxval = (double)strtol(&out[0], NULL, 10);
    // Current axis1 min from cache → UI slider lower bound.
    minval = ta_MountStatus.getCfgAxis1Min() / fact;
    val    = ta_MountStatus.getCfgAxis1Max() / fact;
    break;
  case 'C':
    sprintf(menu, T_AXIS2MIN);
    if (!DisplayMessageLX200(m_client->get(cmd, out, sizeof(out)))) return;
    minval = (double)strtol(&out[0], NULL, 10);
    maxval = ta_MountStatus.getCfgAxis2Max() / fact;
    val    = ta_MountStatus.getCfgAxis2Min() / fact;
    break;
  case 'D':
    sprintf(menu, T_AXIS2MAX);
    if (!DisplayMessageLX200(m_client->get(cmd, out, sizeof(out)))) return;
    maxval = (double)strtol(&out[0], NULL, 10);
    minval = ta_MountStatus.getCfgAxis2Min() / fact;
    val    = ta_MountStatus.getCfgAxis2Max() / fact;
    break;
  default:
    return;
  }

  float fval = (float)val;
  if (display->UserInterfaceInputValueFloat(&buttonPad, menu, "", &fval, minval, maxval, 3, 1, " " T_DEGREE))
  {
    sprintf(cmd, ":SXLX,%+03d#", (int)(fval * fact));
    cmd[4] = mode;
    if (DisplayMessageLX200(m_client->set(cmd), false))
      ta_MountStatus.updateAllConfig(true);
  }
}

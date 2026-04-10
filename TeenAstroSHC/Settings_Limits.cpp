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
  float angle = (float)ta_MountStatus.getCfgMinAlt();
  if (display->UserInterfaceInputValueFloat(&buttonPad, T_HORIZONLIMIT, "", &angle, -10, 20, 2, 0, " " T_DEGREE))
  {
    if (DisplayMessageLX200(m_client->setMinAltitude((int)angle), false))
      ta_MountStatus.updateAllConfig(true);
  }
}

void SmartHandController::menuOverhead()
{
  if (!ta_MountStatus.hasConfig()) { DisplayMessage(T_LX200COMMAND, T_FAILED, 500); return; }
  float angle = (float)ta_MountStatus.getCfgMaxAlt();
  if (display->UserInterfaceInputValueFloat(&buttonPad, T_OVERHEADLIMIT, "", &angle, 60, 91, 2, 0, " " T_DEGREE))
  {
    if (DisplayMessageLX200(m_client->setMaxAltitude((int)angle), false))
      ta_MountStatus.updateAllConfig(true);
  }
}

void SmartHandController::menuUnderPole()
{
  if (!ta_MountStatus.hasConfig()) { DisplayMessage(T_LX200COMMAND, T_FAILED, 500); return; }
  // getCfgUnderPole10() returns underPoleLimitGOTO × 10 (same as :GXLU# response).
  float angle = ta_MountStatus.getCfgUnderPole10() / 10.0f;
  if (display->UserInterfaceInputValueFloat(&buttonPad, T_MAXHOURANGLE, "+-", &angle, 9, 12, 2, 1, " " T_HOURS))
  {
    if (DisplayMessageLX200(m_client->setUnderPoleLimit(angle), false))
      ta_MountStatus.updateAllConfig(true);
  }
}

#ifdef keepTrackingOnWhenFarFromPole
void SmartHandController::menuFarFromPole()
{
  if (!ta_MountStatus.hasConfig()) { DisplayMessage(T_LX200COMMAND, T_FAILED, 500); return; }
  float angle = (float)ta_MountStatus.getCfgMinDistPole();
  if (display->UserInterfaceInputValueFloat(&buttonPad, T_DISTANCE, "", &angle, 0, 181, 2, 0, " " T_DEGREE))
  {
    if (DisplayMessageLX200(m_client->setMinDistFromPole((int)angle), false))
      ta_MountStatus.updateAllConfig(true);
  }
}
#endif

void SmartHandController::menuMeridian(bool east)
{
  if (!ta_MountStatus.hasConfig()) { DisplayMessage(T_LX200COMMAND, T_FAILED, 500); return; }
  // getCfgMeridianE/W() return arcminutes × 4 (same as :GXLE#/:GXLW# responses).
  int16_t rawVal = east ? ta_MountStatus.getCfgMeridianE() : ta_MountStatus.getCfgMeridianW();
  float angle = rawVal / 4.0f;
  if (display->UserInterfaceInputValueFloat(&buttonPad, east ? T_MERIDIANLIMITE : T_MERIDIANLIMITW, "", &angle, -45, 45, 2, 0, " " T_DEGREE))
  {
    const int lim = (int)(angle * 4.0f);
    LX200RETURN ret = east ? m_client->setLimitEast(lim) : m_client->setLimitWest(lim);
    if (DisplayMessageLX200(ret, false))
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

  char menu[32];
  const double fact = 10.;
  double minval = 0;
  double maxval = 360;
  double val    = 0;
  int mountBound = 0;

  switch (mode)
  {
  case 'A':
    sprintf(menu, T_AXIS1MIN);
    if (!DisplayMessageLX200(m_client->getMountTypeAxisLimit('A', mountBound))) return;
    minval = (double)mountBound;
    maxval = ta_MountStatus.getCfgAxis1Max() / fact;
    val    = ta_MountStatus.getCfgAxis1Min() / fact;
    break;
  case 'B':
    sprintf(menu, T_AXIS1MAX);
    if (!DisplayMessageLX200(m_client->getMountTypeAxisLimit('B', mountBound))) return;
    maxval = (double)mountBound;
    minval = ta_MountStatus.getCfgAxis1Min() / fact;
    val    = ta_MountStatus.getCfgAxis1Max() / fact;
    break;
  case 'C':
    sprintf(menu, T_AXIS2MIN);
    if (!DisplayMessageLX200(m_client->getMountTypeAxisLimit('C', mountBound))) return;
    minval = (double)mountBound;
    maxval = ta_MountStatus.getCfgAxis2Max() / fact;
    val    = ta_MountStatus.getCfgAxis2Min() / fact;
    break;
  case 'D':
    sprintf(menu, T_AXIS2MAX);
    if (!DisplayMessageLX200(m_client->getMountTypeAxisLimit('D', mountBound))) return;
    maxval = (double)mountBound;
    minval = ta_MountStatus.getCfgAxis2Min() / fact;
    val    = ta_MountStatus.getCfgAxis2Max() / fact;
    break;
  default:
    return;
  }

  float fval = (float)val;
  if (display->UserInterfaceInputValueFloat(&buttonPad, menu, "", &fval, minval, maxval, 3, 1, " " T_DEGREE))
  {
    if (DisplayMessageLX200(m_client->setAxisLimit(mode, fval), false))
      ta_MountStatus.updateAllConfig(true);
  }
}

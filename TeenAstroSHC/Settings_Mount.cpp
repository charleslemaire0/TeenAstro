#include "SmartController.h"
#include "SHC_text.h"


void SmartHandController::menuMount()
{
  ta_MountStatus.updateAllConfig(true);
  static uint8_t s_sel = 1;
  uint8_t tmp_sel;
  while (!exitMenu)
  {
    const char *string_list = T_MOUNT "\n" T_MOUNTTYPE "\n" T_MOTORS " \n" T_ENCODERS" \n"
      T_LIMITS "\n" T_REFRACTION "\n" T_RETICULE;
    tmp_sel = display->UserInterfaceSelectionList(&buttonPad, T_MOUNT, s_sel, string_list);
    s_sel = tmp_sel > 0 ? tmp_sel : s_sel;
    switch (tmp_sel)
    {
    case 0:
      return;
    case 1:
      menuMounts();
      break;
    case 2:
      menuMountType();
      break;
    case 3:
      menuMotors();
      break;
    case 4:
      menuEncoders();
      break;
    case 5:
      menuLimits();
      break;
    case 6:
      MenuRefraction();
      break;
    case 7:
      menuReticule();
      break;
    default:
      break;
    }
  }
}


void SmartHandController::MenuTrackingRefraction()
{
  bool corr_on = ta_MountStatus.hasConfig() ? ta_MountStatus.getCfgRefrTracking() : false;
  const char* string_list_tracking = corr_on ? T_OFF : T_ON;
  uint8_t tmp_sel = display->UserInterfaceSelectionList(&buttonPad, T_REFRACTION, 0, string_list_tracking);
  switch (tmp_sel)
  {
  case 1:
    if ((corr_on ? m_client->enableRefraction(false) : m_client->enableRefraction(true)) == LX200_VALUESET)
    {
      DisplayMessage(T_REFRACTION, corr_on ? T_OFF : T_ON, 500);
      ta_MountStatus.updateAllConfig(true);
    }
    else
    {
      DisplayMessage(T_LX200COMMAND, T_FAILED, 1000);
    }
    break;
  default:
    break;
  }
}

void SmartHandController::menuMounts()
{
  char mountname[15];
  char txt[70] = "";
  for (int i = 0; i < 2; i++)
  {
    m_client->getMountName(i, mountname, sizeof(mountname));
    strcat(txt, mountname);
    if (i != 1)
    {
      strcat(txt, "\n");
    }
  }

  if (!ta_MountStatus.hasConfig()) { DisplayMessage(T_LX200COMMAND, T_FAILED, 500); return; }
  int val = (int)ta_MountStatus.getCfgMountIdx();
  uint8_t tmp_in = val + 1;
  uint8_t tmp_sel = display->UserInterfaceSelectionList(&buttonPad, T_SELECT_MOUNT, tmp_in, txt);
  if (tmp_sel != 0)
  {
    if (tmp_in != tmp_sel)
    {
      val = (int)tmp_sel - 1;
      m_client->setMount(val);
      powerCycleRequired = true;
      exitMenu = true;
    }
  }
}

void SmartHandController::DisplayAccMaxRateSettings()
{
  char line1[32] = "";
  char line2[32] = T_SLEWSETTING;
  char line3[32] = "";
  char line4[32] = "";
  DisplayMessageLX200(m_client->getMountDescription(line1, sizeof(line1)));

  if (ta_MountStatus.hasConfig())
  {
    sprintf(line3, T_ACCELERATION ": %.1f", ta_MountStatus.getCfgAcceleration());
    sprintf(line4, T_MaxSlew ": %dx", (int)ta_MountStatus.getCfgMaxRate());
  }
  DisplayLongMessage(line1, line2, line3, line4, -1);
}

void SmartHandController::menuMountType()
{
  uint8_t tmp_sel = ta_MountStatus.getMount();
  if (tmp_sel == 0)
  {
    DisplayLongMessage("!" T_WARNING "!", NULL, T_MOUNTTYPE, T_NOTDEFINED "!", -1);
    tmp_sel = 1;
  }
  const char *string_list_Mount = T_GERMANEQUATORIAL "\n" T_EQUATORIALFORK "\n" T_ALTAAZIMUTAL "\n" T_ALTAAZIMUTALFORK;
  tmp_sel = display->UserInterfaceSelectionList(&buttonPad, T_MOUNTTYPE, tmp_sel, string_list_Mount);
  if (tmp_sel)
  {
    char out[10];
    sprintf(out, ":S!%u#", tmp_sel);
    LX200RETURN answ = m_client->set(out);

    DisplayMessageLX200(answ, false);
    if (answ == LX200_VALUESET)
    {
      delay(1000);
      Serial.end();
      exitMenu = true;
      powerCycleRequired = true;
    }
  }
}

void SmartHandController::MenuRefraction()
{
  static uint8_t s_sel = 1;
  uint8_t tmp_sel;
  while (!exitMenu)
  {
    ta_MountStatus.updateMount();
    const char* string_list_tracking = ta_MountStatus.isAltAz() ?
      T_GOTO : T_GOTO "\n" T_POLAR_ALIGNMENT;
    tmp_sel = display->UserInterfaceSelectionList(&buttonPad, T_REFRACTION, 0, string_list_tracking);
    s_sel = tmp_sel > 0 ? tmp_sel : s_sel;
    switch (tmp_sel)
    {
    case 0:
      return;
    case 1:
      MenuRefractionForGoto();
      break;
    case 2:
      menuPolarAlignment();
      break;
    default:
      break;
    }
  }
}

void SmartHandController::menuPolarAlignment()
{
  if (!ta_MountStatus.hasConfig()) { DisplayMessage(T_LX200COMMAND, T_FAILED, 500); return; }
  uint8_t tmp_in = ta_MountStatus.getCfgRefrPole() ? 1 : 2;
  const char* string_list_tracking = T_APPARENT_POLE "\n" T_TRUE_POLE;
  uint8_t tmp_sel = display->UserInterfaceSelectionList(&buttonPad, T_POLAR_ALIGNMENT, tmp_in, string_list_tracking);
  if (tmp_sel == tmp_in || tmp_sel == 0)
  {
    return;
  }
  switch (tmp_sel)
  {
  case 1:
  case 2:
  {
    if (tmp_sel == 1 ? m_client->enablePolarAlign(true) == LX200_VALUESET : m_client->enablePolarAlign(false) == LX200_VALUESET)
    {
      DisplayMessage(T_SET, (tmp_sel == 1) ? T_APPARENT_POLE : T_TRUE_POLE, 1000);
      ta_MountStatus.updateAllConfig(true);
    }
    else
    {
      DisplayMessage(T_LX200COMMAND, T_FAILED, 1000);
    }
    break;
  }
  default:
    break;
  }
}

void SmartHandController::MenuRefractionForGoto()
{
  if (!ta_MountStatus.hasConfig()) { DisplayMessage(T_LX200COMMAND, T_FAILED, 500); return; }
  uint8_t tmp_in = ta_MountStatus.getCfgRefrGoto() ? 1 : 2;
  const char* string_list_tracking = T_ON "\n" T_OFF;
  uint8_t tmp_sel = display->UserInterfaceSelectionList(&buttonPad, T_REFRACTION, tmp_in, string_list_tracking);
  if (tmp_sel == tmp_in || tmp_sel == 0)
  {
    return;
  }
  switch (tmp_sel)
  {
  case 1:
  case 2:
  {
    if (tmp_sel == 1 ? m_client->enableGoTo(true) == LX200_VALUESET : m_client->enableGoTo(false) == LX200_VALUESET)
    {
      DisplayMessage(T_REFRACTION, (tmp_sel == 1) ? T_ON : T_OFF, 1000);
      ta_MountStatus.updateAllConfig(true);
    }
    else
    {
      DisplayMessage(T_LX200COMMAND, T_FAILED, 1000);
    }
    break;
  }
  default:
    break;
  }
}


void SmartHandController::menuGuideRate()
{
  if (!ta_MountStatus.hasConfig()) { DisplayMessage(T_LX200COMMAND, T_FAILED, 500); return; }
  char cmd[20];
  float guiderate = ta_MountStatus.getCfgGuideRate();
  if (display->UserInterfaceInputValueFloat(&buttonPad, T_GUIDESPEED, "", &guiderate, 0.1f, 1.f, 4u, 2u, "x"))
  {
    sprintf(cmd, ":SXR0,%03d#", (int)(guiderate * 100));
    if (DisplayMessageLX200(m_client->set(cmd), false))
      ta_MountStatus.updateAllConfig(true);
  }
}

void SmartHandController::menuTrackRate()
{
  static uint8_t s_sel = 1;
  uint8_t tmp_sel;
  while (!exitMenu)
  {
    const char* string_list_TrackRate = T_RIGHTASC "\n" T_DECLINAISON;
    tmp_sel = display->UserInterfaceSelectionList(&buttonPad, T_DRIFTSPEED, s_sel, string_list_TrackRate);
    s_sel = tmp_sel > 0 ? tmp_sel : s_sel;
    switch (tmp_sel)
    {
    case 0:
      return;
    case 1:
      menuSetDriftRate(0);
      break;
    case 2:
      menuSetDriftRate(1);
      break;
    }
  }
}

void SmartHandController::menuSetDriftRate(int axis)
{
  char outRate[20];
  char cmd[20];
  sprintf(cmd, ":GXRx#");
  cmd[4] = axis == 0 ? 'e' : 'f';
  char title[20];
  char unit[20];
  axis == 0 ? strcpy(title, T_RIGHTASC) : strcpy(title, T_DECLINAISON);
  axis == 0 ? strcpy(unit, "s/SI") : strcpy(unit, "\"/SI");
  if (DisplayMessageLX200(m_client->get(cmd, outRate, sizeof(outRate))))
  {
    float rate = (float)atol(&outRate[0])/10000.0;
    if (display->UserInterfaceInputValueFloat(&buttonPad, title, "", &rate, -2.f, 2.f, 6u, 4u, unit))
    {
      sprintf(cmd, ":SXRx,%06ld#", (long)(rate*10000));
      cmd[4] = axis == 0 ? 'e' : 'f';
      DisplayMessageLX200(m_client->set(cmd),false);
    }
  }
}




void SmartHandController::menuReticule()
{
  char *options = T_BRIGHTER "\n" T_LESSBRIGHT;
  uint8_t selection = 1;
  while (selection != 0)
  {
    selection = display->UserInterfaceSelectionList(&buttonPad, T_RETICULE, selection, options);
    if (selection > 0)
    {
      char cmd[5] = ":Bn#";
      cmd[2] = selection == 1 ? '+' : '-';
      DisplayMessageLX200(m_client->set(cmd), false);
    }
  }
}

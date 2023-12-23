#include "SmartController.h"
#include "SHC_text.h"


void SmartHandController::menuMount()
{
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
  char out[10];
  uint8_t tmp_sel;
  if (GetLX200(":GXrt#", out, sizeof(out)) == LX200_GETVALUEFAILED) strcpy(out, "n");
  bool corr_on = out[0] == 'y';
  const char* string_list_tracking = corr_on ? T_OFF : T_ON;
  tmp_sel = display->UserInterfaceSelectionList(&buttonPad, T_REFRACTION, 0, string_list_tracking);
  switch (tmp_sel)
  {
  case 1:
  {
    char out[20];
    memset(out, 0, sizeof(out));
    if ((corr_on ? SetLX200(":SXrt,n#") : SetLX200(":SXrt,y#")) == LX200_VALUESET)
    {
      DisplayMessage(T_REFRACTION, corr_on ? T_OFF : T_ON, 500);
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


void SmartHandController::MenuTrackingAlignment()
{
  char out[10];
  uint8_t tmp_sel;
  if (GetLX200(":GXAc#", out, sizeof(out)) == LX200_GETVALUEFAILED) strcpy(out, "n");
  bool corr_on = out[0] == 'y';
  const char* string_list_tracking = corr_on ? T_OFF : T_ON;
  tmp_sel = display->UserInterfaceSelectionList(&buttonPad, T_ALIGNMENT, 0, string_list_tracking);
  switch (tmp_sel)
  {
  case 1:
  {
    char out[20];
    memset(out, 0, sizeof(out));
    if ((corr_on ? SetLX200(":SXAc,n#") : SetLX200(":SXAc,y#")) == LX200_VALUESET)
    {
      DisplayMessage(T_ALIGNMENT, corr_on ? T_OFF : T_ON, 500);
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

void SmartHandController::menuMounts()
{
  int val;
  char mountname[15];
  char txt[70] = "";
  for (int i = 0; i < 2; i++)
  {
    GetMountNameLX200(i, mountname, sizeof(mountname));
    strcat(txt, mountname);
    if (i != 1)
    {
      strcat(txt, "\n");
    }
  }

  if (DisplayMessageLX200(GetMountIdxLX200(val)))
  {
    uint8_t tmp_in = val + 1;
    uint8_t tmp_sel = display->UserInterfaceSelectionList(&buttonPad, T_SELECT_MOUNT, tmp_in, txt);
    if (tmp_sel != 0)
    {   
      if (tmp_in != tmp_sel)
      {
        val = (int)tmp_sel - 1;
        SetMountLX200(val);
        powerCycleRequired = true;
        exitMenu = true;
      }
    }
  }
}

void SmartHandController::DisplayAccMaxRateSettings()
{
  char out[20];
  char line1[32] = "";
  char line2[32] = T_SLEWSETTING;
  char line3[32] = "";
  char line4[32] = "";
  DisplayMessageLX200(GetLX200(":GXOA#", line1, sizeof(line1)));

  if (DisplayMessageLX200(GetLX200(":GXRX#", out, sizeof(out))))
  {
    int maxrate = (float)strtol(&out[0], NULL, 10);
    sprintf(line4, T_MaxSlew ": %dx", maxrate);
  }
  if (DisplayMessageLX200(GetLX200(":GXRA#", out, sizeof(out))))
  {
    float acc = atof(&out[0]);
    sprintf(line3, T_ACCELERATION ": %.1f", acc);
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
    LX200RETURN answ = SetLX200(out);

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
  char out[10];
  uint8_t tmp_sel;

  if (!DisplayMessageLX200(GetLX200(":GXrp#", out, sizeof(out))))
    return;
  uint8_t tmp_in = out[0] == 'y' ? 1 : 2;
  const char* string_list_tracking = T_APPARENT_POLE "\n" T_TRUE_POLE  ;
  tmp_sel = display->UserInterfaceSelectionList(&buttonPad, T_POLAR_ALIGNMENT, tmp_in, string_list_tracking);
  if (tmp_sel == tmp_in)
  {
    return;
  }
  switch (tmp_sel)
  {
  case 1:
  case 2:
  {
    if (tmp_sel == 1 ? SetLX200(":SXrp,y#") == LX200_VALUESET : SetLX200(":SXrp,n#") == LX200_VALUESET)
    {
      DisplayMessage(T_SET, (tmp_sel == 1) ? T_APPARENT_POLE : T_TRUE_POLE, 1000);
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
  char out[10];
  uint8_t tmp_sel;
  if (!DisplayMessageLX200(GetLX200(":GXrg#", out, sizeof(out))))
    return;
  uint8_t tmp_in = out[0] == 'y' ? 1 : 2;
  const char* string_list_tracking = T_ON "\n" T_OFF;
  tmp_sel = display->UserInterfaceSelectionList(&buttonPad, T_REFRACTION, tmp_in, string_list_tracking);
  if (tmp_sel == tmp_in)
  {
    return;
  }
  switch (tmp_sel)
  {
  case 1:
  case 2:
  {
    if (tmp_sel == 1 ? SetLX200(":SXrg,y#") == LX200_VALUESET : SetLX200(":SXrg,n#") == LX200_VALUESET)
    {
      DisplayMessage(T_REFRACTION, (tmp_sel == 1) ? T_ON : T_OFF, 1000);
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
  char outRate[20];
  char cmd[20];
  if (DisplayMessageLX200(GetLX200(":GXR0#", outRate, sizeof(outRate))))
  {
    float guiderate = atof(&outRate[0]);
    if (display->UserInterfaceInputValueFloat(&buttonPad, T_GUIDESPEED, "", &guiderate, 0.1f, 1.f, 4u, 2u, "x"))
    {
      sprintf(cmd, ":SXR0,%03d#", (int)(guiderate * 100));
      DisplayMessageLX200(SetLX200(cmd),false);
    }
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
  if (DisplayMessageLX200(GetLX200(cmd, outRate, sizeof(outRate))))
  {
    float rate = (float)atol(&outRate[0])/10000.0;
    if (display->UserInterfaceInputValueFloat(&buttonPad, title, "", &rate, -2.f, 2.f, 6u, 4u, unit))
    {
      sprintf(cmd, ":SXRx,%06ld#", (long)(rate*10000));
      cmd[4] = axis == 0 ? 'e' : 'f';
      DisplayMessageLX200(SetLX200(cmd),false);
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
      DisplayMessageLX200(SetLX200(cmd), false);
    }
  }

}


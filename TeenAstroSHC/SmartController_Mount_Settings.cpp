#include "SmartController.h"
#include "SHC_text.h"

void SmartHandController::menuMount()
{
  static uint8_t s_sel = 1;
  uint8_t tmp_sel;
  while (!exitMenu)
  {
    const char *string_list_Mount = T_SHOWSETTINGS "\n" T_MOUNTTYPE "\n" T_MOTOR " 1\n" T_MOTOR " 2\n" T_ACCELERATION "\n" T_SPEED "\n" T_RETICULE;
    tmp_sel = display->UserInterfaceSelectionList(&buttonPad, T_MOUNT, s_sel, string_list_Mount);
    s_sel = tmp_sel > 0 ? tmp_sel : s_sel;
    switch (tmp_sel)
    {
    case 0:
      return;
    case 1:
      DisplayMountSettings();
      break;
    case 2:
      menuMountType();
      break;
    case 3:
      menuMotor(1);
      break;
    case 4:
      menuMotor(2);
      break;
    case 5:
      menuAcceleration();
      break;
    case 6:
      MenuRates();
      break;
    case 8:
      menuReticule();
      break;
    default:
      break;
    }
  }
}

void SmartHandController::MenuRates()
{
  static uint8_t s_sel = 1;
  uint8_t tmp_sel;
  while (!exitMenu)
  {
    const char *string_list = T_MAX "\n" T_FAST "\n" T_MEDIUM " \n" T_SLOW " \n" T_GUIDESPEED;
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
    default:
      break;
    }
  }
}

void SmartHandController::DisplayMountSettings()
{
  DisplayAccMaxRateSettings();
  DisplayMotorSettings(1);
  DisplayMotorSettings(2);
}

void SmartHandController::DisplayAccMaxRateSettings()
{
  char out[20];
  char line1[32] = T_SLEWSETTING;
  char line3[32] = "";
  char line4[32] = "";
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
  DisplayLongMessage(line1, NULL, line3, line4, -1);
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
    DisplayMessageLX200(SetLX200(out), false);
    delay(1000);
    Serial.end();
    exitMenu = true;
    powerCycleRequired = true;
    Serial.println(out);
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
      sprintf(cmd, ":SXR0:%03d#", (int)(guiderate * 100));
      DisplayMessageLX200(SetLX200(cmd));
    }
  }
}

void SmartHandController::menuRate(int r)
{
  char outRate[20];
  char cmd[20];
  sprintf(cmd, ":GXR%d#", r);
  char title[20] ;
  r == 3 ? strcpy(title, T_FASTSPEED) : (r == 2 ? strcpy(title, T_MEDIUMSPEED) : strcpy(title, T_SLOWSPEED));
  if (DisplayMessageLX200(GetLX200(cmd, outRate, sizeof(outRate))))
  {
    float rate = atof(&outRate[0]);
    if (display->UserInterfaceInputValueFloat(&buttonPad, title, "", &rate, 1.f, 255.f, 4u, 0u, "x"))
    {
      sprintf(cmd, ":SXR%d:%03d#", r, (int)(rate));
      DisplayMessageLX200(SetLX200(cmd));
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
    if (display->UserInterfaceInputValueFloat(&buttonPad, T_MAXSPEED, "", &maxrate, 32, 4000, 4, 0, ""))
    {
      sprintf(cmd, ":SXRX:%04d#", (int)maxrate);
      DisplayMessageLX200(SetLX200(cmd));
    }
  }
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
      sprintf(cmd, ":SXRA:%04d#", (int)(acc*10.));
      DisplayMessageLX200(SetLX200(cmd));
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
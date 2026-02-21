#include "SmartController.h"
#include "SHC_text.h"

void SmartHandController::menuFocuserSettings()
{
  
  if (!ta_MountStatus.hasFocuser())
  {
    DisplayMessage(T_FOCUSER, T_NOT_CONNECTED "!", 500);
    return;
  }
  buttonPad.setMenuMode();
  const char *string_list_Focuser = T_CONFIG "\n" T_MOTOR "\n" T_FOCUSERINFO;
  static uint8_t s_sel = 1;
  uint8_t tmp_sel;
  while (!exitMenu)
  {
    tmp_sel = display->UserInterfaceSelectionList(&buttonPad, T_FOCUSERSETTINGS, s_sel, string_list_Focuser);
    s_sel = tmp_sel > 0 ? tmp_sel : s_sel;
    switch (tmp_sel)
    {
    case 0:
      exitMenu = true;
      break;
    case 1:
    {
      menuFocuserConfig();
      break;
    }
    case 2:
    {
      menuFocuserMotor();
      break;
    }
    case 3:
    {
      menuFocuserInfo();
      break;
    }
    default:
      break;
    }
  }
  buttonPad.setControlerMode();
}
void SmartHandController::menuFocuserInfo()
{
  char out[50];
  const char* string_list_Focuser = T_SHOWVERSION "\n" T_REBOOT "\n" T_RESETTOFACTORY;

  static uint8_t s_sel = 1;
  uint8_t tmp_sel;

  while (!exitMenu)
  {

    tmp_sel = display->UserInterfaceSelectionList(&buttonPad, T_FOCUSERINFO, s_sel, string_list_Focuser);
    s_sel = tmp_sel > 0 ? tmp_sel : s_sel;
    switch (tmp_sel)
    {
    case 0:
      return;
    case 1:
      if (DisplayMessageLX200(m_client->getFocuserVersion(out, sizeof(out))))
      {
        out[31] = 0;
        DisplayMessage(T_FIRMWAREVERSION, &out[26], -1);
      }
      return;
      break;
    case 2:
      DisplayMessageLX200(m_client->focuserResetConfig());
      DisplayMessage(T_FOCUSER, T_REBOOT, -1);
      delay(500);
      powerCycleRequired = true;
      exitMenu = true;
      return;
    case 3:
      if (display->UserInterfaceMessage(&buttonPad, "Reset", "To", "Factory?", "NO\nYES") == 2)
      {
        DisplayMessageLX200(m_client->focuserSaveConfig(), false);
        DisplayMessage(T_FOCUSER, T_RESET, -1);
        delay(500);
        powerCycleRequired = true;
        exitMenu = true;
        return;
      }
      break;
    }

  }
}
void SmartHandController::menuFocuserConfig()
{
  char cmd[50];
  const char *string_list_Focuser = T_DISPLAYSETTINGS "\n" T_PARKPOSITION "\n" T_MAXPOSITION "\n" T_MANUALSPEED "\n" T_GOTOSPEED "\n" T_ACCFORMAN "\n" T_ACCFORGOTO;
  unsigned int sP, maxP, minS, maxS, cmdAcc, manAcc, manDec;
  static uint8_t s_sel = 1;
  uint8_t tmp_sel;

  float value;
  while (!exitMenu)
  {
    if (DisplayMessageLX200(m_client->readFocuserConfig(sP, maxP, minS, maxS, cmdAcc, manAcc, manDec)))
    {
      tmp_sel = display->UserInterfaceSelectionList(&buttonPad, T_FOCUSERSETTINGS, s_sel, string_list_Focuser);
      s_sel = tmp_sel > 0 ? tmp_sel : s_sel;
      bool ValueSetRequested = false;
      switch (tmp_sel)
      {
      case 0:
        return;
        break;
      case 1:
      {
        char line1[32] = "";
        char line2[32] = "";
        char line3[32] = "";
        char line4[32] = "";
        sprintf(line1, T_FOCUSERSETTINGS);
        sprintf(line3, T_STARTPOSITION2 ": %05u", sP);
        sprintf(line4, T_MAXPOSITION2 ": %05u", maxP);
        DisplayLongMessage(line1, line2, line3, line4, -1);
        line2[0] = 0;
        sprintf(line3, T_MANUALSPEED2 ": %03u", minS);
        sprintf(line4, T_GOTOSPEED2 ": %03u", maxS);
        DisplayLongMessage(line1, line2, line3, line4, -1);
        sprintf(line2, T_ACCFORGOTO2 ": %03u", cmdAcc);
        sprintf(line3, T_ACCFORMAN2 ": %03u", manAcc);
        sprintf(line4, T_DECFORMAN2 ": %03u", manDec);
        DisplayLongMessage(line1, line2, line3, line4, -1);
        break;
      }
      case 2:
      {
        value = sP;
        ValueSetRequested = display->UserInterfaceInputValueFloat(&buttonPad, T_PARKPOSITION, "", &value, 0, 65535, 5, 0, "");
        sprintf(cmd, ":F0,%05d#", (int)(value));
        break;
      }
      case 3:
      {
        value = maxP;
        ValueSetRequested = display->UserInterfaceInputValueFloat(&buttonPad, T_MAXPOSITION, "", &value, 0, 65535, 5, 0, "");
        sprintf(cmd, ":F1,%05d#", (int)(value));
        break;
      }
      case 4:
      {
        value = minS;
        ValueSetRequested = display->UserInterfaceInputValueFloat(&buttonPad, T_MANUALSPEED, "", &value, 1, 999, 5, 0, "");
        sprintf(cmd, ":F2,%03d#", (int)(value));
        break;
      }
      case 5:
      {
        value = maxS;
        ValueSetRequested = display->UserInterfaceInputValueFloat(&buttonPad, T_GOTOSPEED, "", &value, 1, 999, 5, 0, "");
        sprintf(cmd, ":F3,%03d#", (int)(value));
        break;
      }
      case 6:
      {
        value = manAcc;
        ValueSetRequested = display->UserInterfaceInputValueFloat(&buttonPad, T_ACCFORMAN, "", &value, 1, 100, 5, 0, "");
        sprintf(cmd, ":F5,%03d#", (int)(value));
        break;
      }
      case 7:
      {
        value = cmdAcc;
        ValueSetRequested = display->UserInterfaceInputValueFloat(&buttonPad, T_ACCFORGOTO, "", &value, 1, 100, 5, 0, "");
        sprintf(cmd, ":F4,%03d#", (int)(value));
        break;
      }
      case 8:
      {
        value = manDec;
        ValueSetRequested = display->UserInterfaceInputValueFloat(&buttonPad, T_DECFORMAN, "", &value, 1, 100, 5, 0, "");
        sprintf(cmd, ":F6,%03d#", (int)(value));
        break;
      }
      default:
        break;
      }
      if (ValueSetRequested)
      {
        DisplayMessageLX200(m_client->set(cmd), false);
      }
    }
    else
      break;
  }
}
void SmartHandController::menuFocuserMotor()
{
  char cmd[50];
  const char *string_list_Focuser = T_DISPLAYSETTINGS "\n" T_RESOLUTION "\n" T_ROTATION "\n" T_STEPSPERROT "\n" T_MICROSTEP "\n" T_CURRENT;
  unsigned int res, mu, curr, steprot;
  bool rev;
  float value;
  static uint8_t s_sel = 1;
  uint8_t tmp_sel;
  while (!exitMenu)
  {
    if (DisplayMessageLX200(m_client->readFocuserMotor(rev, mu, res, curr, steprot)))
    {
      tmp_sel = display->UserInterfaceSelectionList(&buttonPad, T_FOCUSERSETTINGS, s_sel, string_list_Focuser);
      s_sel = tmp_sel > 0 ? tmp_sel : s_sel;
      bool ValueSetRequested = false;
      switch (tmp_sel)
      {
      case 0:
        return;
        break;
      case 1:
      {
        char line1[32] = "";
        char line2[32] = "";
        char line3[32] = "";
        char line4[32] = "";
        sprintf(line1, "Motor Settings");//todo
        rev ? sprintf(line2, T_REVERSEDROTATION) : sprintf(line2, T_DIRECTROTATION);
        sprintf(line3, T_RESOLUTION"  : %03u", res);
        DisplayLongMessage(line1, line2, line3, line4, -1);

        sprintf(line2, T_STEPS " : %03u", steprot);
        sprintf(line3, T_MICROSTEP " : %03u", (unsigned int)pow(2, mu));
        sprintf(line4, T_CURRENT " : %04umA", curr * 10);
        DisplayLongMessage(line1, line2, line3, line4, -1);
        break;
      }
      case 2:
      {
        value = res;
        ValueSetRequested = display->UserInterfaceInputValueFloat(&buttonPad, T_INCREMENTATION, "", &value, 1, 512, 5, 0, " " T_MICROSTEP);
        sprintf(cmd, ":F8,%03d#", (int)(value));
        break;
      }
      case 3:
      {
        const char* string_list = T_DIRECT "\n" T_REVERSEDROTATION;
        uint8_t choice = display->UserInterfaceSelectionList(&buttonPad, T_ROTATION, (uint8_t)rev + 1, string_list);
        if (choice)
        {
          sprintf(cmd, ":F7,%d#", (int)(choice - 1));
          ValueSetRequested = true;
        }
        break;
      }
      case 4:
      {
        value = steprot;
        ValueSetRequested = display->UserInterfaceInputValueFloat(&buttonPad, T_STEPSPERROT, "", &value, 10, 800, 3, 0, " " T_MICROSTEP);
        sprintf(cmd, ":Fr,%03d#", (int)(value));
        break;
      }
      case 5:
      {
        uint8_t microStep = mu;
        const char* string_list_micro = "4\n8\n16\n32\n64\n128";
        uint8_t choice = microStep - 2 + 1;
        choice = display->UserInterfaceSelectionList(&buttonPad, T_MICROSTEP, choice, string_list_micro);
        if (choice)
        {
          microStep = choice - 1 + 2;
          sprintf(cmd, ":Fm,%d#", microStep);
          ValueSetRequested = true;
        }
        break;
      }
      case 6:
      {
        value = curr;
        ValueSetRequested = display->UserInterfaceInputValueFloat(&buttonPad, T_CURRENT, "", &value, 1, 160, 10, 0, "0 mA");
        sprintf(cmd, ":Fc,%03d#", (int)(value));
        break;
      }
      default:
        break;
      }
      if (ValueSetRequested)
      {
        DisplayMessageLX200(m_client->set(cmd), false);
        delay(250);
      }
    }
    else
      break;
  }
}

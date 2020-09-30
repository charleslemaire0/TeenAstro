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
  const char *string_list_Focuser = T_CONFIG "\n" T_MOTOR "\n" T_SHOWVERSION;
  static uint8_t s_sel = 1;
  uint8_t tmp_sel;
  while (!exitMenu)
  {
    tmp_sel = display->UserInterfaceSelectionList(&buttonPad, T_FOCUSERSETTINGS, s_sel, string_list_Focuser);
    s_sel = tmp_sel > 0 ? tmp_sel : s_sel;
    bool ValueSetRequested = false;
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
      char out1[50];
      if (DisplayMessageLX200(GetLX200(":FV#", out1, 50)))
      {
        out1[31] = 0;
        DisplayMessage(T_FIRMWAREVERSION, &out1[26], -1);
      }
      break;
    }
    default:
      break;
    }
  }
  buttonPad.setControlerMode();
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
    if (DisplayMessageLX200(readFocuserConfig(sP, maxP, minS, maxS, cmdAcc, manAcc, manDec)))
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
        DisplayMessageLX200(SetLX200(cmd), false);
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
    if (DisplayMessageLX200(readFocuserMotor(rev, mu, res, curr, steprot)))
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
        char * string_list = T_DIRECT "\n" T_REVERSEDROTATION;
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
        char * string_list_micro = "4\n8\n16\n32\n64\n128";
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
        DisplayMessageLX200(SetLX200(cmd), false);
        delay(250);
      }
    }
    else
      break;
  }
}
void SmartHandController::menuFocuserAction()
{
  if (!ta_MountStatus.hasFocuser())
  {
    DisplayMessage(T_FOCUSER, T_NOT_CONNECTED "!", 500);
    return;
  }
  static  uint8_t current_selection = 1;
  uint8_t choice;
  buttonPad.setMenuMode();
  int pos[10];
  int idx = 0;
  int idxs[10] = { -1,-1,-1,-1,-1,-1,-1,-1,-1,-1 };
  char temp[20] = { 0 };
  char txt[150] = { 0 };
  char out[20];

  for (int k = 0; k < 10; k++)
  {
    sprintf(temp, ":Fx%d#", k);
    GetLX200(temp, out, 20);
    if (out[0] == 'P')
    {
      strcat(txt, &out[7]);
      strcat(txt, "\n");
      idxs[idx] = k;
      idx++;
    }
    else
      continue;
  }
  while (!exitMenu)
  {
    char menustxt[200] = {};
    if (focuserlocked)
    {
      strcat(menustxt, T_UNLOCK);
    }
    else
    {
      strcat(menustxt, txt);
      strcat(menustxt, T_GOTO "\n" T_SYNC "\n" T_PARK "\n" T_LOCK);
    }

    const char *string_list_Focuser = &menustxt[0];
    choice = display->UserInterfaceSelectionList(&buttonPad, T_FOCUSERACTION, current_selection, string_list_Focuser);
    if (choice != 0)
    {
      current_selection = choice;
    }
    if (focuserlocked)
    {
      switch (choice)
      {
      case 0:
        exitMenu = true;
        break;
      case 1:
        focuserlocked = false;
        exitMenu = true;
        break;
      default:
        break;
      }
    }
    else
    {
      if (choice == 0)
      {
        exitMenu = true;
        break;
      }
      else if (choice - 1 < idx)
      {
        char cmd[15];
        sprintf(cmd, ":Fg%d#", idxs[choice - 1]);
        DisplayMessage(T_GOTO, T_POSITION, 1000);
        SetLX200(cmd);
        exitMenu = true;
      }
      else
      {
        switch (choice - idx)
        {
        case 1:
        {
          if (display->UserInterfaceInputValueFloat(&buttonPad, T_GOTOPOSITION, "", &FocuserPos, 0, 65535, 5, 0, ""))
          {
            char cmd[15];
            sprintf(cmd, ":FG,%05d#", (int)(FocuserPos));
            DisplayMessage(T_GOTO, T_POSITION, 1000);
            SetLX200(cmd);
            exitMenu = true;
          }
          break;
        }
        case 2:
        {
          if (display->UserInterfaceInputValueFloat(&buttonPad, T_SYNCPOSITION, "", &FocuserPos, 0, 65535, 5, 0, ""))
          {
            char cmd[15];
            sprintf(cmd, ":FS,%05d#", (int)(FocuserPos));
            DisplayMessage(T_SYNCEDAT, T_POSITION, 1000);
            SetLX200(cmd);
            exitMenu = true;
          }
          break;
        }
        case 3:
        {
          SetLX200(":FP#");
          exitMenu = true;
          break;
        }
        case 4:
          focuserlocked = true;
          exitMenu = true;
          break;
        default:
          break;
        }
      }
    }
  }
  buttonPad.setControlerMode();
}



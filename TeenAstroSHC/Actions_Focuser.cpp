#include "SmartController.h"
#include "SHC_text.h"

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

    const char* string_list_Focuser = &menustxt[0];
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

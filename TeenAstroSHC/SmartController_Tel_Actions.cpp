#include "SmartController.h"
#include "SHC_text.h"

void SmartHandController::menuSpeedRate()
{
  buttonPad.setMenuMode();
  char * string_list_Speed = T_GUIDE "\n" T_SLOW "\n" T_MEDIUM "\n" T_FAST "\n" T_MAX;
  static unsigned char current_selection_speed = 3;
  ta_MountStatus.updateMount();
  if (!ta_MountStatus.getGuidingRate(current_selection_speed))
    return;
  uint8_t selected_speed = display->UserInterfaceSelectionList(&buttonPad, T_SETSPEED, current_selection_speed + 1, string_list_Speed);
  if (selected_speed > 0)
  {
    char cmd[5] = ":Rn#";
    cmd[2] = '0' + selected_speed - 1;
    SetLX200(cmd);
    current_selection_speed = selected_speed;
  }
  buttonPad.setControlerMode();
}

void SmartHandController::menuTelAction()
{
  buttonPad.setMenuMode();

  static uint8_t s_sel = 1;
  uint8_t tmp_sel;
  while (!exitMenu)
  {
    ta_MountStatus.updateMount();
    TeenAstroMountStatus::ParkState currentstate = ta_MountStatus.getParkState();

    if (currentstate == TeenAstroMountStatus::PRK_PARKED)
    {
      const char *string_list_main_ParkedL0 = T_UNPARK;
      tmp_sel = display->UserInterfaceSelectionList(&buttonPad, T_TELESCOPEACTION, s_sel, string_list_main_ParkedL0);
      s_sel = tmp_sel > 0 ? tmp_sel : s_sel;
      switch (tmp_sel)
      {
      case 0:
        exitMenu = true;
        break;
      case 1:
        SetLX200(":hR#");
        exitMenu = true;
        break;
      default:
        break;
      }
    }
    else if (currentstate == TeenAstroMountStatus::PRK_UNPARKED)
    {
      const char *string_list_main_UnParkedL0 = telescoplocked ? T_UNLOCK : T_GOTO "\n" T_SYNC "\n" T_ALIGN "\n" T_TRACKING "\n" T_SIDEOFPIER "\n" T_SAVE " RADEC\n" T_LOCK "\n" T_SPIRAL;
      tmp_sel = display->UserInterfaceSelectionList(&buttonPad, T_TELESCOPEACTION, s_sel, string_list_main_UnParkedL0);
      s_sel = tmp_sel > 0 ? tmp_sel : s_sel;
      MENU_RESULT answer = MR_CANCEL;
      if (telescoplocked)
      {
        switch (tmp_sel)
        {
        case 0:
          exitMenu = true;
          break;
        case 1:
          telescoplocked = false;
          exitMenu = true;
          break;
        default:
          break;
        }
      }
      else
      {
        switch (tmp_sel)
        {
        case 0:
          exitMenu = true;
          break;
        case 1:
          answer = menuSyncGoto(false);
          answer == MR_OK || answer == MR_QUIT ? exitMenu = true : exitMenu = false;
          break;
        case 2:
          answer = menuSyncGoto(true);
          answer == MR_OK || answer == MR_QUIT ? exitMenu = true : exitMenu = false;
          break;
        case 3:
          answer = menuAlignment();
          answer == MR_OK || answer == MR_QUIT ? exitMenu = true : exitMenu = false;
          break;
        case 4:
          menuTrack();
          break;
        case 5:
          menuPier();
          break;
        case 6:
          if (SetLX200(":SU#") == LX200VALUESET)
          {
            DisplayMessage("RA DEC", T_SAVED, 500);
          }
          else
          {
            DisplayMessage(T_LX200COMMAND, T_FAILED, 1000);
          }
          break;
        case 7:
          telescoplocked = true;
          exitMenu = true;
          break;
        case 8:
          if (SetLX200(":M@#") == LX200VALUESET)
          {
            DisplayMessage(T_SPIRAL, T_STARTED, 500);
          }
          else
          {
            DisplayMessage(T_LX200COMMAND, T_FAILED, 1000);
          }
          exitMenu = true;
          break;
        default:
          break;
        }
      }
    }
  }
  if (!sleepDisplay)
  {
    buttonPad.setControlerMode();
  }
}

void SmartHandController::menuTrack()
{
  ta_MountStatus.updateMount();
  TeenAstroMountStatus::TrackState currentstate = ta_MountStatus.getTrackingState();
  uint8_t tmp_sel;
  if (currentstate == TeenAstroMountStatus::TRK_ON)
  {
    const char *string_list_tracking = T_STOPTRACKING "\n" T_SIDEREAL "\n" T_LUNAR "\n" T_SOLAR;
    tmp_sel = display->UserInterfaceSelectionList(&buttonPad, T_TRACKINGSTATE, 0, string_list_tracking);
    switch (tmp_sel)
    {
    case 1:
      char out[20];
      memset(out, 0, sizeof(out));
      if (SetLX200(":Td#") == LX200VALUESET)
      {
        DisplayMessage(T_TRACKING, T_OFF, 500);
        exitMenu = true;
      }
      else
      {
        DisplayMessage(T_LX200COMMAND, T_FAILED, 1000);
      }
      break;
    case 2:
      exitMenu = DisplayMessageLX200(SetLX200(":TQ#"));
      break;
    case 3:
      exitMenu = DisplayMessageLX200(SetLX200(":TL#"));
      break;
    case 4:
      exitMenu = DisplayMessageLX200(SetLX200(":TS#"));
      break;
    default:
      break;
    }
  }
  else if (currentstate == TeenAstroMountStatus::TRK_OFF)
  {
    const char *string_list_tracking = T_STARTTRACKING;
    tmp_sel = display->UserInterfaceSelectionList(&buttonPad, T_TRACKINGSTATE, 0, string_list_tracking);
    switch (tmp_sel)
    {
    case 1:
      if (SetLX200(":Te#") == LX200VALUESET)
      {
        DisplayMessage(T_TRACKING, T_ON, 500);
        exitMenu = true;
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
  else
  {
    DisplayMessage(T_CURRENTLYTRACKING, T_CANNOTBECHANGED, 1000);
  }
}

SmartHandController::MENU_RESULT SmartHandController::menuAlignment()
{
  bool alignInProgress = ta_MountStatus.isAligning();
  static int current_selection = 1;
  while (true)
  {
    const char* string_list = alignInProgress ? T_CANCEL : "2 " T_STAR "\n3 " T_STAR "\n" T_PC " " T_ALIGNMENT  "\n" T_SAVE "\n" T_Clear;
    int selection = display->UserInterfaceSelectionList(&buttonPad, T_ALIGNMENT, current_selection, string_list);
    if (selection == 0) return MR_CANCEL;
    current_selection = selection;
    switch (current_selection)
    {
    case 1:
      if (alignInProgress)
      {
        ta_MountStatus.stopAlign();
        DisplayMessage(T_ALIGNMENT, T_CANCELED, -1);
        return MR_QUIT;
      }
      else
      {
        DisplayLongMessage("!" T_WARNING "!", T_THEMOUNTMUSTBEATHOME1, T_THEMOUNTMUSTBEATHOME2, T_THEMOUNTMUSTBEATHOME3, -1);
        if (display->UserInterfaceMessage(&buttonPad, T_READYFOR, "2 " T_STAR, T_ALIGNMENT "?", T_NO "\n" T_YES) == 2)
        {
          if (SetLX200(":A0#") == LX200VALUESET)
          {
            ta_MountStatus.startAlign(TeenAstroMountStatus::AlignMode::ALIM_TWO);
            return MR_QUIT;
          }
          else
          {
            DisplayMessage(T_INITIALISATION, T_FAILED, -1);
          }
        }
      }
      break;
    case 2:
      DisplayLongMessage("!" T_WARNING "!", T_THEMOUNTMUSTBEATHOME1, T_THEMOUNTMUSTBEATHOME2, T_THEMOUNTMUSTBEATHOME3, -1);
      if (display->UserInterfaceMessage(&buttonPad, T_READYFOR, "3 " T_STAR, T_ALIGNMENT "?", T_NO "\n" T_YES) == 2)
      {
        if (SetLX200(":A0#") == LX200VALUESET)
        {
          ta_MountStatus.startAlign(TeenAstroMountStatus::AlignMode::ALIM_THREE);
          return MR_QUIT;
        }
        else
        {
          DisplayMessage(T_INITIALISATION, T_FAILED, -1);
        }
      }
      break;
      case 4:
      if (display->UserInterfaceMessage(&buttonPad, T_SAVE, T_STAR, T_ALIGNMENT "?", T_NO "\n" T_YES) == 2)
      {
        if (SetLX200(":AW#") == LX200VALUESET)
        {
          DisplayMessage(T_ALIGNMENT, T_SAVED, -1);
          return MR_QUIT;
        }
        else
        {
          DisplayMessage(T_SAVING, T_FAILED, -1);
        }
      }
      break;
    case 5:
      if (display->UserInterfaceMessage(&buttonPad, T_Clear, T_STAR, T_ALIGNMENT "?", T_NO "\n" T_YES) == 2)
      {
        if (SetLX200(":AC#") == LX200VALUESET)
        {
          DisplayMessage(T_MOUNTSYNCED, T_ATHOME, -1);
          return MR_QUIT;
        }
        else
        {
          DisplayMessage(T_Clear, T_FAILED, -1);
        }
      }
      break;
      case 3:
      DisplayLongMessage("!" T_WARNING "!", T_THEMOUNTMUSTBEATHOME1, T_THEMOUNTMUSTBEATHOME2, T_THEMOUNTMUSTBEATHOME3, -1);
      if (display->UserInterfaceMessage(&buttonPad, T_READYFOR, T_PC, T_ALIGNMENT "?", T_NO "\n" T_YES) == 2)
      {
        if (SetLX200(":AA#") == LX200VALUESET)
        {
          DisplayMessage(T_MOUNTSYNCED, T_ATHOME, -1);
          return MR_QUIT;
        }
        else
        {
          DisplayMessage(T_Clear, T_FAILED, -1);
        }
      }
      break;
    }
  }
}

bool SmartHandController::SelectStarAlign()
{
  buttonPad.setMenuMode();
  double lat, LT0;
  while (!ta_MountStatus.getLat(lat))
  {
  }
  while (!ta_MountStatus.getLstT0(LT0))
  {
  }
  cat_mgr.setLat(lat);
  cat_mgr.setLstT0(LT0);
  bool ok = menuCatalogAlign() != SmartHandController::MENU_RESULT::MR_CANCEL;
  buttonPad.setControlerMode();
  return ok;
}
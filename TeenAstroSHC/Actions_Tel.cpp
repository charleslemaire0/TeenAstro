#include "SmartController.h"
#include "SHC_text.h"

void SmartHandController::menuSpeedRate()
{
  ta_MountStatus.updateMount();
  if (ta_MountStatus.isPushTo())
    return;
  buttonPad.setMenuMode();
  char* string_list_Speed = T_GUIDE "\n" T_SLOW "\n" T_MEDIUM "\n" T_FAST "\n" T_MAX;
  static unsigned char current_selection_speed = 3;
  TeenAstroMountStatus::GuidingRate cur_GR = ta_MountStatus.getGuidingRate();
  if (cur_GR == TeenAstroMountStatus::GuidingRate::UNKNOW)
    return;
  current_selection_speed = static_cast<unsigned char>(cur_GR);
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

#ifdef NO_SPEED_MENU
void SmartHandController::increaseSpeed(bool increase)
{
  ta_MountStatus.updateMount();
  if (ta_MountStatus.isPushTo())
    return;
  char* string_list_Speed = T_GUIDE "\n" T_SLOW "\n" T_MEDIUM "\n" T_FAST "\n" T_MAX;
  static unsigned char current_speed = 3;
  TeenAstroMountStatus::GuidingRate cur_GR = ta_MountStatus.getGuidingRate();
  if (cur_GR == TeenAstroMountStatus::GuidingRate::UNKNOW)
    return;
  current_speed = static_cast<unsigned char>(cur_GR);
  char cmd[5] = ":Rn#";
  if (increase && cur_GR < TeenAstroMountStatus::MAX )
  {    
    cmd[2] = '0' + current_speed + 1;
    SetLX200(cmd);
  }
  else if (!increase && cur_GR > TeenAstroMountStatus::GUIDING )
  {
    cmd[2] = '0' + current_speed - 1;
    SetLX200(cmd);
  }
}
#endif

void SmartHandController::menuTelActionPushTo()
{
  buttonPad.setMenuMode();
  telescoplocked = false;
  static uint8_t s_sel = 1;
  uint8_t tmp_sel;
  while (!exitMenu)
  {
    ta_MountStatus.updateMount();
    const char* string_list = T_PUSHTO "\n" T_SYNC "\n" T_ALIGN "\n" T_SIDEOFPIER "\n" T_SAVE " RADEC";
    tmp_sel = display->UserInterfaceSelectionList(&buttonPad, T_TELESCOPEACTION, s_sel, string_list);
    s_sel = tmp_sel > 0 ? tmp_sel : s_sel;
    MENU_RESULT answer = MR_CANCEL;
    switch (tmp_sel)
    {
    case 0:
      exitMenu = true;
      break;
    case 1:
      answer = menuSyncGoto(NAV_PUSHTO);
      answer == MR_OK || answer == MR_QUIT ? exitMenu = true : exitMenu = false;
      break;
    case 2:
      answer = menuSyncGoto(NAV_SYNC);
      answer == MR_OK || answer == MR_QUIT ? exitMenu = true : exitMenu = false;
      break;
    case 3:
      answer = menuAlignment();
      answer == MR_OK || answer == MR_QUIT ? exitMenu = true : exitMenu = false;
      break;
    case 4:
      menuPier();
      break;
    case 5:
      if (SetLX200(":SU#") == LX200_VALUESET)
      {
        DisplayMessage("RA DEC", T_SAVED, 500);
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
  if (!sleepDisplay)
  {
    buttonPad.setControlerMode();
  }

}

void SmartHandController::menuTelActionPushToGoto()
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
      const char* string_list_main_ParkedL0 = T_UNPARK;
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
      const char* string_list_main_UnParkedL0 = telescoplocked ? T_UNLOCK : T_GOTO "\n" T_PUSHTO "\n" T_SYNC "\n" T_ALIGN "\n" T_TRACKING "\n" T_SIDEOFPIER "\n" T_SAVE " RADEC\n" T_LOCK "\n" T_SPIRAL;
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
          answer = menuSyncGoto(NAV_GOTO);
          answer == MR_OK || answer == MR_QUIT ? exitMenu = true : exitMenu = false;
          break;
        case 2:
          answer = menuSyncGoto(NAV_PUSHTO);
          answer == MR_OK || answer == MR_QUIT ? exitMenu = true : exitMenu = false;
          break;
        case 3:
          answer = menuSyncGoto(NAV_SYNC);
          answer == MR_OK || answer == MR_QUIT ? exitMenu = true : exitMenu = false;
          break;
        case 4:
          answer = menuAlignment();
          answer == MR_OK || answer == MR_QUIT ? exitMenu = true : exitMenu = false;
          break;
        case 5:
          menuTrack();
          break;
        case 6:
          menuPier();
          break;
        case 7:
          if (SetLX200(":SU#") == LX200_VALUESET)
          {
            DisplayMessage("RA DEC", T_SAVED, 500);
          }
          else
          {
            DisplayMessage(T_LX200COMMAND, T_FAILED, 1000);
          }
          break;
        case 8:
          telescoplocked = true;
          exitMenu = true;
          break;
        case 9:
        {
          menuSpirale();
          break;
        }
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

void SmartHandController::menuTelActionGoto()
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
      const char* string_list_main_ParkedL0 = T_UNPARK;
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
      const char* string_list_main_UnParkedL0 = telescoplocked ? T_UNLOCK : T_GOTO "\n" T_SYNC "\n" T_ALIGN "\n" T_TRACKING "\n" T_SIDEOFPIER "\n" T_SAVE " RADEC\n" T_LOCK "\n" T_SPIRAL;
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
          answer = menuSyncGoto(NAV_GOTO);
          answer == MR_OK || answer == MR_QUIT ? exitMenu = true : exitMenu = false;
          break;
        case 2:
          answer = menuSyncGoto(NAV_SYNC);
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
          if (SetLX200(":SU#") == LX200_VALUESET)
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
        {
          menuSpirale();
          break;
        }
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
    const char* string_list_tracking = T_STOPTRACKING "\n" T_SIDEREAL "\n" T_LUNAR "\n" T_SOLAR "\n" T_TARGET;
    tmp_sel = display->UserInterfaceSelectionList(&buttonPad, T_TRACKINGSTATE, 0, string_list_tracking);
    switch (tmp_sel)
    {
    case 1:
      char out[20];
      memset(out, 0, sizeof(out));
      if (SetLX200(":Td#") == LX200_VALUESET)
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
    case 5:
      exitMenu = DisplayMessageLX200(SetLX200(":TT#"));
      break;
    default:
      break;
    }
  }
  else if (currentstate == TeenAstroMountStatus::TRK_OFF)
  {
    const char* string_list_tracking = T_STARTTRACKING;
    tmp_sel = display->UserInterfaceSelectionList(&buttonPad, T_TRACKINGSTATE, 0, string_list_tracking);
    switch (tmp_sel)
    {
    case 1:
      if (SetLX200(":Te#") == LX200_VALUESET)
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
    const char* string_list = alignInProgress ? T_CANCEL :
      (ta_MountStatus.isAligned() ?
        "2 " T_STARS "\n" T_PC " " T_ALIGNMENT  "\n" T_SAVE "\n" T_Clear "\nShow align. error" :
        "2 " T_STARS "\n" T_PC " " T_ALIGNMENT//  "\n" T_SAVE "\n" T_Clear
        );
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
        int ret = display->UserInterfaceMessage(&buttonPad, T_SELECTMODE, "2 " T_STAR, T_ALIGNMENT , T_HOME "\n" T_STAR);
        if (ret == 1)
        {
          DisplayLongMessage("!" T_WARNING "!", T_THEMOUNTMUSTBEATHOME1, T_THEMOUNTMUSTBEATHOME2, T_THEMOUNTMUSTBEATHOME3, -1);

          if (display->UserInterfaceMessage(&buttonPad, T_READYFOR, "2 " T_STAR, T_ALIGNMENT "?",T_YES "\n" T_NO  ) == 1)
          {
            if (SetLX200(":A0#") == LX200_VALUESET)
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
        else if (ret == 2)
        {
          ta_MountStatus.updateMount();
          uint8_t choice = ((uint8_t)ta_MountStatus.getPierState());
          choice = display->UserInterfaceSelectionList(&buttonPad, T_SETSIDEOFPIER, choice, T_EAST "\n" T_WEST);
          bool ok = false;
          if (choice)
          {
            if (choice == 1)
              ok = DisplayMessageLX200(SetLX200(":SmE#"));
            else
              ok = DisplayMessageLX200(SetLX200(":SmW#"));
            if (ok)
            {
              DisplayMessage("Please Sync", "with a Target", 1000);
              cat_mgr.select(0);
              char title[20] = "";
              cat_mgr.filtersClear();
              cat_mgr.filterAdd(FM_OBJ_HAS_NAME);
              cat_mgr.filterAdd(FM_ABOVE_HORIZON, 1);

              strcat(title, cat_mgr.catalogTitle());
              double lat, LT0;
              while (!ta_MountStatus.getLat(lat))
              {
              }
              while (!ta_MountStatus.getLstT0(LT0))
              {
              }
              cat_mgr.setLat(lat);
              cat_mgr.setLstT0(LT0);
              if (cat_mgr.isInitialized())
              {
                if (cat_mgr.setIndex(cat_mgr.getIndex()))
                {
                  if (display->UserInterfaceCatalog(&buttonPad, title))
                  {
                    LX200RETURN out = SyncGotoCatLX200(NAV_SYNC);
                    if (out == LX200_SYNCED)
                    {
                      DisplayMessageLX200(SetLX200(":A*#"));
                      ta_MountStatus.startAlignSecondStar(TeenAstroMountStatus::AlignMode::ALIM_TWO);           
                      return MR_QUIT;
                    }
                    else
                    {
                      DisplayMessageLX200(out);
                    }
                  }
                }
                else DisplayMessage(cat_mgr.catalogTitle(), "No Object", -1);
              }
              else DisplayMessage(cat_mgr.catalogTitle(), "Not Init'd?", -1);
              cat_mgr.filtersClear();

              DisplayMessageLX200(SetLX200(":SmN#"));
       
            }
          }
        }
      }
      break;
    case 2:
      DisplayLongMessage("!" T_WARNING "!", T_THEMOUNTMUSTBEATHOME1, T_THEMOUNTMUSTBEATHOME2, T_THEMOUNTMUSTBEATHOME3, -1);
      if (display->UserInterfaceMessage(&buttonPad, T_READYFOR, T_PC, T_ALIGNMENT "?", T_NO "\n" T_YES) == 2)
      {
        if (SetLX200(":AA#") == LX200_VALUESET)
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
      if (display->UserInterfaceMessage(&buttonPad, T_SAVE, T_STAR, T_ALIGNMENT "?", T_NO "\n" T_YES) == 2)
      {
        if (SetLX200(":AW#") == LX200_VALUESET)
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
    case 4:
      if (display->UserInterfaceMessage(&buttonPad, T_Clear, T_STAR, T_ALIGNMENT "?", T_NO "\n" T_YES) == 2)
      {
        if (SetLX200(":AC#") == LX200_VALUESET)
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
    case 5:
      char text[20];
      if (GetLX200(":AE#", text, sizeof(text)) == LX200_VALUEGET)
      {
        text[3]='°';
        text[6]='\'';
        text[9]='\"';
        //DisplayLongMessage("Alignment error:", "", "sDD*mm:ss", text, -1);
        DisplayMessage(T_ERROR, text, -1);
      }
      else
        DisplayMessage(T_ERROR, "?", -1);
      break;
        
    
/*      char err_az[15] = { "?" };
      char err_alt[15] = { "?" };
      char err_pol[15] = { "?" };
      if (
        GetLX200(":GXAw#", err_pol, sizeof(err_pol)) == LX200_VALUEGET
        &&
        GetLX200(":GXAz#", err_az, sizeof(err_az)) == LX200_VALUEGET
        &&
        GetLX200(":GXAa#", err_alt, sizeof(err_alt)) == LX200_VALUEGET)
      {
        DisplayLongMessage("[Sep.;Az.;Alt.]:", err_pol, err_az, err_alt, -1);
      }
      else
        DisplayMessage("Alignment error:", "?", -1);
      break;
*/
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
  bool ok = menuCatalogAlign(ta_MountStatus.isPushTo()?NAV_PUSHTO: NAV_GOTO) != SmartHandController::MENU_RESULT::MR_CANCEL;
  buttonPad.setControlerMode();
  return ok;
}
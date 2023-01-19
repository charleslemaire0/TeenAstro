#include "SmartController.h"
#include "SHC_text.h"

static void secondsToFloat(const long& v, float& f)
{
  f = (double)v / 3600.0;
  return;
}

SmartHandController::MENU_RESULT SmartHandController::menuSyncGoto(bool sync)
{
  static int current_selection = 1;

  while (true)
  {
    // build the list of star/dso catalogs
    const char* string_list_gotoL1 = sync ?
      T_CATALOGS "\n" T_SOLARSYSTEM "\n" T_COORDINATES "\n" T_USERDEFINED "\n" T_HOME "\n" T_PARK :
      T_CATALOGS "\n" T_SOLARSYSTEM "\n" T_COORDINATES "\n" T_USERDEFINED "\n" T_HOME "\n" T_PARK "\n" T_FLIP;
    int selection = display->UserInterfaceSelectionList(&buttonPad, sync ? T_SYNC : T_GOTO, current_selection, string_list_gotoL1);
    if (selection == 0) return MR_CANCEL;
    current_selection = selection;
    switch (current_selection)
    {
    case 1:
      if (menuCatalogs(sync) == MR_QUIT) return MR_QUIT;
      break;
    case 2:
      if (menuSolarSys(sync) == MR_QUIT) return MR_QUIT;
      break;
    case 3:
      if (menuCoordinates(sync) == MR_QUIT) return MR_QUIT;
      break;
    case 4:
      if (DisplayMessageLX200(SyncGotoUserLX200(sync), false)) return MR_QUIT;
      break;
    case 5:
      if (DisplayMessageLX200(SyncGoHomeLX200(sync), false)) return MR_QUIT;
      break;
    case 6:
      if (DisplayMessageLX200(SyncGoParkLX200(sync), false)) return MR_QUIT;
      break;
    case 7:
      if (DisplayMessageLX200(SetLX200(":MF#")))
        return MR_QUIT;
      break;
    }
  }
}

SmartHandController::MENU_RESULT SmartHandController::menuCoordinates(bool sync)
{
  static int current_selection = 1;
  float azm, alt = 0;
  while (true)
  {
    const char* string_list = "J2000\nJNow\nAlt Az\n" T_NORTH "\n" T_SOUTH "\n" T_EAST "\n" T_WEST;
    int selection = display->UserInterfaceSelectionList(&buttonPad, sync ? "Sync Coord." : "Goto Coord.", current_selection, string_list);
    if (selection == 0) return MR_CANCEL;
    current_selection = selection;
    switch (current_selection)
    {
    case 1:
      if (menuRADecJ2000(sync) == MR_QUIT) return MR_QUIT;
      break;
    case 2:
      if (menuRADecNow(sync) == MR_QUIT) return MR_QUIT;
      break;
    case 3:
      if (menuAltAz(sync) == MR_QUIT) return MR_QUIT;
      break;
    case 4:
      azm = 0;
      alt = 0;
      if (DisplayMessageLX200(SyncGotoLX200AltAz(sync, azm, alt))) return MR_QUIT;
      break;
    case 5:
      azm = 180;
      alt = 0;
      if (DisplayMessageLX200(SyncGotoLX200AltAz(sync, azm, alt))) return MR_QUIT;
      break;
    case 6:
      azm = 90;
      alt = 0;
      if (DisplayMessageLX200(SyncGotoLX200AltAz(sync, azm, alt))) return MR_QUIT;
      break;
    case 7:
      azm = 270;
      alt = 0;
      if (DisplayMessageLX200(SyncGotoLX200AltAz(sync, azm, alt))) return MR_QUIT;
      break;
    }
  }
}

SmartHandController::MENU_RESULT SmartHandController::menuPier()
{
  MENU_RESULT answer = MR_CANCEL;
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
      answer = menuSyncGoto(true);
      DisplayMessageLX200(SetLX200(":SmN#"));
    }
  }
  return answer;
}

SmartHandController::MENU_RESULT SmartHandController::menuSpirale()
{
  MENU_RESULT answer = MR_CANCEL;
  static long angle = 3600;
  char DEGREE_SYMBOL[] = { 0xB0, '\0' };
  if (display->UserInterfaceInputValueDMS(&buttonPad, T_FIELDOFVIEW, &angle, 60, 3600 * 3, 1, DEGREE_SYMBOL, "'", "", "", "", false))
  {
    char out[32];
    sprintf(out, ":M@%03d#", (int)(angle / 60));
    if (SetLX200(out) == LX200_VALUESET)
    {
      answer = MR_OK;
      DisplayMessage(T_SPIRAL, T_STARTED, 500);
      exitMenu = true;
    }
    else
    {
      DisplayMessage(T_LX200COMMAND, T_FAILED, 1000);
    }
  }
  return answer;
}


SmartHandController::MENU_RESULT SmartHandController::subMenuSyncGoto(char sync, int subMenuNum)
{
  static uint8_t current_selection[64] = { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 };

  while (true)
  {
    // build the list of star/dso catalogs
    char string_list_gotoL1[60 + NUM_CAT * 10] = "";
    int  catalog_index[NUM_CAT];
    int  catalog_index_count = 0;
    char title[16] = "";
    char lastSubmenu[16] = "";
    char thisSubmenu[16] = "";
    // build the list of subMenu catalogs
    for (int i = subMenuNum; i < NUM_CAT; i++)
    {
      cat_mgr.select(i);
      strcpy(title, cat_mgr.catalogTitle());
      if (i == subMenuNum) strcpy(lastSubmenu, cat_mgr.catalogSubMenu()); else strcpy(lastSubmenu, thisSubmenu);
      strcpy(thisSubmenu, cat_mgr.catalogSubMenu());

      bool duplicate = false;
      if (strlen(thisSubmenu) > 0)
      {
        if ((strlen(thisSubmenu) == strlen(lastSubmenu)) && (strstr(thisSubmenu, lastSubmenu))) duplicate = true; else break;
      }

      // add it to the list (if it isn't a duplicate)
      if (duplicate)
      {
        if (i != subMenuNum) strcat(string_list_gotoL1, "\n"); strcat(string_list_gotoL1, title);
        catalog_index[catalog_index_count++] = i;
      }
    }

    if (sync) strcpy(title, "Sync "); else strcpy(title, "Goto "); strcat(title, lastSubmenu);
    int selection = display->UserInterfaceSelectionList(&buttonPad, title, current_selection[subMenuNum], string_list_gotoL1);
    if (selection == 0) return MR_CANCEL;
    current_selection[subMenuNum] = selection;

    if (current_selection[subMenuNum] <= NUM_CAT)
    {
      int catalogNum = catalog_index[current_selection[subMenuNum] - 1];
      if ((catalogNum >= 0) && (catalogNum < NUM_CAT))
      {
        if (menuCatalog(sync, catalogNum) == MR_QUIT) return MR_QUIT;
      }
    }
  }
}

SmartHandController::MENU_RESULT SmartHandController::menuCatalog(bool sync, int number)
{
  cat_mgr.select(number);
  char title[20] = "";
  setCatMgrFilters();
  if (cat_mgr.hasActiveFilter())
  {
    if (sync) strcpy(title, "!Sync "); else strcat(title, "!Goto ");
  }
  else
  {
    if (sync) strcpy(title, "Sync "); else strcat(title, "Goto ");
  }
  strcat(title, cat_mgr.catalogTitle());
  if (cat_mgr.hasActiveFilter()) strcat(title, "!");

  if (cat_mgr.isInitialized())
  {
    if (cat_mgr.setIndex(cat_mgr.getIndex()))
    {
      if (display->UserInterfaceCatalog(&buttonPad, title))
      {
        if (DisplayMessageLX200(SyncGotoCatLX200(sync), false)) return MR_QUIT;
      }
    }
    else DisplayMessage(cat_mgr.catalogTitle(), "No Object", -1);
  }
  else DisplayMessage(cat_mgr.catalogTitle(), "Not Init'd?", -1);
  return MR_CANCEL;
}

SmartHandController::MENU_RESULT SmartHandController::menuCatalogAlign()
{
  cat_mgr.select(0);
  char title[20] = "";
  cat_mgr.filtersClear();
  cat_mgr.filterAdd(FM_OBJ_HAS_NAME);
  cat_mgr.filterAdd(FM_ABOVE_HORIZON, 1);
  strcat(title, "Goto ");
  strcat(title, cat_mgr.catalogTitle());
  if (cat_mgr.isInitialized())
  {
    if (cat_mgr.setIndex(cat_mgr.getIndex()))
    {
      if (display->UserInterfaceCatalog(&buttonPad, title))
      {
        if (DisplayMessageLX200(SyncGotoCatLX200(false), false))
        {
          cat_mgr.filtersClear();
          return MR_QUIT;
        }
      }
    }
    else DisplayMessage(cat_mgr.catalogTitle(), "No Object", -1);
  }
  else DisplayMessage(cat_mgr.catalogTitle(), "Not Init'd?", -1);
  cat_mgr.filtersClear();
  return MR_CANCEL;
}

SmartHandController::MENU_RESULT SmartHandController::menuCatalogs(bool sync)
{
  static int current_selection = 1;
  double lat, LT0;
  while (!ta_MountStatus.getLat(lat))
  {
  }
  while (!ta_MountStatus.getLstT0(LT0))
  {
  }
  cat_mgr.setLat(lat);
  cat_mgr.setLstT0(LT0);
  while (true)
  {

    // build the list of star/dso catalogs
    char string_list_gotoL1[60 + NUM_CAT * 10] = "";
    int  catalog_index[NUM_CAT];
    int  catalog_index_count = 0;
    char title[16] = "";
    char thisSubmenu[16] = "";
    char lastSubmenu[16] = "";
    for (int i = 1; i <= NUM_CAT; i++)
    {
      cat_mgr.select(i - 1);
      strcpy(title, cat_mgr.catalogTitle());
      strcpy(lastSubmenu, thisSubmenu);
      strcpy(thisSubmenu, cat_mgr.catalogSubMenu());

      bool duplicate = false;
      if (strlen(thisSubmenu) > 0)
      {
        if ((strlen(thisSubmenu) == strlen(lastSubmenu)) && (strstr(thisSubmenu, lastSubmenu))) duplicate = true;
      }
      if ((!duplicate) && (strlen(thisSubmenu) > 0))
      {
        strcpy(title, thisSubmenu); /*strcat(title,">");*/
      }

      // add it to the list (if it isn't a duplicate)
      if (!duplicate)
      {
        strcat(string_list_gotoL1, title); strcat(string_list_gotoL1, "\n");
        if (strlen(thisSubmenu) == 0) catalog_index[catalog_index_count++] = i; else catalog_index[catalog_index_count++] = -i;
      }
    }
    // add the normal filtering, solarsys, etc. items
    strcat(string_list_gotoL1, T_FILTERS);

    int selection = display->UserInterfaceSelectionList(&buttonPad, sync ? T_SYNC T_CATALOGS : T_GOTO " " T_CATALOGS, current_selection, string_list_gotoL1);
    if (selection == 0) return MR_CANCEL;
    current_selection = selection;

    if (current_selection <= catalog_index_count)
    {
      int catalogNum = catalog_index[current_selection - 1];
      if (catalogNum >= 0)
      {
        if (menuCatalog(sync, catalogNum - 1) == MR_QUIT) return MR_QUIT;
      }
      else
      {
        if (subMenuSyncGoto(sync, (-catalogNum) - 1) == MR_QUIT) return MR_QUIT;
      }
    }
    else
      switch (current_selection - catalog_index_count)
      {
      case 1:
        menuFilters();
        break;
      }
  }
}

SmartHandController::MENU_RESULT SmartHandController::menuSolarSys(bool sync)
{
  static int current_selection = 1;
  if (current_selection < 1) current_selection = 1;

  const char *string_list_SolarSyst = T_SUN "\n" T_MERCURY "\n" T_VENUS "\n" T_MARS "\n"  T_JUPITER "\n"
    T_SATURN "\n" T_URANUS "\n" T_NEPTUNE "\n" T_MOON;
  current_selection = display->UserInterfaceSelectionList(&buttonPad, sync ? T_SYNC " " T_SOLARSYSTEM : T_GOTO " " T_SOLARSYSTEM, current_selection, string_list_SolarSyst);
  if (current_selection == 0) return MR_CANCEL;
  int selected_planet = current_selection;
  if (current_selection > 3) selected_planet++;
  //if (current_selection == 1)
  //{ 
  //  DisplayMessage("Pointing at the Sun", "can be dangerous", 2000);
  //  boolean GotoSun=false;
  //  if (display->UserInterfaceInputValueBoolean(&buttonPad, "Goto Sun?", &GotoSun)) { if (!GotoSun) return MR_CANCEL; } else return MR_CANCEL;
  //}

  if (DisplayMessageLX200(SyncGotoPlanetLX200(sync, selected_planet - 1), false)) return MR_QUIT;
  return MR_CANCEL;
}

SmartHandController::MENU_RESULT SmartHandController::menuRADecNow(bool sync)
{
  if (display->UserInterfaceInputValueRA(&buttonPad, T_RIGHTASC, &angleRA))
  {
    float fR;
    secondsToFloat(angleRA, fR);
    if (display->UserInterfaceInputValueDec(&buttonPad, T_DECLINAISON, &angleDEC))
    {
      float fD;
      secondsToFloat(angleDEC, fD);
      if (DisplayMessageLX200(SyncGotoLX200(sync, fR, fD))) return MR_QUIT;
    }
  }
  return MR_CANCEL;
}

SmartHandController::MENU_RESULT SmartHandController::menuRADecJ2000(bool sync)
{
  if (display->UserInterfaceInputValueRA(&buttonPad, T_RIGHTASC, &angleRA))
  {
    float fR;
    secondsToFloat(angleRA, fR);
    if (display->UserInterfaceInputValueDec(&buttonPad, T_DECLINAISON, &angleDEC))
    {
      float fD;
      secondsToFloat(angleDEC, fD);
      if (DisplayMessageLX200(SyncGotoLX200(sync, fR, fD, 2000))) return MR_QUIT;
    }
  }
  return MR_CANCEL;
}

SmartHandController::MENU_RESULT SmartHandController::menuAltAz(bool sync)
{
  if (display->UserInterfaceInputValueAz(&buttonPad, T_AZIMUTH, &angleRA))
  {
    float fAz;
    secondsToFloat(angleRA, fAz);
    if (display->UserInterfaceInputValueAlt(&buttonPad, T_ALTITUDE, &angleDEC))
    {
      float fAlt;
      secondsToFloat(angleDEC, fAlt);
      if (DisplayMessageLX200(SyncGotoLX200AltAz(sync, fAz, fAlt))) return MR_QUIT;
    }
  }
  return MR_CANCEL;
}
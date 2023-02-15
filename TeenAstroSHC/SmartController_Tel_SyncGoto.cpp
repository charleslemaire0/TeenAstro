#include "SmartController.h"
#include "SHC_text.h"



void putText(NAV mode, char* dst, const char* synctxt, const char* gototxt, const char* pushtotxt)
{
  switch (mode)
  {
  case NAV_SYNC:
    strcpy(dst, synctxt);
    break;
  case NAV_GOTO:
    strcpy(dst, gototxt);
    break;
  case NAV_PUSHTO:
    strcpy(dst, pushtotxt);
    break;
  }
}

static void secondsToFloat(const long& v, float& f)
{
  f = (double)v / 3600.0;
  return;
}

SmartHandController::MENU_RESULT SmartHandController::menuSyncGoto(NAV mode)
{
  static int current_selection = 1;

  if (mode == NAV_PUSHTO && !ta_MountStatus.hasEncoder())
  {
    DisplayMessage(T_ENCODERS, T_NOT_CONNECTED,-1);
    return MR_OK;
  }

  while (true)
  {
    // build the list of star/dso catalogs
    const char* string_list_gotoL1 = mode == NAV_SYNC ?
      T_CATALOGS "\n" T_SOLARSYSTEM "\n" T_COORDINATES "\n" T_USERDEFINED "\n" T_HOME "\n" T_PARK :
      mode == NAV_GOTO ?
      T_CATALOGS "\n" T_SOLARSYSTEM "\n" T_COORDINATES "\n" T_USERDEFINED "\n" T_HOME "\n" T_PARK "\n" T_FLIP :
      T_CATALOGS "\n" T_SOLARSYSTEM "\n" T_COORDINATES "\n" T_USERDEFINED;

    const char* menu = mode == NAV_SYNC ? T_SYNC : (mode == NAV_GOTO ? T_GOTO : T_PUSHTO);
    int selection = display->UserInterfaceSelectionList(&buttonPad, menu
      , current_selection, string_list_gotoL1);
    if (selection == 0) return MR_CANCEL;
    current_selection = selection;
    switch (current_selection)
    {
    case 1:
      if (menuCatalogs(mode) == MR_QUIT) return MR_QUIT;
      break;
    case 2:
      if (menuSolarSys(mode) == MR_QUIT) return MR_QUIT;
      break;
    case 3:
      if (menuCoordinates(mode) == MR_QUIT) return MR_QUIT;
      break;
    case 4:
      if (DisplayMessageLX200(SyncGotoUserLX200(mode), false)) return MR_QUIT;
      break;
    case 5:
      if (DisplayMessageLX200(SyncGoHomeLX200(mode), false)) return MR_QUIT;
      break;
    case 6:
      if (DisplayMessageLX200(SyncGoParkLX200(mode), false)) return MR_QUIT;
      break;
    case 7:
      if (DisplayMessageLX200(SetLX200(":MF#")))
        return MR_QUIT;
      break;
    }
    if (mode == NAV_PUSHTO)
    {
      current_page = PAGES::P_PUSH;
    }
  }
}

SmartHandController::MENU_RESULT SmartHandController::menuCoordinates(NAV mode)
{
  static int current_selection = 1;
  float azm, alt = 0;
  while (true)
  {
    const char* string_list = "J2000\nJNow\nAlt Az\n" T_NORTH "\n" T_SOUTH "\n" T_EAST "\n" T_WEST;
    char menu[32];
    putText(mode, menu, "Sync Coord.", "Goto Coord.", "Pushto Coord.");
    int selection = display->UserInterfaceSelectionList(&buttonPad, menu, current_selection, string_list);
    if (selection == 0) return MR_CANCEL;
    current_selection = selection;
    switch (current_selection)
    {
    case 1:
      if (menuRADecJ2000(mode) == MR_QUIT) return MR_QUIT;
      break;
    case 2:
      if (menuRADecNow(mode) == MR_QUIT) return MR_QUIT;
      break;
    case 3:
      if (menuAltAz(mode) == MR_QUIT) return MR_QUIT;
      break;
    case 4:
      azm = 0;
      alt = 0;
      if (DisplayMessageLX200(SyncGotoLX200AltAz(mode, azm, alt))) return MR_QUIT;
      break;
    case 5:
      azm = 180;
      alt = 0;
      if (DisplayMessageLX200(SyncGotoLX200AltAz(mode, azm, alt))) return MR_QUIT;
      break;
    case 6:
      azm = 90;
      alt = 0;
      if (DisplayMessageLX200(SyncGotoLX200AltAz(mode, azm, alt))) return MR_QUIT;
      break;
    case 7:
      azm = 270;
      alt = 0;
      if (DisplayMessageLX200(SyncGotoLX200AltAz(mode, azm, alt))) return MR_QUIT;
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
      answer = menuSyncGoto(NAV_SYNC);
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


SmartHandController::MENU_RESULT SmartHandController::subMenuSyncGoto(NAV mode, int subMenuNum)
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
    putText(mode, title, "Sync ", "Goto ", "Pushto ");

    strcat(title, lastSubmenu);
    int selection = display->UserInterfaceSelectionList(&buttonPad, title, current_selection[subMenuNum], string_list_gotoL1);
    if (selection == 0) return MR_CANCEL;
    current_selection[subMenuNum] = selection;

    if (current_selection[subMenuNum] <= NUM_CAT)
    {
      int catalogNum = catalog_index[current_selection[subMenuNum] - 1];
      if ((catalogNum >= 0) && (catalogNum < NUM_CAT))
      {
        if (menuCatalog(mode, catalogNum) == MR_QUIT) return MR_QUIT;
      }
    }
  }
}

SmartHandController::MENU_RESULT SmartHandController::menuCatalog(NAV mode, int number)
{
  cat_mgr.select(number);
  char title[20] = "";
  setCatMgrFilters();
  if (cat_mgr.hasActiveFilter())
  {
    putText(mode, title, "!Sync ", "!Goto ", "!Pushto ");
  }
  else
  {
    putText(mode, title, "Sync ", "Goto ", "Pushto ");
  }
  strcat(title, cat_mgr.catalogTitle());
  if (cat_mgr.hasActiveFilter()) strcat(title, "!");

  if (cat_mgr.isInitialized())
  {
    if (cat_mgr.setIndex(cat_mgr.getIndex()))
    {
      if (display->UserInterfaceCatalog(&buttonPad, title))
      {
        if (DisplayMessageLX200(SyncGotoCatLX200(mode), false)) return MR_QUIT;
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
        if (DisplayMessageLX200(SyncGotoCatLX200(NAV_GOTO), false))
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

SmartHandController::MENU_RESULT SmartHandController::menuCatalogs(NAV mode)
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
    char menu[32] = "";
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
    putText(mode, menu, T_SYNC " " T_CATALOGS, T_GOTO " " T_CATALOGS, T_PUSHTO " " T_CATALOGS);
    int selection = display->UserInterfaceSelectionList(&buttonPad, menu, current_selection, string_list_gotoL1);
    if (selection == 0) return MR_CANCEL;
    current_selection = selection;

    if (current_selection <= catalog_index_count)
    {
      int catalogNum = catalog_index[current_selection - 1];
      if (catalogNum >= 0)
      {
        if (menuCatalog(mode, catalogNum - 1) == MR_QUIT) return MR_QUIT;
      }
      else
      {
        if (subMenuSyncGoto(mode, (-catalogNum) - 1) == MR_QUIT) return MR_QUIT;
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

SmartHandController::MENU_RESULT SmartHandController::menuSolarSys(NAV mode)
{
  static int current_selection = 1;
  if (current_selection < 1) current_selection = 1;

  const char *string_list_SolarSyst = T_SUN "\n" T_MERCURY "\n" T_VENUS "\n" T_MARS "\n"  T_JUPITER "\n"
    T_SATURN "\n" T_URANUS "\n" T_NEPTUNE "\n" T_MOON;
  char menu[32] = "";
  putText(mode, menu, T_SYNC " " T_SOLARSYSTEM, T_GOTO " " T_SOLARSYSTEM, T_PUSHTO " " T_SOLARSYSTEM);
  current_selection = display->UserInterfaceSelectionList(&buttonPad, menu, current_selection, string_list_SolarSyst);
  if (current_selection == 0) return MR_CANCEL;
  int selected_planet = current_selection;
  if (current_selection > 3) selected_planet++;
  //if (current_selection == 1)
  //{ 
  //  DisplayMessage("Pointing at the Sun", "can be dangerous", 2000);
  //  boolean GotoSun=false;
  //  if (display->UserInterfaceInputValueBoolean(&buttonPad, "Goto Sun?", &GotoSun)) { if (!GotoSun) return MR_CANCEL; } else return MR_CANCEL;
  //}

  if (DisplayMessageLX200(SyncGotoPlanetLX200(mode, selected_planet - 1), false)) return MR_QUIT;
  return MR_CANCEL;
}

SmartHandController::MENU_RESULT SmartHandController::menuRADecNow(NAV mode)
{
  if (display->UserInterfaceInputValueRA(&buttonPad, T_RIGHTASC, &angleRA))
  {
    float fR;
    secondsToFloat(angleRA, fR);
    if (display->UserInterfaceInputValueDec(&buttonPad, T_DECLINAISON, &angleDEC))
    {
      float fD;
      secondsToFloat(angleDEC, fD);
      if (DisplayMessageLX200(SyncGotoLX200(mode, fR, fD))) return MR_QUIT;
    }
  }
  return MR_CANCEL;
}

SmartHandController::MENU_RESULT SmartHandController::menuRADecJ2000(NAV mode)
{
  if (display->UserInterfaceInputValueRA(&buttonPad, T_RIGHTASC, &angleRA))
  {
    float fR;
    secondsToFloat(angleRA, fR);
    if (display->UserInterfaceInputValueDec(&buttonPad, T_DECLINAISON, &angleDEC))
    {
      float fD;
      secondsToFloat(angleDEC, fD);
      if (DisplayMessageLX200(SyncGotoLX200(mode, fR, fD, 2000))) return MR_QUIT;
    }
  }
  return MR_CANCEL;
}

SmartHandController::MENU_RESULT SmartHandController::menuAltAz(NAV mode)
{
  if (display->UserInterfaceInputValueAz(&buttonPad, T_AZIMUTH, &angleRA))
  {
    float fAz;
    secondsToFloat(angleRA, fAz);
    if (display->UserInterfaceInputValueAlt(&buttonPad, T_ALTITUDE, &angleDEC))
    {
      float fAlt;
      secondsToFloat(angleDEC, fAlt);
      if (DisplayMessageLX200(SyncGotoLX200AltAz(mode, fAz, fAlt))) return MR_QUIT;
    }
  }
  return MR_CANCEL;
}
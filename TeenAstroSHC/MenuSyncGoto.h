// SyncGoto menu

void secondsToFloat(const long& v, float& f)
{
  f = (double)v / 3600.0;
  return;
}

SmartHandController::MENU_RESULT SmartHandController::menuSyncGoto(bool sync)
{
  static int current_selection = 1;

  while (true) {
    // build the list of star/dso catalogs
    const char* string_list_gotoL1 = sync ?
      T_CATALOGS "\n" T_SOLARSYSTEM "\n" T_COORDINATES "\n" T_USERDEFINED "\n" T_HOME "\n" T_PARK :
      T_CATALOGS "\n" T_SOLARSYSTEM "\n" T_COORDINATES "\n" T_USERDEFINED "\n" T_HOME "\n" T_PARK "\n" T_FLIP;
    int selection = display->UserInterfaceSelectionList(&buttonPad, sync ? T_SYNC : T_GOTO, current_selection, string_list_gotoL1);
    if (selection == 0) return MR_CANCEL;
    current_selection=selection;
    switch (current_selection) {
      case 1:
        if (menuCatalogs(sync) == MR_QUIT) return MR_QUIT;
        break;
      case 2:
        if (menuSolarSys(sync)==MR_QUIT) return MR_QUIT;
        break;
      case 3:
        if (menuCoordinates(sync)==MR_QUIT) return MR_QUIT;
        break;
      case 4:
        if ( DisplayMessageLX200(SyncGotoUserLX200(sync), false)) return MR_QUIT;
        break;
      case 5:      
        if ( DisplayMessageLX200(SyncGoHomeLX200(sync), false)) return MR_QUIT;
        break;
      case 6:
        if ( DisplayMessageLX200(SyncGoParkLX200(sync), false)) return MR_QUIT;
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
  float azm,alt = 0;
  while (true) {
    const char* string_list="J2000\nJNow\nAlt Az\n" T_NORTH "\n" T_SOUTH "\n" T_EAST "\n" T_WEST;
    int selection = display->UserInterfaceSelectionList(&buttonPad, sync ? "Sync Coord." : "Goto Coord.", current_selection, string_list);
    if (selection == 0) return MR_CANCEL;
    current_selection=selection;
    switch (current_selection) {
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

SmartHandController::MENU_RESULT SmartHandController::subMenuSyncGoto(char sync, int subMenuNum)
{
  static uint8_t current_selection[64] = {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};

  while (true) {
    // build the list of star/dso catalogs
    char string_list_gotoL1[60+ NUM_CAT *10]="";
    int  catalog_index[NUM_CAT];
    int  catalog_index_count=0;
    char title[16]="";
    char lastSubmenu[16]="";
    char thisSubmenu[16]="";
    // build the list of subMenu catalogs
    for (int i=subMenuNum; i<NUM_CAT; i++) {
      cat_mgr.select(i);
      strcpy(title,cat_mgr.catalogTitle());
      if (i==subMenuNum) strcpy(lastSubmenu,cat_mgr.catalogSubMenu()); else strcpy(lastSubmenu,thisSubmenu);
      strcpy(thisSubmenu,cat_mgr.catalogSubMenu());

      bool duplicate=false;
      if (strlen(thisSubmenu)>0) { 
        if ((strlen(thisSubmenu)==strlen(lastSubmenu)) && (strstr(thisSubmenu,lastSubmenu))) duplicate=true; else break;
      }

      // add it to the list (if it isn't a duplicate)
      if (duplicate) {
        if (i!=subMenuNum) strcat(string_list_gotoL1,"\n"); strcat(string_list_gotoL1,title);
        catalog_index[catalog_index_count++]=i;
      }
    }

    if (sync) strcpy(title,"Sync "); else strcpy(title,"Goto "); strcat(title, lastSubmenu);
    int selection = display->UserInterfaceSelectionList(&buttonPad, title, current_selection[subMenuNum], string_list_gotoL1);
    if (selection == 0) return MR_CANCEL;
    current_selection[subMenuNum]=selection;
    
    if (current_selection[subMenuNum]<= NUM_CAT) {
      int catalogNum=catalog_index[current_selection[subMenuNum]-1];
      if ((catalogNum>=0) && (catalogNum< NUM_CAT)) {
        if (menuCatalog(sync,catalogNum)==MR_QUIT) return MR_QUIT;
      }
    }
  }
}

SmartHandController::MENU_RESULT SmartHandController::menuCatalog(bool sync, int number)
{
  cat_mgr.select(number);
  char title[20]="";
  setCatMgrFilters();
  if (cat_mgr.hasActiveFilter())
  {
    if (sync) strcpy(title, "!Sync "); else strcat(title, "!Goto ");
  }
  else
  {
    if (sync) strcpy(title, "Sync "); else strcat(title, "Goto ");
  }
  strcat(title,cat_mgr.catalogTitle());
  if (cat_mgr.hasActiveFilter()) strcat(title,"!");

  if (cat_mgr.isInitialized()) {
    if (cat_mgr.setIndex(cat_mgr.getIndex())) {
      if (display->UserInterfaceCatalog(&buttonPad, title)) {
        if (DisplayMessageLX200(SyncGotoCatLX200(sync),false)) return MR_QUIT;
      }
    } else DisplayMessage(cat_mgr.catalogTitle(), "No Object", -1);
  } else DisplayMessage(cat_mgr.catalogTitle(), "Not Init'd?", -1);
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
  if (cat_mgr.isInitialized()) {
    if (cat_mgr.setIndex(cat_mgr.getIndex())) {
      if (display->UserInterfaceCatalog(&buttonPad, title)) {
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
  while (true) {
   
    // build the list of star/dso catalogs
    char string_list_gotoL1[60+NUM_CAT*10]="";
    int  catalog_index[NUM_CAT];
    int  catalog_index_count=0;
    char title[16]="";
    char thisSubmenu[16]="";
    char lastSubmenu[16]="";
    for (int i=1; i<= NUM_CAT; i++) {
      cat_mgr.select(i-1);
      strcpy(title,cat_mgr.catalogTitle());
      strcpy(lastSubmenu,thisSubmenu);
      strcpy(thisSubmenu,cat_mgr.catalogSubMenu());

      bool duplicate=false;
      if (strlen(thisSubmenu)>0) {
        if ((strlen(thisSubmenu)==strlen(lastSubmenu)) && (strstr(thisSubmenu,lastSubmenu))) duplicate=true;
      }
      if ((!duplicate) && (strlen(thisSubmenu)>0)) { strcpy(title,thisSubmenu); /*strcat(title,">");*/ }

      // add it to the list (if it isn't a duplicate)
      if (!duplicate) {
        strcat(string_list_gotoL1,title); strcat(string_list_gotoL1,"\n");
        if (strlen(thisSubmenu)==0) catalog_index[catalog_index_count++]=i; else catalog_index[catalog_index_count++]=-i;
      }
    }
    // add the normal filtering, solarsys, etc. items
    strcat(string_list_gotoL1, T_FILTERS);

    int selection = display->UserInterfaceSelectionList(&buttonPad, sync ? T_SYNC T_CATALOGS : T_GOTO " " T_CATALOGS, current_selection, string_list_gotoL1);
    if (selection == 0) return MR_CANCEL;
    current_selection=selection;

    if (current_selection<=catalog_index_count) {
      int catalogNum=catalog_index[current_selection-1];
      if (catalogNum>=0) { if (menuCatalog(sync,catalogNum-1)==MR_QUIT) return MR_QUIT; } else { if (subMenuSyncGoto(sync,(-catalogNum)-1)==MR_QUIT) return MR_QUIT; }
    } else
    switch (current_selection-catalog_index_count) {
      case 1:
        menuFilters();
        break;
    }
  }
}

SmartHandController::MENU_RESULT SmartHandController::menuSolarSys(bool sync)
{
  static int current_selection = 1;
  if (current_selection<1) current_selection=1;

  const char *string_list_SolarSyst = T_SUN "\n" T_MERCURY "\n" T_VENUS "\n" T_MARS "\n"  T_JUPITER "\n"
                                      T_SATURN "\n" T_URANUS "\n" T_NEPTUNE "\n" T_MOON;
  current_selection = display->UserInterfaceSelectionList(&buttonPad, sync ? T_SYNC " " T_SOLARSYSTEM : T_GOTO " " T_SOLARSYSTEM, current_selection, string_list_SolarSyst);
  if (current_selection == 0) return MR_CANCEL;
  int selected_planet = current_selection;
  if (current_selection>3) selected_planet++;
  //if (current_selection == 1)
  //{ 
  //  DisplayMessage("Pointing at the Sun", "can be dangerous", 2000);
  //  boolean GotoSun=false;
  //  if (display->UserInterfaceInputValueBoolean(&buttonPad, "Goto Sun?", &GotoSun)) { if (!GotoSun) return MR_CANCEL; } else return MR_CANCEL;
  //}

  if (DisplayMessageLX200(SyncGotoPlanetLX200(sync, selected_planet-1),false)) return MR_QUIT;
  return MR_CANCEL;
}

SmartHandController::MENU_RESULT SmartHandController::menuFilters()
{
  int current_selection = 1;
  while (true) {
    char string_list_Filters[200] = T_RESET " " T_FILTERS;
    char s[8];
    if (current_selection_filter_horizon) strcpy(s,"+"); else strcpy(s,"");
    strcat(string_list_Filters,"\n"); strcat(string_list_Filters,s); strcat(string_list_Filters,"Above Horizon"); strcat(string_list_Filters,s);
    if (current_selection_filter_con>1) strcpy(s,"+"); else strcpy(s,"");
    strcat(string_list_Filters,"\n"); strcat(string_list_Filters,s); strcat(string_list_Filters,"Constellation"); strcat(string_list_Filters,s);
    if (current_selection_filter_type>1) strcpy(s,"+"); else strcpy(s,"");
    strcat(string_list_Filters,"\n"); strcat(string_list_Filters,s); strcat(string_list_Filters,"Type"); strcat(string_list_Filters,s);
    if (current_selection_filter_byMag>1) strcpy(s,"+"); else strcpy(s,"");
    strcat(string_list_Filters,"\n"); strcat(string_list_Filters,s); strcat(string_list_Filters,"Magnitude"); strcat(string_list_Filters,s);
  /*  if (current_selection_filter_nearby>1) strcpy(s,"+"); else strcpy(s,"");
    strcat(string_list_Filters,"\n"); strcat(string_list_Filters,s); strcat(string_list_Filters,"Nearby"); strcat(string_list_Filters,s);*/
    if (cat_mgr.hasVarStarCatalog()) {
      if (current_selection_filter_varmax>1) strcpy(s,"+"); else strcpy(s,"");
      strcat(string_list_Filters,"\n"); strcat(string_list_Filters,s); strcat(string_list_Filters,"Var* Max Per."); strcat(string_list_Filters,s);
    }
    if (cat_mgr.hasDblStarCatalog()) {
      if (current_selection_filter_dblmin>1) strcpy(s,"+"); else strcpy(s,"");
      strcat(string_list_Filters,"\n"); strcat(string_list_Filters,s); strcat(string_list_Filters,"Dbl* Min Sep."); strcat(string_list_Filters,s);
      if (current_selection_filter_dblmax>1) strcpy(s,"+"); else strcpy(s,"");
      strcat(string_list_Filters,"\n"); strcat(string_list_Filters,s); strcat(string_list_Filters,"Dbl* Max Sep."); strcat(string_list_Filters,s);
    }
    current_selection = display->UserInterfaceSelectionList(&buttonPad, T_FILTERS T_ALLOW, current_selection_filter, string_list_Filters);
    if (current_selection == 0) return MR_CANCEL;
    current_selection_filter = current_selection;
    switch (current_selection) {
      case 1:
        current_selection_filter_con = 1;
        current_selection_filter_horizon = 1;
        current_selection_filter_type = 1;
        current_selection_filter_byMag = 1;
        current_selection_filter_nearby = 1;
        current_selection_filter_dblmin = 1;
        current_selection_filter_dblmax = 1;
        current_selection_filter_varmax = 1;
        DisplayMessage(T_FILTERS, T_RESET, 1000);
      break;
      case 2:
        menuFilterHorizon();
      break;
      case 3:
        menuFilterCon();
      break;
      case 4:
        menuFilterType();
      break;
      case 5:
        menuFilterByMag();
      break;
      //case 6:
      //  return menuFilterNearby();
      //break;
      case 6:
        if (cat_mgr.hasVarStarCatalog()) menuFilterVarMaxPer(); else { if (cat_mgr.hasDblStarCatalog()) menuFilterDblMinSep(); }
      break;
      case 7:
        if (cat_mgr.hasVarStarCatalog()) { if (cat_mgr.hasDblStarCatalog()) menuFilterDblMinSep(); } else { if (cat_mgr.hasDblStarCatalog()) menuFilterDblMaxSep(); }
      break;
      case 8:
        if (cat_mgr.hasVarStarCatalog()) { if (cat_mgr.hasDblStarCatalog()) menuFilterDblMaxSep(); }
      break;
    }
  }
}

void SmartHandController::setCatMgrFilters()
{
  cat_mgr.filtersClear();
  
  if (current_selection_filter_horizon>0)   cat_mgr.filterAdd(FM_ABOVE_HORIZON, current_selection_filter_horizon-1);
  if (current_selection_filter_con>1)   cat_mgr.filterAdd(FM_CONSTELLATION,current_selection_filter_con-2);
  if (current_selection_filter_type>1)  cat_mgr.filterAdd(FM_OBJ_TYPE,current_selection_filter_type-2);
  if (current_selection_filter_byMag>1) cat_mgr.filterAdd(FM_BY_MAG,current_selection_filter_byMag-2);
  //if (current_selection_filter_nearby>1) { 
  //  double r,d;
  //  if (ta_MountStatus.getRA(r) && ta_MountStatus.getDec(d)) {
  //    cat_mgr.setLastTeleEqu(r,d);
  //    cat_mgr.filterAdd(FM_NEARBY,current_selection_filter_nearby-2);
  //  } else current_selection_filter_nearby=1;
  //}
  if (current_selection_filter_dblmin>1) cat_mgr.filterAdd(FM_DBL_MIN_SEP,current_selection_filter_dblmin-2);
  if (current_selection_filter_dblmax>1) cat_mgr.filterAdd(FM_DBL_MAX_SEP,current_selection_filter_dblmax-2);
  if (current_selection_filter_varmax>1) cat_mgr.filterAdd(FM_VAR_MAX_PER,current_selection_filter_varmax-2);
}

SmartHandController::MENU_RESULT SmartHandController::menuFilterCon()
{
  char string_list_fCon[1000]="";
  for (int l=0; l<89; l++) {
    if (l==0) strcat(string_list_fCon,T_ALL); else strcat(string_list_fCon,cat_mgr.constellationCodeToStr(l-1));
    if (l<88) strcat(string_list_fCon,"\n");
  }
  int last_selection_filter_con = current_selection_filter_con;
  current_selection_filter_con = display->UserInterfaceSelectionList(&buttonPad, T_FILTERBYCON, current_selection_filter_con, string_list_fCon);
  if (current_selection_filter_con == 0) { current_selection_filter_con=last_selection_filter_con; return MR_CANCEL; }
  return MR_OK;
}

SmartHandController::MENU_RESULT SmartHandController::menuFilterType()
{
  char string_list_fType[500]="";
  for (int l=0; l<22; l++) {
    if (l==0) strcat(string_list_fType,T_ALL); else strcat(string_list_fType,cat_mgr.objectTypeCodeToStr(l-1));
    if (l<21) strcat(string_list_fType,"\n");
  }
  int last_selection_filter_type = current_selection_filter_type;
  current_selection_filter_type = display->UserInterfaceSelectionList(&buttonPad, T_FILTERBYTYPE, current_selection_filter_type, string_list_fType);
  if (current_selection_filter_type == 0) { current_selection_filter_type=last_selection_filter_type; return MR_CANCEL; }
  return MR_OK;
}

SmartHandController::MENU_RESULT SmartHandController::menuFilterHorizon()
{
  const char* string_list_fHorizon=T_ABOVE " " T_HORIZON "\n" T_ABOVE " 10 " T_DEG "\n" T_ABOVE " 20 " T_DEG "\n"
                                   T_ABOVE " 30 " T_DEG "\n" T_ABOVE " 40 " T_DEG"\n" T_ABOVE " 50 " T_DEG "\n"
                                   T_ABOVE " 60 " T_DEG "\n" T_ABOVE " 70 " T_DEG;
  int last_selection_filter_horizon = current_selection_filter_horizon;
  current_selection_filter_horizon = display->UserInterfaceSelectionList(&buttonPad, T_FILTER " " T_HORIZON, current_selection_filter_horizon, string_list_fHorizon);
  if (current_selection_filter_horizon == 0) { current_selection_filter_horizon=last_selection_filter_horizon; return MR_CANCEL; }
  return MR_OK;
}

SmartHandController::MENU_RESULT SmartHandController::menuFilterByMag()
{
  const char* string_list_fMag= T_ALL "\n" "10\n" "11\n" "12\n" "13\n" "14\n" "15\n" "16";
  int last_selection_filter_byMag = current_selection_filter_byMag;

  current_selection_filter_byMag = display->UserInterfaceSelectionList(&buttonPad, T_FILTER " Magnitude", current_selection_filter_byMag, string_list_fMag);
  if (current_selection_filter_byMag == 0) { current_selection_filter_byMag=last_selection_filter_byMag; return MR_CANCEL; }
  return MR_OK;
}

SmartHandController::MENU_RESULT SmartHandController::menuFilterNearby()
{
  const char* string_list_fNearby="Off\nWithin  1\xb0\nWithin  5\xb0\nWithin 10\xb0\nWithin 15\xb0";
  int last_selection_filter_nearby = current_selection_filter_nearby;
  current_selection_filter_nearby = display->UserInterfaceSelectionList(&buttonPad, "Filter Nearby", current_selection_filter_nearby, string_list_fNearby);
  if (current_selection_filter_nearby == 0) { current_selection_filter_nearby=last_selection_filter_nearby; return MR_CANCEL; }
  return MR_OK;
}

SmartHandController::MENU_RESULT SmartHandController::menuFilterDblMinSep()
{
  const char* string_list_fDblMin="Off\nMin 0.2\"\nMin 0.5\"\nMin 1.0\"\nMin 1.5\"\nMin 2.0\"\nMin 3.0\"\nMin 5.0\"\nMin 10\"\nMin 20\"\nMin 50\"";
  int last_selection_filter_dblmin = current_selection_filter_dblmin;

  while (true) {
    current_selection_filter_dblmin = display->UserInterfaceSelectionList(&buttonPad, "Filter Dbl* Sep.", current_selection_filter_dblmin, string_list_fDblMin);
    if (current_selection_filter_dblmin == 0) { current_selection_filter_dblmin=last_selection_filter_dblmin; return MR_CANCEL; }
    
    if (current_selection_filter_dblmin<=1) break;                               // abort or inactive
    if (current_selection_filter_dblmax<=1) break;                               // any minimum is ok
    if (current_selection_filter_dblmin<=current_selection_filter_dblmax) break; // minimum is below max, all is well exit
    DisplayMessage("Min Sep must", "be < Max Sep.", 2000);                       // provide a hint
    current_selection_filter_dblmin=current_selection_filter_dblmax;             // 
  }
  return MR_OK;
}

SmartHandController::MENU_RESULT SmartHandController::menuFilterDblMaxSep()
{
  const char* string_list_fDblMax="Off\nMax 0.5\"\nMax 1.0\"\nMax 1.5\"\nMax 2.0\"\nMax 3.0\"\nMax 5.0\"\nMax 10\"\nMax 20\"\nMax 50\"\nMax 100\"";
  int last_selection_filter_dblmax = current_selection_filter_dblmax;

  while (true) {
    current_selection_filter_dblmax = display->UserInterfaceSelectionList(&buttonPad, "Filter Dbl* Sep.", current_selection_filter_dblmax, string_list_fDblMax);
    if (current_selection_filter_dblmax == 0) { current_selection_filter_dblmax=last_selection_filter_dblmax; return MR_CANCEL; }
    if (current_selection_filter_dblmax<=1) break;                               // abort or inactive
    if (current_selection_filter_dblmin<=1) break;                               // any maximum is ok
    if (current_selection_filter_dblmax>=current_selection_filter_dblmin) break; // maximum is above min, all is well exit
    DisplayMessage("Max Sep must", "be > Min Sep.", 2000);                       // provide a hint
    current_selection_filter_dblmax=current_selection_filter_dblmin;             // 
  }
  return MR_OK;
}

SmartHandController::MENU_RESULT SmartHandController::menuFilterVarMaxPer()
{
  const char* string_list_fVarMax="Off\nMax 0.5 days\nMax 1.0 days\nMax 2.0 days\nMax 5.0 days\nMax 10 days\nMax 20 days\nMax 50 days\nMax 100 days";
  int last_selection_filter_varmax = current_selection_filter_varmax;
  current_selection_filter_varmax = display->UserInterfaceSelectionList(&buttonPad, "Filter Var* Period", current_selection_filter_varmax, string_list_fVarMax);
  if (current_selection_filter_varmax == 0) { current_selection_filter_dblmax=last_selection_filter_varmax; return MR_CANCEL; }
  return MR_OK;
}

SmartHandController::MENU_RESULT SmartHandController::menuRADecNow(bool sync)
{
  if (display->UserInterfaceInputValueRA(&buttonPad, &angleRA))
  {
    float fR;
    secondsToFloat(angleRA,fR);
    if (display->UserInterfaceInputValueDec(&buttonPad, &angleDEC))
    {
      float fD;
      secondsToFloat(angleDEC,fD);
      if (DisplayMessageLX200(SyncGotoLX200(sync, fR, fD))) return MR_QUIT;
    }
  }
  return MR_CANCEL;
}

SmartHandController::MENU_RESULT SmartHandController::menuRADecJ2000(bool sync)
{
  if (display->UserInterfaceInputValueRA(&buttonPad, &angleRA))
  {
    float fR;
    secondsToFloat(angleRA,fR);
    if (display->UserInterfaceInputValueDec(&buttonPad, &angleDEC))
    {
      float fD;
      secondsToFloat(angleDEC,fD);
      if (DisplayMessageLX200(SyncGotoLX200(sync, fR, fD, 2000))) return MR_QUIT;
    }
  }
  return MR_CANCEL;
}

SmartHandController::MENU_RESULT SmartHandController::menuAltAz(bool sync)
{
  if (display->UserInterfaceInputValueAz(&buttonPad, &angleRA))
  {
    float fAz;
    secondsToFloat(angleRA,fAz);
    if (display->UserInterfaceInputValueAlt(&buttonPad, &angleDEC))
    {
      float fAlt;
      secondsToFloat(angleDEC,fAlt);
      if (DisplayMessageLX200(SyncGotoLX200AltAz(sync, fAz, fAlt))) return MR_QUIT;
    }
  }
  return MR_CANCEL;
}
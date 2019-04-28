#include <TeenAstroFunction.hpp>

void SmartHandController::menuSyncGoto(bool sync)
{
  current_selection_L1 = 1;
  while (!exitMenu)
  {
    const char *string_list_gotoL1 = "Messier\nHerschel\nNGC\nIC\nSolar System\nBright Star\nCoordinates\nHome\nPark";
    current_selection_L1 = display->UserInterfaceSelectionList(&buttonPad, sync ? "Sync" : "Goto", current_selection_L1, string_list_gotoL1);
    switch (current_selection_L1)
    {
    case 0:
      return;
    case 1:
      menuMessier(sync);
      break;
    case 2:
      menuHerschel(sync);
      break;
    case 3:
      menuNGC(sync);
      break;
    case 4:
      menuIC(sync);
      break;
    case 5:
      menuSolarSys(sync);
      break;
    case 6:
      menuStar(sync);
      break;
    case 7:
      menuRADec(sync);
      break;
    case 8:
      exitMenu = DisplayMessageLX200(SyncGoHomeLX200(sync), false);
      break;
    case 9:
      exitMenu = DisplayMessageLX200(SyncGoParkLX200(sync), false);
      break;
    default:
      break;
    }
  }
}

void SmartHandController::menuStar(bool sync)
{
  double lat, T0;
  ta_MountStatus.getLat(lat);
  ta_MountStatus.getLstT0(T0);
  cat_mgr.setLat(lat);
  cat_mgr.setLstT0(T0);
  cat_mgr.select(STAR);
  cat_mgr.filter(FM_ABOVE_HORIZON);
  if (cat_mgr.alt()<0)
    cat_mgr.setIndex(0);
  if (cat_mgr.canFilter()) {
    if (display->UserInterfaceCatalog(&buttonPad, sync ? "Sync Star" : "Goto Star")) {
      exitMenu = DisplayMessageLX200(SyncGotoCatLX200(sync), false);
    }
  }
}
void SmartHandController::menuHerschel(bool sync)
{
  double lat, T0;
  ta_MountStatus.getLat(lat);
  ta_MountStatus.getLstT0(T0);
  cat_mgr.setLat(lat);
  cat_mgr.setLstT0(T0);
  cat_mgr.select(HERSCHEL);
  cat_mgr.filter(FM_ABOVE_HORIZON);
  if (cat_mgr.alt()<0)
    cat_mgr.setIndex(0);
  if (cat_mgr.canFilter()) {
    if (display->UserInterfaceCatalog(&buttonPad, sync ? "Sync Herschel" : "Goto Herschel")) {
      exitMenu = DisplayMessageLX200(SyncGotoCatLX200(sync), false);
    }
  }
}


void SmartHandController::menuMessier(bool sync)
{
  double lat, T0;
  ta_MountStatus.getLat(lat);
  ta_MountStatus.getLstT0(T0);
  cat_mgr.setLat(lat);
  cat_mgr.setLstT0(T0);
  cat_mgr.select(MESSIER);
  cat_mgr.filter(FM_ABOVE_HORIZON);
  if (cat_mgr.alt()<0)
    cat_mgr.setIndex(0);
  if (cat_mgr.canFilter()) {
    if (display->UserInterfaceCatalog(&buttonPad, sync ? "Sync Messier" : "Goto Messier")) {
      exitMenu = DisplayMessageLX200(SyncGotoCatLX200(sync), false);
    }
  }
}

void SmartHandController::menuIC(bool sync)
{
  double lat, T0;
  ta_MountStatus.getLat(lat);
  ta_MountStatus.getLstT0(T0);
  cat_mgr.setLat(lat);
  cat_mgr.setLstT0(T0);
  cat_mgr.select(IC);
  cat_mgr.filter(FM_ABOVE_HORIZON);
  if (cat_mgr.alt()<0)
    cat_mgr.setIndex(0);
  if (cat_mgr.canFilter()) {
    if (display->UserInterfaceCatalog(&buttonPad, sync ? "Sync IC" : "Goto IC")) {
      exitMenu = DisplayMessageLX200(SyncGotoCatLX200(sync), false);
    }
  }
}

void SmartHandController::menuNGC(bool sync)
{
  double lat, T0;
  ta_MountStatus.getLat(lat);
  ta_MountStatus.getLstT0(T0);
  cat_mgr.setLat(lat);
  cat_mgr.setLstT0(T0);
  cat_mgr.select(NGC);
  cat_mgr.filter(FM_ABOVE_HORIZON);
  if (cat_mgr.alt()<0)
    cat_mgr.setIndex(0);
  if (cat_mgr.canFilter()) {
    if (display->UserInterfaceCatalog(&buttonPad, sync ? "Sync NGC" : "Goto NGC")) {
      exitMenu = DisplayMessageLX200(SyncGotoCatLX200(sync), false);
    }
  }
}

void SmartHandController::menuSolarSys(bool sync)
{
  while (!exitMenu)
  {
    if (current_selection_SolarSys < 1) current_selection_SolarSys = 1;

    const char *string_list_SolarSyst = "Sun\nMercure\nVenus\nMars\nJupiter\nSaturn\nUranus\nNeptun\nMoon";
    current_selection_SolarSys = display->UserInterfaceSelectionList(&buttonPad, sync ? "Sync" : "Goto", current_selection_SolarSys, string_list_SolarSyst);
    if (current_selection_SolarSys == 0)
    {
      return;
    }
    current_selection_SolarSys > 3 ? current_selection_SolarSys : current_selection_SolarSys--;
    exitMenu = DisplayMessageLX200(SyncGotoPlanetLX200(sync, current_selection_SolarSys), false);
  }
}

void SmartHandController::menuPier()
{
  ta_MountStatus.updateMount();
  uint8_t choice = ((uint8_t)ta_MountStatus.getPierState());
  choice = display->UserInterfaceSelectionList(&buttonPad, "Set Side of Pier", choice, "East\nWest");
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
      menuSyncGoto(true);
      DisplayMessageLX200(SetLX200(":SmN#"));
    }
  }
}

void SmartHandController::menuRADec(bool sync)
{
  if (display->UserInterfaceInputValueRA(&buttonPad, &angleRA))
  {
    uint8_t vr1, vr2, vr3, vd2, vd3;
    short vd1;
    gethms(angleRA, vr1, vr2, vr3);
    if (display->UserInterfaceInputValueDec(&buttonPad, &angleDEC))
    {
      getdms(angleDEC, vd1, vd2, vd3);
      exitMenu = DisplayMessageLX200(SyncGotoLX200(sync, vr1, vr2, vr3, vd1, vd2, vd3));
    }
  }
}
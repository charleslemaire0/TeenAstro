#pragma once
#include <Arduino.h>
#include <EEPROM.h>
#include <TeenAstroLX200io.h>
#include "Pad.h"
#include "u8g2_ext.h"
#define NUMPAGES 6
class SmartHandController
{
public:
  enum OLED { OLED_SH1106, OLED_SSD1306, OLED_SSD1309 };
  enum PAGES { P_RADEC, P_ALTAZ, P_TIME, P_AXIS, P_FOCUSER, P_ALIGN };
  void update();
  void manualMove(bool &moving);
  void drawIntro();
  void drawLoad();
  void setup(const char version[], const int pin[7], const bool active[7], const int SerialBaud, const OLED model);
  struct pageInfo
  {
    PAGES p;
    bool show;
  };
private:
  U8G2_EXT *display = NULL;
  Pad buttonPad;
  char _version[20] = "Version ?";

  void updateMainDisplay(PAGES page);
  bool sleepDisplay = false;
  bool lowContrast = false;

  bool powerCycleRequired = false;
  bool buttonCommand = false;
  bool Move[6] = { false, false, false, false, false, false };
  uint8_t displayT1;
  uint8_t displayT2;
  uint8_t maxContrast;
  float  FocuserPos;
  unsigned long lastpageupdate = millis();
  unsigned long time_last_action = millis();
  unsigned long top = millis();
  bool forceDisplayoff = false;
  bool focuserlocked = false;
  bool telescoplocked = false;
  pageInfo pages[NUMPAGES] = { {P_RADEC,true},{P_ALTAZ,true}, {P_TIME,true}, {P_AXIS,true}, {P_FOCUSER,true}, {P_ALIGN,false} };
  byte current_page;
  bool exitMenu = false;
  uint8_t current_selection_L0 = 1;
  uint8_t current_selection_L1 = 1;
  uint8_t current_selection_L2 = 1;
  uint8_t current_selection_L3 = 1;
  uint8_t current_selection_L4 = 1;
  uint8_t current_timelocation = 1;
  uint8_t current_selection_SHC = 1;
  uint8_t current_selection_guide = 3;
  uint8_t current_selection_FocuserConfig = 1;
  uint8_t current_selection_FocuserMotor = 1;
  unsigned short current_selection_SolarSys = 1;
  long angleRA = 0;
  long angleDEC = 0;
  void tickButtons();
  bool buttonPressed();
  bool isSleeping();
  void resetSHC();
  void menuTelAction();
  void menuSpeedRate();
  void menuReticule();
  void menuTrack();

  enum MENU_RESULT { MR_OK, MR_CANCEL, MR_QUIT };
  uint8_t current_selection_filter = 1;
  uint8_t current_selection_filter_con = 1;
  uint8_t current_selection_filter_horizon = 1;
  uint8_t current_selection_filter_type = 1;
  uint8_t current_selection_filter_byMag = 1;
  uint8_t current_selection_filter_nearby = 1;
  uint8_t current_selection_filter_dblmin = 1;
  uint8_t current_selection_filter_dblmax = 1;
  uint8_t current_selection_filter_varmax = 1;
  MENU_RESULT menuSyncGoto(bool sync);
  MENU_RESULT menuCoordinates(bool Sync);
  MENU_RESULT menuPier();
  MENU_RESULT subMenuSyncGoto(char sync, int subMenuNum);
  MENU_RESULT menuCatalog(bool sync, int number);
  MENU_RESULT menuCatalogAlign();
  MENU_RESULT menuCatalogs(bool sync);
  MENU_RESULT menuSolarSys(bool sync);
  MENU_RESULT menuFilters();
  void setCatMgrFilters();
  MENU_RESULT menuFilterCon();
  MENU_RESULT menuFilterHorizon();
  MENU_RESULT menuFilterType();
  MENU_RESULT menuFilterByMag();
  MENU_RESULT menuFilterNearby();
  MENU_RESULT menuFilterDblMinSep();
  MENU_RESULT menuFilterDblMaxSep();
  MENU_RESULT menuFilterVarMaxPer();
  MENU_RESULT menuRADecNow(bool sync);
  MENU_RESULT menuRADecJ2000(bool sync);
  MENU_RESULT menuAltAz(bool sync);
  MENU_RESULT menuAlignment();


  bool SelectStarAlign();

  void menuTelSettings();
  void menuSHCSettings();
  void menuTimeAndSite();
  void menuDateAndTime();
  void menuMount();
  void menuMountType();
  void menuMotor(uint8_t idx);
  void menuAcceleration();
  void menuMaxRate();
  void menuGuideRate();
  void menuSite();
  void menuSites();
  void menuLocalTime();
  void menuLocalTimeZone();
  void menuFocuserAction();
  void menuFocuserSettings();
  void menuFocuserConfig();
  void menuFocuserMotor();
  void menuDisplay();
  void menuContrast();
  void menuButtonSpeed();
  void menuLocalDate();
  void menuLatitude();
  void menuLongitude();
  void menuElevation();
  void menuMainUnitInfo();
  /* void menuHCInfo();*/
  void menuLimits();
  void menuWifi();
  void menuWifiMode();
  void menuHorizon();
  void menuOverhead();
  void menuMeridian(bool east);
  void menuUnderPole();
  bool menuSetReverse(const uint8_t &axis);
  bool menuSetBacklash(const uint8_t &axis);
  bool menuSetTotGear(const uint8_t &axis);
  bool menuSetStepPerRot(const uint8_t &axis);
  bool menuSetMicro(const uint8_t &axis);
  bool menuSetLowCurrent(const uint8_t &axis);
  bool menuSetHighCurrent(const uint8_t &axis);
  void DisplayMountSettings();
  void DisplayAccMaxRateSettings();
  void DisplayMotorSettings(const uint8_t &axis);
  void DisplayMessage(const char* txt1, const char* txt2 = NULL, int duration = 0);
  void DisplayLongMessage(const char* txt1, const char* txt2 = NULL, const char* txt3 = NULL, const char* txt4 = NULL, int duration = 0);
  bool DisplayMessageLX200(LX200RETURN val, bool silentOk = true);
};

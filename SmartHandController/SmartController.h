#pragma once
#include <Arduino.h>
#include <EEPROM.h>
#include <TeenAstroLX200io.h>
#include "Pad.h"
#include "u8g2_ext.h"

class SmartHandController
{
public:
  enum OLED { OLED_SH1106, OLED_SSD1306, OLED_SSD1309 };
  void update();
  void manualMove(bool &moving);
  void drawIntro();
  void drawLoad();
  void setup(const char version[], const int pin[7], const bool active[7], const int SerialBaud, const OLED model);
private:
  U8G2_EXT *display = NULL;
  Pad buttonPad;
  char _version[20]="Version ?";

  void updateMainDisplay( u8g2_uint_t page);
  bool sleepDisplay = false;
  bool lowContrast = false;

  bool powerCylceRequired = false;
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
  byte page = 0;
  bool exitMenu = false;
  uint8_t current_selection_L0 = 1;
  uint8_t current_selection_L1 = 1;
  uint8_t current_selection_L2 = 1;
  uint8_t current_selection_L3 = 1;
  uint8_t current_selection_L4 = 1;
  uint8_t current_timelocation = 1;
  uint8_t current_selection_speed = 5;
  uint8_t current_selection_guide = 3;
  uint8_t current_selection_FocuserConfig = 1;
  uint8_t current_selection_FocuserMotor = 1;
  unsigned short current_selection_SolarSys = 1;
  long angleRA = 0;
  long angleDEC = 0;
  void tickButtons();
  bool buttonPressed();
  bool isSleeping();
  void menuTelAction();
  void menuSpeedRate();
  void menuTrack();
#ifdef OLDCAT
  void menuSyncGoto(bool sync);
  void menuSolarSys(bool sync);
  void menuHerschel(bool sync);
  void menuNGC(bool sync);
  void menuIC(bool sync);
  void menuMessier(bool sync);
  void menuStar(bool sync);
  void menuRADec(bool sync);
  void menuPier();
#endif
#ifdef NEWCAT
  enum MENU_RESULT { MR_OK, MR_CANCEL, MR_QUIT };
  bool    current_selection_filter_above = true;
  uint8_t current_selection_filter_con = 1;
  uint8_t current_selection_filter_horizon = 1;
  uint8_t current_selection_filter_type = 1;
  uint8_t current_selection_filter_byMag = 1;
  uint8_t current_selection_filter_nearby = 1;
  uint8_t current_selection_filter_dblmin = 1;
  uint8_t current_selection_filter_dblmax = 1;
  uint8_t current_selection_filter_varmax = 1;
  MENU_RESULT menuSyncGoto(bool sync);
  MENU_RESULT subMenuSyncGoto(char sync, int subMenuNum);
  MENU_RESULT menuCatalog(bool sync, int number);
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
  MENU_RESULT menuRADec(bool sync);
#endif

  void menuAlignment();


  bool SelectStarAlign();

  void menuTelSettings();
  void menuTimeAndSite();
  void menuDateAndTime();
  void menuMount();
  void menuMountType();
  void menuPredefinedMount();
  void menuAltMount();
  void menuSideres();
  void menuFornax();
  void menuKnopf();
  void menuLosmandy();
  void menuTakahashi();
  void menuVixen();
  bool menuMotorKit(int& gearBox,int& stepRot, int& current);
  void writeDefaultMount(const bool& r1, const int& ttgr1, const bool& r2, const int& ttgr2, const int& stprot, const int& cL, const int& cH);
  void menuMotor(uint8_t idx);
  void menuAcceleration();
  void menuMaxRate();
  void menuGuideRate();
  void menuSite();
  void menuSites();
  void menuUTCTime();
  void menuFocuserAction();
  void menuFocuserSettings();
  void menuFocuserConfig();
  void menuFocuserMotor();
  void menuDisplay();
  void menuContrast();
  void menuDate();
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
  bool menuSetStepperGearBox(const uint8_t &axis, unsigned short &worm);
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

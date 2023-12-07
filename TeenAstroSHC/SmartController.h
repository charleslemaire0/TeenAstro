#pragma once
#include <Arduino.h>
#include <TeenAstroLX200io.h>
#include <u8g2_ext.h>
#include <TeenAstroPad.h>

#define Product "Teenastro SHC"
#define SHCFirmwareDate          __DATE__
#define SHCFirmwareTime          __TIME__
#define SHCFirmwareVersionMajor  "1"
#define SHCFirmwareVersionMinor  "4"
#define SHCFirmwareVersionPatch  "10"

#define NUMPAGES 9
class SmartHandController
{
public:
  enum OLED
  {
    OLED_SH1106, OLED_SSD1306, OLED_SSD1309
  };
  void setup(const char version[], const int pin[7], const bool active[7], const int SerialBaud, const OLED model, const uint8_t nSubmodel);
  void update();
private:
  void getNextpage();
  void updateAlign(bool moving);
  void updatePushing(bool moving);
  enum PAGES
  {
    P_RADEC, P_HADEC , P_ALTAZ, P_PUSH, P_TIME, P_AXIS_STEP, P_AXIS_DEG, P_FOCUSER, P_ALIGN
  };
  enum MENU_RESULT
  {
    MR_OK, MR_CANCEL, MR_QUIT
  };
  struct pageInfo
  {
    PAGES p;
    bool show;
  };
  U8G2_EXT *display = NULL;
  Pad buttonPad;
  char _version[20] = "Version ?";
  bool sleepDisplay = false;
  bool lowContrast = false;
  bool powerCycleRequired = false;
  bool buttonCommand = false;
  bool Move[6] = { false, false, false, false, false, false };
  bool SHCrotated = false;
  uint8_t displayT1;
  uint8_t displayT2;
  uint8_t maxContrast;
  uint8_t num_supported_display;
  float  FocuserPos;
  unsigned long lastpageupdate = millis();
  unsigned long time_last_action = millis();
  unsigned long top = millis();
  bool forceDisplayoff = false;
  bool focuserlocked = false;
  bool telescoplocked = false;
  pageInfo pages[NUMPAGES] = { {P_RADEC,false}, {P_HADEC,false}, {P_ALTAZ,false}, {P_PUSH,false}, {P_TIME,false}, {P_AXIS_STEP,false}, {P_AXIS_DEG,false}, {P_FOCUSER,false}, {P_ALIGN,false} };
  byte current_page = 0;
  bool exitMenu = false;
  
  long angleRA = 0;
  long angleDEC = 0;
  void manualMove(bool &moving);
  void drawIntro();
  void updateMainDisplay(PAGES page);
  void tickButtons();
  bool buttonPressed();
  bool isSleeping();
  void resetSHC();
  void menuTelAction();
  void menuSpeedRate();
  #ifdef NO_SPEED_MENU
  void increaseSpeed(bool increase);
  #endif
  void menuReticule();
  void menuTrack();

  uint8_t current_selection_filter = 1;
  uint8_t current_selection_filter_con = 1;
  uint8_t current_selection_filter_horizon = 1;
  uint8_t current_selection_filter_type = 1;
  uint8_t current_selection_filter_byMag = 1;
  uint8_t current_selection_filter_nearby = 1;
  uint8_t current_selection_filter_dblmin = 1;
  uint8_t current_selection_filter_dblmax = 1;
  uint8_t current_selection_filter_varmax = 1;



  MENU_RESULT menuSyncGoto(NAV mode);
  MENU_RESULT menuCoordinates(NAV mode);
  MENU_RESULT menuPier();
  MENU_RESULT menuSpirale();
  MENU_RESULT subMenuSyncGoto(NAV mode, int subMenuNum);
  MENU_RESULT menuCatalog(NAV mode, int number);
  MENU_RESULT menuCatalogAlign();
  MENU_RESULT menuCatalogs(NAV mode);
  MENU_RESULT menuSolarSys(NAV mode);
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
  MENU_RESULT menuRADecNow(NAV mode);
  MENU_RESULT menuRADecJ2000(NAV mode);
  MENU_RESULT menuAltAz(NAV mode);
  MENU_RESULT menuAlignment();

  bool SelectStarAlign();

  void menuTelSettings();
  void menuSHCSettings();
  void menuTimeAndSite();
  void menuDateAndTime();
  void menuMount();
  void MenuRates();
  void MenuDefaultSpeed();

  void MenuTracking();
  void MenuTrackingCorrection();
  void MenuTrackingRefraction();
  void MenuTrackingAlignment();
  void menuMountType();
  void MenuRefraction();
  void MenuRefractionForGoto();
  void menuPolarAlignment();
  void menuMotor(uint8_t idx);
  void menuAcceleration();
  void menuMaxRate();
  void menuGuideRate();
  void menuRate(int r);
  void menuTrackRate();
  void menuSetDriftRate(int axis);
  void menuSite();
  void menuSites();
  void menuLocalTime();
  void menuLocalTimeZone();
  void menuFocuserAction();
  void menuFocuserSettings();
  void menuFocuserConfig();
  void menuFocuserMotor();
  void menuFocuserInfo();
  void menuDisplay();
  void menuContrast();
  void menuErgonomy();
  void menuButtonSpeed();
  void menuLocalDate();
  void menuLatitude();
  void menuLongitude();
  void menuElevation();
  void menuMainUnitInfo();
  void menuParkAndHome();
  void menuLimits();
  void menuEncoders();
  void menuAutoSyncEncoder();
  void menuCalibrationEncoder();
  void menuLimitGEM();
  void menuLimitAxis();
  void menuWifi();
  void menuWifiMode();
  void menuHorizon();
  void menuOverhead();
  void menuMeridian(bool east);
  void menuAxis(char mode);
  void menuUnderPole();
  #ifdef keepTrackingOnWhenFarFromPole
  void menuFarFromPole();
  #endif
  bool menuSetReverse(const uint8_t &axis);
  bool menuSetBacklash(const uint8_t &axis);
  bool menuSetBacklashRate(const uint8_t& axis);
  bool menuSetTotGear(const uint8_t &axis);
  bool menuSetStepPerRot(const uint8_t &axis);
  bool menuSetMicro(const uint8_t &axis);
  bool menuSetSilentStep(const uint8_t &axis);
  bool menuSetCurrent(const uint8_t &axis, bool high);
  bool menuSetEncoderPulsePerDegree(const uint8_t& axis);
  bool menuSetEncoderReverse(const uint8_t& axis);
  void DisplayMountSettings();
  void menuMounts();
  void DisplayAccMaxRateSettings();
  void DisplayMotorSettings(const uint8_t &axis);
  void DisplayMessage(const char* txt1, const char* txt2 = NULL, int duration = 0);
  void DisplayLongMessage(const char* txt1, const char* txt2 = NULL, const char* txt3 = NULL, const char* txt4 = NULL, int duration = 0);
  bool DisplayMessageLX200(LX200RETURN val, bool silentOk = true);
};

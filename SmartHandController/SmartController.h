#pragma once
#include <Arduino.h>
#include "Pad.h"
#include "u8g2_ext.h"
#include "Telescope.h"
#include "LX200.h"

#define SH1106 0
#define SSD1306 1

class SmartHandController
{
public:
  enum OLED { OLED_SH1106, OLED_SSD1306 };
  void update();
  void drawIntro();
  void drawLoad();
  void drawReady();
  void setup(const char version[], const int pin[7], const bool active[7], const int SerialBaud, const OLED model);
private:
  U8G2_EXT *display = NULL;
  Pad buttonPad;
  Telescope telInfo;
  char _version[20]="Version ?";

  void updateMainDisplay( u8g2_uint_t page);
  bool sleepDisplay = false;
  bool lowContrast = false;
  uint8_t maxContrast = 255;
  bool powerCylceRequired = false;
  bool buttonCommand = false;
  bool Move[4] = { false, false, false, false };
  uint8_t displayT1;
  uint8_t displayT2;
  unsigned long lastpageupdate = millis();
  unsigned long time_last_action = millis();

  byte page = 0;
  bool exitMenu = false;
  uint8_t current_selection_L0 = 1;
  uint8_t current_selection_L1 = 1;
  uint8_t current_selection_L2 = 1;
  uint8_t current_selection_L3 = 1;
  uint8_t current_selection_L4 = 1;
  uint8_t current_selection_speed = 5;
  uint8_t current_selection_guide = 3;
  unsigned short current_selection_Herschel = 1;
  unsigned short current_selection_Messier = 1;
  unsigned short current_selection_SolarSys = 1;
  unsigned short current_selection_Star = 1;
  long angleRA = 0;
  long angleDEC = 0;
  void tickButtons();
  bool buttonPressed();
  void menuMain();
  void menuSpeedRate();
  void menuTrack();
  void menuSyncGoto(bool sync);
  void menuSolarSys(bool sync);
  void menuHerschel(bool sync);
  void menuMessier(bool sync);
  void menuAlignment();
  void menuPier();
  void menuStar(bool sync);
  bool SelectStarAlign();
  void menuRADec(bool sync);
  void menuSettings();
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
  void menuMaxRate();
  void menuGuideRate();
  void menuSite();
  void menuSites();
  void menuUTCTime();
  void menuDisplay();
  void menuContrast();
  void menuDate();
  void menuLatitude();
  void menuLongitude();
  //void menuAltitude();
  void menuLimits();
  void menuWifi();
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
  void DisplayMotorSettings(const uint8_t &axis);
  void DisplayMessage(const char* txt1, const char* txt2 = NULL, int duration = 0);
  void DisplayLongMessage(const char* txt1, const char* txt2 = NULL, const char* txt3 = NULL, const char* txt4 = NULL, int duration = 0);
  bool DisplayMessageLX200(LX200RETURN val, bool silentOk = true);
};

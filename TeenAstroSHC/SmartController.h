#pragma once
#include <Arduino.h>
#include <TeenAstroLX200io.h>
#include <u8g2_ext.h>
#include <TeenAstroPad.h>
#include "XEEPROM.hpp"

#define Product "Teenastro SHC"
#define SHCFirmwareDate          __DATE__
#define SHCFirmwareTime          __TIME__
#define SHCFirmwareVersionMajor  "1"
#define SHCFirmwareVersionMinor  "2"
#define SHCFirmwareVersionPatch  "4"

#define NUMPAGES 6
class SmartHandController
{
public:
  enum OLED
  {
    OLED_SH1106, OLED_SSD1306, OLED_SSD1309, EINK
  };
  void setup(const char version[], const int pin[7], const bool active[7], const int SerialBaud, const OLED model, const uint8_t nSubmodel);
  void update();
  Pad buttonPad;
private:
  enum PAGES
  {
    P_RADEC, P_ALTAZ, P_TIME, P_AXIS, P_FOCUSER, P_ALIGN
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

  char _version[20] = "Version ?";
  bool sleepDisplay = false;
  bool lowContrast = false;
  bool powerCycleRequired = false;
  bool buttonCommand = false;
  bool Move[6] = { false, false, false, false, false, false };

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
  pageInfo pages[NUMPAGES] = { {P_RADEC,true},{P_ALTAZ,true}, {P_TIME,true}, {P_AXIS,false}, {P_FOCUSER,true}, {P_ALIGN,false} };
  byte current_page;
  bool exitMenu = false;
  
  void drawIntro();
  void updateMainDisplay(PAGES page);
  void tickButtons();
  bool buttonPressed();

  void DisplayMessage(const char* txt1, const char* txt2 = NULL, int duration = 0);
  void DisplayLongMessage(const char* txt1, const char* txt2 = NULL, const char* txt3 = NULL, const char* txt4 = NULL, int duration = 0);
  bool DisplayMessageLX200(LX200RETURN val, bool silentOk = true);
};

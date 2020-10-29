
#include <TeenAstroMountStatus.h>
#include "XEEPROM.hpp"
#include "SHC_text.h"
#include "SmartController.h"



void SmartHandController::setup(const char version[], const int pin[7], const bool active[7], const int SerialBaud, const OLED model, const uint8_t nSubmodel)
{
  if (XEEPROM.length() == 0)
    XEEPROM.begin(1024);
  if (strlen(version) <= 19) strcpy(_version, version);

  //choose a 128x64 display supported by U8G2lib (if not listed below there are many many others in u8g2 library example Sketches)
  Serial.begin(SerialBaud);

  num_supported_display = nSubmodel;
  uint8_t submodel = XEEPROM.read(EEPROM_DISPLAYSUBMODEL);
  switch (model)
  {
  case OLED_SH1106:
    display = new U8G2_EXT_SH1106_128X64_NONAME_1_HW_I2C(U8G2_R0);
    break;
  case OLED_SSD1306:
    display = new U8G2_EXT_SSD1306_128X64_NONAME_F_HW_I2C(U8G2_R0);
    break;
  case OLED_SSD1309:
    if (!(submodel < num_supported_display))
    {
      submodel = 0;
      XEEPROM.write(EEPROM_DISPLAYSUBMODEL, 0);
    }
    if (submodel == 0)
      display = new U8G2_EXT_SSD1309_128X64_NONAME_F_HW_I2C(U8G2_R0);
    else if (submodel == 1)
      display = new U8G2_EXT_SSD1309_128X64_NONAME2_F_HW_I2C(U8G2_R0);
    else
      display = new U8G2_EXT_SSD1309_128X64_NONAME_F_HW_I2C(U8G2_R0);
    break;
  case EINK:
    display = new U8G2_EXT_IL3820_V2_296x128_1_SW(U8G2_R0, D5, D7, D8, D6, D2);
    break;
  }
  display->begin();
  drawIntro();
  buttonPad.setup(pin, active);

  tickButtons();
  maxContrast = EEPROM.read(EEPROM_Contrast);
  display->setContrast(maxContrast);
  displayT1 = EEPROM.read(EEPROM_T1);
  if (displayT1 < 3)
  {
    displayT1 = 3;
    EEPROM.write(EEPROM_T1, displayT1);
    EEPROM.commit();
  }
  displayT2 = EEPROM.read(EEPROM_T2);
  if (displayT2 < displayT1)
  {
    displayT2 = displayT1;
    EEPROM.write(EEPROM_T2, displayT2);
    EEPROM.commit();
  }

#ifdef DEBUG_ON
  DebugSer.begin(9600);
  delay(1000);
#endif
  display->setFont(u8g2_font_helvR12_te);
  DisplayMessage("SHC " T_VERSION, _version, 1500);
  int k = 0;
  while (!ta_MountStatus.isConnectionValid() && k < 10)
  {
    ta_MountStatus.checkConnection(SHCFirmwareVersionMajor, SHCFirmwareVersionMinor);
    delay(200);
    k++;
  }
  DisplayMessage("Main Unit " T_VERSION, ta_MountStatus.getVN(), 1500);
}

void SmartHandController::update()
{
  tickButtons();
  top = millis();
  DisplayMessage("last message", buttonPad.m_wbt.writeWifiBuffer, 100);
  return;
//  if (!ta_MountStatus.isConnectionValid() && ta_MountStatus.hasInfoV())
//  {
//    ta_MountStatus.checkConnection(SHCFirmwareVersionMajor, SHCFirmwareVersionMinor);
//    //buttonPad.setMenuMode();
//    DisplayMessage("!! " T_ERROR " !!", T_VERSION, -1);
//    DisplayMessage("SHC " T_VERSION, _version, 1500);
//    DisplayMessage("Main Unit " T_VERSION, ta_MountStatus.getVN(), 1500);
//    //buttonPad.setControlerMode();
//    return;
//  }
//  if (powerCycleRequired)
//  {
//    display->sleepOff();
//    DisplayMessage(T_PRESS_KEY, T_TO_REBOOT "...", -1);
//    DisplayMessage(T_DEVICE, T_WILL_REBOOT "...", 1000);
//
//#ifdef ARDUINO_D1_MINI32
//    ESP.restart();
//#endif
//#ifdef ARDUINO_ESP8266_WEMOS_D1MINI
//    ESP.reset();
//#endif
//    return;
//  }
//  if (ta_MountStatus.notResponding())
//  {
//    display->sleepOff();
//    DisplayMessage("!! " T_ERROR " !!", T_NOT_CONNECTED, -1);
//    DisplayMessage(T_DEVICE, T_WILL_REBOOT "...", 1000);
//
//#ifdef ARDUINO_D1_MINI32
//    ESP.restart();
//#endif
//#ifdef ARDUINO_ESP8266_WEMOS_D1MINI
//    ESP.reset();
//#endif
//
//  }
//
//  if (top - lastpageupdate > 200)
//  {
//    updateMainDisplay(pages[current_page].p);
//  }
//  if (!ta_MountStatus.connected())
//    return;
}

void SmartHandController::tickButtons()
{
  buttonPad.tickButtons();
}

bool SmartHandController::buttonPressed()
{
  return buttonPad.buttonPressed();
}


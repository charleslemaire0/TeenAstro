#include "SmartController.h"
#include "SHC_text.h"

void SmartHandController::menuSHCSettings()
{
  static uint8_t s_sel = 1;
  uint8_t tmp_sel;
  const char *string_list_SettingsL3 =
    T_RIGHTS "\n" T_DISPLAY "\n" T_PAGES "\n" T_BUTTONSPEED "\n" T_ERGONOMICS "\n" T_RESET;
  while (!exitMenu)
  {
    tmp_sel = display->UserInterfaceSelectionList(&buttonPad, T_SHCSETTINGS, s_sel, string_list_SettingsL3);
    s_sel = tmp_sel > 0 ? tmp_sel : s_sel;
    switch (tmp_sel)
    {
    case 0:
      return;
    case 1:
      menuVisitor();
      break;
    case 2:
      menuDisplaySettings();
      break;
    case 3:
      menuPages();
      break;
    case 4:
      menuButtonSpeed();
      break;
    case 5:
      menuErgonomy();
      break;
    case 6:
      resetSHC();
      break;
    }
  }
}

void SmartHandController::menuPageToggle(PAGES page, const char* title)
{
  const bool on = pages[(int)page].show;
  uint8_t tmp_in = on ? 1 : 2;
  uint8_t tmp_sel = display->UserInterfaceSelectionList(&buttonPad, title, tmp_in, T_ON "\n" T_OFF);
  if (tmp_sel == 0 || tmp_sel == tmp_in)
    return;

  uint16_t mask = readPageMask();
  if (mask == 0 || mask == 0xFFFF)
    mask = kDefaultPageMask;
  mask &= (uint16_t)((1u << NUMPAGES) - 1u);

  if (tmp_sel == 1)
    mask |= (uint16_t)(1u << page);
  else
  {
    uint16_t cleared = (uint16_t)(mask & ~(1u << page));
    // Keep at least one non-align page so the main display can cycle.
    uint16_t core = (uint16_t)(cleared & ~(1u << P_ALIGN));
    if (core == 0)
    {
      DisplayMessage(title, T_ON, 800);
      return;
    }
    mask = cleared;
  }
  writePageMask(mask);
  applyPageMask(mask);
  DisplayMessage(title, (tmp_sel == 1) ? T_ON : T_OFF, 500);
}

void SmartHandController::menuPages()
{
  static uint8_t s_sel = 1;
  uint8_t tmp_sel;
  const char* string_list =
    T_PAGE_RADEC "\n" T_PAGE_HADEC "\n" T_PAGE_ALTAZ "\n" T_PAGE_PUSH "\n"
    T_PAGE_TIME "\n" T_PAGE_AXISSTEP "\n" T_PAGE_AXISDEG "\n" T_PAGE_FOCUSER "\n" T_PAGE_ALIGN;
  while (!exitMenu)
  {
    tmp_sel = display->UserInterfaceSelectionList(&buttonPad, T_PAGES, s_sel, string_list);
    s_sel = tmp_sel > 0 ? tmp_sel : s_sel;
    switch (tmp_sel)
    {
    case 0:
      return;
    case 1: menuPageToggle(P_RADEC, T_PAGE_RADEC); break;
    case 2: menuPageToggle(P_HADEC, T_PAGE_HADEC); break;
    case 3: menuPageToggle(P_ALTAZ, T_PAGE_ALTAZ); break;
    case 4: menuPageToggle(P_PUSH, T_PAGE_PUSH); break;
    case 5: menuPageToggle(P_TIME, T_PAGE_TIME); break;
    case 6: menuPageToggle(P_AXIS_STEP, T_PAGE_AXISSTEP); break;
    case 7: menuPageToggle(P_AXIS_DEG, T_PAGE_AXISDEG); break;
    case 8: menuPageToggle(P_FOCUSER, T_PAGE_FOCUSER); break;
    case 9: menuPageToggle(P_ALIGN, T_PAGE_ALIGN); break;
    default:
      break;
    }
  }
}

void SmartHandController::menuDisplayActions()
{
  const char *string_list_Display = T_TURNOFF "\n" T_CONTRAST;
  static uint8_t s_sel = 1;
  uint8_t tmp_sel;
  buttonPad.setMenuMode();
  while (!exitMenu)
  {
    tmp_sel = display->UserInterfaceSelectionList(&buttonPad, T_DISPLAY, s_sel, string_list_Display);
    s_sel = tmp_sel > 0 ? tmp_sel : s_sel;
    switch (tmp_sel)
    {
    case 0:
      return;
    case 1:
      DisplayMessage(T_PRESSSHIFTKEY, T_TOTURNON, 1500);
      forceDisplayoff = true;
      sleepDisplay = true;
      display->sleepOn();
      exitMenu = true;
      break;
    case 2:
      menuContrast();
      break;
    default:
      break;
    }
  }
  exitMenu = false;
  buttonPad.setControlerMode();
}

void SmartHandController::menuDisplaySettings()
{
  const char* string_list_Display = T_SLEEP "\n" T_DEEPSLEEP "\n" T_SUBMODEL;
  static uint8_t s_sel = 1;
  uint8_t tmp_sel;
  while (!exitMenu)
  {
    tmp_sel = display->UserInterfaceSelectionList(&buttonPad, T_DISPLAY, s_sel, string_list_Display);
    s_sel = tmp_sel > 0 ? tmp_sel : s_sel;
    switch (tmp_sel)
    {
    case 0:
      return;
    case 1:
    {
      if (display->UserInterfaceInputValueInteger(&buttonPad, T_LOWCONTRAST, T_AFTER " ", &displayT1, 3, 255, 3, "0 s"))
      {
        EEPROM.write(EEPROM_T1, displayT1);
        EEPROM.commit();
      }
      break;
    }
    case 2:
    {
      if (display->UserInterfaceInputValueInteger(&buttonPad, T_TURNDISPLAYOFF, T_AFTER " ", &displayT2, displayT1, 255, 3, "0 s"))
      {
        EEPROM.write(EEPROM_T2, displayT2);
        EEPROM.commit();
      }
      break;
    }
    case 3:
    {
      uint8_t val = EEPROM.read(EEPROM_DISPLAYSUBMODEL);
      if (display->UserInterfaceInputValueInteger(&buttonPad, T_SUBMODEL, "OLED ", &val, 0, num_supported_display - 1, 1, ""))
      {
        EEPROM.write(EEPROM_DISPLAYSUBMODEL, val);
        EEPROM.commit();
        powerCycleRequired = true;
        exitMenu = true;
      }
      break;
    }
    default:
      break;
    }
  }
}

void SmartHandController::menuErgonomy()
{
  const char* string_list_Display = T_RIGHT_HANDER "\n" T_LEFT_HANDER ;
  uint8_t s_sel = SHCrotated ? 2 : 1;
  uint8_t tmp_sel;
  tmp_sel = display->UserInterfaceSelectionList(&buttonPad, T_ERGONOMICS, s_sel, string_list_Display);
  if (tmp_sel == s_sel|| tmp_sel == 0)
    return;

  switch (tmp_sel)
  {
  case 1:
    EEPROM.write(EEPROM_DISPLAY180, 0);
    break;
  case 2:
    // Store as 1 (not 255). Erased flash is 0xFF and must not mean "rotated"
    // or orientation flips after brown-outs when the MainUnit cuts SHC power.
    EEPROM.write(EEPROM_DISPLAY180, 1);
    break;
  default:
    break;
  }
  EEPROM.commit();
  powerCycleRequired = true;
  exitMenu = true;
}

void SmartHandController::menuContrast()
{
  const char *string_list_Display = T_MIN "\n" T_LOW "\n" T_HIGH "\n" T_MAX;
  static uint8_t s_sel = 1;
  uint8_t tmp_sel;
  if (maxContrast == 0)
    tmp_sel = 1;
  else if (maxContrast < 64)
    tmp_sel = 2;
  else if (maxContrast < 128)
    tmp_sel = 3;
  else
    tmp_sel = 4;

  tmp_sel = display->UserInterfaceSelectionList(&buttonPad, T_CONTRAST, tmp_sel, string_list_Display);
  s_sel = tmp_sel > 0 ? tmp_sel : s_sel;
  switch (tmp_sel)
  {
  case 0:
    return;
  case 1:
    maxContrast = 0;
    break;
  case 2:
    maxContrast = 63;
    break;
  case 3:
    maxContrast = 127;
    break;
  case 4:
    maxContrast = 255;
    break;
  default:
    maxContrast = 255;
  }
  EEPROM.write(EEPROM_Contrast, maxContrast);
  EEPROM.commit();
  display->setContrast(maxContrast);
}

void SmartHandController::menuButtonSpeed()
{
  const char *string_list_Display = T_SLOW "\n" T_MEDIUM "\n" T_FAST;
  
  uint8_t tmp_sel = static_cast<uint8_t>(buttonPad.getButtonSpeed()) + 1;
  tmp_sel = display->UserInterfaceSelectionList(&buttonPad, T_BUTTONSPEED, tmp_sel, string_list_Display);
  switch (tmp_sel)
  {
  case 0:
    return;
  case 1:
  case 2:
  case 3:
  {
    Pad::ButtonSpeed val = static_cast<Pad::ButtonSpeed>(tmp_sel - 1);
    buttonPad.setButtonSpeed(val);
    buttonPad.setMenuMode();
    break;
  }
  default:
    return;
  }
}

void SmartHandController::resetSHC()
{
  if (display->UserInterfaceMessage(&buttonPad, T_RESET, T_TO, T_FACTORY, T_NO "\n" T_YES) == 2)
  {
    int l = EEPROM.length();
    for (int k = 0; k < l; k++)
    {
      EEPROM.write(k, 0);
    }
    // Keep OLED readable after wipe
    EEPROM.write(EEPROM_Contrast, 127);
    EEPROM.write(EEPROM_BSPEED, 1);   // Medium — matches invalid-value default
    EEPROM.write(EEPROM_PAGES, (uint8_t)(kDefaultPageMask & 0xFF));
    EEPROM.write(EEPROM_PAGES_HI, (uint8_t)((kDefaultPageMask >> 8) & 0xFF));
    EEPROM.commit();
    applyPageMask(kDefaultPageMask);
#if defined(ARDUINO_ARCH_ESP32)
    WiFi.disconnect(true, true);
    WiFi.mode(WIFI_OFF);
#elif defined(ARDUINO_ARCH_ESP8266)
    ESP.eraseConfig();
#endif
    powerCycleRequired = true;
    exitMenu = true;
    return;
  }
}
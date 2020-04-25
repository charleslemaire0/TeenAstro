#include "SmartController.h"
#include "SHC_text.h"

void SmartHandController::menuSHCSettings()
{
  static uint8_t s_sel = 1;
  uint8_t tmp_sel;
  const char *string_list_SettingsL3 = T_DISPLAY "\n" T_BUTTONSPEED "\n" T_RESET;
  while (!exitMenu)
  {
    tmp_sel = display->UserInterfaceSelectionList(&buttonPad, T_SHCSETTINGS, s_sel, string_list_SettingsL3);
    s_sel = tmp_sel > 0 ? tmp_sel : s_sel;
    switch (tmp_sel)
    {
    case 0:
      return;
    case 1:
      menuDisplay();
      break;
    case 2:
      menuButtonSpeed();
      break;
    case 3:
      resetSHC();
      break;
    }
  }
}

void SmartHandController::menuDisplay()
{
  const char *string_list_Display = T_TURNOFF "\n" T_CONTRAST "\n" T_SLEEP "\n" T_DEEPSLEEP;
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
      DisplayMessage(T_PRESSSHIFTKEY, T_TOTURNON, 1500);
      forceDisplayoff = true;
      sleepDisplay = true;
      display->sleepOn();
      exitMenu = true;
      break;
    case 2:
      menuContrast();
      break;
    case 3:
    {
      if (display->UserInterfaceInputValueInteger(&buttonPad, T_LOWCONTRAST, T_AFTER " ", &displayT1, 3, 255, 3, "0 sec"))
      {
        EEPROM.write(EEPROM_T1, displayT1);
        EEPROM.commit();
      }
      break;
    }
    case 4:
    {
      if (display->UserInterfaceInputValueInteger(&buttonPad, T_TURNDISPLAYOFF, T_AFTER " ", &displayT2, displayT1, 255, 3, "0 sec"))
      {
        EEPROM.write(EEPROM_T2, displayT2);
        EEPROM.commit();
      }
      break;
    }
    default:
      break;
    }
  }
}

void SmartHandController::menuContrast()
{
  const char *string_list_Display = T_MIN "\n" T_LOW "\n" T_HIGH "\n" T_MAX;
  static uint8_t s_sel = 1;
  uint8_t tmp_sel;
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
  uint8_t tmp_sel = (uint8_t)buttonPad.getButtonSpeed() + 1;
  tmp_sel = display->UserInterfaceSelectionList(&buttonPad, T_BUTTONSPEED, tmp_sel, string_list_Display);
  switch (tmp_sel)
  {
  case 0:
    return;
  case 1:
  case 2:
  case 3:
    buttonPad.setButtonSpeed(static_cast<Pad::ButtonSpeed>(tmp_sel - 1));
    buttonPad.setMenuMode();
    break;
  }
}

void SmartHandController::resetSHC()
{
  if (display->UserInterfaceMessage(&buttonPad, T_RESET, T_TO, T_FACTORY, T_NO "\n" T_YES) == 2)
  {
    XEEPROM.writeInt(0, 0);
    XEEPROM.commit();
    powerCycleRequired = true;
    exitMenu = true;
    return;
  }
}
#include "SmartController.h"
#include "SHC_text.h"
#include "_EEPROM_ext.h"

void SmartHandController::menuSHCSettings()
{

  const char *string_list_SettingsL3 = T_DISPLAY "\n" T_BUTTONSPEED "\n" T_RESET;
  while (!exitMenu)
  {
    current_selection_SHC = display->UserInterfaceSelectionList(&buttonPad, T_SHCSETTINGS, current_selection_SHC, string_list_SettingsL3);
    switch (current_selection_SHC)
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
  current_selection_L2 = 1;
  while (!exitMenu)
  {
    current_selection_L2 = display->UserInterfaceSelectionList(&buttonPad, T_DISPLAY, current_selection_L2, string_list_Display);
    switch (current_selection_L2)
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
  current_selection_L3 = 1;

  current_selection_L3 = display->UserInterfaceSelectionList(&buttonPad, T_CONTRAST, current_selection_L3, string_list_Display);
  switch (current_selection_L3)
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
  current_selection_L3 = (uint8_t)buttonPad.getButtonSpeed() + 1;
  current_selection_L3 = display->UserInterfaceSelectionList(&buttonPad, T_BUTTONSPEED, current_selection_L3, string_list_Display);
  switch (current_selection_L3)
  {
  case 0:
    return;
  case 1:
  case 2:
  case 3:
    buttonPad.setButtonSpeed(static_cast<Pad::ButtonSpeed>(current_selection_L3 - 1));
    buttonPad.setMenuMode();
    break;
  }
}

void SmartHandController::resetSHC()
{
  if (display->UserInterfaceMessage(&buttonPad, T_RESET, T_TO, T_FACTORY, T_NO "\n" T_YES) == 2)
  {
    EEPROM_writeInt(0, 0);
    EEPROM.commit();
    powerCycleRequired = true;
    exitMenu = true;
    return;
  }
}
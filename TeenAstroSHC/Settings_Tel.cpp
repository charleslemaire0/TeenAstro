#include "SmartController.h"
#include "SHC_text.h"



void SmartHandController::menuTelSettings()
{
  buttonPad.setMenuMode();

  static uint8_t s_sel = 1;
  uint8_t tmp_sel;
  if (!SHCvisitor)
  {
    while (!exitMenu)
    {
      const char* string_list_TelSettings = T_HANDCONTROLLER "\n" T_TIME " & " T_SITE "\n" T_PARKANDHOME "\n" T_MOUNT "\n" T_MAINUNITINFO "\nWifi";
      tmp_sel = display->UserInterfaceSelectionList(&buttonPad, T_TELESCOPESETTINGS, s_sel, string_list_TelSettings);
      s_sel = tmp_sel > 0 ? tmp_sel : s_sel;
      switch (tmp_sel)
      {
      case 0:
        exitMenu = true;
        break;
      case 1:
        menuSHCSettings();
        break;
      case 2:
        menuTimeAndSite();
        break;
      case 3:
        menuParkAndHome();
        break;
      case 4:
        menuMount();
        break;
      case 5:
        menuMainUnitInfo();
        break;
      case 6:
        menuWifi();
        break;
      default:
        break;
      }
    }
  }
  else
  {
    while (!exitMenu)
    {
      const char* string_list_TelSettings = T_RIGHTS ;
      tmp_sel = display->UserInterfaceSelectionList(&buttonPad, T_TELESCOPESETTINGS, s_sel, string_list_TelSettings);
      s_sel = tmp_sel > 0 ? tmp_sel : s_sel;
      switch (tmp_sel)
      {
      case 0:
        exitMenu = true;
        break;
      case 1:
        menuVisitor();
        break;
      default:
        break;
      }
    }
  }

  buttonPad.setControlerMode();
}
void SmartHandController::menuMainUnitInfo()
{
  const char* string_list_MainUnitInfo = T_SHOWVERSION "\n" T_REBOOT "\n" T_RESETTOFACTORY;
  static uint8_t s_sel = 1;
  uint8_t tmp_sel = s_sel;
  while (tmp_sel)
  {
    tmp_sel = display->UserInterfaceSelectionList(&buttonPad, T_MAINUNITINFO, tmp_sel, string_list_MainUnitInfo);
    s_sel = tmp_sel > 0 ? tmp_sel : s_sel;
    switch (tmp_sel)
    {
    case 0:
      return;
    case 1:
      ta_MountStatus.updateV();
      if (ta_MountStatus.hasInfoV())
      {
        DisplayMessage(ta_MountStatus.getVN(), ta_MountStatus.getVD(), -1);
      }
      break;
    case 2:
      DisplayMessageLX200(m_client->reboot(), false);
      delay(500);
      powerCycleRequired = true;
      return;
    case 3:
      if (display->UserInterfaceMessage(&buttonPad, "Reset", "To", "Factory?", "NO\nYES") == 2)
      {
        DisplayMessageLX200(m_client->factoryReset(), false);
        delay(500);
        powerCycleRequired = true;
        return;
      }
      break;
    default:
      break;
    }
  }
}
void SmartHandController::menuParkAndHome()
{
  const char* string_list_ParkAndHome = T_SETPARK "\n" T_SETHOME "\n" T_RESETHOME;
  static uint8_t s_sel = 1;
  uint8_t tmp_sel = s_sel;
  while (tmp_sel)
  {
    tmp_sel = display->UserInterfaceSelectionList(&buttonPad, T_PARKANDHOME, tmp_sel, string_list_ParkAndHome);
    s_sel = tmp_sel > 0 ? tmp_sel : s_sel;
    switch (tmp_sel)
    {
    case 0:
      return;
    case 1:
      DisplayMessageLX200(m_client->setPark(), false);
      return;
    case 2:
      DisplayMessageLX200(m_client->setHomeCurrent(), false);
      return;
    case 3:
      DisplayMessageLX200(m_client->resetHomeCurrent(), false);
      return;
    default:
      break;
    }
  }
}

void SmartHandController::menuVisitor()
{
  const char* string_list_Display = T_ADMIN "\n" T_VISITOR;
  uint8_t s_sel = SHCvisitor ? 2 : 1;
  uint8_t tmp_sel;
  tmp_sel = display->UserInterfaceSelectionList(&buttonPad, T_RIGHTS, s_sel, string_list_Display);
  if (tmp_sel == s_sel || tmp_sel == 0)
    return;

  switch (tmp_sel)
  {
  case 1:
    EEPROM.write(EEPROM_VISITOR, 0);
    SHCvisitor = false;
    break;
  case 2:
    EEPROM.write(EEPROM_VISITOR, 255);
    SHCvisitor = true;
    break;
  default:
    break;
  }
  EEPROM.commit();
  exitMenu = true;
}
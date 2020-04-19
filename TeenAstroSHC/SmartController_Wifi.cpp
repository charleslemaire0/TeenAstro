#include "SmartController.h"
#include "SHC_text.h"

void SmartHandController::menuWifi()
{
  const char *string_list = buttonPad.isWifiOn() ?
    T_TURNWIFIOFF "\n" T_SHOWPASSWORD "\n" T_SELECTMODE "\n" T_SHOWIP "\n" T_RESETTOFACTORY :
    T_TURNWIFION "\n" T_SHOWPASSWORD "\n" T_SELECTMODE "\n" T_SHOWIP "\n" T_RESETTOFACTORY;
  current_selection_L2 = 1;
  while (!exitMenu)
  {
    current_selection_L2 = display->UserInterfaceSelectionList(&buttonPad, "Wifi", 1, string_list);
    switch (current_selection_L2)
    {
    case 0:
      return;
    case 1:
      buttonPad.turnWifiOn(!buttonPad.isWifiOn());
      exitMenu = true;
      powerCycleRequired = true;
      break;
    case 2:
      DisplayMessage(T_PASSWORDIS, buttonPad.getPassword(), -1);
      break;
    case 3:
    {
      menuWifiMode();
      break;
    }
    case 4:
    {
      uint8_t ip[4] = { 0,0,0,0 };
      buttonPad.getIP(&ip[0]);
      char iptxt[16];
      sprintf(iptxt, "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
      DisplayMessage("IP Adress", iptxt, -1);
      break;
    }
    case 5:
      resetSHC();
      break;
    default:
      break;
    }
  }
}

void SmartHandController::menuWifiMode()
{
  uint8_t idx = 0;
  uint8_t idxs[4] = { 3,3,3,3 };
  char temp[20] = { 0 };
  char txt[150] = { 0 };
  char out[40] = { 0 };
  uint8_t selected_item = 10;
  for (uint8_t k = 0; k < 3; k++)
  {
    buttonPad.getStationName((int)k, out);
    if (out != NULL && out[0] != 0)
    {
      strcat(txt, out);
      strcat(txt, "\n");
      idxs[idx] = k;
      if (buttonPad.getWifiMode() == (int)k)
      {
        selected_item = idx;
      }
      idx++;
    }
  }
  if (buttonPad.getWifiMode() == 3)
  {
    selected_item = idx;
  }
  selected_item++;
  while (!exitMenu)
  {
    char menustxt[200] = {};
    strcat(menustxt, txt);
    strcat(menustxt, T_ACCESPOINT);
    const char *string_list_WifiMode = &menustxt[0];
    selected_item = display->UserInterfaceSelectionList(&buttonPad, T_WIFIINTERFACE, selected_item, string_list_WifiMode);
    if (selected_item == 0)
    {
      return;
    }
    else
    {
      buttonPad.setWifiMode(idxs[selected_item - 1]);
      powerCycleRequired = true;
      exitMenu = true;
    }
  }
}

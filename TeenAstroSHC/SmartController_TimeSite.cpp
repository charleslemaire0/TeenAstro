#include "SmartController.h"
#include "SHC_text.h"

void SmartHandController::menuTimeAndSite()
{
  static uint8_t s_sel = 1;
  uint8_t tmp_sel;
  const char* string_list_TimeAndSite = T_TIME "\n" T_SITE "\n" T_SYNCWITHGNSS;
  while (!exitMenu)
  {
    tmp_sel = display->UserInterfaceSelectionList(&buttonPad, T_TIME " & " T_SITE, s_sel, string_list_TimeAndSite);
    s_sel = tmp_sel > 0 ? tmp_sel : s_sel;
    switch (tmp_sel)
    {
    case 0:
      return;
    case 1:
      menuDateAndTime();
      break;
    case 2:
      menuSite();
      break;
    case 3:
      if (ta_MountStatus.isGNSSValid())
        DisplayMessageLX200(SetLX200(":gs#"), false);
      else
        DisplayMessage(T_NOGNSS, T_SIGNAL, -1);
      break;
    }
  }
}

void SmartHandController::menuDateAndTime()
{
  const char* string_list_DateAndTime = T_CLOCK "\n" T_TIMEZONE "\n" T_DATE "\n" T_GNSSTIME;
  static uint8_t s_sel = 1;
  uint8_t tmp_sel;
  while (!exitMenu)
  {
    tmp_sel = display->UserInterfaceSelectionList(&buttonPad, T_TIMESETTINGS, s_sel, string_list_DateAndTime);
    s_sel = tmp_sel > 0 ? tmp_sel : s_sel;
    switch (tmp_sel)
    {
    case 0:
      return;
    case 1:
      menuLocalTime();
      break;
    case 2:
      menuLocalTimeZone();
      break;
    case 3:
      menuLocalDate();
      break;
    case 4:
      if (ta_MountStatus.isGNSSValid())
        DisplayMessageLX200(SetLX200(":gt#"), false);
      else
        DisplayMessage(T_NOGNSS, T_SIGNAL, -1);
      break;
    default:
      break;
    }
  }
}

void SmartHandController::menuSite()
{
  static uint8_t s_sel = 1;
  uint8_t tmp_sel = s_sel;
  while (tmp_sel)
  {
    const char* string_list_Site = T_LATITUDE "\n" T_LONGITUDE "\n" T_SITEELEVATION "\n" T_SELECTSITE;
    tmp_sel = display->UserInterfaceSelectionList(&buttonPad, "Menu Site", s_sel, string_list_Site);
    s_sel = tmp_sel > 0 ? tmp_sel : s_sel;
    switch (tmp_sel)
    {
    case 1:
      menuLatitude();
      break;
    case 2:
      menuLongitude();
      break;
    case 3:
      menuElevation();
      break;
    case 4:
      menuSites();
      break;
    default:
      break;
    }
  }
}

void SmartHandController::menuSites()
{
  int val;
  char m[15];
  char n[15];
  char o[15];
  char p[15];
  char txt[70] = "";
  GetLX200(":GM#", m, sizeof(m));
  GetLX200(":GN#", n, sizeof(n));
  GetLX200(":GO#", o, sizeof(o));
  GetLX200(":GP#", p, sizeof(p));
  strcat(txt, m);
  strcat(txt, "\n");
  strcat(txt, n);
  strcat(txt, "\n");
  strcat(txt, o);
  strcat(txt, "\n");
  strcat(txt, p);

  if (DisplayMessageLX200(GetSiteLX200(val)))
  {
    uint8_t tmp_sel = val;
    tmp_sel = display->UserInterfaceSelectionList(&buttonPad, "Menu Sites", tmp_sel, txt);
    if (tmp_sel != 0)
    {
      val = tmp_sel - 1;
      SetSiteLX200(val);
    }
  }
}

void SmartHandController::menuLocalTime()
{
  long value;
  if (DisplayMessageLX200(GetLocalTimeLX200(value)))
  {
    if (display->UserInterfaceInputValueLocalTime(&buttonPad, T_LOCALTIME, &value))
    {
      DisplayMessageLX200(SetLocalTimeLX200(value), false);
    }
  }
}

void SmartHandController::menuLocalTimeZone()
{
  float val = 0;
  if (DisplayMessageLX200(GetLX200Float(":GG#", &val)))
  {
    val *= -1;
    if (display->UserInterfaceInputValueFloatIncr(&buttonPad, T_TIMEZONE, "UTC ", &val, -12, 12, 3, 1, 0.5, " " T_HOUR))
    {
      char cmd[15];
      sprintf(cmd, ":SG%+05.1f#", -val);
      if (DisplayMessageLX200(SetLX200(cmd)))
        exitMenu = true;
    }
  }
}

void SmartHandController::menuLocalDate()
{
  char out[20];
  if (DisplayMessageLX200(GetLX200(":GC#", out, sizeof(out))))
  {
    char* pEnd;
    uint8_t month = strtol(&out[0], &pEnd, 10);
    uint8_t day = strtol(&out[3], &pEnd, 10);
    uint8_t year = strtol(&out[6], &pEnd, 10);
    if (display->UserInterfaceInputValueDate(&buttonPad, T_DATE, year, month, day))
    {
      sprintf(out, ":SC%02d/%02d/%02d#", month, day, year);
      DisplayMessageLX200(SetLX200(out), false);
    }
  }
}

void SmartHandController::menuLatitude()
{
  double degree_d;
  int degree, minute, seconds;
  if (DisplayMessageLX200(GetLatitudeLX200(degree_d)))
  {
    long angle = degree_d * 3600;
    if (display->UserInterfaceInputValueLatitude(&buttonPad, T_LATITUDE, T_N " ", T_S " ", &angle))
    {
      char cmd[20];
      char sign = angle < 0 ? '-' : '+';
      angle = abs(angle);
      seconds = angle % 60;
      // seconds = angle;
      angle /= 60;
      // seconds -=angle*60;
      minute = angle % 60;
      degree = angle / 60;
      sprintf(cmd, ":St%+03d:%02d:%02d#", degree, minute, seconds);
      //sprintf(cmd, ":St%+03d*%02d#", degree, minute);
      cmd[3] = sign;
      DisplayMessageLX200(SetLX200(cmd), false);
    }
  }
}

void SmartHandController::menuLongitude()
{
  double degree_d;
  int degree, minute, seconds;
  if (DisplayMessageLX200(GetLongitudeLX200(degree_d)))
  {
    long angle = degree_d * 3600;
    if (display->UserInterfaceInputValueLongitude(&buttonPad, T_LONGITUDE, T_W " ", T_E " ", &angle))
    {
      char cmd[20];
      char sign = angle < 0 ? '-' : '+';
      angle = abs(angle);
      seconds = angle % 60;
      angle /= 60;
      //seconds -= angle*60;
      minute = angle % 60;
      degree = angle / 60;
      sprintf(cmd, ":Sg%+04d:%02d:%02d#", degree, minute, seconds);
      //sprintf(cmd, ":Sg%+04d*%02d#", degree, minute);
      cmd[3] = sign;
      DisplayMessageLX200(SetLX200(cmd), false);
    }
  }
}

void SmartHandController::menuElevation()
{
  char out[20];
  if (DisplayMessageLX200(GetLX200(":Ge#", out, sizeof(out))))
  {
    float alt = (float)strtol(&out[0], NULL, 10);
    if (display->UserInterfaceInputValueFloat(&buttonPad, T_SITEELEVATION, "", &alt, -200, 8000, 2, 0, " meters"))
    {
      sprintf(out, ":Se%+04d#", (int)alt);
      DisplayMessageLX200(SetLX200(out), false);
    }
  }
}
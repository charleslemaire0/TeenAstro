#include "SmartController.h"
#include "SHC_text.h"

void SmartHandController::DisplayMessage(const char* txt1, const char* txt2, int duration)
{
  display->setFont(u8g2_font_helvR12_tf);
  uint8_t x;
  uint8_t y = 40;
  display->firstPage();
  do
  {
    if (txt2 != NULL)
    {
      y = 50;
      x = (display->getDisplayWidth() - display->getUTF8Width(txt2)) / 2;
      display->drawUTF8(x, y, txt2);
      y = 25;
    }
    x = (display->getDisplayWidth() - display->getUTF8Width(txt1)) / 2;
    display->drawUTF8(x, y, txt1);
  }
  while (display->nextPage());
  if (duration >= 0)
    delay(duration);
  else
  {
    for (;;)
    {
      tickButtons();
      delay(50);
      if (buttonPressed())
        break;
    }
  }
}

void SmartHandController::DisplayLongMessage(const char* txt1, const char* txt2, const char* txt3, const char* txt4, int duration)
{
  display->setFont(u8g2_font_helvR10_te);
  uint8_t h = 15;
  uint8_t x = 0;
  uint8_t y = h;
  display->firstPage();
  do
  {
    y = h;
    x = (display->getDisplayWidth() - display->getUTF8Width(txt1)) / 2;
    display->drawUTF8(x, y, txt1);
    y += h;
    if (txt2 != NULL)
    {
      x = 0;
      display->drawUTF8(x, y, txt2);
    }
    else
    {
      y -= 7;
    }
    y += 15;
    if (txt3 != NULL)
    {
      x = 0;
      display->drawUTF8(x, y, txt3);
    }

    y += 15;
    if (txt4 != NULL)
    {
      x = 0;
      display->drawUTF8(x, y, txt4);
    }
  }
  while (display->nextPage());
  if (duration >= 0)
    delay(duration);
  else
  {
    for (;;)
    {
      tickButtons();
      delay(50);
      if (buttonPressed())
        break;
    }
  }
  display->setFont(u8g2_font_helvR12_te);
}

bool SmartHandController::DisplayMessageLX200(LX200RETURN val, bool silentOk)
{
  char text1[20] = "";
  char text2[20] = "";
  int time = -1;
  if (val < LX200_OK)
  {
    if (val == LX200_NOTOK)
    {
      sprintf(text1, T_LX200COMMAND);
      sprintf(text2, T_HASFAILED "!");
    }
    else if (val == LX200_SETVALUEFAILED)
    {
      sprintf(text1, T_SETVEALUE);
      sprintf(text2, T_HASFAILED "!");
    }
    else if (val == LX200_GETVALUEFAILED)
    {
      sprintf(text1, T_GETVEALUE);
      sprintf(text2, T_HASFAILED "!");
    }
    else if (val == LX200_SYNCFAILED)
    {
      sprintf(text1, T_SYNC);
      sprintf(text2, T_HASFAILED "!");
    }
    else if (val == LX200_SETTARGETFAILED)
    {
      sprintf(text1, T_SETTARGET);
      sprintf(text2, T_HASFAILED "!");
    }
    else if (val == LX200_TARGETBELOWHORIZON)
    {
      sprintf(text1, T_TARGETIS);
      sprintf(text2, T_BELOWHORIZON "!");
    }
    else if (val == LX200_TARGETABOVEOVERHEAD)
    {
      sprintf(text1, T_TARGETIS);
      sprintf(text2, T_ABOVEOVERHEAD "!");
    }
    else if (val == LX200_ERR_MOTOR_FAULT)
    {
      sprintf(text1, T_TELESCOPEMOTOR);
      sprintf(text2, T_FAULT "!");
    }
    else if (val == LX200_ERR_ALT)
    {
      sprintf(text1, T_TELESCOPEIS);
      sprintf(text2, T_BELOWHORIZON "!");
    }
    else if (val == LX200_ERR_LIMIT_SENSE)
    {
      sprintf(text1, T_TELESCOPEEXCEED);
      sprintf(text2, T_SENSORLIMIT "!");
    }
    else if (val == LX200_ERR_UNDER_POLE)
    {
      sprintf(text1, T_TELESCOPEEXCEED);
      sprintf(text2, T_UNDERPOLELIMIT "!");
    }
    else if (val == LX200_ERR_MERIDIAN)
    {
      sprintf(text1, T_TELESCOPEEXCEED);
      sprintf(text2, T_MERIDIANLIMIT "!");
    }
    else if (val == LX200_ERR_AXIS1)
    {
      sprintf(text1, T_TELESCOPEIS);
      sprintf(text2, T_OUTSIDELIMITS " A1!");
    }
    else if (val == LX200_ERR_AXIS2)
    {
      sprintf(text1, T_TELESCOPEIS);
      sprintf(text2, T_OUTSIDELIMITS " A2!");
    }
    else if (val == LX200_ERRGOTO_NOOBJECTSELECTED)
    {
      sprintf(text1, T_NOOBJECT);
      sprintf(text2, T_SELECTED "!");
    }
    else if (val == LX200_ERRGOTO_PARKED)
    {
      sprintf(text1, T_TELESCOPEIS);
      sprintf(text2, T_PARKED "!");
    }
    else if (val == LX200_ERRGOTO_BUSY)
    {
      sprintf(text1, T_TELESCOPEIS);
      sprintf(text2, T_BUSY "!");
    }
    else if (val == LX200_ERRGOTO_LIMITS)
    {
      sprintf(text1, T_TARGETIS);
      sprintf(text2, T_OUTSIDELIMITS "!");
    }
    else if (val == LX200_ERRGOTO_UNKOWN)
    {
      sprintf(text1, T_TUNKOWN);
      sprintf(text2, T_ERROR "!");
    }
    else if (val == LX200_GOPARK_FAILED)
    {
      sprintf(text1, T_TELESCOPE);
      sprintf(text2, T_CANTPARK "!");
    }
    else if (val == LX200_GOHOME_FAILED)
    {
      sprintf(text1, T_TELESCOPE);
      sprintf(text2, T_CANTGOHOME "!");
    }
    else
    {
      sprintf(text1, T_ERROR);
      sprintf(text2, "-1");
    }
    DisplayMessage(text1, text2, -1);
  }
  else if (!silentOk)
  {
    time = 1000;
    if (val == LX200_OK)
    {
      sprintf(text1, T_LX200COMMAND);
      sprintf(text2, T_DONE "!");
    }
    else if (val == LX200_VALUESET)
    {
      sprintf(text1, T_VALUE);
      sprintf(text2, T_SET "!");
    }
    else if (val == LX200_VALUEGET)
    {
      sprintf(text1, T_VALUE);
      sprintf(text2, T_GET "!");
    }
    else if (val == LX200_SYNCED)
    {
      sprintf(text1, T_TELESCOPE);
      sprintf(text2, T_SYNCED "!");
    }
    else if (val == LX200_GOTO_TARGET)
    {
      sprintf(text1, T_SLEWINGTO);
      sprintf(text2, T_TARGET);
    }
    else if (val == LX200_PUSHTO_TARGET)
    {
      sprintf(text1, T_PUSHINGTO);
      sprintf(text2, T_TARGET);
    }
    else if (val == LX200_GOPARK)
    {
      sprintf(text1, T_SLEWINGTO);
      sprintf(text2, T_PARK);
    }
    else if (val == LX200_GOHOME)
    {
      sprintf(text1, T_SLEWINGTO);
      sprintf(text2, T_HOME);
    }
    DisplayMessage(text1, text2, time);
  }
  return isOk(val);
}
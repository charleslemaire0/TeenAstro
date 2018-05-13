#include "Helper.h"

void DisplayMessage(const char* txt1, const char* txt2, int duration)
{
  uint8_t x;
  uint8_t y = 40;
  display.firstPage();
  do {
    if (txt2 != NULL)
    {
      y = 50;
      x = (display.getDisplayWidth() - display.getStrWidth(txt2)) / 2;
      display.drawStr(x, y, txt2);
      y = 25;
    }
    x= (display.getDisplayWidth() - display.getStrWidth(txt1)) / 2;
    display.drawStr(x, y, txt1);
  } while (display.nextPage());
  if (duration >= 0)
    delay(duration);
  else
  {
    for (;;)
    {
      //updateWifi();
      HdCrtlr.tickButtons();
      delay(50);
      if (buttonPressed)
        break;
    }
  }
}

void DisplayLongMessage(const char* txt1, const char* txt2, const char* txt3, const char* txt4, int duration)
{
  display.setFont(u8g2_font_helvR10_tr);
  uint8_t h = 15;
  uint8_t x = 0;
  uint8_t y = h;
  display.firstPage();
  do {

    y = h;
    x = (display.getDisplayWidth() - display.getStrWidth(txt1)) / 2;
    display.drawStr(x, y, txt1);
    y += h;
    if (txt2 != NULL)
    {
      x = 0;
      display.drawStr(x, y, txt2);
    }
    else
    {
      y -= 7;
    }
    y += 15;
    if (txt3 != NULL)
    {
      x = 0;
      display.drawStr(x, y, txt3);
    }

    y += 15;
    if (txt4 != NULL)
    {
      x = 0;
      display.drawStr(x, y, txt4);
    }
  } while (display.nextPage());
  if (duration >= 0)
    delay(duration);
  else
  {
    for (;;)
    {
      //updateWifi();
      HdCrtlr.tickButtons();
      delay(50);
      if (buttonPressed)
        break;
    }
  }

  display.setFont(u8g2_font_helvR12_te);
}


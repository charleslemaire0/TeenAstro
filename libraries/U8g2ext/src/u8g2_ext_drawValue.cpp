#include "u8g2_ext_value.h"

uint8_t ext_drawRA(u8g2_t *u8g2, uint8_t x, uint8_t y, const char* Ra)
{
  u8g2_uint_t step0 = u8g2_GetUTF8Width(u8g2, "00");
  u8g2_uint_t step1 = u8g2_GetUTF8Width(u8g2, "00") + u8g2_GetUTF8Width(u8g2, "0") / 3;
  u8g2_uint_t step2 = u8g2_GetUTF8Width(u8g2, "0") - u8g2_GetUTF8Width(u8g2, "0") / 3;

  char Rah[3];
  char Ram[3];
  char Ras[3];

  memcpy(Rah, Ra, 2);
  Rah[2] = '\0';
  memcpy(Ram, &Ra[3], 2);
  Ram[2] = '\0';
  memcpy(Ras, &Ra[6], 2);
  Ras[2] = '\0';
  if (x == u8g2_GetDisplayWidth(u8g2))
  {
    x -= step0 + 1;
    u8g2_DrawUTF8(u8g2, x, y, Ras);
    x -= step2;
    u8g2_DrawUTF8(u8g2, x, y, ":");
    x -= step1;
    u8g2_DrawUTF8(u8g2, x, y, Ram);
    x -= step2;
    u8g2_DrawUTF8(u8g2, x, y, ":");
    x -= step1;
    u8g2_DrawUTF8(u8g2, x, y, Rah);
    return x;
  }
  else
  {
    x += u8g2_GetUTF8Width(u8g2, "+") + u8g2_GetUTF8Width(u8g2, "0") / 3;
    u8g2_DrawUTF8(u8g2, x, y, Rah);
    x += step1;
    u8g2_DrawUTF8(u8g2, x, y, ":");
    x += step2;
    u8g2_DrawUTF8(u8g2, x, y, Ram);
    x += step1;
    u8g2_DrawUTF8(u8g2, x, y, ":");
    x += step2;
    x += u8g2_DrawUTF8(u8g2, x, y, Ras);
    return x;
  }

}

uint8_t ext_drawDec(u8g2_t *u8g2, uint8_t x, uint8_t y, const char* Dec)
{
  char DEGREE_SYMBOL[] = { 0xB0, '\0' };
  u8g2_uint_t step1 = u8g2_GetUTF8Width(u8g2, "00");
  u8g2_uint_t step2 = u8g2_GetUTF8Width(u8g2, "0");
  char decsign[2];
  char decdeg[3];
  char decmin[3];
  char decsec[3];
  memcpy(decsign, Dec, 1);
  decsign[1] = '\0';
  memcpy(decdeg, &Dec[1], 2);
  decdeg[2] = '\0';
  memcpy(decmin, &Dec[4], 2);
  decmin[2] = '\0';
  memcpy(decsec, &Dec[7], 2);
  decsec[2] = '\0';

  if (x == u8g2_GetDisplayWidth(u8g2))
  {
    x -= step1 + 1;
    u8g2_DrawUTF8(u8g2, x, y, decsec);
    x -= step2 - u8g2_GetUTF8Width(u8g2, " ") / 2;
    u8g2_DrawUTF8(u8g2, x, y, "'");
    x -= step1 + u8g2_GetUTF8Width(u8g2, " ") / 2;
    u8g2_DrawUTF8(u8g2, x, y, decmin);
    x -= step2;
    u8g2_DrawUTF8(u8g2, x, y, DEGREE_SYMBOL);
    x -= step1;
    u8g2_DrawUTF8(u8g2, x, y, decdeg);
    x -= u8g2_GetUTF8Width(u8g2, "+");
    if (decsign[0] == '-')
    {
      x += u8g2_GetUTF8Width(u8g2, "0") / 3;
      u8g2_DrawUTF8(u8g2, x, y, decsign);
    }
    else
    {
      x -= u8g2_GetUTF8Width(u8g2, "0") / 3;
      u8g2_DrawUTF8(u8g2, x, y, decsign);
    }
  }
  else
  {
    if (decsign[0] == '-')
    {
      x += u8g2_GetUTF8Width(u8g2, "0") / 3;
      u8g2_DrawUTF8(u8g2, x, y, decsign);
    }
    else
    {
      u8g2_DrawUTF8(u8g2, x, y, decsign);
      x += u8g2_GetUTF8Width(u8g2, "0") / 3;
    }
    x += u8g2_GetUTF8Width(u8g2, "+");
    u8g2_DrawUTF8(u8g2, x, y, decdeg);
    x += step1;
    u8g2_DrawUTF8(u8g2, x, y, DEGREE_SYMBOL);
    x += step2;
    u8g2_DrawUTF8(u8g2, x, y, decmin);
    x += step1 + u8g2_GetUTF8Width(u8g2, " ") / 2;
    u8g2_DrawUTF8(u8g2, x, y, "'");
    x += step2 - u8g2_GetUTF8Width(u8g2, " ") / 2;
    x += u8g2_DrawUTF8(u8g2, x, y, decsec);
  }
  return 0;
}

uint8_t ext_drawAz(u8g2_t *u8g2, uint8_t x, uint8_t y, const char* Az)
{
  char DEGREE_SYMBOL[] = { 0xB0, '\0' };
  char Azdeg[4];
  char Azmin[3];
  char Azsec[3];
  memcpy(Azdeg, Az, 3);
  Azdeg[3] = '\0';
  memcpy(Azmin, &Az[4], 2);
  Azmin[2] = '\0';
  memcpy(Azsec, &Az[7], 2);
  Azsec[2] = '\0';
  u8g2_uint_t step0 = u8g2_GetUTF8Width(u8g2, "000");
  u8g2_uint_t step1 = u8g2_GetUTF8Width(u8g2, "00");
  u8g2_uint_t step2 = u8g2_GetUTF8Width(u8g2, "0");
  if (x == u8g2_GetDisplayWidth(u8g2))
  {
    x -= step1 + 1;
    u8g2_DrawUTF8(u8g2, x, y, Azsec);
    x -= step2 - u8g2_GetUTF8Width(u8g2, " ") / 2;
    u8g2_DrawUTF8(u8g2, x, y, "'");
    x -= step1 + u8g2_GetUTF8Width(u8g2, " ") / 2;
    u8g2_DrawUTF8(u8g2, x, y, Azmin);
    x -= step2;
    u8g2_DrawUTF8(u8g2, x, y, DEGREE_SYMBOL);
    x -= step0;
    u8g2_DrawUTF8(u8g2, x, y, Azdeg);
  }
  else
  {
    u8g2_DrawUTF8(u8g2, x, y, Azdeg);
    x += step1 + u8g2_GetUTF8Width(u8g2, "0");
    u8g2_DrawUTF8(u8g2, x, y, DEGREE_SYMBOL);
    x += step2;
    u8g2_DrawUTF8(u8g2, x, y, Azmin);
    x += step1 + u8g2_GetUTF8Width(u8g2, " ") / 2;
    u8g2_DrawUTF8(u8g2, x, y, "'");
    x += step2 - u8g2_GetUTF8Width(u8g2, " ") / 2;
    x += u8g2_DrawUTF8(u8g2, x, y, Azsec);
  }
  return 0;
}

uint8_t ext_drawFoc(u8g2_t *u8g2, uint8_t y, uint8_t line_height, const char* Foc)
{
  char pos[6];
  char spd[4];
  uint8_t x = u8g2_GetDisplayWidth(u8g2) - u8g2_GetUTF8Width(u8g2, "00000");
  u8g2_DrawUTF8(u8g2, 0, y, "F Position");
  memcpy(pos, &Foc[1], 5);
  pos[5] = 0;
  u8g2_DrawUTF8(u8g2, x, y, pos);
  y += line_height + 4;
  x = u8g2_GetDisplayWidth(u8g2) - u8g2_GetUTF8Width(u8g2, "000");
  u8g2_DrawUTF8(u8g2, 0, y, "F Speed");
  memcpy(spd, &Foc[7], 3);
  spd[3] = 0;
  u8g2_DrawUTF8(u8g2, x, y, spd);
  return 0;
}


uint8_t ext_DrawFwNumeric(u8g2_t *u8g2, uint8_t x, uint8_t y, const char* text)
{
  int w=0;
  int width=0;
  char ws[2];
  int l=strlen(text);
  for (int i=0; i<l; i++) {
    ws[0]=text[i]; ws[1]=0;
    if ( ((text[i]>='0') && (text[i]<='9')) || (text[i]==':') || (text[i]=='?') || (text[i]==' ') || (text[i]=='\xb0') || (text[i]=='\'') || (text[i]=='+') || (text[i]=='-')) w=u8g2_GetUTF8Width(u8g2,"0"); else w=u8g2_GetUTF8Width(u8g2,ws);
    if (text[i]==':') {
      x+=2; u8g2_DrawUTF8(u8g2, x, y, ws); x-=2;
    } else {
      u8g2_DrawUTF8(u8g2, x, y, ws); 
    }
    x+=w+1;
    width+=w+1;
  }
  return width;
}

uint8_t ext_GetFwNumericWidth(u8g2_t *u8g2, const char* text)
{
  int w=0;
  int width=0;
  char ws[2];
  int l=strlen(text);
  for (int i=0; i<l; i++) {
    ws[0]=text[i]; ws[1]=0;
    if ( ((text[i]>='0') && (text[i]<='9')) || (text[i]==':') || (text[i]=='?') || (text[i]==' ') || (text[i]=='\xb0') || (text[i]=='\'') || (text[i]=='+') || (text[i]=='-')) w=u8g2_GetUTF8Width(u8g2,"0"); else w=u8g2_GetUTF8Width(u8g2,ws);
    width+=w+1;
  }
  return width;
}

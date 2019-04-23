/*

u8g2_selection_list.c

selection list with scroll option

Universal 8bit Graphics Library (https://github.com/olikraus/u8g2/)

Copyright (c) 2016, olikraus@gmail.com
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this list
of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice, this
list of conditions and the following disclaimer in the documentation and/or other
materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/
#include <U8g2lib.h>
#include "u8g2_ext_catalog.h"
#include "u8g2_ext_value.h"
#include "Catalog.h"
#include "u8g2_ext_event.h"

#define OC_width 18
#define OC_height 10
static unsigned char OC_bits[] U8X8_PROGMEM = {
  0x80, 0x02, 0x00, 0x00, 0x08, 0x00, 0x20, 0x00, 0x00, 0x80, 0x24, 0x00,
  0x10, 0x00, 0x00, 0x00, 0x20, 0x00, 0x90, 0x04, 0x00, 0x00, 0x10, 0x00,
  0x40, 0x00, 0x00, 0x00, 0x05, 0x00 };

#define GC_width 18
#define GC_height 10
static unsigned char GC_bits[] U8X8_PROGMEM = {
  0x80, 0x07, 0x00, 0x40, 0x09, 0x00, 0x20, 0x11, 0x00, 0x10, 0x21, 0x00,
  0x10, 0x3f, 0x00, 0xf0, 0x23, 0x00, 0x10, 0x22, 0x00, 0x20, 0x12, 0x00,
  0x40, 0x0a, 0x00, 0x80, 0x07, 0x00 };

#define EN_width 18
#define EN_height 10
static unsigned char EN_bits[] U8X8_PROGMEM = {
  0xff, 0xff, 0x03, 0xff, 0xff, 0x03, 0xff, 0xff, 0x01, 0xff, 0x7f, 0x00,
  0xff, 0x70, 0x00, 0x7e, 0x60, 0x00, 0x7c, 0x20, 0x00, 0x78, 0x20, 0x00,
  0xf0, 0x00, 0x00, 0xe0, 0x01, 0x00 };

#define PN_width 18
#define PN_height 10
static unsigned char PN_bits[] U8X8_PROGMEM = {
  0x00, 0x00, 0x00, 0xc0, 0x0f, 0x00, 0xf8, 0x7f, 0x00, 0x1e, 0xe0, 0x01,
  0x07, 0x83, 0x03, 0x07, 0x83, 0x03, 0x1e, 0xe0, 0x01, 0xf8, 0x7f, 0x00,
  0xc0, 0x0f, 0x00, 0x00, 0x00, 0x00 };

#define GX_width 18
#define GX_height 10
static unsigned char GX_bits[] U8X8_PROGMEM = {
  0xf0, 0x3f, 0x00, 0x38, 0x60, 0x00, 0x1c, 0xc0, 0x00, 0x9c, 0x07, 0x00,
  0xf8, 0x3f, 0x00, 0xf0, 0x7f, 0x00, 0x80, 0xe7, 0x00, 0x0c, 0xe0, 0x00,
  0x18, 0x70, 0x00, 0xf0, 0x3f, 0x00 };



#define MY_BORDER_SIZE 1


/*
selection list with string line
returns line height
*/

static uint8_t ext_draw_catalog_list_line(u8g2_t *u8g2, uint8_t y)
{
  char DEGREE_SYMBOL[] = { 0xB0, '\0' };
  u8g2_uint_t x = 0;
  u8g2_uint_t yy;
  uint8_t pos0;
  uint8_t pos1;
  uint8_t pos2;

  const uint8_t* myfont = u8g2->font;
  u8g2_uint_t  pixel_width;
  u8g2_uint_t line_height = u8g2_GetAscent(u8g2) - u8g2_GetDescent(u8g2) + MY_BORDER_SIZE;

  char line[16];

  // for Star Catalog
  if (cat_mgr.getCat() == STAR)
  {

    pos1 = u8g2_GetUTF8Width(u8g2, "W ");
    pos2 = u8g2_GetUTF8Width(u8g2, "W Www ");

    // Bayer designation of the star (Greek letter)
    u8g2_SetFont(u8g2, u8g2_font_unifont_t_greek);
    x = 0;
    u8g2_DrawGlyph(u8g2, x, y, 944 + cat_mgr.primaryId());
    u8g2_SetFont(u8g2, myfont);

    // Constellation Abbreviation
    u8g2_DrawUTF8(u8g2, pos1, y, cat_mgr.constellationStr());

    // Common name for the star
    // Width of constellation abbreviation
    u8g2_DrawUTF8(u8g2, x, y, cat_mgr.objectName());

    // Magnitude
    y += line_height;
    sprintf(line, "mag %0.1f", (float)cat_mgr.magnitude());
    u8g2_DrawUTF8(u8g2, 0, y, line);

    // Azimuth
    y += line_height;
    pos0 = 0;
    pos1 = u8g2_GetUTF8Width(u8g2, "Azm ");
    pos2 = u8g2_GetUTF8Width(u8g2, "Azm 999");
    u8g2_DrawUTF8(u8g2, pos0, y, "Azm ");
    sprintf(line, "%03d", (int)cat_mgr.azm());   
    u8g2_DrawUTF8(u8g2, pos1 , y, line);
    u8g2_DrawUTF8(u8g2, pos2, y, DEGREE_SYMBOL);

    // Altitude
    pos0 = u8g2_GetUTF8Width(u8g2, "Azm 999__");
    pos1 = u8g2_GetUTF8Width(u8g2, "Azm 999__Alt ");
    pos2 = u8g2_GetUTF8Width(u8g2, "Azm 999__Alt 99");
    u8g2_DrawUTF8(u8g2, pos0, y, "Alt ");
    sprintf(line, "%02d", (int)cat_mgr.alt());
    u8g2_DrawUTF8(u8g2, pos1, y, line);
    u8g2_DrawUTF8(u8g2, pos2, y, DEGREE_SYMBOL);

    return line_height;
  }

  // Object Catalogs
  //
  // Catalog letter and Object ID

  if (cat_mgr.getCat() == MESSIER)
  {
    pos1 = u8g2_GetUTF8Width(u8g2, "M 999 ");
    pos2 = u8g2_GetUTF8Width(u8g2, "M 999 Www ");
  }
  else
  {
    pos1 = u8g2_GetUTF8Width(u8g2, "NGC 9999 ");
    pos2 = u8g2_GetUTF8Width(u8g2, "NGC 9999 Www ");
  }
  sprintf(line, "%s%u", cat_mgr.catalogStr(), cat_mgr.primaryId());
  x = 0;
  u8g2_DrawUTF8(u8g2, x, y, line);

  // Constellation Abbreviation
  u8g2_DrawUTF8(u8g2, pos1, y, cat_mgr.constellationStr());
  // Magnitude
  y += line_height;
  pos1 = u8g2_GetUTF8Width(u8g2, "NGC 9999 ");
  if (cat_mgr.magnitude() < 18)
  {
    sprintf(line, "mag %0.1f", (float)cat_mgr.magnitude());
  }
  u8g2_DrawUTF8(u8g2, 0, y, line);
  // Object icon if exist
  switch (cat_mgr.objectType())
  {
  case 0:
  case 5:
  case 6:
  case 7:
    u8g2_DrawXBMP(u8g2, pos1, y - GX_height, GX_width, GX_height, GX_bits);
    break;
  case 8:
    u8g2_DrawXBMP(u8g2, pos1, y - GC_height, GC_width, GC_height, GC_bits);
    break;
  case 1:
    u8g2_DrawXBMP(u8g2, pos1, y - OC_height, OC_width, OC_height, OC_bits);
    break;
  case 9:
    u8g2_DrawXBMP(u8g2, pos1, y - PN_height, PN_width, PN_height, PN_bits);
    break;
  case 10:
  case 11:
  case 13:
  case 14:
  case 15:
    u8g2_DrawXBMP(u8g2, pos1, y - EN_height, EN_width, EN_height, EN_bits);
  default:
    break;
  }

  // Object type text
  y += line_height;
  x = 0;
  u8g2_DrawUTF8(u8g2, x, y, cat_mgr.objectTypeStr());

  return line_height;
}

/*
title: NULL for no title, valid str for title line. Can contain mutliple lines, separated by '\n'
uses: cat_mgr which has catalog selected, cursor line set, and filter set.  On return cat_mgr has object of interest selected.
returns false if user has pressed the home key
returns true if user has pressed the select key
side effects:
u8g2_SetFontDirection(u8g2, 0);
u8g2_SetFontPosBaseline(u8g2);
*/
bool ext_UserInterfaceCatalog(u8g2_t *u8g2, Pad* extPad, const char *title)
{
  u8g2_SetFont(u8g2, u8g2_font_helvR10_te);
  u8g2_uint_t yy;

  uint8_t event;

  u8g2_uint_t line_height = u8g2_GetAscent(u8g2) - u8g2_GetDescent(u8g2) + MY_BORDER_SIZE;
  uint8_t title_lines = u8x8_GetStringLineCnt(title);
  uint8_t display_lines;

  u8g2_SetFontPosBaseline(u8g2);

  for (;;) {
    u8g2_FirstPage(u8g2);
    do {
      yy = u8g2_GetAscent(u8g2);
      if (title_lines > 0) {
        yy += u8g2_DrawUTF8Lines(u8g2, 0, yy, u8g2_GetDisplayWidth(u8g2), line_height, title);
        u8g2_DrawHLine(u8g2, 0, yy - line_height - u8g2_GetDescent(u8g2) + 1, u8g2_GetDisplayWidth(u8g2));
        yy += 3;
      }
      ext_draw_catalog_list_line(u8g2, yy);
    } while (u8g2_NextPage(u8g2));

#ifdef U8G2_REF_MAN_PIC
    return 0;
#endif

    for (;;) {
      event = ext_GetMenuEvent(extPad);
      if (event == U8X8_MSG_GPIO_MENU_SELECT || event == U8X8_MSG_GPIO_MENU_NEXT) return true; else
        if (event == U8X8_MSG_GPIO_MENU_HOME || event == U8X8_MSG_GPIO_MENU_PREV) return false; else
          if (event == U8X8_MSG_GPIO_MENU_DOWN) { cat_mgr.incIndex(); break; }
          else
            if (event == U8X8_MSG_GPIO_MENU_UP) { cat_mgr.decIndex(); break; }
    }
  }
}


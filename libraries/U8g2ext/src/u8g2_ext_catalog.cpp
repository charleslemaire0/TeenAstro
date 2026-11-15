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
#include "u8g2_ext_catalog.h"
#include <U8g2lib.h>
#include <TeenAstroCatalog.h>
#include "u8g2_ext_value.h"
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

enum CATALOG_DISPLAY_MODES { DM_INFO, DM_EQ_COORDS, DM_HOR_COORDS };

/*
selection list with string line
returns line height
*/

static uint8_t ext_draw_catalog_list_line(u8g2_t *u8g2, uint8_t y, CATALOG_DISPLAY_MODES displayMode, bool firstPass)
{
  u8g2_uint_t x = 0;

  uint8_t step0;
  const uint8_t* myfont = u8g2->font;
  u8g2_uint_t line_height = u8g2_GetAscent(u8g2) - u8g2_GetDescent(u8g2) + MY_BORDER_SIZE;

  char line[32];


  int dx = u8g2_GetDisplayWidth(u8g2);
  if (cat_mgr.isStarCatalog())
  {
    // Normal star mode
    // |bAnd              15.2|
    // |Alpheratz             |
    // |                      |

    // Double star mode
    // |Σ0060AB ηCas          |
    // |----name---- 3.52,7.36|
    // |Sep 2.5" PA 225       |

    // Variable star mode
    // |bAnd                  |
    // |----name---- 11.2,15.2|
    // |Per 2.5d              |

    x = 0;

    // Prefix. The bright-star catalog omits "Star "; STF/STT use Greek letters.
    snprintf(line, sizeof(line), "%s", cat_mgr.catalogPrefix());
    const bool brightStar = strstr(line, "Star") != NULL;
    if (!brightStar) {
      if (strstr(line, "STF")) {
        u8g2_SetFont(u8g2, u8g2_font_unifont_t_greek);
        x += u8g2_DrawGlyph(u8g2, x, y, 931); // sigma
        u8g2_SetFont(u8g2, myfont);
      }
      else if (strstr(line, "STT")) {
        u8g2_SetFont(u8g2, u8g2_font_unifont_t_greek);
        x += u8g2_DrawGlyph(u8g2, x, y, 927); // omicron
        x += u8g2_DrawGlyph(u8g2, x, y, 931); // sigma
        u8g2_SetFont(u8g2, myfont);
      }
      else {
        u8g2_SetFont(u8g2, u8g2_font_6x13_tr);
        x += u8g2_DrawUTF8(u8g2, x, y, line);
        u8g2_SetFont(u8g2, myfont);
      }

      if (!cat_mgr.hasPrimaryIdInPrefix()) {
        long id = cat_mgr.primaryId();
        if (id > 0) snprintf(line, sizeof(line), "%ld", id); else snprintf(line, sizeof(line), "?");
        u8g2_SetFont(u8g2, u8g2_font_6x13_tr);
        x += u8g2_DrawUTF8(u8g2, x, y, line);
        u8g2_SetFont(u8g2, myfont);
      }
    }

    // Star SubId
    snprintf(line, sizeof(line), "%s", cat_mgr.subIdStr());
    u8g2_SetFont(u8g2, u8g2_font_6x13_tr);
    x += u8g2_DrawUTF8(u8g2, x, y, line);
    u8g2_SetFont(u8g2, myfont);

    if (x != 0) {
      x += u8g2_DrawUTF8(u8g2, x, y, " ");
    }

    // Bayer letter or Flamsteed number, immediately before the constellation, drawn once.
    int bf = cat_mgr.bayerFlam();
    if ((bf >= 0) && (bf < 24)) {
      u8g2_SetFont(u8g2, u8g2_font_unifont_t_greek);
      x += u8g2_DrawGlyph(u8g2, x, y, 945 + bf);
      u8g2_SetFont(u8g2, myfont);
    }
    else if (bf > 24) {
      snprintf(line, sizeof(line), "%d", bf - 24);
      u8g2_SetFont(u8g2, u8g2_font_6x13_tr);
      x += u8g2_DrawUTF8(u8g2, x, y, line);
      u8g2_SetFont(u8g2, myfont);
    }

    u8g2_DrawUTF8(u8g2, x, y, cat_mgr.constellationStr());


    // Magnitudes
    float mf = cat_mgr.magnitude();
    float mf2 = cat_mgr.magnitude2();
    if ((mf2 > 99) && (mf > 99)) {
      sprintf(line, "?.?");
    }
    else  if (mf2 > 99) {
      dtostrf(mf, 4, 1, line);
      ext_DrawFwNumeric(u8g2, dx - ext_GetFwNumericWidth(u8g2, line), y, line);
    }
    else if (displayMode == DM_INFO) {
      char mfs[8], mf2s[8];
      dtostrf(mf, 4, 1, mfs);
      dtostrf(mf2, 4, 1, mf2s);
      sprintf(line, "%s", mf2s);
      x = ext_DrawFwNumeric(u8g2, dx - ext_GetFwNumericWidth(u8g2, line), y + line_height, line);
      sprintf(line, "%s", mfs);
      ext_DrawFwNumeric(u8g2, dx - (ext_GetFwNumericWidth(u8g2, line) + x + 4), y + line_height, line);
    }


    // Display line 2 and 3
    x = 0;
    if (displayMode == DM_INFO) {
      y += line_height;
      // Name on the left. Suitability (*…****) stays on the right, and the name is clipped before it.
      {
        const char* name = cat_mgr.objectNameStr();
        u8g2_uint_t nameLimit = (u8g2_uint_t)dx;
        if (cat_mgr.isAlignRanked()) {
          const char* suit = cat_mgr.alignSuitabilityStr();
          if (suit[0] != 0) {
            u8g2_uint_t suitW = u8g2_GetUTF8Width(u8g2, suit);
            u8g2_DrawUTF8(u8g2, dx - suitW, y, suit);
            if (suitW + 2 < nameLimit)
              nameLimit = (u8g2_uint_t)(dx - suitW - 2);
          }
        }
        u8g2_SetClipWindow(u8g2, 0, 0, nameLimit, u8g2_GetDisplayHeight(u8g2));
        u8g2_DrawUTF8(u8g2, x, y, name);
        u8g2_SetMaxClipWindow(u8g2);
      }

      // line 3
      x = 0;
      y += line_height;
      if (cat_mgr.isDblStarCatalog()) {
        // |Sep 2.5" PA 225       |
        char seps[16];
        float f = cat_mgr.separation();
        if (f >= 0) dtostrf(f, 5, 1, seps); else strcpy(seps, "?.?");
        x = u8g2_DrawUTF8(u8g2, 0, y, "Sep");
        sprintf(line, "%s\"", seps);
        ext_DrawFwNumeric(u8g2, x, y, line);

        int p = cat_mgr.positionAngle();
        if (p >= 0) sprintf(line, "%3d\xb0", p); else sprintf(line, "  ?\xb0");
        x = ext_DrawFwNumeric(u8g2, dx - ext_GetFwNumericWidth(u8g2, line), y, line);
        u8g2_DrawUTF8(u8g2, dx - (u8g2_GetUTF8Width(u8g2, "PA ") + x), y, "PA ");
      }
      else if (cat_mgr.isVarStarCatalog()) {
        // |Per 2.5d              |
        char pers[16];
        float p = cat_mgr.period();
        // Period 0.00 to 9.99 days, period 10.0 to 3186.6 days, -1 = Unknown, -2 = Irregular
        if (abs(p + 1.0) < 0.1) { sprintf(line, "Period Unknown"); }
        else if (abs(p + 2.0) < 0.1) { sprintf(line, "Period Irregular"); }
        else if ((p >= 0.0) && (p < 10.0)) { dtostrf(p, 7, 2, pers); sprintf(line, "Period%sd", pers); }
        else if ((p >= 10.0) && (p <= 3186.6)) { dtostrf(p, 7, 1, pers); sprintf(line, "Period%sd", pers); }
        else sprintf(line, "Period ?");
        ext_DrawFwNumeric(u8g2, x, y, line);
      }
    }

    u8g2_SetFont(u8g2, myfont);
  }
  else {
    // DSO Catalogs (current)
    // |M44 NGC2632    S 2.2|
    // |Open Cluster     CnC|
    // |Beehive Cluster     |

    // Prefix and catalog number. The id is drawn only after it is written into line.
    x = 0;
    u8g2_uint_t magSlot = u8g2_GetUTF8Width(u8g2, "99.9");
    u8g2_uint_t iconX = (u8g2_uint_t)dx - magSlot - GX_width + 1;
    u8g2_SetClipWindow(u8g2, 0, 0, iconX > 1 ? iconX - 1 : 0, u8g2_GetDisplayHeight(u8g2));
    x += u8g2_DrawUTF8(u8g2, x, y, cat_mgr.catalogPrefix());
    if (!cat_mgr.hasPrimaryIdInPrefix()) {
      sprintf(line, "%ld", cat_mgr.primaryId());
      x += u8g2_DrawUTF8(u8g2, x, y, line);
    }

    // Object SubId
    u8g2_SetFont(u8g2, u8g2_font_6x13_tr);
    x += u8g2_DrawUTF8(u8g2, x, y, cat_mgr.subIdStr());
    u8g2_SetFont(u8g2, myfont);
    u8g2_SetMaxClipWindow(u8g2);

    //draw element on the right
    x = iconX;
    switch (cat_mgr.objectType())
    {
    case 0:
    case 5:
    case 6:
    case 7:
      u8g2_DrawXBMP(u8g2, x, y - GX_height, GX_width, GX_height, GX_bits);
      break;
    case 8:
      u8g2_DrawXBMP(u8g2, x, y - GC_height, GC_width, GC_height, GC_bits);
      break;
    case 1:
    case 13:
      u8g2_DrawXBMP(u8g2, x, y - OC_height, OC_width, OC_height, OC_bits);
      break;
    case 9:
      u8g2_DrawXBMP(u8g2, x, y - PN_height, PN_width, PN_height, PN_bits);
      break;
    case 10:
    case 11:
    case 12:
    case 14:
    case 15:
    case 16:
      u8g2_DrawXBMP(u8g2, x, y - EN_height, EN_width, EN_height, EN_bits);
      break;
    default:
      break;
    }

    // Magnitude, flush with the right edge
    float mf = cat_mgr.magnitude();
    if (mf > 99) sprintf(line, "?.?"); else dtostrf(mf, 3, 1, line);
    step0 = u8g2_GetUTF8Width(u8g2, line);
    u8g2_DrawUTF8(u8g2, dx - step0, y, line);

    // Display lines 2 and 3
    if (displayMode == DM_INFO) {
      y += line_height;
      x = 0;

      // Object type on the left, constellation kept clear on the right
      sprintf(line, "%s", cat_mgr.constellationStr());
      step0 = u8g2_GetUTF8Width(u8g2, line);
      {
        u8g2_uint_t typeLimit = (step0 + 2 < (u8g2_uint_t)dx) ? (u8g2_uint_t)(dx - step0 - 2) : 0;
        u8g2_SetClipWindow(u8g2, 0, 0, typeLimit, u8g2_GetDisplayHeight(u8g2));
        u8g2_DrawUTF8(u8g2, x, y, cat_mgr.objectTypeStr());
        u8g2_SetMaxClipWindow(u8g2);
      }
      u8g2_DrawUTF8(u8g2, dx - step0, y, line);

      // Object Name, when the DSO catalogs include it
      y += line_height;
      x = 0;

      const char* objName = cat_mgr.objectNameStr();
      int len = strlen(objName);
      int txt_w = u8g2_GetUTF8Width(u8g2, objName);
      if (txt_w < dx)
      {
        u8g2_DrawUTF8(u8g2, x, y, objName);
      }
      else if (len > 0 && len * 2 + 3 < 128)
      {
        static int scroll = 0;
        static long scrollCat = -1;
        static long scrollIdx = -1;
        long catId = (long)cat_mgr.catalogType();
        long idx = cat_mgr.getIndex();
        if (catId != scrollCat || idx != scrollIdx) {
          scroll = 0;
          scrollCat = catId;
          scrollIdx = idx;
        }
        char movingtext[128];
        memcpy(movingtext, objName, len);
        memcpy(movingtext + len, "...", 3);
        memcpy(movingtext + len + 3, objName, len);
        movingtext[len * 2 + 3] = 0;
        txt_w = u8g2_GetUTF8Width(u8g2, movingtext);
        if (scroll >= txt_w / 2)
          scroll = 0;
        x = -scroll;
        scroll += 5;
        u8g2_DrawUTF8(u8g2, x, y, movingtext);
      }
      else
      {
        u8g2_DrawUTF8(u8g2, x, y, objName);
      }
    
    
    }
  }

  if (displayMode == DM_EQ_COORDS) {
    char epoch[6];
    uint8_t vr1, vr2, vr3;
    short vd1; uint8_t vd2, vd3;

    // RA
    sprintf(epoch, "%04d", cat_mgr.epoch());
    cat_mgr.raHMS(vr1, vr2, vr3);
    sprintf(line, " %02d:%02d:%02d", vr1, vr2, vr3);
    y += line_height;
    x = u8g2_DrawUTF8(u8g2, 0, y, "RA"); u8g2_SetFont(u8g2, u8g2_font_6x13_tr); u8g2_DrawUTF8(u8g2, x, y, epoch); u8g2_SetFont(u8g2, myfont);
    ext_DrawFwNumeric(u8g2, dx - ext_GetFwNumericWidth(u8g2, line), y, line);

    // Declination. decDMS drops the sign when the degree field is 0, so -0°..-1°
    // would otherwise be drawn with a plus. Take the sign from the angle itself.
    cat_mgr.decDMS(vd1, vd2, vd3);
    sprintf(line, "%s%02d\xb0%02d'%02d", cat_mgr.dec() < 0 ? "-" : "+", abs(vd1), vd2, vd3);
    y += line_height;
    x = u8g2_DrawUTF8(u8g2, 0, y, "DE"); u8g2_SetFont(u8g2, u8g2_font_6x13_tr); u8g2_DrawUTF8(u8g2, x, y, epoch); u8g2_SetFont(u8g2, myfont);
    ext_DrawFwNumeric(u8g2, dx - ext_GetFwNumericWidth(u8g2, line), y, line);
  }

  if (displayMode == DM_HOR_COORDS) {
    static short va1, vz1;
    static uint8_t va2, va3, vz2, vz3;
    static bool altNeg = false;

    u8g2_SetFont(u8g2, myfont);

    // Horizon coords. Sample once per frame: repeating the /6 on a later page
    // turns tenths of a degree into zeros. altDMS also loses the sign of -0°.
    if (firstPass) {
      cat_mgr.azmDMS(vz1, vz2, vz3);
      cat_mgr.altDMS(va1, va2, va3);
      altNeg = cat_mgr.alt() < 0;
      vz2 = vz2 / 6;
      va2 = va2 / 6;
    }
    sprintf(line, "%3d.%d\xb0", vz1, vz2);
    x = 0; y += line_height;
    x += u8g2_DrawUTF8(u8g2, x, y, "Az") + 2;
    x += (ext_DrawFwNumeric(u8g2, x, y, line));
    if (va1 == 0 && altNeg)
      sprintf(line, " -0.%d\xb0", va2);
    else
      sprintf(line, "%3d.%d\xb0", va1, va2);
    x = dx - ext_DrawFwNumeric(u8g2, dx - ext_GetFwNumericWidth(u8g2, line), y, line);
    u8g2_DrawUTF8(u8g2, x - (u8g2_GetUTF8Width(u8g2, "Alt") + 4), y, "Alt");
  }

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
  int incr = 0;
  uint8_t event;
  CATALOG_DISPLAY_MODES thisDisplayMode = DM_INFO;

  u8g2_uint_t line_height = u8g2_GetAscent(u8g2) - u8g2_GetDescent(u8g2) + MY_BORDER_SIZE;
  uint8_t title_lines = u8x8_GetStringLineCnt(title);

  u8g2_SetFontPosBaseline(u8g2);

  for (;;) {
    bool firstPass = true;
    u8g2_FirstPage(u8g2);
    do {
      yy = u8g2_GetAscent(u8g2);
      if (title_lines > 0) {
        yy += u8g2_DrawUTF8Lines(u8g2, 0, yy, u8g2_GetDisplayWidth(u8g2), line_height, title);
        u8g2_DrawHLine(u8g2, 0, yy - line_height - u8g2_GetDescent(u8g2) + 1, u8g2_GetDisplayWidth(u8g2));
        yy += 3;
      }
      ext_draw_catalog_list_line(u8g2, yy, thisDisplayMode, firstPass);
      firstPass = false;
    } while (u8g2_NextPage(u8g2));

    for (;;) {
      event = ext_GetMenuEvent(extPad);
      if (event == U8X8_MSG_GPIO_MENU_SELECT || event == U8X8_MSG_GPIO_MENU_NEXT) return true;
      else if (event == U8X8_MSG_GPIO_MENU_HOME)
      {
        thisDisplayMode = (CATALOG_DISPLAY_MODES)((int)thisDisplayMode + 1);
        if (thisDisplayMode > DM_HOR_COORDS) thisDisplayMode = DM_INFO;
        break;
      }
      else if (event == U8X8_MSG_GPIO_MENU_PREV) return false;
      else if (event == U8X8_MSG_GPIO_MENU_DOWN)
      {
        for (int k = 0; k < incr/100 + 1; k++)
        {
          cat_mgr.incIndex();
        }
        if (cat_mgr.getMaxIndex() > 1000) incr = min(500,incr+1);
        break;
      }
      else if (event == U8X8_MSG_GPIO_MENU_UP)
      {
        for (int k = 0; k < incr/100 + 1; k++)
        {
          cat_mgr.decIndex(); 
        }
        if (cat_mgr.getMaxIndex() > 1000) incr = min(500,incr+1);
        break;
      }
      else
      {
        incr = 0;
      }
      // auto-refresh display
      static unsigned long lastRefresh = 0;
      if (millis() - lastRefresh > 500) { lastRefresh = millis(); break; }
    }
  }
}


/*
selection list with string line
returns line height
*/

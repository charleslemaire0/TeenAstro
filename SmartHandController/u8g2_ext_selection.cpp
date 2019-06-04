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


#define MY_BORDER_SIZE 1
#include "u8g2_ext_selection.h"


/*
selection list with string line
returns line height
*/
static u8g2_uint_t u8g2_draw_selection_list_line(u8g2_t *u8g2, u8sl_t *u8sl, u8g2_uint_t y, uint8_t idx, const char *s) U8G2_NOINLINE;
static u8g2_uint_t u8g2_draw_selection_list_line(u8g2_t *u8g2, u8sl_t *u8sl, u8g2_uint_t y, uint8_t idx, const char *s)
{
  u8g2_uint_t yy;
  uint8_t border_size = 0;
  uint8_t is_invert = 0;

  u8g2_uint_t line_height = u8g2_GetAscent(u8g2) - u8g2_GetDescent(u8g2) + MY_BORDER_SIZE;

  /* calculate offset from display upper border */
  yy = idx;
  yy -= u8sl->first_pos;
  yy *= line_height;
  yy += y;

  /* check whether this is the current cursor line */
  if (idx == u8sl->current_pos)
  {
    border_size = MY_BORDER_SIZE;
    is_invert = 1;
  }

  /* get the line from the array */
  s = u8x8_GetStringLineStart(idx, s);

  /* draw the line */
  if (s == NULL)
    s = "";
  u8g2_DrawUTF8Line(u8g2, MY_BORDER_SIZE, y, u8g2_GetDisplayWidth(u8g2) - 2 * MY_BORDER_SIZE, s, border_size, is_invert);
  return line_height;
}

void u8g2_DrawSelectionList(u8g2_t *u8g2, u8sl_t *u8sl, u8g2_uint_t y, const char *s)
{
  uint8_t i;
  for (i = 0; i < u8sl->visible; i++)
  {
    y += u8g2_draw_selection_list_line(u8g2, u8sl, y, i + u8sl->first_pos, s);
  }
}


/*
title: 		NULL for no title, valid str for title line. Can contain mutliple lines, separated by '\n'
start_pos: 	default position for the cursor, first line is 1.
sl:			string list (list of strings separated by \n)
returns 0 if user has pressed the home key
returns the selected line if user has pressed the select key
side effects:
u8g2_SetFontDirection(u8g2, 0);
u8g2_SetFontPosBaseline(u8g2);

*/
uint8_t ext_UserInterfaceSelectionList(u8g2_t *u8g2, Pad *extPad, const char *title, uint8_t start_pos, const char *sl)
{
  u8g2_SetFont(u8g2, u8g2_font_helvR10_tr);
  u8sl_t u8sl;
  u8g2_uint_t yy;

  uint8_t event;

  u8g2_uint_t line_height = u8g2_GetAscent(u8g2) - u8g2_GetDescent(u8g2) + MY_BORDER_SIZE;

  uint8_t title_lines = u8x8_GetStringLineCnt(title);
  uint8_t display_lines;


  if (start_pos > 0)	/* issue 112 */
    start_pos--;		/* issue 112 */


  if (title_lines > 0)
  {
    display_lines = (u8g2_GetDisplayHeight(u8g2) - 3) / line_height;
    u8sl.visible = display_lines;
    u8sl.visible -= title_lines;
  }
  else
  {
    display_lines = u8g2_GetDisplayHeight(u8g2) / line_height;
    u8sl.visible = display_lines;
  }

  u8sl.total = u8x8_GetStringLineCnt(sl);
  u8sl.first_pos = 0;
  u8sl.current_pos = start_pos;

  if (u8sl.current_pos >= u8sl.total)
    u8sl.current_pos = u8sl.total - 1;
  if (u8sl.first_pos + u8sl.visible <= u8sl.current_pos)
    u8sl.first_pos = u8sl.current_pos - u8sl.visible + 1;

  u8g2_SetFontPosBaseline(u8g2);

  for (;;)
  {
    u8g2_FirstPage(u8g2);
    do
    {
      yy = u8g2_GetAscent(u8g2);
      if (title_lines > 0)
      {
        yy += u8g2_DrawUTF8Lines(u8g2, 0, yy, u8g2_GetDisplayWidth(u8g2), line_height, title);

        u8g2_DrawHLine(u8g2, 0, yy - line_height - u8g2_GetDescent(u8g2) + 1, u8g2_GetDisplayWidth(u8g2));

        yy += 3;
      }
      u8g2_DrawSelectionList(u8g2, &u8sl, yy, sl);
    } while (u8g2_NextPage(u8g2));

#ifdef U8G2_REF_MAN_PIC
    return 0;
#endif
    for (;;)
    {
      event = ext_GetMenuEvent(extPad);
      if (event == U8X8_MSG_GPIO_MENU_NEXT)
      {
        return u8sl.current_pos + 1;		/* +1, issue 112 */
      }

      else if (event == U8X8_MSG_GPIO_MENU_PREV)
      {
        return 0;				/* issue 112: return 0 instead of start_pos */
      }

      else if (event == U8X8_MSG_GPIO_MENU_DOWN)
      {
        u8sl_Next(&u8sl);
        break;
      }
      else if (event == U8X8_MSG_GPIO_MENU_UP)
      {
        u8sl_Prev(&u8sl);
        break;
      }
    }
  }
}
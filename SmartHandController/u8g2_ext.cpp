#include "u8g2_ext.h"
#include "u8g2_ext_catalog2.h"

#include "u8g2_ext_input.h"
#include "u8g2_ext_selection.h"
#include "u8g2_ext_message.h"
#include "u8g2_ext_value.h"


bool U8G2_EXT::UserInterfaceCatalog(Pad *extPad, const char *title)
{
  return ext_UserInterfaceCatalog2(U8G2_EXT::getU8g2(), extPad, title);
};

uint8_t U8G2_EXT::UserInterfaceMessage(Pad *extPad, const char *title1, const char *title2, const char *title3, const char *buttons)
{
  return ext_UserInterfaceMessage(U8G2_EXT::getU8g2(), extPad, title1, title2, title3, buttons);
};
uint8_t U8G2_EXT::UserInterfaceSelectionList(Pad *extPad, const char *title, uint8_t start_pos, const char *sl)
{
  return ext_UserInterfaceSelectionList(U8G2_EXT::getU8g2(), extPad, title, start_pos, sl);
};

uint8_t U8G2_EXT::UserInterfaceInputValueInteger(Pad *extPad, const char *title, const char *pre, uint8_t *value, uint8_t lo, uint8_t hi, uint8_t digits, const char *post)
{
  return ext_UserInterfaceInputValueInteger(U8G2_EXT::getU8g2(), extPad, title, pre, value, lo, hi, digits, post);
};


uint8_t U8G2_EXT::UserInterfaceInputValueFloatIncr(Pad *extPad, const char *title, const char *pre, float *value, float lo, float hi, uint8_t len, uint8_t dec, float incr_ref, const char *post)
{
  return ext_UserInterfaceInputValueFloatIncr(U8G2_EXT::getU8g2(), extPad, title, pre, value, lo,  hi,  len,  dec, incr_ref, post);
}

uint8_t U8G2_EXT::UserInterfaceInputValueFloat(Pad *extPad, const char *title, const char *pre, float *value, float lo, float hi, uint8_t len, uint8_t dec, const char *post)
{
  return ext_UserInterfaceInputValueFloat(U8G2_EXT::getU8g2(), extPad, title, pre, value, lo,  hi,  len,  dec, post);
}

uint8_t U8G2_EXT::UserInterfaceInputValueDMS(Pad *extPad, const char *title, long *value, long lo, long hi,
  uint8_t digits1, char* symb1, char* symb2, char* symb3, char* symb_plus, char* symb_minus, bool display_seconds)
{
  return ext_UserInterfaceInputValueDMS(U8G2_EXT::getU8g2(), extPad, title, value, lo, hi, digits1, symb1, symb2, symb3, symb_plus, symb_minus, display_seconds);
};


uint8_t U8G2_EXT::UserInterfaceInputValueDate(Pad *extPad, const char *title, uint8_t& year, uint8_t& month, uint8_t& day)
{
  return ext_UserInterfaceInputValueDate(U8G2_EXT::getU8g2(), extPad, title, year, month, day);
};

uint8_t U8G2_EXT::UserInterfaceInputValueRA(Pad *extPad, long *value)
{
  return ext_UserInterfaceInputValueRA(U8G2_EXT::getU8g2(), extPad, value);
};

uint8_t U8G2_EXT::UserInterfaceInputValueDec(Pad *extPad, long *value)
{
  return ext_UserInterfaceInputValueDec(U8G2_EXT::getU8g2(), extPad, value);
};

uint8_t U8G2_EXT::UserInterfaceInputValueAlt(Pad *extPad, long *value)
{
  return ext_UserInterfaceInputValueAlt(U8G2_EXT::getU8g2(), extPad, value);
};

uint8_t U8G2_EXT::UserInterfaceInputValueAz(Pad *extPad, long *value)
{
  return ext_UserInterfaceInputValueAz(U8G2_EXT::getU8g2(), extPad, value);
};

uint8_t U8G2_EXT::UserInterfaceInputValueLocalTime(Pad *extPad, long *value)
{
  return ext_UserInterfaceInputValueLocalTime(U8G2_EXT::getU8g2(), extPad, value);
};

uint8_t U8G2_EXT::UserInterfaceInputValueLatitude(Pad *extPad, long *value)
{
  return ext_UserInterfaceInputValueLatitude(U8G2_EXT::getU8g2(), extPad, value);
};

uint8_t U8G2_EXT::UserInterfaceInputValueLongitude(Pad *extPad, long *value)
{
  return ext_UserInterfaceInputValueLongitude(U8G2_EXT::getU8g2(), extPad, value);
};

uint8_t U8G2_EXT::drawRA(uint8_t x, uint8_t y, const char* Ra)
{
  return ext_drawRA(U8G2_EXT::getU8g2(), x, y, Ra);
};


uint8_t U8G2_EXT::drawDec(uint8_t x, uint8_t y, const char* Dec)
{
  return ext_drawDec(U8G2_EXT::getU8g2(), x,  y, Dec);
};

uint8_t U8G2_EXT::drawAz(uint8_t x, uint8_t y, const char* Az)
{
  return ext_drawAz(U8G2_EXT::getU8g2(), x, y, Az);
};


uint8_t U8G2_EXT::drawFoc(uint8_t y, uint8_t line_height, const char* Foc)
{
  return ext_drawFoc(U8G2_EXT::getU8g2(), y, line_height,  Foc);
};
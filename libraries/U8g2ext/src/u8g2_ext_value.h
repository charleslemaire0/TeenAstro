#pragma once
#include <U8g2lib.h>

uint8_t ext_drawRA(u8g2_t *u8g2, uint8_t x, uint8_t y, const char* Ra);

uint8_t ext_drawDec(u8g2_t *u8g2, uint8_t x, uint8_t y, const char* Dec);

uint8_t ext_drawIDeg(u8g2_t* u8g2, uint8_t x, uint8_t y, const char* IDeg);

uint8_t ext_drawAz(u8g2_t *u8g2, uint8_t x, uint8_t y, const char* Az);

uint8_t ext_drawFoc(u8g2_t *u8g2, uint8_t y, uint8_t line_height, const char* Foc);

uint8_t ext_DrawFwNumeric(u8g2_t *u8g2, uint8_t x, uint8_t y, const char* text);

uint8_t ext_GetFwNumericWidth(u8g2_t *u8g2, const char* text);
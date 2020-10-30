#pragma once
#include <U8g2lib.h>
#include <TeenAstroCatalog.h>
#include "u8g2_ext_event.h"

class U8G2_EXT : public U8G2
{
public:

  bool UserInterfaceCatalog(Pad *extPad, const char *title);

  uint8_t UserInterfaceMessage(Pad *extPad, const char *title1, const char *title2, const char *title3, const char *buttons);

  uint8_t UserInterfaceSelectionList(Pad *extPad, const char *title, uint8_t start_pos, const char *sl);

  uint8_t UserInterfaceInputValueInteger(Pad *extPad, const char *title, const char *pre, uint8_t *value, uint8_t lo, uint8_t hi, uint8_t digits, const char *post);

  uint8_t UserInterfaceInputValueFloatIncr(Pad *extPad, const char *title, const char *pre, float *value, float lo, float hi, uint8_t len, uint8_t dec, float incr_ref, const char *post);

  uint8_t UserInterfaceInputValueFloat(Pad *extPad, const char *title, const char *pre, float *value, float lo, float hi, uint8_t len, uint8_t dec, const char *post);

  uint8_t UserInterfaceInputValueDMS(Pad *extPad, const char *title, long *value, long lo, long hi,
    uint8_t digits1, char* symb1, char* symb2, char* symb3, char* symb_plus, char* symb_minus, bool display_seconds);

  uint8_t UserInterfaceInputValueDate(Pad *extPad, const char *title, uint8_t& year, uint8_t& month, uint8_t& day);

  uint8_t UserInterfaceInputValueRA(Pad *extPad, const char* label, long *value);

  uint8_t UserInterfaceInputValueDec(Pad *extPad, const char* label, long *value);

  uint8_t UserInterfaceInputValueAlt(Pad *extPad, const char* label, long *value);

  uint8_t UserInterfaceInputValueAz(Pad *extPad, const char* label, long *value);

  uint8_t UserInterfaceInputValueLocalTime(Pad *extPad, const char* label, long *value);

  uint8_t UserInterfaceInputValueLatitude(Pad *extPad, const char* label, const char* labelN, const char* labelS, long *value);

  uint8_t UserInterfaceInputValueLongitude(Pad *extPad, const char* label, const char* labelW, const char* labelE, long *value);

  uint8_t drawRA(uint8_t x, uint8_t y, const char* Ra);

  uint8_t drawDec(uint8_t x, uint8_t y, const char* Dec);

  uint8_t drawAz(uint8_t x, uint8_t y, const char* Az);

  uint8_t drawFoc(uint8_t y, uint8_t line_height, const char* Foc);

};

class U8G2_EXT_SH1106_128X64_NONAME_1_HW_I2C : public U8G2_EXT {
public: U8G2_EXT_SH1106_128X64_NONAME_1_HW_I2C(const u8g2_cb_t *rotation, uint8_t reset = U8X8_PIN_NONE, uint8_t clock = U8X8_PIN_NONE, uint8_t data = U8X8_PIN_NONE) : U8G2_EXT() {
  u8g2_Setup_sh1106_i2c_128x64_noname_1(&u8g2, rotation, u8x8_byte_arduino_hw_i2c, u8x8_gpio_and_delay_arduino);
  u8x8_SetPin_HW_I2C(getU8x8(), reset, clock, data);
};
};
class U8G2_EXT_SSD1306_128X64_NONAME_F_HW_I2C : public U8G2_EXT {
public: U8G2_EXT_SSD1306_128X64_NONAME_F_HW_I2C(const u8g2_cb_t *rotation, uint8_t reset = U8X8_PIN_NONE, uint8_t clock = U8X8_PIN_NONE, uint8_t data = U8X8_PIN_NONE) : U8G2_EXT() {
  u8g2_Setup_ssd1306_i2c_128x64_noname_f(&u8g2, rotation, u8x8_byte_arduino_hw_i2c, u8x8_gpio_and_delay_arduino);
  u8x8_SetPin_HW_I2C(getU8x8(), reset, clock, data);
}
};
class U8G2_EXT_SSD1309_128X64_NONAME_F_HW_I2C : public U8G2_EXT {
public: U8G2_EXT_SSD1309_128X64_NONAME_F_HW_I2C(const u8g2_cb_t *rotation, uint8_t reset = U8X8_PIN_NONE, uint8_t clock = U8X8_PIN_NONE, uint8_t data = U8X8_PIN_NONE) : U8G2_EXT() {
  u8g2_Setup_ssd1309_i2c_128x64_noname0_f(&u8g2, rotation, u8x8_byte_arduino_hw_i2c, u8x8_gpio_and_delay_arduino);
  u8x8_SetPin_HW_I2C(getU8x8(), reset, clock, data);
}
};
class U8G2_EXT_SSD1309_128X64_NONAME2_F_HW_I2C : public U8G2_EXT {
public: U8G2_EXT_SSD1309_128X64_NONAME2_F_HW_I2C(const u8g2_cb_t *rotation, uint8_t reset = U8X8_PIN_NONE, uint8_t clock = U8X8_PIN_NONE, uint8_t data = U8X8_PIN_NONE) : U8G2_EXT() {
  u8g2_Setup_ssd1309_i2c_128x64_noname2_f(&u8g2, rotation, u8x8_byte_arduino_hw_i2c, u8x8_gpio_and_delay_arduino);
  u8x8_SetPin_HW_I2C(getU8x8(), reset, clock, data);
}
};
class U8G2_EXT_IL3820_V2_296x128_1_SW : public U8G2_EXT
{
public: U8G2_EXT_IL3820_V2_296x128_1_SW(const u8g2_cb_t *rotation, uint8_t clock = U8X8_PIN_NONE, uint8_t data = U8X8_PIN_NONE, uint8_t cs = U8X8_PIN_NONE, uint8_t dc = U8X8_PIN_NONE, uint8_t reset = U8X8_PIN_NONE) : U8G2_EXT()
{
  u8g2_Setup_il3820_v2_296x128_1(&u8g2, rotation, u8x8_byte_arduino_4wire_sw_spi, u8x8_gpio_and_delay_arduino);
  u8x8_SetPin_4Wire_SW_SPI(getU8x8(), clock, data, cs, dc, reset);
}
};



        //U8G2_SSD1306_128X64_NONAME_F_SW_I2C display(U8G2_R0, /* clock=*/ SCL, /* data=*/ SDA, /* reset=*/ U8X8_PIN_NONE);

/*
 * u8g2_ext_tdisplay.h — U8g2 128×64 mono FB blit to LilyGO T-Display S3 (ST7789 170×320).
 *
 * Same pattern as the SDL emulator: real U8g2 SSD1306 full buffer with empty I/O
 * callbacks; after each page cycle, scale 2× and push to TFT_eSPI via present hook.
 * Backlight (GPIO38) is PWM-dimmed via setContrast() (SHC Contrast menu / timeouts).
 */
#pragma once

#include <u8g2_ext.h>
#include <TFT_eSPI.h>

class U8G2_EXT_TDisplay : public U8G2_EXT {
public:
  static constexpr int OLED_W = 128;
  static constexpr int OLED_H = 64;
  static constexpr int SCALE  = 2;
  static constexpr int TFT_W  = 320;  // landscape
  static constexpr int TFT_H  = 170;
  static constexpr int OX = (TFT_W - OLED_W * SCALE) / 2;  // 32
  static constexpr int OY = (TFT_H - OLED_H * SCALE) / 2;  // 21

  // Red-on-black (night vision); RGB565
  static constexpr uint16_t COL_ON  = 0xF800;  // red
  static constexpr uint16_t COL_OFF = 0x0000;  // black

  // T-Display S3 LCD power enable + backlight
  static constexpr int PIN_LCD_POWER = 15;
  static constexpr int PIN_BL = 38;
  static constexpr int BL_LEDC_CH = 0;
  static constexpr int BL_LEDC_FREQ = 5000;
  static constexpr int BL_LEDC_RES = 8;  // 0..255 duty

  U8G2_EXT_TDisplay(const u8g2_cb_t *rotation = U8G2_R0) : U8G2_EXT() {
    u8g2_Setup_ssd1306_i2c_128x64_noname_f(
      &u8g2, rotation, u8x8_byte_empty, u8x8_dummy_cb);
    self() = this;
  }

  ~U8G2_EXT_TDisplay() {
    if (self() == this) {
      u8g2_ext_present_cb = nullptr;
      self() = nullptr;
    }
  }

  bool initTft() {
    pinMode(PIN_LCD_POWER, OUTPUT);
    digitalWrite(PIN_LCD_POWER, HIGH);
    delay(10);

    tft_.init();
    tft_.setRotation(1);  // 320×170 landscape
    tft_.fillScreen(COL_OFF);

    // Take over backlight from TFT_eSPI digital on → PWM (GPIO38, active HIGH).
    ledcSetup(BL_LEDC_CH, BL_LEDC_FREQ, BL_LEDC_RES);
    ledcAttachPin(PIN_BL, BL_LEDC_CH);
    setContrast(220);

    tftReady_ = true;
    u8g2_ext_present_cb = &U8G2_EXT_TDisplay::presentThunk;
    return true;
  }

  void setContrast(uint8_t value) override {
    // SHC menu: Min=0, Low=63, High=127, Max=255; idle timeout uses 0.
    ledcWrite(BL_LEDC_CH, value);
  }

  void blitToTft() {
    uint8_t* buf = getBufferPtr();
    if (!buf || !tftReady_)
      return;

    // 2× nearest-neighbour into a line buffer (RGB565).
    uint16_t line[OLED_W * SCALE];
    for (int y = 0; y < OLED_H; y++) {
      for (int x = 0; x < OLED_W; x++) {
        const int byteIdx = (y / 8) * OLED_W + x;
        const int bitIdx  = y % 8;
        const bool on = (buf[byteIdx] >> bitIdx) & 1;
        const uint16_t c = on ? COL_ON : COL_OFF;
        const int dx = x * SCALE;
        line[dx] = c;
        line[dx + 1] = c;
      }
      const int dy = OY + y * SCALE;
      tft_.pushImage(OX, dy, OLED_W * SCALE, 1, line);
      tft_.pushImage(OX, dy + 1, OLED_W * SCALE, 1, line);
    }
  }

  void sendBuffer() override {
    U8G2::sendBuffer();
    blitToTft();
  }

private:
  static U8G2_EXT_TDisplay*& self() {
    static U8G2_EXT_TDisplay* inst = nullptr;
    return inst;
  }

  static void presentThunk() {
    if (self())
      self()->blitToTft();
  }

  TFT_eSPI tft_;
  bool tftReady_ = false;
};

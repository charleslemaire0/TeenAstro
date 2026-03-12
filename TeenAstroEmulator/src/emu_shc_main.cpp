/*
 * emu_shc_main.cpp -- Single-TU build of the TeenAstro SHC emulator.
 *
 * Build:  pio run -d TeenAstroEmulator -e emu_shc
 * Run:    .pio\build\emu_shc\program.exe
 */

#ifdef EMU_SHC

/* Include Winsock BEFORE anything that defines INPUT/OUTPUT macros */
#ifdef _WIN32
#  ifndef _WIN32_WINNT
#    define _WIN32_WINNT 0x0601
#  endif
#  include <winsock2.h>
#  include <ws2tcpip.h>
#  undef lst1
#  undef lst2
#endif

/* Entry point */
#include "../emu/shc_emu.cpp"

/* ---- U8g2 C library core ---- */
extern "C" {
#include "../../libraries/U8g2/src/clib/u8g2_bitmap.c"
#include "../../libraries/U8g2/src/clib/u8g2_box.c"
#include "../../libraries/U8g2/src/clib/u8g2_buffer.c"
#include "../../libraries/U8g2/src/clib/u8g2_button.c"
#include "../../libraries/U8g2/src/clib/u8g2_circle.c"
#include "../../libraries/U8g2/src/clib/u8g2_cleardisplay.c"
#include "../../libraries/U8g2/src/clib/u8g2_d_memory.c"
/* Provide only the setup functions we need instead of the full u8g2_d_setup.c
   which references ALL display drivers. */
void u8g2_Setup_ssd1306_i2c_128x64_noname_f(u8g2_t *u8g2, const u8g2_cb_t *rotation,
    u8x8_msg_cb byte_cb, u8x8_msg_cb gpio_and_delay_cb)
{
  uint8_t tile_buf_height;
  uint8_t *buf;
  u8g2_SetupDisplay(u8g2, u8x8_d_ssd1306_128x64_noname, u8x8_cad_ssd13xx_fast_i2c, byte_cb, gpio_and_delay_cb);
  buf = u8g2_m_16_8_f(&tile_buf_height);
  u8g2_SetupBuffer(u8g2, buf, tile_buf_height, u8g2_ll_hvline_vertical_top_lsb, rotation);
}
#include "../../libraries/U8g2/src/clib/u8g2_font.c"
#include "../../libraries/U8g2/src/clib/u8g2_fonts.c"
#include "../../libraries/U8g2/src/clib/u8g2_hvline.c"
#include "../../libraries/U8g2/src/clib/u8g2_intersection.c"
#include "../../libraries/U8g2/src/clib/u8g2_kerning.c"
#include "../../libraries/U8g2/src/clib/u8g2_line.c"
#include "../../libraries/U8g2/src/clib/u8g2_ll_hvline.c"
#include "../../libraries/U8g2/src/clib/u8g2_message.c"
#include "../../libraries/U8g2/src/clib/u8g2_polygon.c"
#define u8g2_DrawSelectionList _orig_u8g2_DrawSelectionList
#define u8g2_draw_selection_list_line _orig_u8g2_draw_selection_list_line
#define u8g2_UserInterfaceSelectionList _orig_u8g2_UserInterfaceSelectionList
#include "../../libraries/U8g2/src/clib/u8g2_selection_list.c"
#undef u8g2_DrawSelectionList
#undef u8g2_draw_selection_list_line
#undef u8g2_UserInterfaceSelectionList
#include "../../libraries/U8g2/src/clib/u8g2_input_value.c"
#include "../../libraries/U8g2/src/clib/u8g2_setup.c"
/* Display drivers used by SHC */
#include "../../libraries/U8g2/src/clib/u8x8_d_ssd1306_128x64_noname.c"
#include "../../libraries/U8g2/src/clib/u8x8_d_ssd1309.c"
#include "../../libraries/U8g2/src/clib/u8x8_d_sh1107.c"
/* U8x8 core */
#include "../../libraries/U8g2/src/clib/u8x8_8x8.c"
#include "../../libraries/U8g2/src/clib/u8x8_byte.c"
#include "../../libraries/U8g2/src/clib/u8x8_cad.c"
#include "../../libraries/U8g2/src/clib/u8x8_display.c"
#include "../../libraries/U8g2/src/clib/u8x8_fonts.c"
#include "../../libraries/U8g2/src/clib/u8x8_gpio.c"
#include "../../libraries/U8g2/src/clib/u8x8_setup.c"
#include "../../libraries/U8g2/src/clib/u8x8_string.c"
#include "../../libraries/U8g2/src/clib/u8x8_u16toa.c"
#include "../../libraries/U8g2/src/clib/u8x8_u8toa.c"
#include "../../libraries/U8g2/src/clib/u8x8_capture.c"
#include "../../libraries/U8g2/src/clib/u8x8_debounce.c"
#include "../../libraries/U8g2/src/clib/u8x8_selection_list.c"
} // extern "C"

/* ---- U8g2ext (C++ UI extensions) ---- */
#include "../../libraries/U8g2ext/src/u8g2_ext.cpp"
#include "../../libraries/U8g2ext/src/u8g2_ext_catalog.cpp"
#include "../../libraries/U8g2ext/src/u8g2_ext_drawValue.cpp"
#include "../../libraries/U8g2ext/src/u8g2_ext_event.cpp"
#include "../../libraries/U8g2ext/src/u8g2_ext_input.cpp"
#include "../../libraries/U8g2ext/src/u8g2_ext_message.cpp"
#include "../../libraries/U8g2ext/src/u8g2_ext_selection.cpp"

/* ---- Library .cpp files ---- */
#define atoi2 _codec_atoi2
#include "../../libraries/TeenAstroCommandDef/src/CommandCodec.cpp"
#undef atoi2
#include "../../libraries/TeenAstroLX200io/src/LX200Client.cpp"
#include "../../libraries/TeenAstroLX200io/src/LX200Navigation.cpp"
#include "../../libraries/TeenAstroMountStatus/TeenAstroMountStatus.cpp"
#include "../../libraries/TeenAstroCatalog/src/TeenAstroCatalog.cpp"
#include "../../libraries/TeenAstroMath/src/TeenAstroMath.cpp"
#include "../../libraries/ephemeris-master/Calendar.cpp"
#include "../../libraries/ephemeris-master/Ephemeris.cpp"

/* ---- SDL2 Pad replacement (instead of TeenAstroPad.cpp) ---- */
#include "../shim/TeenAstroPad_sdl.cpp"

/* ---- WiFi stub (instead of TeenAstroWifi .cpp files) ---- */
#include "../shim/TeenAstroWifi_stub.cpp"

/* ---- SHC firmware .cpp files ---- */
#include "../../TeenAstroSHC/SmartController.cpp"
#include "../../TeenAstroSHC/SmartController_Display.cpp"
#include "../../TeenAstroSHC/SmartController_Message.cpp"
#include "../../TeenAstroSHC/SmartController_CatFilter.cpp"
#include "../../TeenAstroSHC/Settings_Wifi.cpp"
#include "../../TeenAstroSHC/Settings_TimeSite.cpp"
#include "../../TeenAstroSHC/Settings_Tel.cpp"
#include "../../TeenAstroSHC/Settings_Mount.cpp"
#include "../../TeenAstroSHC/Settings_Motors.cpp"
#include "../../TeenAstroSHC/Settings_Motor.cpp"
#include "../../TeenAstroSHC/Settings_Limits.cpp"
#include "../../TeenAstroSHC/Settings_Focuser.cpp"
#include "../../TeenAstroSHC/Settings_Encoders.cpp"
#include "../../TeenAstroSHC/Settings_SHC.cpp"
#include "../../TeenAstroSHC/Actions_Tel.cpp"
#include "../../TeenAstroSHC/Actions_SyncGoto.cpp"
#include "../../TeenAstroSHC/Actions_Focuser.cpp"

#endif /* EMU_SHC */

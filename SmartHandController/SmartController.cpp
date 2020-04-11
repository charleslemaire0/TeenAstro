
#include <TeenAstroMountStatus.h>
#include "_EEPROM_ext.h"
#include "SHC_text.h"
#include "SmartController.h"
#include "MenuSyncGoto.h"

static char* BreakRC[6] = { ":Qn#" ,":Qs#" ,":Qe#" ,":Qw#", ":Fo#", ":Fi#" };
static char* RC[6] = { ":Mn#" , ":Ms#" ,":Me#" ,":Mw#", ":FO#", ":FI#" };

#define MY_BORDER_SIZE 1
#define icon_width 16
#define icon_height 16

#define teenastro_width 128
#define teenastro_height 68

static unsigned char wifi_bits[] U8X8_PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0x80, 0x20, 0x40, 0x4e, 0x00, 0x11,
  0x00, 0x04, 0x00, 0x04, 0x00, 0x04, 0xfe, 0x7f, 0x02, 0x40, 0xda, 0x5f,
  0xda, 0x5f, 0x02, 0x40, 0xfe, 0x7f, 0x00, 0x00 };

static unsigned char wifi_not_connected_bits[] U8X8_PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x04, 0x00, 0x04, 0x00, 0x04, 0xfe, 0x7f, 0x02, 0x40, 0x02, 0x40,
  0x02, 0x40, 0x02, 0x40, 0xfe, 0x7f, 0x00, 0x00 };

static unsigned char align1_bits[] U8X8_PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x1c, 0x00, 0x1c, 0x00, 0x00, 0x00,
  0xc0, 0x03, 0xc0, 0x00, 0x40, 0x01, 0x50, 0x02, 0x18, 0x04, 0x10, 0x08,
  0x10, 0x10, 0x10, 0x20, 0x00, 0x00, 0x00, 0x00 };

static unsigned char align2_bits[] U8X8_PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x1c, 0x00, 0x1c, 0x00, 0x00, 0x00,
  0xc0, 0x03, 0xc0, 0x00, 0x40, 0x01, 0x48, 0x02, 0x14, 0x04, 0x10, 0x08,
  0x08, 0x10, 0x04, 0x20, 0x1c, 0x00, 0x00, 0x00 };

static unsigned char align3_bits[] U8X8_PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x1c, 0x00, 0x1c, 0x00, 0x00, 0x00,
  0xc0, 0x03, 0xc0, 0x00, 0x40, 0x01, 0x40, 0x02, 0x1c, 0x04, 0x10, 0x08,
  0x18, 0x10, 0x10, 0x20, 0x1c, 0x00, 0x00, 0x00 };

static unsigned char home_bits[] U8X8_PROGMEM = {
  0x00, 0x02, 0x00, 0x07, 0x80, 0x0f, 0xc0, 0x1f, 0x80, 0x3f, 0x00, 0x7f,
  0x00, 0x7e, 0x00, 0x7f, 0x80, 0xfb, 0xc0, 0xc1, 0xe0, 0x01, 0xbc, 0x49,
  0x9e, 0x49, 0x9e, 0x79, 0x8c, 0x49, 0x80, 0x49 };

static unsigned char parked_bits[] U8X8_PROGMEM = {
  0x00, 0x00, 0xfe, 0x7f, 0x02, 0x40, 0x02, 0x40, 0xe2, 0x43, 0x62, 0x46,
  0x62, 0x46, 0x62, 0x46, 0xe2, 0x43, 0x62, 0x40, 0x62, 0x40, 0x62, 0x40,
  0x62, 0x40, 0x02, 0x40, 0xfe, 0x7f, 0x00, 0x00 };

static unsigned char parking_bits[] U8X8_PROGMEM = {
  0xff, 0xff, 0x01, 0x80, 0x01, 0x80, 0xf9, 0x80, 0x99, 0x81, 0x99, 0x81,
  0x99, 0x81, 0xf9, 0x80, 0x19, 0x80, 0x99, 0x84, 0x99, 0x8d, 0x99, 0x9f,
  0x81, 0x8d, 0x81, 0x84, 0x01, 0x80, 0xff, 0xff };

static unsigned char parkingFailed_bits[] U8X8_PROGMEM = {
  0xff, 0xff, 0x01, 0x80, 0x01, 0x80, 0xf9, 0x90, 0x99, 0x91, 0x99, 0x91,
  0x99, 0x91, 0xf9, 0x90, 0x19, 0x90, 0xd9, 0x93, 0x59, 0x90, 0xd9, 0x91,
  0x41, 0x80, 0x41, 0x90, 0x01, 0x80, 0xff, 0xff };

static unsigned char guiding_bits[] U8X8_PROGMEM = {
  0x00, 0x00, 0x80, 0x01, 0x80, 0x01, 0xc0, 0x03, 0x20, 0x04, 0x10, 0x08,
  0x08, 0x10, 0x8e, 0x71, 0x8e, 0x71, 0x08, 0x10, 0x10, 0x08, 0x20, 0x04,
  0xc0, 0x03, 0x80, 0x01, 0x80, 0x01, 0x00, 0x00 };

static unsigned char guiding_W_bits[] U8X8_PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x00, 0x70,
  0x00, 0x78, 0x00, 0x7c, 0x00, 0x7c, 0x00, 0x78, 0x00, 0x70, 0x00, 0x60,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

static unsigned char guiding_N_bits[] U8X8_PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x01, 0xc0, 0x03,
  0xe0, 0x07, 0xf0, 0x0f, 0xf0, 0x0f, 0x00, 0x00 };

static unsigned char guiding_S_bits[] U8X8_PROGMEM = {
  0x00, 0x00, 0xf0, 0x0f, 0xf0, 0x0f, 0xe0, 0x07, 0xc0, 0x03, 0x80, 0x01,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

static unsigned char guiding_E_bits[] U8X8_PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x0e, 0x00,
  0x1e, 0x00, 0x3e, 0x00, 0x3e, 0x00, 0x1e, 0x00, 0x0e, 0x00, 0x06, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

static unsigned char guiding__bits[] U8X8_PROGMEM = {
  0x00, 0x00, 0xf0, 0x0f, 0x10, 0x08, 0x20, 0x04, 0x46, 0x62, 0x8a, 0x51,
  0x12, 0x48, 0x22, 0x44, 0x22, 0x44, 0x12, 0x48, 0x8a, 0x51, 0x46, 0x62,
  0x20, 0x04, 0x10, 0x08, 0xf0, 0x0f, 0x00, 0x00 };

static unsigned char no_tracking_bits[] U8X8_PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x38, 0x1c, 0x7c, 0x3e, 0x7c, 0x3e, 0x7c, 0x3e,
  0x7c, 0x3e, 0x7c, 0x3e, 0x7c, 0x3e, 0x7c, 0x3e, 0x7c, 0x3e, 0x7c, 0x3e,
  0x7c, 0x3e, 0x7c, 0x3e, 0x38, 0x1c, 0x00, 0x00 };

static unsigned char tracking_bits[] U8X8_PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x06, 0x00, 0x0e, 0x00, 0x1e, 0x00,
  0x3e, 0x00, 0x7e, 0x00, 0xfe, 0x00, 0x7e, 0x00, 0x3e, 0x00, 0x1e, 0x00,
  0x0e, 0x00, 0x06, 0x00, 0x02, 0x00, 0x00, 0x00 };

static unsigned char tracking_moon_bits[] U8X8_PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3c, 0x00, 0x0e, 0x00, 0x0e,
  0x00, 0x0e, 0x00, 0x0e, 0x00, 0x3c, 0x00, 0x00 };

static unsigned char tracking_star_bits[] U8X8_PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x38, 0x00, 0x04, 0x00, 0x04, 0x00, 0x18,
  0x00, 0x20, 0x00, 0x20, 0x00, 0x1c, 0x00, 0x00 };

static unsigned char tracking_sun_bits[] U8X8_PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3c, 0x00, 0x42, 0x00, 0x5a,
  0x00, 0x5a, 0x00, 0x42, 0x00, 0x3c, 0x00, 0x00 };

static unsigned char sleewing_bits[] U8X8_PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x02, 0x01, 0x06, 0x03, 0x0e, 0x07, 0x1e, 0x0f,
  0x3e, 0x1f, 0x7e, 0x3f, 0xfe, 0x7f, 0x7e, 0x3f, 0x3e, 0x1f, 0x1e, 0x0f,
  0x0e, 0x07, 0x06, 0x03, 0x02, 0x01, 0x00, 0x00 };

static unsigned char W_bits[] U8X8_PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0xe0, 0x03, 0x10, 0x04, 0x08, 0x08, 0x24, 0x12,
  0x22, 0x22, 0x22, 0x22, 0xa2, 0x22, 0xa2, 0x22, 0x42, 0x21, 0x44, 0x11,
  0x08, 0x08, 0x10, 0x04, 0xe0, 0x03, 0x00, 0x00 };

static unsigned char E_bits[] U8X8_PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0xe0, 0x03, 0x10, 0x04, 0x08, 0x08, 0xe4, 0x13,
  0x22, 0x20, 0x22, 0x20, 0xe2, 0x21, 0x22, 0x20, 0x22, 0x20, 0xe4, 0x13,
  0x08, 0x08, 0x10, 0x04, 0xe0, 0x03, 0x00, 0x00 };

static unsigned char ErrDe_bits[] U8X8_PROGMEM = {
  0xff, 0xff, 0x00, 0x80, 0x0e, 0xb0, 0x02, 0xb0, 0x66, 0xb3, 0x22, 0xb1,
  0x2e, 0xb1, 0x00, 0xb0, 0x1e, 0xb0, 0x22, 0xb0, 0xa2, 0xb3, 0xa2, 0xb2,
  0xa2, 0x83, 0xa2, 0xb0, 0x9e, 0xb3, 0x00, 0x80 };

static unsigned char ErrAz_bits[] U8X8_PROGMEM = {
   0xff, 0xff, 0x00, 0x80, 0x0e, 0xb0, 0x02, 0xb0, 0x66, 0xb3, 0x22, 0xb1,
   0x2e, 0xb1, 0x00, 0xb0, 0x00, 0xb0, 0x08, 0xb0, 0x94, 0xb3, 0x22, 0xb2,
   0x3e, 0x81, 0xa2, 0xb0, 0xa2, 0xb3, 0x00, 0x80 };

static unsigned char ErrHo_bits[] U8X8_PROGMEM = {
  0xff, 0xff, 0x00, 0x80, 0x0e, 0xb0, 0x02, 0xb0, 0x66, 0xb3, 0x22, 0xb1,
  0x2e, 0xb1, 0x00, 0xb0, 0x22, 0xb0, 0x22, 0xb0, 0x22, 0xb3, 0xbe, 0xb4,
  0xa2, 0x84, 0xa2, 0xb4, 0x22, 0xb3, 0x00, 0x80 };

static unsigned char ErrMe_bits[] U8X8_PROGMEM = {
  0xff, 0xff, 0x00, 0x80, 0x0e, 0xb0, 0x02, 0xb0, 0x66, 0xb3, 0x22, 0xb1,
  0x2e, 0xb1, 0x00, 0xb0, 0x22, 0xb0, 0x36, 0xb0, 0xaa, 0xb3, 0xa2, 0xb2,
  0xa2, 0x83, 0xa2, 0xb0, 0xa2, 0xb3, 0x00, 0x80 };

static unsigned char ErrMf_bits[] U8X8_PROGMEM = {
  0xff, 0xff, 0x00, 0x80, 0x0e, 0xb0, 0x02, 0xb0, 0x66, 0xb3, 0x22, 0xb1,
  0x2e, 0xb1, 0x00, 0xb0, 0x22, 0xb0, 0x36, 0xb0, 0xaa, 0xb3, 0xa2, 0xb0,
  0xa2, 0x81, 0xa2, 0xb0, 0xa2, 0xb0, 0x00, 0x80 };

static unsigned char ErrUp_bits[] U8X8_PROGMEM = {
  0xff, 0xff, 0x00, 0x80, 0x0e, 0xb0, 0x02, 0xb0, 0x66, 0xb3, 0x22, 0xb1,
  0x2e, 0xb1, 0x00, 0xb0, 0x22, 0xb0, 0x22, 0xb0, 0xa2, 0xb3, 0xa2, 0xb2,
  0xa2, 0x83, 0xa2, 0xb0, 0x9c, 0xb0, 0x00, 0x80 };

static unsigned char Lock___bits[] U8X8_PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x21,
  0x00, 0x21, 0x80, 0x7f, 0x80, 0x7f, 0x80, 0x7f, 0x80, 0x7f, 0x80, 0x7f,
  0x80, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

static unsigned char Lock_F_bits[] U8X8_PROGMEM = {
  0x00, 0x00, 0x3e, 0x00, 0x02, 0x00, 0x02, 0x00, 0x1e, 0x00, 0x02, 0x00,
  0x02, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

static unsigned char Lock_T_bits[] U8X8_PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x08, 0x00, 0x08, 0x00,
  0x08, 0x00, 0x08, 0x00, 0x08, 0x00, 0x00, 0x00 };

static unsigned char GNSS_bits[] U8X8_PROGMEM = {
  0x00, 0x00, 0x40, 0x00, 0xa0, 0x00, 0x10, 0x01, 0x08, 0x01, 0x10, 0x07,
  0xe0, 0x07, 0x80, 0x1f, 0x80, 0x23, 0x2a, 0x42, 0x4a, 0x22, 0x12, 0x14,
  0x64, 0x08, 0x08, 0x00, 0x70, 0x00, 0x00, 0x00 };

static unsigned char Spiral_bits[] U8X8_PROGMEM = {
   0x00, 0x00, 0xfc, 0x3f, 0xfe, 0x7f, 0x06, 0x60, 0x06, 0x60, 0xc6, 0x63,
   0xe6, 0x67, 0x66, 0x66, 0x66, 0x66, 0x06, 0x66, 0x06, 0x66, 0xfe, 0x67,
   0xfc, 0x63, 0x00, 0x60, 0x00, 0x60, 0x00, 0x00 };

static const unsigned char teenastro_bits[] U8X8_PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0x03,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x78, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x80, 0x1f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00,
  0x00, 0x00, 0x00, 0x10, 0x00, 0x80, 0x1f, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x0f, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00,
  0x00, 0xe0, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0x07, 0x00,
  0x00, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0xe0, 0x07, 0x00, 0x00, 0x00, 0x01, 0x00, 0xfc, 0x01, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00,
  0xfe, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0x07, 0x00,
  0x00, 0x00, 0x00, 0x00, 0xfe, 0x0f, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
  0x00, 0xe0, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfe, 0x3f, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00,
  0xfc, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x1f, 0x00,
  0x00, 0x00, 0x00, 0x00, 0xf0, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x80, 0x3f, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0xff, 0x03, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0xff, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x7e, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0x1f, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0x7f, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0x07, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x80, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfe, 0x03, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfc, 0x0f,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0xf8, 0x1f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x9c, 0x7f, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xfe,
  0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x06, 0xf8, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x0f, 0xe0, 0x07, 0x00, 0x00, 0x00,
  0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x1f, 0xc0,
  0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0xc0, 0x1e, 0x00, 0x3f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x1e, 0x00, 0xfc, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x3c, 0x00,
  0xf0, 0x81, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x60, 0x2c, 0x00, 0x80, 0xc7, 0x1f, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x01, 0x00, 0x00, 0x00, 0x20, 0x68, 0x00, 0x00, 0xce, 0x3f, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x48, 0x00,
  0x00, 0xc0, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x10, 0xd8, 0x00, 0x00, 0xf0, 0x7f, 0x00, 0x00, 0x20, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0xd8, 0x00, 0x00, 0xf0, 0x7f, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x98, 0x01,
  0x00, 0xf0, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x0c, 0x90, 0x01, 0x00, 0xf0, 0x3f, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x30, 0x01, 0x00, 0xf0, 0x3f, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x30, 0x03,
  0x00, 0xc0, 0x1f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x02, 0x30, 0x07, 0x00, 0x80, 0x3f, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x30, 0x06, 0x00, 0x80, 0x7f, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x20, 0x04,
  0x00, 0x80, 0xff, 0x0f, 0xfe, 0x01, 0x00, 0x00, 0x1c, 0x00, 0x02, 0x00,
  0x80, 0x01, 0x20, 0x0c, 0x00, 0xc0, 0xff, 0x3f, 0xfe, 0x01, 0x00, 0x00,
  0x1c, 0x00, 0x03, 0x00, 0x80, 0x00, 0x20, 0x08, 0x00, 0xe0, 0xff, 0xff,
  0x30, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x03, 0x00, 0xc0, 0x00, 0x20, 0x18,
  0x00, 0xf0, 0xff, 0xff, 0x30, 0x38, 0x1c, 0x1b, 0x3e, 0x9c, 0xb7, 0x18,
  0x40, 0x00, 0x60, 0x30, 0x00, 0xfc, 0xff, 0xff, 0x30, 0x6c, 0x36, 0x3f,
  0x36, 0xbe, 0xf7, 0x3c, 0x60, 0x00, 0x40, 0x30, 0x80, 0xff, 0xff, 0xff,
  0x30, 0x6c, 0x36, 0x33, 0x36, 0x26, 0x33, 0x66, 0x20, 0x00, 0x40, 0x20,
  0xc0, 0x8f, 0xff, 0xff, 0x30, 0x7c, 0x3e, 0x33, 0x63, 0x1e, 0x33, 0x66,
  0x20, 0x00, 0x40, 0x60, 0xc0, 0x07, 0xff, 0xff, 0x30, 0x0c, 0x06, 0x33,
  0x7f, 0x38, 0x33, 0x66, 0x30, 0x00, 0x40, 0x40, 0xc0, 0x1f, 0xff, 0xff,
  0x30, 0x6c, 0x36, 0x33, 0x7f, 0x32, 0x33, 0x66, 0x10, 0x00, 0xc0, 0xc0,
  0xc0, 0x7f, 0xff, 0xff, 0x30, 0x6c, 0x36, 0xb3, 0xc1, 0x3e, 0x37, 0x3c,
  0x18, 0x00, 0xc0, 0xc0, 0xc0, 0xff, 0xff, 0xff, 0x30, 0x38, 0x1c, 0xb3,
  0xc1, 0x1c, 0x36, 0x1c, 0x0c, 0x00, 0xc0, 0x80, 0xc1, 0xff, 0xff, 0xff,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x00, 0xc0, 0x81,
  0xc1, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x0e, 0x00, 0xc0, 0x81, 0xf3, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x80, 0xc3, 0xf3, 0xff, 0xff, 0xff,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x00, 0x80, 0xff,
  0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x07, 0xf8, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
  0x00, 0xf0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };


void SmartHandController::setup(const char version[], const int pin[7], const bool active[7], const int SerialBaud, const OLED model)
{
  if (EEPROM.length() == 0)
    EEPROM.begin(1024);
  if (strlen(version) <= 19) strcpy(_version, version);

  //choose a 128x64 display supported by U8G2lib (if not listed below there are many many others in u8g2 library example Sketches)
  Serial.begin(SerialBaud);
  switch (model)
  {
  case OLED_SH1106:
    display = new U8G2_EXT_SH1106_128X64_NONAME_1_HW_I2C(U8G2_R0);
    break;
  case OLED_SSD1306:
    display = new U8G2_EXT_SSD1306_128X64_NONAME_F_HW_I2C(U8G2_R0);
    break;
  case OLED_SSD1309:
    display = new U8G2_EXT_SSD1309_128X64_NONAME_F_HW_I2C(U8G2_R0);
    break;
  }
  display->begin();
  drawIntro();
  buttonPad.setup(pin, active);
  tickButtons();
  maxContrast = EEPROM.read(EEPROM_Contrast);
  display->setContrast(maxContrast);
  displayT1 = EEPROM.read(EEPROM_T1);
  if (displayT1 < 3)
  {
    displayT1 = 3;
    EEPROM.write(EEPROM_T1, displayT1);
    EEPROM.commit();
  }
  displayT2 = EEPROM.read(EEPROM_T2);
  if (displayT2 < displayT1)
  {
    displayT2 = displayT1;
    EEPROM.write(EEPROM_T2, displayT2);
    EEPROM.commit();
  }

#ifdef DEBUG_ON
  DebugSer.begin(9600);
  delay(1000);
#endif

  drawLoad();
}

void SmartHandController::tickButtons()
{
  buttonPad.tickButtons();
}

bool SmartHandController::buttonPressed()
{
  return buttonPad.buttonPressed();
}

bool SmartHandController::isSleeping()
{
  if (forceDisplayoff)
  {
    if (!buttonPad.shiftPressed())
    {
      bool moving = false;
      manualMove(moving);
      return true;
    }
    else
      forceDisplayoff = false;
  }
  if (buttonPressed())
  {
    time_last_action = millis();
    if (sleepDisplay)
    {
      display->setContrast(maxContrast);
      display->sleepOff();
      sleepDisplay = false;
      lowContrast = false;
      buttonPad.setControlerMode();
      return true;
    }
    if (lowContrast)
    {
      lowContrast = false;
      display->setContrast(maxContrast);
      buttonPad.setControlerMode();
      return true;
    }
  }
  else if (sleepDisplay)
  {
    return true;
  }
  else if ((top - time_last_action) / 10000 > displayT2)
  {
    display->sleepOn();
    sleepDisplay = true;
    buttonPad.setMenuMode();
    return false;
  }
  else if ((top - time_last_action) / 10000 > displayT1 && !lowContrast)
  {
    display->setContrast(0);
    lowContrast = true;
    buttonPad.setMenuMode();
    return true;
  }
  return false;
}

void SmartHandController::manualMove(bool &moving)
{
  moving = ta_MountStatus.getTrackingState() == TeenAstroMountStatus::TRK_SLEWING ||
    ta_MountStatus.getParkState() == TeenAstroMountStatus::PRK_PARKING ||
    ta_MountStatus .isSpiralRunning();
  if (moving)
  {
    bool stop = (eventbuttons[0] == E_LONGPRESS || eventbuttons[0] == E_LONGPRESSTART || eventbuttons[0] == E_DOUBLECLICK) ? true : false;
    int it = 1;
    while (!stop && it < 5)
    {
      stop = (eventbuttons[it] == E_LONGPRESS || eventbuttons[it] == E_CLICK || eventbuttons[it] == E_LONGPRESSTART);
      it++;
    }
    if (stop)
    {
      Ser.print(":Q#");
      Ser.flush();
      time_last_action = millis();
      display->sleepOff();
      ta_MountStatus.backStepAlign();
      return;
    }
  }
  else
  {
    buttonCommand = false;
    for (int k = 1; k < 7; k++)
    {
      if (Move[k - 1] && (eventbuttons[k] == E_LONGPRESSSTOP || eventbuttons[k] == E_NONE))
      {
        buttonCommand = true;
        Move[k - 1] = false;
        if (k < 5)
          SetBoolLX200(BreakRC[k - 1]);
        else
          Move[k - 1] = !(SetBoolLX200(BreakRC[k - 1]) == LX200VALUESET);
        continue;
      }
      else if (eventbuttons[0] == E_NONE && !Move[k - 1] && (eventbuttons[k] == E_LONGPRESS || eventbuttons[k] == E_CLICK || eventbuttons[k] == E_LONGPRESSTART))
      {
        buttonCommand = true;

        if (k < 5)
        {
          if (!telescoplocked)
          {
            Move[k - 1] = true;
            SetBoolLX200(RC[k - 1]);
          }
        }
        else if (!focuserlocked)
          Move[k - 1] = (SetBoolLX200(RC[k - 1]) == LX200VALUESET);
        continue;
      }
      moving = moving || Move[k - 1];
    }
    if (buttonCommand)
    {
      time_last_action = millis();
      return;
    }
  }
}

void SmartHandController::update()
{
  tickButtons();
  top = millis();
  if (isSleeping())
    return;
  if (powerCycleRequired)
  {
    display->sleepOff();
    DisplayMessage(T_PRESS_KEY, T_TO_REBOOT "...", -1);
    DisplayMessage(T_DEVICE, T_WILL_REBOOT "...", 1000);
    ESP.reset();
    return;
  }
  if (ta_MountStatus.notResponding())
  {
    display->sleepOff();
    DisplayMessage("!! " T_ERROR " !!", T_NOT_CONNECTED, -1);
    DisplayMessage(T_DEVICE, T_WILL_REBOOT "...", 1000);
    ESP.reset();
  }
  if (ta_MountStatus.isAlignSelect())
  {
    char message[10] = T_STAR "#?";
    message[6] = '0' + ta_MountStatus.getAlignStar();
    DisplayLongMessage(T_SELECTASTAR, T_FROMFOLLOWINGLIST, "", message, -1);
    if (!SelectStarAlign())
    {
      DisplayMessage(T_SELECTION, T_ABORTED, -1);
      ta_MountStatus.backStepAlign();
      return;
    }
    else
    {
      ta_MountStatus.nextStepAlign();
    }
  }
  else if (top - lastpageupdate > 200)
  {
    updateMainDisplay(pages[current_page].p);
  }
  if (!ta_MountStatus.connected())
    return;
  bool moving = false;
  manualMove(moving);
  if (eventbuttons[0] == E_CLICK && !ta_MountStatus.isAligning())
  {
    for (int k = 1; k < NUMPAGES + 1; k++)
    {     
      current_page++;
      if (current_page >= NUMPAGES)
        current_page = 0;
      if (pages[current_page].show)
      {
        if (pages[current_page].p == P_FOCUSER && !ta_MountStatus.hasFocuser())
        {
          pages[current_page].show = false;
          continue;
        }
        break;
      }
    }
  
    time_last_action = millis();
  }
  else if (moving)
  {
    return;
  }
  else if (eventbuttons[0] == E_LONGPRESS || eventbuttons[0] == E_LONGPRESSTART && !ta_MountStatus.isAligning())
  {
    if (eventbuttons[3] == E_LONGPRESS || eventbuttons[3] == E_CLICK || eventbuttons[3] == E_LONGPRESSTART)
    {
      menuTelAction();
    }
    else if (eventbuttons[1] == E_LONGPRESS || eventbuttons[3] == E_CLICK || eventbuttons[3] == E_LONGPRESSTART)
    {
      menuSpeedRate();
      time_last_action = millis();
    }
    else if (eventbuttons[4] == E_LONGPRESS || eventbuttons[3] == E_CLICK || eventbuttons[3] == E_LONGPRESSTART)
    {
      menuTelSettings();
    }
    else if (eventbuttons[6] == E_LONGPRESS || eventbuttons[3] == E_CLICK || eventbuttons[3] == E_LONGPRESSTART)
    {
      menuFocuserAction();
    }
    else if (eventbuttons[5] == E_LONGPRESS || eventbuttons[3] == E_CLICK || eventbuttons[3] == E_LONGPRESSTART)
    {
      menuFocuserSettings();
    }
    exitMenu = false;
    time_last_action = millis();
  }
  else if (eventbuttons[0] == E_CLICK && ta_MountStatus.isAlignRecenter())
  {
    TeenAstroMountStatus::AlignReply reply = ta_MountStatus.addStar();
    switch (reply)
    {
    case TeenAstroMountStatus::AlignReply::ALIR_FAILED1:
      DisplayMessage(T_ALIGNMENT, T_FAILED"!", -1);
      break;
    case TeenAstroMountStatus::AlignReply::ALIR_FAILED2:
      DisplayMessage(T_ALIGNMENT, T_WRONG"!", -1);
      break;
    case TeenAstroMountStatus::AlignReply::ALIR_DONE:
      DisplayMessage(T_ALIGNMENT, T_SUCESS"!", -1);
      break;
    case TeenAstroMountStatus::AlignReply::ALIR_ADDED:
      DisplayMessage(T_STARADDED, "=>", 1000);
      break;
    }
  }
}

void SmartHandController::updateMainDisplay(PAGES page)
{
  u8g2_t *u8g2 = display->getU8g2();
  display->setFont(u8g2_font_helvR12_te);
  u8g2_uint_t line_height = u8g2_GetAscent(u8g2) - u8g2_GetDescent(u8g2) + MY_BORDER_SIZE;
  u8g2_uint_t step1 = u8g2_GetUTF8Width(u8g2, "44");
  u8g2_uint_t step2 = u8g2_GetUTF8Width(u8g2, "4") + 1;
  ta_MountStatus.removeLastConnectionFailure();
  ta_MountStatus.updateMount();
  if (ta_MountStatus.isAligning())
    page = P_ALIGN;

  if (ta_MountStatus.hasInfoMount() && page == P_ALIGN)
  {
    TeenAstroMountStatus::TrackState curT = ta_MountStatus.getTrackingState();  
    if (curT != TeenAstroMountStatus::TRK_SLEWING && ta_MountStatus.isAlignSlew())
    {
      ta_MountStatus.nextStepAlign();
    }
  }
  else if (page == P_RADEC && !ta_MountStatus.isPulseGuiding())
  {
    ta_MountStatus.updateRaDec();
  }
  else if (page == P_ALTAZ && !ta_MountStatus.isPulseGuiding())
  {
    ta_MountStatus.updateAzAlt();
  }
  else if (page == P_TIME && !ta_MountStatus.isPulseGuiding())
  {
    ta_MountStatus.updateTime();
  }
  else if (page == P_FOCUSER && !ta_MountStatus.isPulseGuiding())
  {
    ta_MountStatus.updateFocuser();
  }
  else if (page == P_AXIS && !ta_MountStatus.isPulseGuiding())
  {
    ta_MountStatus.updateAxis();
  }
  u8g2_FirstPage(u8g2);

  do
  {
    u8g2_uint_t x = u8g2_GetDisplayWidth(u8g2);
    u8g2_uint_t xl = 0;
    int k = 0;
    if (buttonPad.isWifiOn())
    {
      buttonPad.isWifiRunning() ? display->drawXBMP(0, 0, icon_width, icon_height, wifi_bits) : display->drawXBMP(0, 0, icon_width, icon_height, wifi_not_connected_bits);
      xl = icon_width + 1;
    }
    if (ta_MountStatus.hasInfoMount())
    {
      TeenAstroMountStatus::ParkState curP = ta_MountStatus.getParkState();
      TeenAstroMountStatus::TrackState curT = ta_MountStatus.getTrackingState();
      TeenAstroMountStatus::SiderealMode currSM = ta_MountStatus.getSiderealMode();
      TeenAstroMountStatus::PierState curPi = ta_MountStatus.getPierState();
      if (ta_MountStatus.isGNSSValid())
      {
        display->drawXBMP(xl, 0, icon_width, icon_height, GNSS_bits);
      }
      if (curP == TeenAstroMountStatus::PRK_PARKED)
      {
        display->drawXBMP(x - icon_width, 0, icon_width, icon_height, parked_bits);
        x -= icon_width + 1;
      }
      else if (curP == TeenAstroMountStatus::PRK_PARKING)
      {
        display->drawXBMP(x - icon_width, 0, icon_width, icon_height, parking_bits);
        x -= icon_width + 1;
      }
      else if (ta_MountStatus.atHome())
      {
        display->drawXBMP(x - icon_width, 0, icon_width, icon_height, home_bits);
        x -= icon_width + 1;
      }
      else
      {
        if (curT == TeenAstroMountStatus::TRK_SLEWING)
        {
          display->drawXBMP(x - icon_width, 0, icon_width, icon_height, sleewing_bits);
          x -= icon_width + 1;
        }
        else if (curT == TeenAstroMountStatus::TRK_ON)
        {
          display->drawXBMP(x - icon_width, 0, icon_width, icon_height, tracking_bits);
          display->setBitmapMode(1);
          if (currSM == TeenAstroMountStatus::SID_STAR)
          {
            display->drawXBMP(x - icon_width, 0, icon_width, icon_height, tracking_star_bits);
          }
          else if (currSM == TeenAstroMountStatus::SID_MOON)
          {
            display->drawXBMP(x - icon_width, 0, icon_width, icon_height, tracking_moon_bits);
          }
          else if (currSM == TeenAstroMountStatus::SID_SUN)
          {
            display->drawXBMP(x - icon_width, 0, icon_width, icon_height, tracking_sun_bits);
          }
          display->setBitmapMode(0);

          x -= icon_width + 1;
        }
        else if (curT == TeenAstroMountStatus::TRK_OFF)
        {
          display->drawXBMP(x - icon_width, 0, icon_width, icon_height, no_tracking_bits);
          x -= icon_width + 1;
        }

        if (curP == TeenAstroMountStatus::PRK_FAILED)
        {
          display->drawXBMP(x - icon_width, 0, icon_width, icon_height, parkingFailed_bits);
          x -= icon_width + 1;
        }

        if (curPi == TeenAstroMountStatus::PIER_E)
        {
          display->drawXBMP(x - icon_width, 0, icon_width, icon_height, E_bits);
          x -= icon_width + 1;
        }
        else if (curPi == TeenAstroMountStatus::PIER_W)
        {
          display->drawXBMP(x - icon_width, 0, icon_width, icon_height, W_bits);
          x -= icon_width + 1;
        }       

        if (ta_MountStatus.isAligning() )
        {
          if (ta_MountStatus.getAlignMode() == TeenAstroMountStatus::ALIM_ONE)
            display->drawXBMP(x - icon_width, 0, icon_width, icon_height, align1_bits);
          else if (ta_MountStatus.getAlignMode() == TeenAstroMountStatus::ALIM_TWO)
            display->drawXBMP(x - icon_width, 0, icon_width, icon_height, align2_bits);
          else if (ta_MountStatus.getAlignMode() == TeenAstroMountStatus::ALIM_THREE)
            display->drawXBMP(x - icon_width, 0, icon_width, icon_height, align3_bits);
          x -= icon_width + 1;
        }
        if (ta_MountStatus.isSpiralRunning())
        {
          display->drawXBMP(x - icon_width, 0, icon_width, icon_height, Spiral_bits);
          x -= icon_width + 1;
        }
        if (ta_MountStatus.isPulseGuiding())
        {
          display->drawXBMP(x - icon_width, 0, icon_width, icon_height, guiding__bits);
          display->setBitmapMode(1);
          if (ta_MountStatus.isGuidingN())
          {
            display->drawXBMP(x - icon_width, 0, icon_width, icon_height, guiding_N_bits);
          }
          else if (ta_MountStatus.isGuidingS())
          {
            display->drawXBMP(x - icon_width, 0, icon_width, icon_height, guiding_S_bits);
          }
          if (ta_MountStatus.isGuidingE())
          {
            display->drawXBMP(x - icon_width, 0, icon_width, icon_height, guiding_E_bits);
          }
          else if (ta_MountStatus.isGuidingW())
          {
            display->drawXBMP(x - icon_width, 0, icon_width, icon_height, guiding_W_bits);
          }
          display->setBitmapMode(0);
          x -= icon_width + 1;
        }
      }
      switch (ta_MountStatus.getError())
      {
      case TeenAstroMountStatus::ERR_MOTOR_FAULT:
        display->drawXBMP(x - icon_width, 0, icon_width, icon_height, ErrMf_bits);
        x -= icon_width + 1;
        break;
      case  TeenAstroMountStatus::ERR_ALT:
        display->drawXBMP(x - icon_width, 0, icon_width, icon_height, ErrHo_bits);
        x -= icon_width + 1;
        break;
      case TeenAstroMountStatus::ERR_DEC:
        display->drawXBMP(x - icon_width, 0, icon_width, icon_height, ErrDe_bits);
        x -= icon_width + 1;
        break;
      case TeenAstroMountStatus::ERR_AZM:
        display->drawXBMP(x - icon_width, 0, icon_width, icon_height, ErrAz_bits);
        x -= icon_width + 1;
        break;
      case TeenAstroMountStatus::ERR_UNDER_POLE:
        display->drawXBMP(x - icon_width, 0, icon_width, icon_height, ErrUp_bits);
        x -= icon_width + 1;
        break;
      case TeenAstroMountStatus::ERR_MERIDIAN:
        display->drawXBMP(x - icon_width, 0, icon_width, icon_height, ErrMe_bits);
        x -= icon_width + 1;
        break;
      default:
        break;
      }
    }
    if (focuserlocked || telescoplocked)
    {
      display->drawXBMP(x - icon_width, 0, icon_width, icon_height, Lock___bits);
      display->setBitmapMode(1);
      if (focuserlocked)
        display->drawXBMP(x - icon_width, 0, icon_width, icon_height, Lock_F_bits);
      if (telescoplocked)
        display->drawXBMP(x - icon_width, 0, icon_width, icon_height, Lock_T_bits);
      display->setBitmapMode(0);
      x -= icon_width + 1;
    }
    if (page == P_RADEC)
    {
      if (ta_MountStatus.hasInfoRa() && ta_MountStatus.hasInfoDec())
      {
        u8g2_uint_t y = 36;
        x = u8g2_GetDisplayWidth(u8g2);
        display->drawRA(x, y, ta_MountStatus.getRa());
        u8g2_DrawUTF8(u8g2, 0, y, "RA");
        y += line_height + 4;
        u8g2_DrawUTF8(u8g2, 0, y, "Dec");
        display->drawDec(x, y, ta_MountStatus.getDec());
      }
    }
    else if (page == P_ALTAZ)
    {
      if (ta_MountStatus.hasInfoAz() && ta_MountStatus.hasInfoAlt())
      {
        u8g2_uint_t y = 36;
        u8g2_uint_t startpos = u8g2_GetUTF8Width(u8g2, "123456");
        x = startpos;
        x = u8g2_GetDisplayWidth(u8g2);
        u8g2_DrawUTF8(u8g2, 0, y, "Az.");
        display->drawAz(x, y, ta_MountStatus.getAz());
        y += line_height + 4;
        x = startpos;
        x = u8g2_GetDisplayWidth(u8g2);
        display->drawDec(x, y, ta_MountStatus.getAlt());
        u8g2_DrawUTF8(u8g2, 0, y, "Alt.");
      }
    }
    else if (page == P_TIME)
    {
      if (ta_MountStatus.hasInfoUTC() && ta_MountStatus.hasInfoSidereal())
      {
        u8g2_uint_t y = 36;
        x = u8g2_GetDisplayWidth(u8g2);
        u8g2_DrawUTF8(u8g2, 0, y, "UTC");
        display->drawRA(x, y, ta_MountStatus.getUTC());
        y += line_height + 4;
        u8g2_DrawUTF8(u8g2, 0, y, "Sidereal");
        display->drawRA(x, y, ta_MountStatus.getSidereal());
      }
    }
    else if (page == P_FOCUSER)
    {
      u8g2_uint_t y = 36;
      if (ta_MountStatus.hasInfoFocuser())
      {
        display->drawFoc(y, line_height, ta_MountStatus.getFocuser());
      }
      else
      {
        u8g2_DrawUTF8(u8g2, 0, y, T_FOCUSER);
        y += line_height + 4;
        u8g2_DrawUTF8(u8g2, 0, y, T_NOT_CONNECTED);
      }
    }
    else if (page == P_AXIS)
    {
      u8g2_uint_t y = 36;
      if (ta_MountStatus.hasInfoAxis1())
      {
        u8g2_DrawUTF8(u8g2, 0, y, ta_MountStatus.getAxis1());
        y += line_height + 4;
        u8g2_DrawUTF8(u8g2, 0, y, ta_MountStatus.getAxis2());
      }
    }
    else if (page == P_ALIGN)
    {
      u8g2_uint_t y = 36;
      static char text1[29] = T_SLEWINGTO " " T_STAR;
      static char text2[28] = T_RECENTER " " T_STAR;
      if (ta_MountStatus.isAlignSlew())
        u8g2_DrawUTF8(u8g2, 0, y,text1);
      else if (ta_MountStatus.isAlignRecenter())
        u8g2_DrawUTF8(u8g2, 0, y, text2);

      y += line_height + 4;
      if (cat_mgr.objectNameStr()[0] != 0)
      {
        u8g2_DrawUTF8(u8g2, 0, y, cat_mgr.objectNameStr());
      }
      else
      {
        u8g2_SetFont(u8g2, u8g2_font_unifont_t_greek);
        u8g2_DrawGlyph(u8g2, 0, y, 945 + cat_mgr.bayerFlam());
        const uint8_t* myfont = u8g2->font; u8g2_SetFont(u8g2, myfont);
        u8g2_DrawUTF8(u8g2, 16, y, cat_mgr.constellationStr());
      }

    }

  } while (u8g2_NextPage(u8g2));
  lastpageupdate = millis();
}

void SmartHandController::drawIntro()
{
  display->firstPage();
  do {
    display->drawXBMP(0, 0, teenastro_width, teenastro_height, teenastro_bits);
  } while (display->nextPage());
  delay(1500);
}

void SmartHandController::drawLoad()
{
  display->firstPage();
  uint8_t x = 0;
  do {
    display->setFont(u8g2_font_helvR14_tr);
    x = (display->getDisplayWidth() - display->getUTF8Width("SHC " T_VERSION)) / 2;
    display->drawStr(x, display->getDisplayHeight() / 2. - 6, "SHC" T_VERSION);
    x = (display->getDisplayWidth() - display->getUTF8Width(_version)) / 2;
    display->drawStr(x, display->getDisplayHeight() / 2. + 22, _version);
  } while (display->nextPage());
  delay(1500);
  display->setFont(u8g2_font_helvR12_te);
}

void SmartHandController::resetSHC()
{
  if (display->UserInterfaceMessage(&buttonPad, T_RESET, T_TO, T_FACTORY, T_NO "\n" T_YES) == 2)
  {
    EEPROM_writeInt(0, 0);
    EEPROM.commit();
    powerCycleRequired = true;
    exitMenu = true;
    return;
  }
}

bool SmartHandController::menuSetReverse(const uint8_t &axis)
{
  bool reverse;
  if (!DisplayMessageLX200(readReverseLX200(axis, reverse)))
    return false;
  char text[20];
  char * string_list_micro = T_DIRECT "\n" T_REVERSE;
  sprintf(text, T_ROTATION " M%u", axis);
  uint8_t choice = display->UserInterfaceSelectionList(&buttonPad, text, (uint8_t)reverse + 1, string_list_micro);
  if (choice)
  {
    reverse = (bool)(choice - 1);
    return DisplayMessageLX200(writeReverseLX200(axis, reverse), false);
  }
  return true;
}
bool SmartHandController::menuSetBacklash(const uint8_t &axis)
{
  float backlash;
  if (!DisplayMessageLX200(readBacklashLX200(axis, backlash)))
    return false;
  char text[20];
  sprintf(text, T_BACKLASH " M%u", axis);
  if (display->UserInterfaceInputValueFloat(&buttonPad, text, "", &backlash, 0, 1000, 4, 0, " " T_INSECONDS))
  {
    return DisplayMessageLX200(writeBacklashLX200(axis, backlash), false);
  }
  return true;
}
bool SmartHandController::menuSetTotGear(const uint8_t &axis)
{
  float totGear;
  if (!DisplayMessageLX200(readTotGearLX200(axis, totGear)))
    return false;
  char text[20];
  sprintf(text, T_GEAR " M%u", axis);
  if (display->UserInterfaceInputValueFloat(&buttonPad, text, T_RATIO, &totGear, 1, 60000, 5, 0, ""))
  {
    return DisplayMessageLX200(writeTotGearLX200(axis, totGear), false);
  }
  return true;
}
bool SmartHandController::menuSetStepPerRot(const uint8_t &axis)
{
  float stepPerRot;
  if (!DisplayMessageLX200(readStepPerRotLX200(axis, stepPerRot)))
    return false;
  char text[20];
  sprintf(text, T_STEPPER " M%u", axis);
  if (display->UserInterfaceInputValueFloat(&buttonPad, text, "", &stepPerRot, 1, 400, 3, 0, " " T_STEPS))
  {
    return DisplayMessageLX200(writeStepPerRotLX200(axis, stepPerRot), false);
  }
  return true;
}
bool SmartHandController::menuSetMicro(const uint8_t &axis)
{
  uint8_t microStep;
  if (!DisplayMessageLX200(readMicroLX200(axis, microStep)))
    return false;
  char text[20];
  char * string_list_micro = "16 (~256)\n32\n64\n128\n256";
  sprintf(text, T_STEPPER " M%u", axis);
  uint8_t choice = microStep - 4 + 1;
  choice = display->UserInterfaceSelectionList(&buttonPad, text, choice, string_list_micro);
  if (choice)
  {
    microStep = choice - 1 + 4;
    return DisplayMessageLX200(writeMicroLX200(axis, microStep), false);
  }
  return true;
}
bool SmartHandController::menuSetLowCurrent(const uint8_t &axis)
{
  uint8_t lowCurr;
  if (!DisplayMessageLX200(readLowCurrLX200(axis, lowCurr)))
  {
    return false;
  }
  char text[20];
  sprintf(text, T_LOWCURR " M%u", axis);
  if (display->UserInterfaceInputValueInteger(&buttonPad, text, "", &lowCurr, 10, 200, 3, "0 mA " T_PEAK))
  {
    return DisplayMessageLX200(writeLowCurrLX200(axis, lowCurr), false);
  }
  return true;
}

bool SmartHandController::menuSetHighCurrent(const uint8_t &axis)
{
  uint8_t highCurr;
  if (!DisplayMessageLX200(readHighCurrLX200(axis, highCurr)))
  {
    return false;
  }
  char text[20];
  sprintf(text, T_HIGHCURR " M%u", axis);
  if (display->UserInterfaceInputValueInteger(&buttonPad, text, "", &highCurr, 10, 200, 3, "0 mA " T_PEAK))
  {
    return DisplayMessageLX200(writeHighCurrLX200(axis, highCurr), false);
  }
  return true;
}

void SmartHandController::DisplayMountSettings()
{
  DisplayMotorSettings(1);
  DisplayMotorSettings(2);
  DisplayAccMaxRateSettings();
}

void SmartHandController::DisplayAccMaxRateSettings()
{
  char out[20];
  char line1[32] = T_SLEWSETTING;
  char line3[32] = "";
  char line4[32] = "";
  if (DisplayMessageLX200(GetLX200(":GXE2#", out, sizeof(out))))
  {
    float acc = atof(&out[0]);
    sprintf(line3, T_ACCELERATION ": %.1f", acc);
  }
  if (DisplayMessageLX200(GetLX200(":GX92#", out, sizeof(out))))
  {
    int maxrate = (float)strtol(&out[0], NULL, 10);
    sprintf(line4, T_MaxSlew ": %dx", maxrate);
  }
  DisplayLongMessage(line1, NULL, line3, line4, -1);
}

void SmartHandController::DisplayMotorSettings(const uint8_t &axis)
{
  char line1[32] = "";
  char line2[32] = "";
  char line3[32] = "";
  char line4[32] = "";
  bool reverse;
  float backlash, totGear, stepPerRot;
  uint8_t microStep, lowCurr, highCurr;
  sprintf(line1, T_MOTORSETTINGS, axis);
  if (DisplayMessageLX200(readReverseLX200(axis, reverse)))
  {
    reverse ? sprintf(line3, T_REVERSEDROTATION ) : sprintf(line3, T_DIRECTROTATION);
  }
  if (DisplayMessageLX200(readTotGearLX200(axis, totGear)))
  {
    sprintf(line4, T_RATIO": %u", (unsigned int)totGear);
  }

  DisplayLongMessage(line1, NULL, line3, line4, -1);

  line2[0] = 0;
  line3[0] = 0;
  line4[0] = 0;

  if (DisplayMessageLX200(readStepPerRotLX200(axis, stepPerRot)))
  {
    sprintf(line2, "%u " T_STEPSPERROT, (unsigned int)stepPerRot);
  }
  if (DisplayMessageLX200(readMicroLX200(axis, microStep)))
  {
    sprintf(line3, T_MICROSTEP ": %u", (unsigned int)pow(2, microStep));
  }
  if (DisplayMessageLX200(readBacklashLX200(axis, backlash)))
  {
    sprintf(line4, T_BACKLASH": %u sec.", (unsigned int)backlash);
  }
  DisplayLongMessage(line1, line2, line3, line4, -1);
  line2[0] = 0;
  line3[0] = 0;
  line4[0] = 0;
  if (DisplayMessageLX200(readLowCurrLX200(axis, lowCurr)))
  {
    sprintf(line3, T_LOWCURR " %u0 mA", (unsigned int)lowCurr);
  }
  if (DisplayMessageLX200(readHighCurrLX200(axis, highCurr)))
  {
    sprintf(line4, T_HIGHCURR " %u0 mA", (unsigned int)highCurr);
  }

  DisplayLongMessage(line1, NULL, line3, line4, -1);
}

void SmartHandController::menuTelAction()
{
  buttonPad.setMenuMode();
  current_selection_L0 = 1;
  while (!exitMenu)
  {
    ta_MountStatus.updateMount();
    TeenAstroMountStatus::ParkState currentstate = ta_MountStatus.getParkState();

    if (currentstate == TeenAstroMountStatus::PRK_PARKED)
    {
      const char *string_list_main_ParkedL0 = T_UNPARK;
      current_selection_L0 = display->UserInterfaceSelectionList(&buttonPad, T_TELESCOPEACTION, current_selection_L0, string_list_main_ParkedL0);
      switch (current_selection_L0)
      {
      case 0:
        exitMenu = true;
        break;
      case 1:
        SetLX200(":hR#");
        exitMenu = true;
        break;
      default:
        break;
      }
    }
    else if (currentstate == TeenAstroMountStatus::PRK_UNPARKED)
    {
      const char *string_list_main_UnParkedL0 = telescoplocked ? T_UNLOCK : T_GOTO "\n" T_SYNC "\n" T_ALIGN "\n" T_TRACKING "\n" T_SIDEOFPIER "\n" T_SAVE " RADEC\n" T_LOCK "\n" T_SPIRAL;
      current_selection_L0 = display->UserInterfaceSelectionList(&buttonPad, T_TELESCOPEACTION, current_selection_L0, string_list_main_UnParkedL0);
      MENU_RESULT answer = MR_CANCEL;
      if (telescoplocked)
      {
        switch (current_selection_L0)
        {
        case 0:
          exitMenu = true;
          break;
        case 1:
          telescoplocked = false;
          exitMenu = true;
          break;
        default:
          break;
        }
      }
      else
      {
        switch (current_selection_L0)
        {
        case 0:
          exitMenu = true;
          break;
        case 1:
          answer = menuSyncGoto(false);
          answer == MR_OK || answer == MR_QUIT ? exitMenu = true : exitMenu = false;
          break;
        case 2:
          answer = menuSyncGoto(true);
          answer == MR_OK || answer == MR_QUIT ? exitMenu = true : exitMenu = false;
          break;
        case 3:
          answer = menuAlignment();
          answer == MR_OK || answer == MR_QUIT ? exitMenu = true : exitMenu = false;
          break;
        case 4:
          menuTrack();
          break;
        case 5:
          menuPier();
          break;
        case 6:
          if (SetLX200(":SU#") == LX200VALUESET)
          {
            DisplayMessage("RA DEC", T_SAVED, 500);
          }
          else
          {
            DisplayMessage(T_LX200COMMAND, T_FAILED, 1000);
          }
          break;
        case 7:
          telescoplocked = true;
          exitMenu = true;
          break;
        case 8:
          if (SetLX200(":M@#") == LX200VALUESET)
          {
            DisplayMessage(T_SPIRAL, T_STARTED, 500);
          }
          else
          {
            DisplayMessage(T_LX200COMMAND, T_FAILED, 1000);
          }
          exitMenu = true;
          break;
        default:
          break;
        }
      }
    }
  }
  if (!sleepDisplay)
  {
    buttonPad.setControlerMode();
  }
}

void SmartHandController::menuSpeedRate()
{
  buttonPad.setMenuMode();
  char * string_list_Speed = "Guide\n0.5x\n1.0x\n2.0x\n4.0x\n16.0x\n32.0x\n64.0x\n0.5 Max\nMax";
  static unsigned char current_selection_speed = 4;
  ta_MountStatus.updateMount();
  if (!ta_MountStatus.getGuidingRate(current_selection_speed))
    return;
  uint8_t selected_speed = display->UserInterfaceSelectionList(&buttonPad, T_SETSPEED, current_selection_speed + 1 , string_list_Speed);
  if (selected_speed > 0)
  {
    char cmd[5] = ":Rn#";
    cmd[2] = '0' + selected_speed - 1;
    SetLX200(cmd);
    current_selection_speed = selected_speed;
  }
  buttonPad.setControlerMode();
}

void SmartHandController::menuTrack()
{
  ta_MountStatus.updateMount();
  TeenAstroMountStatus::TrackState currentstate = ta_MountStatus.getTrackingState();
  if (currentstate == TeenAstroMountStatus::TRK_ON)
  {
    const char *string_list_tracking = T_STOPTRACKING "\n" T_SIDEREAL "\n" T_LUNAR "\n" T_SOLAR;
    current_selection_L1 = display->UserInterfaceSelectionList(&buttonPad, T_TRACKINGSTATE, 0, string_list_tracking);
    switch (current_selection_L1)
    {
    case 1:
      char out[20];
      memset(out, 0, sizeof(out));
      if (SetLX200(":Td#") == LX200VALUESET)
      {
        DisplayMessage(T_TRACKING, T_OFF, 500);
        exitMenu = true;
      }
      else
      {
        DisplayMessage(T_LX200COMMAND, T_FAILED, 1000);
      }
      break;
    case 2:
      exitMenu = DisplayMessageLX200(SetLX200(":TQ#"));
      break;
    case 3:
      exitMenu = DisplayMessageLX200(SetLX200(":TL#"));
      break;
    case 4:
      exitMenu = DisplayMessageLX200(SetLX200(":TS#"));
      break;
    default:
      break;
    }
  }
  else if (currentstate == TeenAstroMountStatus::TRK_OFF)
  {
    const char *string_list_tracking = T_STARTTRACKING;
    current_selection_L1 = display->UserInterfaceSelectionList(&buttonPad, T_TRACKINGSTATE, 0, string_list_tracking);
    switch (current_selection_L1)
    {
    case 1:
      if (SetLX200(":Te#") == LX200VALUESET)
      {
        DisplayMessage(T_TRACKING, T_ON, 500);
        exitMenu = true;
      }
      else
      {
        DisplayMessage(T_LX200COMMAND, T_FAILED, 1000);
      }
      break;
    default:
      break;
    }
  }
  else
  {
    DisplayMessage(T_CURRENTLYTRACKING, T_CANNOTBECHANGED, 1000);
  }
}

SmartHandController::MENU_RESULT SmartHandController::menuAlignment()
{
  bool alignInProgress=ta_MountStatus.isAligning();
  static int current_selection = 1;
  while (true) {
    const char* string_list = alignInProgress ? T_CANCEL : "2 " T_STAR "\n3 " T_STAR "\n" T_SAVE "\n" T_Clear;
    int selection = display->UserInterfaceSelectionList(&buttonPad, T_ALIGNMENT, current_selection, string_list);
    if (selection == 0) return MR_CANCEL;
    current_selection = selection;
    switch (current_selection) {
    case 1:
      if (alignInProgress)
      {
        ta_MountStatus.stopAlign();
        DisplayMessage(T_ALIGNMENT, T_CANCELED, -1);
        return MR_QUIT;
      }
      else 
      {
        DisplayLongMessage("!" T_WARNING "!", T_THEMOUNTMUSTBEATHOME1, T_THEMOUNTMUSTBEATHOME2, T_THEMOUNTMUSTBEATHOME3, -1);
        if (display->UserInterfaceMessage(&buttonPad, T_READYFOR, "2 " T_STAR, T_ALIGNMENT "?", T_NO "\n" T_YES) == 2)
        {
          if (SetLX200(":A0#") == LX200VALUESET)
          {
          ta_MountStatus.startAlign(TeenAstroMountStatus::AlignMode::ALIM_TWO);
          return MR_QUIT;
          }
          else
          {
            DisplayMessage(T_INITIALISATION, T_FAILED, -1);
          }
        }
      }
      break;
    case 2:
      DisplayLongMessage("!" T_WARNING "!", T_THEMOUNTMUSTBEATHOME1, T_THEMOUNTMUSTBEATHOME2, T_THEMOUNTMUSTBEATHOME3, -1);
      if (display->UserInterfaceMessage(&buttonPad, T_READYFOR, "3 " T_STAR, T_ALIGNMENT "?", T_NO "\n" T_YES) == 2)
      {
        if (SetLX200(":A0#") == LX200VALUESET)
        {
          ta_MountStatus.startAlign(TeenAstroMountStatus::AlignMode::ALIM_THREE);
          return MR_QUIT;
        }
        else
        {
          DisplayMessage(T_INITIALISATION, T_FAILED, -1);
        }
      }
      break;
    case 4:
      if (display->UserInterfaceMessage(&buttonPad, T_Clear, T_STAR, T_ALIGNMENT "?", T_NO "\n" T_YES) == 2)
      {
        if (SetLX200(":AC#") == LX200VALUESET)
        {
          DisplayMessage(T_MOUNTSYNCED, T_ATHOME, -1);
          return MR_QUIT;
        }
        else
        {
          DisplayMessage(T_Clear, T_FAILED, -1);
        }
      }
      break;
    }
  }
}

bool SmartHandController::SelectStarAlign()
{
  buttonPad.setMenuMode();
  double lat, LT0;
  while (!ta_MountStatus.getLat(lat))
  {
  }
  while (!ta_MountStatus.getLstT0(LT0))
  {
  }
  cat_mgr.setLat(lat);
  cat_mgr.setLstT0(LT0);
  bool ok = menuCatalogAlign() != SmartHandController::MENU_RESULT::MR_CANCEL;
  buttonPad.setControlerMode();
  return ok;
}

void SmartHandController::menuDateAndTime()
{
  const char *string_list_SettingsL2 = T_CLOCK "\n" T_TIMEZONE "\n" T_DATE "\n" T_GNSSTIME;
  while (!exitMenu)
  {
    current_selection_L2 = display->UserInterfaceSelectionList(&buttonPad, T_TIMESETTINGS, current_selection_L2, string_list_SettingsL2);
    switch (current_selection_L2)
    {
    case 0:
      return;
    case 1:
      menuLocalTime();
      break;
    case 2:
      menuLocalTimeZone();
      break;
    case 3:
      menuLocalDate();
      break;
    case 5:
      if (ta_MountStatus.isGNSSValid())
        DisplayMessageLX200(SetLX200(":gt#"), false);
      else
        DisplayMessage(T_NOGNSS, T_SIGNAL, -1);
      break;
      break;
    }
  }
}

void SmartHandController::menuSHCSettings()
{

  const char *string_list_SettingsL3 = T_DISPLAY "\n" T_BUTTONSPEED "\n" T_RESET;
  while (!exitMenu)
  {
    current_selection_SHC = display->UserInterfaceSelectionList(&buttonPad, T_SHCSETTINGS, current_selection_SHC, string_list_SettingsL3);
    switch (current_selection_SHC)
    {
    case 0:
      return;
    case 1:
      menuDisplay();
      break;
    case 2:
      menuButtonSpeed();
      break;
    case 3:
      resetSHC();
      break;
    }
  }
}

void SmartHandController::menuTimeAndSite()
{
  current_timelocation = 1;
  const char *string_list_SettingsL2 = T_TIME "\n" T_SITE "\n" T_SYNCWITHGNSS;
  while (!exitMenu)
  {
    current_timelocation = display->UserInterfaceSelectionList(&buttonPad, T_TIME " & " T_SITE, current_timelocation, string_list_SettingsL2);
    switch (current_timelocation)
    {
    case 0:
      return;
    case 1:
      menuDateAndTime();
      break;
    case 2:
      menuSite();
      break;
    case 3:
      if (ta_MountStatus.isGNSSValid())
        DisplayMessageLX200(SetLX200(":gs#"), false);
      else
        DisplayMessage(T_NOGNSS, T_SIGNAL, -1);
      break;
    }
  }
}

void SmartHandController::menuTelSettings()
{
  buttonPad.setMenuMode();
  current_selection_L1 = 1;
  while (!exitMenu)
  {
    const char *string_list_SettingsL1 = T_HANDCONTROLLER "\n"/*"Alignment\n"*/T_TIME " & " T_SITE "\n" T_SETPARK "\n" T_MOUNT "\n" T_LIMITS "\n" T_MAINUNITINFO "\nWifi";
    current_selection_L1 = display->UserInterfaceSelectionList(&buttonPad, T_TELESCOPESETTINGS, current_selection_L1, string_list_SettingsL1);
    switch (current_selection_L1)
    {
    case 0:
      exitMenu = true;
    case 1:
      menuSHCSettings();
      break;
    //case 2:
    //  menuAlignment();
    //  break;
    case 2:
      menuTimeAndSite();
      break;
    case 3:
      DisplayMessageLX200(SetLX200(":hQ#"), false);
      break;
    case 4:
      menuMount();
      break;
    case 5:
      menuLimits();
      break;
    case 6:
      menuMainUnitInfo();
      break;
    case 7:
      menuWifi();
      break;
    default:
      break;
    }
  }
  buttonPad.setControlerMode();
}

void SmartHandController::menuMount()
{
  current_selection_L2 = 1;
  while (!exitMenu)
  {
    const char *string_list_Mount = T_SHOWSETTINGS "\n" T_MOUNTTYPE "\n" T_MOTOR " 1\n" T_MOTOR " 2\n" T_GUIDERATE "\n" T_MAXRATE "\n" T_ACCELERATION;
    current_selection_L2 = display->UserInterfaceSelectionList(&buttonPad, T_MOUNT, current_selection_L2, string_list_Mount);
    switch (current_selection_L2)
    {
    case 0:
      return;
    case 1:
      DisplayMountSettings();
      break;
    case 2:
      menuMountType();
      break;
    case 3:
      menuMotor(1);
      break;
    case 4:
      menuMotor(2);
      break;
    case 5:
      menuGuideRate();
      break;
    case 6:
      menuMaxRate();
      break;
    case 7:
      menuAcceleration();
      break;
    default:
      break;
    }
  }
}

void SmartHandController::menuMountType()
{
  current_selection_L3 = ta_MountStatus.getMount();
  if (current_selection_L3 == 0)
  {
    DisplayLongMessage("!" T_WARNING "!", NULL, T_MOUNTTYPE, T_NOTDEFINED "!", -1);
    current_selection_L3 = 1;
  }
  const char *string_list_Mount = T_GERMANEQUATORIAL "\n" T_EQUATORIALFORK "\n" T_ALTAAZIMUTAL "\n" T_ALTAAZIMUTALFORK;
  current_selection_L3 = display->UserInterfaceSelectionList(&buttonPad, T_MOUNTTYPE, current_selection_L3, string_list_Mount);
  if (current_selection_L3)
  {
    char out[10];
    sprintf(out, ":S!%u#", current_selection_L3);
    DisplayMessageLX200(SetLX200(out), false);
    delay(1000);
    Serial.end();
    exitMenu = true;
    powerCycleRequired = true;
    Serial.println(out);
  }
}


void SmartHandController::menuMotor(const uint8_t axis)
{
  current_selection_L3 = 1;

  while (current_selection_L3 != 0)
  {
    const char *string_list_Motor = T_SHOWSETTINGS "\n" T_ROTATION "\n" T_GEAR "\n" T_STEPSPERROT "\n"
                                     T_MICROSTEP "\n" T_BACKLASH "\n" T_LOWCURR "\n" T_HIGHCURR;
    current_selection_L3 = display->UserInterfaceSelectionList(&buttonPad, axis == 1 ? T_MOTOR " 1" : T_MOTOR " 2", current_selection_L3, string_list_Motor);
    switch (current_selection_L3)
    {
    case 1:
      DisplayMotorSettings(axis);
      break;
    case 2:
      menuSetReverse(axis);
      break;
    case 3:
      menuSetTotGear(axis);
      break;
    case 4:
      menuSetStepPerRot(axis);
      break;
    case 5:
      menuSetMicro(axis);
      break;
    case 6:
      menuSetBacklash(axis);
      break;
    case 7:
      menuSetLowCurrent(axis);
      break;
    case 8:
      menuSetHighCurrent(axis);
      break;
    default:
      break;
    }
  }

}

void SmartHandController::menuAcceleration()
{
  char outAcc[20];
  char outStepsPerDegree[20];
  char cmd[20];
  if (DisplayMessageLX200(GetLX200(":GXE2#", outAcc, sizeof(outAcc))))
  {
    float acc =atof(&outAcc[0]);
    if (display->UserInterfaceInputValueFloat(&buttonPad, T_ACCELERATION, "", &acc, 0.1, 25, 4, 1, " deg."))
    {
      sprintf(cmd, ":SXE2:%04d#", (int)(acc*10.));
      DisplayMessageLX200(SetLX200(cmd));
    }
  }
}

void SmartHandController::menuMaxRate()
{
  char outRate[20];
  char outStepsPerDegree[20];
  char cmd[20];
  if (DisplayMessageLX200(GetLX200(":GX92#", outRate, sizeof(outRate))))
  {
    float maxrate = (float)strtol(&outRate[0], NULL, 10);
    if (display->UserInterfaceInputValueFloat(&buttonPad, T_MAXRATE, "", &maxrate, 32, 4000, 4, 0, ""))
    {
      sprintf(cmd, ":SX92:%04d#", (int)maxrate);
      DisplayMessageLX200(SetLX200(cmd));
    }
  }
}

void SmartHandController::menuGuideRate()
{
  char outRate[20];
  char cmd[20];
  if (DisplayMessageLX200(GetLX200(":GX90#", outRate, sizeof(outRate))))
  {
    float guiderate = atof(&outRate[0]);
    if (display->UserInterfaceInputValueFloat(&buttonPad, T_GUIDERATE, "", &guiderate, 0.1f, 1.f, 4u, 2u, "x"))
    {
      sprintf(cmd, ":SX90:%03d#", (int)(guiderate * 100));
      DisplayMessageLX200(SetLX200(cmd));
    }
  }
}


void SmartHandController::menuSite()
{
  current_selection_L2 = 1;
  while (current_selection_L2 != 0)
  {
    const char *string_list_SiteL2 = T_LATITUDE "\n" T_LONGITUDE "\n" T_SITEELEVATION "\n" T_SELECTSITE;
    current_selection_L2 = display->UserInterfaceSelectionList(&buttonPad, "Menu Site", current_selection_L2, string_list_SiteL2);
    switch (current_selection_L2)
    {
    case 1:
      menuLatitude();
      break;
    case 2:
      menuLongitude();
      break;
    case 3:
      menuElevation();
      break;
    case 4:
      menuSites();
      break;
    default:
      break;
    }
  }
}

void SmartHandController::menuSites()
{
  int val;
  char m[15];
  char n[15];
  char o[15];
  char p[15];
  char txt[70]="";
  GetLX200(":GM#", m, sizeof(m));
  GetLX200(":GN#", n, sizeof(n));
  GetLX200(":GO#", o, sizeof(o));
  GetLX200(":GP#", p, sizeof(p));
  strcat(txt, m);
  strcat(txt, "\n");
  strcat(txt, n);
  strcat(txt, "\n");
  strcat(txt, o);
  strcat(txt, "\n");
  strcat(txt, p);

  if (DisplayMessageLX200(GetSiteLX200(val)))
  {
    current_selection_L3 = val;
    current_selection_L3 = display->UserInterfaceSelectionList(&buttonPad, "Menu Sites", current_selection_L3, txt);
    if (current_selection_L3 != 0)
    {
      val = current_selection_L3 - 1;
      SetSiteLX200(val);
    }
  }
}

void SmartHandController::menuLocalTime()
{
  long value;
  if (DisplayMessageLX200(GetLocalTimeLX200(value)))
  {
    if (display->UserInterfaceInputValueLocalTime(&buttonPad, &value))
    {
      DisplayMessageLX200(SetLocalTimeLX200(value), false);
    }
  }
}

void SmartHandController::menuLocalTimeZone()
{
  float val = 0;
  if (DisplayMessageLX200(GetLX200Float(":GG#", &val)))
  {
    val *=-1;
    if (display->UserInterfaceInputValueFloatIncr(&buttonPad, T_TIMEZONE, "UTC ", &val, -12, 12, 3, 1, 0.5, " hour"))
    {
      char cmd[15];
      sprintf(cmd, ":SG%+05.1f#", -val);
      if (DisplayMessageLX200(SetLX200(cmd)))
        exitMenu = true;
    }
  }
}

void SmartHandController::menuFocuserAction()
{
  if (!ta_MountStatus.hasFocuser())
  {
    DisplayMessage(T_FOCUSER, T_NOT_CONNECTED "!",500);
    return;
  }
  static  uint8_t current_selection = 1;
  uint8_t choice;
  buttonPad.setMenuMode();
  int pos[10];
  int idx = 0;
  int idxs[10] = { -1,-1,-1,-1,-1,-1,-1,-1,-1,-1 };
  char temp[20] = { 0 };
  char txt[150] = { 0 };
  char out[20];
  
  for (int k = 0; k < 10; k++)
  {
    sprintf(temp, ":Fx%d#", k);
    GetLX200(temp, out, 20);
    if (out[0] == 'P')
    {
      strcat(txt, &out[7]);
      strcat(txt, "\n");
      idxs[idx] = k;
      idx++;
    }
    else
      continue;
  }
  while (!exitMenu)
  {
    char menustxt[200] = {};
    if (focuserlocked)
    {
      strcat(menustxt, T_UNLOCK);
    }
    else
    {
      strcat(menustxt, txt);
      strcat(menustxt, T_GOTO "\n" T_SYNC "\n" T_PARK "\n" T_LOCK);
    }

    const char *string_list_Focuser = &menustxt[0];
    choice = display->UserInterfaceSelectionList(&buttonPad, T_FOCUSERACTION, current_selection, string_list_Focuser);
    if (choice!=0)
    {
      current_selection = choice;
    }
    if (focuserlocked)
    {
      switch (choice)
      {
      case 0:
        exitMenu = true;
        break;
      case 1:
        focuserlocked = false;
        exitMenu = true;
        break;
      default:
        break;
      }
    }
    else
    {
      if (choice == 0)
      {
        exitMenu = true;
        break;
      }
      else if (choice - 1 < idx)
      {
        char cmd[15];
        sprintf(cmd, ":Fg%d#", idxs[choice - 1]);
        DisplayMessage(T_GOTO, T_POSITION, 1000);
        SetLX200(cmd);
        exitMenu = true;
      }
      else
      {
        switch (choice - idx)
        {
        case 1:
        {
          if (display->UserInterfaceInputValueFloat(&buttonPad, T_GOTOPOSITION, "", &FocuserPos, 0, 65535, 5, 0, ""))
          {
            char cmd[15];
            sprintf(cmd, ":FG %05d#", (int)(FocuserPos));
            DisplayMessage(T_GOTO, T_POSITION, 1000);
            SetLX200(cmd);
            exitMenu = true;
          }
          break;
        }
        case 2:
        {
          if (display->UserInterfaceInputValueFloat(&buttonPad, T_SYNCPOSITION, "", &FocuserPos, 0, 65535, 5, 0, ""))
          {
            char cmd[15];
            sprintf(cmd, ":FS %05d#", (int)(FocuserPos));
            DisplayMessage(T_SYNCEDAT, T_POSITION, 1000);
            SetLX200(cmd);
            exitMenu = true;
          }
          break;
        }
        case 3:
        {
          SetLX200(":FP#");
          exitMenu = true;
          break;
        }
        case 4:
          focuserlocked = true;
          exitMenu = true;
          break;
        default:
          break;
        }
      }
    }
  }
  buttonPad.setControlerMode();
}

void SmartHandController::menuFocuserConfig()
{
  char cmd[50];
  const char *string_list_Focuser = T_DISPLAYSETTINGS "\n" T_PARKPOSITION "\n" T_MAXPOSITION "\n" T_MANUALSPEED "\n" T_GOTOSPEED "\n" T_ACCFORMAN "\n" T_ACCFORGOTO;
  unsigned int sP, maxP, minS, maxS, cmdAcc, manAcc, manDec;
  float value;
  while (!exitMenu)
  {
    if (DisplayMessageLX200(readFocuserConfig(sP, maxP, minS, maxS, cmdAcc, manAcc, manDec)))
    {
      current_selection_FocuserConfig = display->UserInterfaceSelectionList(&buttonPad, T_FOCUSERSETTINGS, current_selection_FocuserConfig, string_list_Focuser);
      bool ValueSetRequested = false;
      switch (current_selection_FocuserConfig)
      {
      case 0:
        return;
        break;
      case 1:
      {
        char line1[32] = "";
        char line2[32] = "";
        char line3[32] = "";
        char line4[32] = "";
        sprintf(line1, T_FOCUSERSETTINGS);
        sprintf(line3, T_STARTPOSITION2 ": %05u", sP);
        sprintf(line4, T_MAXPOSITION2 ": %05u", maxP);
        DisplayLongMessage(line1, line2, line3, line4, -1);
        line2[0] = 0;
        sprintf(line3, T_MANUALSPEED2 ": %03u", minS);
        sprintf(line4, T_GOTOSPEED2 ": %03u", maxS);
        DisplayLongMessage(line1, line2, line3, line4, -1);
        sprintf(line2, T_ACCFORGOTO2 ": %03u", cmdAcc);
        sprintf(line3, T_ACCFORMAN2 ": %03u", manAcc);
        sprintf(line4, T_DECFORMAN2 ": %03u", manDec);
        DisplayLongMessage(line1, line2, line3, line4, -1);
        break;
      }
      case 2:
      {
        value = sP;
        ValueSetRequested = display->UserInterfaceInputValueFloat(&buttonPad, T_PARKPOSITION, "", &value, 0, 65535, 5, 0, "");
        sprintf(cmd, ":F0 %05d#", (int)(value));
        break;
      }
      case 3:
      {
        value = maxP;
        ValueSetRequested = display->UserInterfaceInputValueFloat(&buttonPad, T_MAXPOSITION, "", &value, 0, 65535, 5, 0, "");
        sprintf(cmd, ":F1 %05d#", (int)(value));
        break;
      }
      case 4:
      {
        value = minS;
        ValueSetRequested = display->UserInterfaceInputValueFloat(&buttonPad, T_MANUALSPEED, "", &value, 1, 999, 5, 0, "");
        sprintf(cmd, ":F2 %03d#", (int)(value));
        break;
      }
      case 5:
      {
        value = maxS;
        ValueSetRequested = display->UserInterfaceInputValueFloat(&buttonPad, T_GOTOSPEED, "", &value, 1, 999, 5, 0, "");
        sprintf(cmd, ":F3 %03d#", (int)(value));
        break;
      }
      case 6:
      {
        value = manAcc;
        ValueSetRequested = display->UserInterfaceInputValueFloat(&buttonPad, T_ACCFORMAN, "", &value, 1, 100, 5, 0, "");
        sprintf(cmd, ":F5 %03d#", (int)(value));
        break;
      }
      case 7:
      {
        value = cmdAcc;
        ValueSetRequested = display->UserInterfaceInputValueFloat(&buttonPad, T_ACCFORGOTO, "", &value, 1, 100, 5, 0, "");
        sprintf(cmd, ":F4 %03d#", (int)(value));
        break;
      }
      case 8:
      {
        value = manDec;
        ValueSetRequested = display->UserInterfaceInputValueFloat(&buttonPad, T_DECFORMAN, "", &value, 1, 100, 5, 0, "");
        sprintf(cmd, ":F6 %03d#", (int)(value));
        break;
      }
      default:
        break;
      }
      if (ValueSetRequested)
      {
        DisplayMessageLX200(SetLX200(cmd), false);
      }
    }
    else
      break;
  }
}


void SmartHandController::menuFocuserMotor()
{
  char cmd[50];
  const char *string_list_Focuser = T_DISPLAYSETTINGS "\n" T_RESOLUTION "\n" T_ROTATION "\n" T_MICROSTEP "\n" T_CURRENT;
  unsigned int res, mu, curr;
  bool rev;
  float value;
  while (!exitMenu)
  {
    if (DisplayMessageLX200(readFocuserMotor(rev, mu, res, curr)))
    {
      current_selection_FocuserMotor = display->UserInterfaceSelectionList(&buttonPad, T_FOCUSERSETTINGS, current_selection_FocuserMotor, string_list_Focuser);
      bool ValueSetRequested = false;
      switch (current_selection_FocuserMotor)
      {
      case 0:
        return;
        break;
      case 1:
      {
        char line1[32] = "";
        char line2[32] = "";
        char line3[32] = "";
        char line4[32] = "";
        sprintf(line1, T_MOTORSETTINGS);
        sprintf(line3, T_RESOLUTION"  : %03u", res);
        rev ? sprintf(line4, T_REVERSEDROTATION) : sprintf(line2, T_DIRECTROTATION);
        DisplayLongMessage(line1, line2, line3, line4, -1);
        sprintf(line3, T_MICROSTEP " : %03u", (unsigned int)pow(2, mu));
        sprintf(line4, T_CURRENT " : %04umA", curr * 10);
        DisplayLongMessage(line1, line2, line3, line4, -1);
        break;
      }
      case 2:
      {
        value = res;
#define T_INCREMENTATION "Incrementation"
        ValueSetRequested = display->UserInterfaceInputValueFloat(&buttonPad, T_INCREMENTATION, "", &value, 1, 512, 5, 0, " " T_MICROSTEP);
        sprintf(cmd, ":F8 %03d#", (int)(value));
        break;
        break;
      }
      case 3:
      {
        char * string_list = T_DIRECT "\n" T_REVERSEDROTATION;
        uint8_t choice = display->UserInterfaceSelectionList(&buttonPad, T_ROTATION, (uint8_t)rev + 1, string_list);
        if (choice)
        {
          sprintf(cmd, ":F7 %d#", (int)(choice -1));
          ValueSetRequested = true;
        }
        break;
      }
      case 4:
      {
        uint8_t microStep = mu;
        char * string_list_micro = "4\n8\n16\n32\n64\n128";
        uint8_t choice = microStep - 2 + 1;
        choice = display->UserInterfaceSelectionList(&buttonPad, T_MICROSTEP, choice, string_list_micro);
        if (choice)
        {
          microStep = choice - 1 + 2;
          sprintf(cmd, ":Fm %d#", microStep);
          ValueSetRequested = true;
        }
        break;
      }
      case 5:
      {
        value = curr;
        ValueSetRequested = display->UserInterfaceInputValueFloat(&buttonPad, T_CURRENT, "", &value, 1, 160, 10, 0, "0 mA");
        sprintf(cmd, ":Fc %03d#", (int)(value));
        break;
      }
      default:
        break;
      }
      if (ValueSetRequested)
      {
        DisplayMessageLX200(SetLX200(cmd), false);
        delay(250);
      }
    }
    else
      break;
  }
}

void SmartHandController::menuFocuserSettings()
{
    if (!ta_MountStatus.hasFocuser())
  {
    DisplayMessage(T_FOCUSER, T_NOT_CONNECTED "!", 500);
    return;
  }
  buttonPad.setMenuMode();
  const char *string_list_Focuser = T_CONFIG "\n" T_MOTOR "\n" T_SHOWVERSION;
  while (!exitMenu)
  {
    current_selection_L2 = display->UserInterfaceSelectionList(&buttonPad, T_FOCUSERSETTINGS, current_selection_L2, string_list_Focuser);
    bool ValueSetRequested = false;
    switch (current_selection_L2)
    {
    case 0:
      exitMenu = true;
      break;
    case 1:
    {
      menuFocuserConfig();
      break;
    }
    case 2:
    {
      menuFocuserMotor();
      break;
    }
    case 3:
    {
      char out1[50];
      if (DisplayMessageLX200(GetLX200(":FV#", out1, 50)))
      {
        out1[31] = 0;
        DisplayMessage(T_FIRMWAREVERSION, &out1[26], -1);
      }
      break;
    }
    default:
      break;
    }
  }
  buttonPad.setControlerMode();
}

void SmartHandController::menuDisplay()
{
  const char *string_list_Display = T_TURNOFF "\n" T_CONTRAST "\n" T_SLEEP "\n" T_DEEPSLEEP;
  current_selection_L2 = 1;
  while (!exitMenu)
  {
    current_selection_L2 = display->UserInterfaceSelectionList(&buttonPad, T_DISPLAY, current_selection_L2, string_list_Display);
    switch (current_selection_L2)
    {
    case 0:
      return;
    case 1:
      DisplayMessage(T_PRESSSHIFTKEY, T_TOTURNON, 1500);
      forceDisplayoff = true;
      sleepDisplay = true;
      display->sleepOn();
      exitMenu = true;
      break;
    case 2:
      menuContrast();
      break;
    case 3:
    {
      if (display->UserInterfaceInputValueInteger(&buttonPad, T_LOWCONTRAST, T_AFTER " ", &displayT1, 3, 255, 3, "0 sec"))
      {
        EEPROM.write(EEPROM_T1, displayT1);
        EEPROM.commit();
      }
      break;
    }
    case 4:
    {
      if (display->UserInterfaceInputValueInteger(&buttonPad, T_TURNDISPLAYOFF, T_AFTER " ", &displayT2, displayT1, 255, 3, "0 sec"))
      {
        EEPROM.write(EEPROM_T2, displayT2);
        EEPROM.commit();
      }
      break;
    }
    default:
      break;
    }
  }
}


void SmartHandController::menuButtonSpeed()
{
  const char *string_list_Display = T_SLOW "\n" T_MEDIUM "\n" T_FAST;
  current_selection_L3 = (uint8_t)buttonPad.getButtonSpeed()+1;
  current_selection_L3 = display->UserInterfaceSelectionList(&buttonPad, T_BUTTONSPEED, current_selection_L3, string_list_Display);
  switch (current_selection_L3)
  {
  case 0:
    return;
  case 1:
  case 2:
  case 3:
    buttonPad.setButtonSpeed(static_cast<Pad::ButtonSpeed>(current_selection_L3-1));
    buttonPad.setMenuMode();
    break;
  }
}

void SmartHandController::menuContrast()
{
  const char *string_list_Display = T_MIN "\n" T_LOW "\n" T_HIGH "\n" T_MAX;
  current_selection_L3 = 1;

  current_selection_L3 = display->UserInterfaceSelectionList(&buttonPad, T_CONTRAST, current_selection_L3, string_list_Display);
  switch (current_selection_L3)
  {
  case 0:
    return;
  case 1:
    maxContrast = 0;
    break;
  case 2:
    maxContrast = 63;
    break;
  case 3:
    maxContrast = 127;
    break;
  case 4:
    maxContrast = 255;
    break;
  default:
    maxContrast = 255;
  }
  EEPROM.write(EEPROM_Contrast, maxContrast);
  EEPROM.commit();
  display->setContrast(maxContrast);
}


void SmartHandController::menuLocalDate()
{
  char out[20];
  if (DisplayMessageLX200(GetLX200(":GC#", out, sizeof(out))))
  {
    char* pEnd;
    uint8_t month = strtol(&out[0], &pEnd, 10);
    uint8_t day = strtol(&out[3], &pEnd, 10);
    uint8_t year = strtol(&out[6], &pEnd, 10);
    if (display->UserInterfaceInputValueDate(&buttonPad, T_DATE, year, month, day))
    {
      sprintf(out, ":SC%02d/%02d/%02d#", month, day, year);
      DisplayMessageLX200(SetLX200(out), false);
    }
  }
}

void SmartHandController::menuLatitude()
{
  double degree_d;
  int degree, minute;
  if (DisplayMessageLX200(GetLatitudeLX200(degree_d)))
  {
    long angle = degree_d * 3600;
    if (display->UserInterfaceInputValueLatitude(&buttonPad, &angle))
    {
      char cmd[20];
      char sign = angle < 0 ? '-' : '+';
      angle = abs(angle);
      angle /= 60;
      minute = angle % 60;
      degree = angle / 60;
      sprintf(cmd, ":St%+03d*%02d#", degree, minute);
      cmd[3] = sign;
      DisplayMessageLX200(SetLX200(cmd), false);
    }
  }
}

void SmartHandController::menuLongitude()
{
  double degree_d;
  int degree, minute;
  if (DisplayMessageLX200(GetLongitudeLX200(degree_d)))
  {
    long angle = degree_d * 3600;
    if (display->UserInterfaceInputValueLongitude(&buttonPad, &angle))
    {
      char cmd[20];
      char sign = angle < 0 ? '-' : '+';
      angle = abs(angle);
      angle /= 60;
      minute = angle % 60;
      degree = angle / 60;
      sprintf(cmd, ":Sg%+04d*%02d#", degree, minute);
      cmd[3] = sign;
      DisplayMessageLX200(SetLX200(cmd), false);
    }
  }
}

void SmartHandController::menuElevation()
{
  char out[20];
  if (DisplayMessageLX200(GetLX200(":Ge#", out, sizeof(out))))
  {
    float alt = (float)strtol(&out[0], NULL, 10);
    if (display->UserInterfaceInputValueFloat(&buttonPad, T_SITEELEVATION, "", &alt, -200, 8000, 2, 0, " meters"))
    {
      sprintf(out, ":Se%+04d#", (int)alt);
      DisplayMessageLX200(SetLX200(out), false);
    }
  }
}

void SmartHandController::menuLimits()
{
  const char *string_list_LimitsL2 = T_HORIZON "\n" T_OVERHEAD "\n" T_MERIDIANE "\n" T_MERIDIANW "\n" T_UNDERPOLE;
  current_selection_L3 = 1;
  while (current_selection_L3 != 0)
  {
    current_selection_L3 = display->UserInterfaceSelectionList(&buttonPad, T_LIMITS, current_selection_L3, string_list_LimitsL2);
    switch (current_selection_L3)
    {
    case 1:
      menuHorizon();
      break;
    case 2:
      menuOverhead();
      break;
    case 3:
      menuMeridian(true);
      break;
    case 4:
      menuMeridian(false);
      break;
    case 5:
      menuUnderPole();
      break;
    default:
      break;
    }
  }
}
void SmartHandController::menuMainUnitInfo()
{
  const char *string_list = T_SHOWVERSION "\n" T_REBOOT "\n" T_RESETTOFACTORY;
  current_selection_L2 = 1;
  while (current_selection_L2 != 0)
  {
    current_selection_L2 = display->UserInterfaceSelectionList(&buttonPad, T_MAINUNITINFO, 1, string_list);
    switch (current_selection_L2)
    {
    case 0:
      return;
    case 1:
      ta_MountStatus.updateV();
      if (ta_MountStatus.hasInfoV())
      {
        DisplayMessage(ta_MountStatus.getVN(), ta_MountStatus.getVD(), -1);
      }
      char out1[20];
      char out2[20];
      if (DisplayMessageLX200(GetLX200(":GVN#", out1, 20)) && DisplayMessageLX200(GetLX200(":GVD#", out2, 20)) )
      { 
        
      }
      break;
    case 2:
      DisplayMessageLX200(SetLX200(":$!#"), false);
      delay(500);
      powerCycleRequired = true;
      return;
    case 3:
      if (display->UserInterfaceMessage(&buttonPad, "Reset", "To", "Factory?", "NO\nYES") == 2)
      {
        DisplayMessageLX200(SetLX200(":$$#"), false);
        delay(500);
        powerCycleRequired = true;
        return;
      }
      break;
    default:
      break;
    }
  }
}

void SmartHandController::menuWifi()
{
  const char *string_list = buttonPad.isWifiOn() ?
    T_TURNWIFIOFF "\n" T_SHOWPASSWORD "\n" T_SELECTMODE "\n" T_SHOWIP "\n" T_RESETTOFACTORY :
    T_TURNWIFION "\n" T_SHOWPASSWORD "\n" T_SELECTMODE "\n" T_SHOWIP "\n" T_RESETTOFACTORY;
  current_selection_L2 = 1;
  while (!exitMenu)
  {
    current_selection_L2 = display->UserInterfaceSelectionList(&buttonPad, "Wifi", 1, string_list);
    switch (current_selection_L2)
    {
    case 0:
      return;
    case 1:
      buttonPad.turnWifiOn(!buttonPad.isWifiOn());
      exitMenu = true;
      powerCycleRequired = true;
      break;
    case 2:
      DisplayMessage(T_PASSWORDIS, buttonPad.getPassword(), -1);
      break;
    case 3:
    {
      menuWifiMode();
      break;
    }
    case 4:
    {
      uint8_t ip[4] = { 0,0,0,0 };
      buttonPad.getIP(&ip[0]);
      char iptxt[16];
      sprintf(iptxt, "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
      DisplayMessage("IP Adress", iptxt, -1);
      break;
    }
    case 5:
      resetSHC();
      break;
    default:
      break;
    }
  }
}

void SmartHandController::menuWifiMode()
{
  uint8_t idx = 0;
  uint8_t idxs[4] = { 3,3,3,3};
  char temp[20] = { 0 };
  char txt[150] = { 0 };
  char out[40] = { 0 };
  uint8_t selected_item=10;
  for (uint8_t k = 0; k < 3; k++)
  {
    buttonPad.getStationName((int)k, out);
    if (out != NULL && out[0] != 0)
    {
      strcat(txt, out);
      strcat(txt, "\n");
      idxs[idx] = k;
      if (buttonPad.getWifiMode() == (int)k)
      {
        selected_item = idx;
      }
      idx++;
    }
  }
  if (buttonPad.getWifiMode() == 3)
  {
    selected_item = idx;
  }
  selected_item++;
  while (!exitMenu)
  {
    char menustxt[200] = {};
    strcat(menustxt, txt);
    strcat(menustxt, T_ACCESPOINT);
    const char *string_list_WifiMode = &menustxt[0];
    selected_item = display->UserInterfaceSelectionList(&buttonPad, T_WIFIINTERFACE, selected_item, string_list_WifiMode);
    if (selected_item == 0)
    {
      return;
    }
    else
    {
      buttonPad.setWifiMode(idxs[selected_item - 1]);
      powerCycleRequired = true;
      exitMenu = true;
    }
  }
}


void SmartHandController::menuHorizon()
{
  char out[20];
  if (DisplayMessageLX200(GetLX200(":Gh#", out, sizeof(out))))
  {
    float angle = (float)strtol(&out[0], NULL, 10);
    if (display->UserInterfaceInputValueFloat(&buttonPad, T_HORIZONLIMIT, "", &angle, -10, 20, 2, 0, " " T_DEGREE))
    {
      sprintf(out, ":Sh%+03d#", (int)angle);
      DisplayMessageLX200(SetLX200(out), false);
    }
  }
}

void SmartHandController::menuOverhead()
{
  char out[20];
  if (DisplayMessageLX200(GetLX200(":Go#", out, sizeof(out))))
  {
    float angle = (float)strtol(&out[0], NULL, 10);
    if (display->UserInterfaceInputValueFloat(&buttonPad, T_OVERHEADLIMIT, "", &angle, 60, 91, 2, 0, " " T_DEGREE))
    {
      sprintf(out, ":So%02d#", (int)angle);
      DisplayMessageLX200(SetLX200(out), false);
    }
  }
}

void SmartHandController::menuUnderPole()
{
  char out[20];
  char cmd[15];
  if (DisplayMessageLX200(GetLX200(":GXEB#", out, sizeof(out))))
  {
    float angle = (float)strtol(&out[0], NULL, 10) / 10;
    if (display->UserInterfaceInputValueFloat(&buttonPad, T_MAXHOURANGLE, "+-", &angle, 9, 12, 2, 1, " " T_HOURS))
    {
      sprintf(cmd, ":SXEB_%03d#", (int)(angle * 10));
      DisplayMessageLX200(SetLX200(cmd), false);
    }
  }
}

void SmartHandController::menuMeridian(bool east)
{
  char out[20];
  char cmd[15];
  sprintf(cmd, ":GXEX#");
  cmd[4] = east ? '9' : 'A';

  if (DisplayMessageLX200(GetLX200(cmd, out, sizeof(out))))
  {
    float angle = (float)strtol(&out[0], NULL, 10) / 4.0;
    if (display->UserInterfaceInputValueFloat(&buttonPad, east ? T_MERIDIANLIMITE : T_MERIDIANLIMITW , "", &angle, -45, 45, 2, 0, " " T_DEGREE))
    {
      sprintf(cmd, ":SXEX:%+03d#", (int)(angle*4.0));
      cmd[4] = east ? '9' : 'A';
      DisplayMessageLX200(SetLX200(cmd), false);
    }
  }
}



void SmartHandController::DisplayMessage(const char* txt1, const char* txt2, int duration)
{
   display->setFont(u8g2_font_helvR12_tf);
  uint8_t x;
  uint8_t y = 40;
  display->firstPage();
  do {
    if (txt2 != NULL)
    {
      y = 50;
      x = (display->getDisplayWidth() - display->getUTF8Width(txt2)) / 2;
      display->drawUTF8(x, y, txt2);
      y = 25;
    }
    x = (display->getDisplayWidth() - display->getUTF8Width(txt1)) / 2;
    display->drawUTF8(x, y, txt1);
  } while (display->nextPage());
  if (duration >= 0)
    delay(duration);
  else
  {
    for (;;)
    {
      tickButtons();
      delay(50);
      if (buttonPressed())
        break;
    }
  }
}

void SmartHandController::DisplayLongMessage(const char* txt1, const char* txt2, const char* txt3, const char* txt4, int duration)
{
  display->setFont(u8g2_font_helvR10_te);
  uint8_t h = 15;
  uint8_t x = 0;
  uint8_t y = h;
  display->firstPage();
  do {
    y = h;
    x = (display->getDisplayWidth() - display->getUTF8Width(txt1)) / 2;
    display->drawUTF8(x, y, txt1);
    y += h;
    if (txt2 != NULL)
    {
      x = 0;
      display->drawUTF8(x, y, txt2);
    }
    else
    {
      y -= 7;
    }
    y += 15;
    if (txt3 != NULL)
    {
      x = 0;
      display->drawUTF8(x, y, txt3);
    }

    y += 15;
    if (txt4 != NULL)
    {
      x = 0;
      display->drawUTF8(x, y, txt4);
    }
  } while (display->nextPage());
  if (duration >= 0)
    delay(duration);
  else
  {
    for (;;)
    {
      tickButtons();
      delay(50);
      if (buttonPressed())
        break;
    }
  }
  display->setFont(u8g2_font_helvR12_te);
}

bool SmartHandController::DisplayMessageLX200(LX200RETURN val, bool silentOk)
{
  char text1[20] = "";
  char text2[20] = "";
  int time = -1;
  if (val < LX200OK)
  {
    if (val == LX200NOTOK)
    {
      sprintf(text1, T_LX200COMMAND);
      sprintf(text2, T_HASFAILED "!");
    }
    else if (val == LX200SETVALUEFAILED)
    {
      sprintf(text1, T_SETVEALUE);
      sprintf(text2, T_HASFAILED "!");
    }
    else if (val == LX200GETVALUEFAILED)
    {
      sprintf(text1, T_GETVEALUE);
      sprintf(text2, T_HASFAILED "!");
    }
    else if (val == LX200SYNCFAILED)
    {
      sprintf(text1, T_SYNC);
      sprintf(text2, T_HASFAILED "!");
    }
    else if (val == LX200SETTARGETFAILED)
    {
      sprintf(text1, T_SETTARGET);
      sprintf(text2, T_HASFAILED "!");
    }
    else if (val == LX200BELOWHORIZON)
    {
      sprintf(text1, T_TARGETIS);
      sprintf(text2,  T_BELOWHORIZON "!");
    }
    else if (val == LX200_ERR_MOTOR_FAULT)
    {
      sprintf(text1, T_TELESCOPEMOTOR);
      sprintf(text2, T_FAULT "!");
    }
    else if (val == LX200_ERR_ALT)
    {
      sprintf(text1, T_TELESCOPEIS);
      sprintf(text2, T_BELOWHORIZON "!");
    }
    else if (val == LX200_ERR_LIMIT_SENSE)
    {
      sprintf(text1, T_TELESCOPEEXCEED);
      sprintf(text2, T_SENSORLIMIT "!");
    }
    else if (val == LX200_ERR_DEC)
    {
      sprintf(text1, T_TELESCOPEEXCEED);
      sprintf(text2, T_DECLIMIT "!");
    }
    else if (val == LX200_ERR_AZM)
    {
      sprintf(text1, T_TELESCOPEEXCEED);
      sprintf(text2, T_AZMLIMIT "!");
    }
    else if (val == LX200_ERR_UNDER_POLE)
    {
      sprintf(text1, T_TELESCOPEEXCEED);
      sprintf(text2, T_UNDERPOLELIMIT "!");
    }
    else if (val == LX200_ERR_MERIDIAN)
    {
      sprintf(text1, T_TELESCOPEEXCEED);
      sprintf(text2, T_MERIDIANLIMIT "!");
    }
    else if (val == LX200_ERR_SYNC)
    {
      sprintf(text1, T_TELESCOPEIS);
      sprintf(text2, T_OUTSIDELIMITS "!");
    }
    else if (val == LX200NOOBJECTSELECTED)
    {
      sprintf(text1, T_NOOBJECT);
      sprintf(text2, T_SELECTED "!");
    }
    else if (val == LX200PARKED)
    {
      sprintf(text1, T_TELESCOPEIS);
      sprintf(text2, T_PARKED "!");
    }
    else if (val == LX200BUSY)
    {
      sprintf(text1, T_TELESCOPEIS);
      sprintf(text2, T_BUSY "!");
    }
    else if (val == LX200LIMITS)
    {
      sprintf(text1, T_TARGETIS);
      sprintf(text2, T_OUTSIDELIMITS "!");
    }
    else if (val == LX200UNKOWN)
    {
      sprintf(text1, T_TUNKOWN);
      sprintf(text2, T_ERROR "!");
    }
    else if (val == LX200GOPARK_FAILED)
    {
      sprintf(text1, T_TELESCOPE);
      sprintf(text2, T_CANTPARK "!");
    }
    else if (val == LX200GOHOME_FAILED)
    {
      sprintf(text1, T_TELESCOPE);
      sprintf(text2, T_CANTGOHOME "!");
    }
    else
    {
      sprintf(text1, T_ERROR);
      sprintf(text2, "-1");
    }
    DisplayMessage(text1, text2, -1);
  }
  else if (!silentOk)
  {
    time = 1000;
    if (val == LX200OK)
    {
      sprintf(text1, T_LX200COMMAND);
      sprintf(text2, T_DONE "!");
    }
    else if (val == LX200VALUESET)
    {
      sprintf(text1, T_VALUE);
      sprintf(text2, T_SET "!");
    }
    else if (val == LX200VALUEGET)
    {
      sprintf(text1, T_VALUE);
      sprintf(text2, T_GET "!");
    }
    else if (val == LX200SYNCED)
    {
      sprintf(text1, T_TELESCOPE);
      sprintf(text2, T_SYNCED "!");
    }
    else if (val == LX200GOINGTO)
    {
      sprintf(text1, T_SLEWINGTO);
      sprintf(text2, T_TARGET);
    }
    else if (val == LX200GOPARK)
    {
      sprintf(text1, T_SLEWINGTO);
      sprintf(text2, T_PARK);
    }
    else if (val == LX200GOHOME)
    {
      sprintf(text1, T_SLEWINGTO);
      sprintf(text2, T_HOME);
    }
    DisplayMessage(text1, text2, time);
  }
  return isOk(val);
}

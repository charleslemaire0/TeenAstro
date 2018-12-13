
#include "_EEPROM_ext.h"
#include "SmartController.h"
#include "LX200.h"


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
  0x00, 0x00, 0x02, 0x00, 0x06, 0x00, 0x0e, 0x00, 0x1e, 0x00, 0x3e, 0x00,
  0x7e, 0x00, 0xfe, 0x38, 0xfe, 0x44, 0x7e, 0x44, 0x3e, 0x20, 0x1e, 0x10,
  0x0e, 0x10, 0x06, 0x00, 0x02, 0x10, 0x00, 0x00 };

static unsigned char tracking_S_bits[] U8X8_PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x06, 0x00, 0x0e, 0x00, 0x1e, 0x00,
  0x3e, 0x00, 0x7e, 0x00, 0xfe, 0x38, 0x7e, 0x04, 0x3e, 0x04, 0x1e, 0x18,
  0x0e, 0x20, 0x06, 0x20, 0x02, 0x1c, 0x00, 0x00 };

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

static unsigned char Lock_T_bits[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x08, 0x00, 0x08, 0x00,
  0x08, 0x00, 0x08, 0x00, 0x08, 0x00, 0x00, 0x00 };

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

void gethms(const long& v, uint8_t& v1, uint8_t& v2, uint8_t& v3)
{
  v3 = v % 60;
  v2 = (v / 60) % 60;
  v1 = v / 3600;
}

void getdms(const long& v, short& v1, uint8_t& v2, uint8_t& v3)
{
  v3 = abs(v) % 60;
  v2 = (abs(v) / 60) % 60;
  v1 = v / 3600;
}

void longRa2Ra(long Ra, int& h, int& m, int& s)
{
  h = Ra / 30;
  m = (Ra - h * 30) / 60;
  s = (Ra / 30) % 60;
}

void longDec2Dec(long Dec, int& deg, int& min)
{
  deg = Dec / 60;
  min = Dec % 60;
}

void SmartHandController::setup(const char version[], const int pin[7], const bool active[7], const int SerialBaud, const OLED model)
{
  if (strlen(version) <= 19) strcpy(_version, version);

  telInfo.lastState = 0;

  //choose a 128x64 display supported by U8G2lib (if not listed below there are many many others in u8g2 library example Sketches)
  //U8G2_SH1106_128X64_NONAME_1_HW_I2C display(U8G2_R0);
  //U8G2_SSD1306_128X64_NONAME_F_SW_I2C display(U8G2_R0, /* clock=*/ SCL, /* data=*/ SDA, /* reset=*/ U8X8_PIN_NONE);   // All Boards without Reset of the Display

  if (model == OLED_SH1106)
    display = new U8G2_EXT_SH1106_128X64_NONAME_1_HW_I2C(U8G2_R0);
  else if (model == OLED_SSD1306)
    display = new U8G2_EXT_SSD1306_128X64_NONAME_F_HW_I2C(U8G2_R0);
  display->begin();
  drawIntro();
  buttonPad.setup(pin, active);
  tickButtons();
  if (EEPROM.length() == 0)
    EEPROM.begin(1024);

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
#ifndef WIFI_ON
  Ser.begin(SerialBaud);
  for (int i = 0; i < 3; i++)
  {
    Ser.print(":#");
    delay(500);
    Ser.flush();
    delay(500);
  }
#endif // !WIFI_ON
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
  moving = telInfo.getTrackingState() == Telescope::TRK_SLEWING || telInfo.getParkState() == Telescope::PRK_PARKING;
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
      if (telInfo.align != Telescope::ALI_OFF)
      {
        telInfo.align = static_cast<Telescope::AlignState>(telInfo.align - 1);
      }
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
  if (powerCylceRequired)
  {
    display->sleepOff();
    DisplayMessage("Press key", "to reboot...", -1);
    DisplayMessage("Device", "will reboot...", 1000);
    ESP.reset();
    return;
  }
  if (telInfo.notResponding())
  {
    display->sleepOff();
    DisplayMessage("!! Error !!", "Not Connected", -1);
    DisplayMessage("Device", "will reboot...", 1000);
    ESP.reset();
  }
  if (telInfo.align == Telescope::ALI_SELECT_STAR_1 || telInfo.align == Telescope::ALI_SELECT_STAR_2 || telInfo.align == Telescope::ALI_SELECT_STAR_3)
  {
    if (telInfo.align == Telescope::ALI_SELECT_STAR_1)
      DisplayLongMessage("Select a Star", "near the Meridian", "& the Celestial Equ.", "in the Western Sky", -1);
    else if (telInfo.align == Telescope::ALI_SELECT_STAR_2)
      DisplayLongMessage("Select a Star", "near the Meridian", "& the Celestial Equ.", "in the Eastern Sky", -1);
    else if (telInfo.align == Telescope::ALI_SELECT_STAR_3)
      DisplayLongMessage("Select a Star", "HA = -3 hour", "Dec = +- 45 degree", "in the Eastern Sky", -1);
    if (!SelectStarAlign())
    {
      DisplayMessage("Alignment", "Aborted", -1);
      telInfo.align = Telescope::ALI_OFF;
      return;
    }
    telInfo.align = static_cast<Telescope::AlignState>(telInfo.align + 1);
  }
  else if (top - lastpageupdate > 100)
  {
    updateMainDisplay(page);
  }
  if (!telInfo.connected())
    return;
  bool moving = false;
  manualMove(moving);
  if (eventbuttons[0] == E_CLICK && telInfo.align == Telescope::ALI_OFF)
  {
    page++;
    if (page > 3) page = 0;
    time_last_action = millis();
  }
  else if (moving)
  {
    return;
  }
  else if (eventbuttons[0] == E_LONGPRESS || eventbuttons[0] == E_LONGPRESSTART && telInfo.align == Telescope::ALI_OFF )
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

  else if (eventbuttons[0] == E_CLICK && (telInfo.align == Telescope::ALI_RECENTER_1 || telInfo.align == Telescope::ALI_RECENTER_2 || telInfo.align == Telescope::ALI_RECENTER_3))
  {
    telInfo.addStar();
  }

}

void SmartHandController::updateMainDisplay(u8g2_uint_t page)
{
  u8g2_t *u8g2 = display->getU8g2();
  display->setFont(u8g2_font_helvR12_te);
  u8g2_uint_t line_height = u8g2_GetAscent(u8g2) - u8g2_GetDescent(u8g2) + MY_BORDER_SIZE;
  u8g2_uint_t step1 = u8g2_GetUTF8Width(u8g2, "44");
  u8g2_uint_t step2 = u8g2_GetUTF8Width(u8g2, "4") + 1;
  telInfo.connectionFailure = max(telInfo.connectionFailure - 1, 0);
  telInfo.updateTel();
  if (telInfo.hasTelStatus && telInfo.align != Telescope::ALI_OFF)
  {
    Telescope::TrackState curT = telInfo.getTrackingState();
    if (curT != Telescope::TRK_SLEWING && (telInfo.align == Telescope::ALI_SLEW_STAR_1 || telInfo.align == Telescope::ALI_SLEW_STAR_2 || telInfo.align == Telescope::ALI_SLEW_STAR_3))
    {
      telInfo.align = static_cast<Telescope::AlignState>(telInfo.align + 1);
    }
    page = 4;
  }
  else if (page == 0)
  {
    telInfo.updateRaDec();
  }
  else if (page == 1)
  {
    telInfo.updateAzAlt();
  }
  else if (page == 2)
  {
    telInfo.updateTime();
  }
  else if (page == 3)
  {
    telInfo.updateFocuser();
  }
  u8g2_FirstPage(u8g2);

  do
  {
    u8g2_uint_t x = u8g2_GetDisplayWidth(u8g2);
    int k = 0;
#ifdef WIFI_ON
    if (buttonPad.isWifiOn())
    {
      buttonPad.isWifiRunning() ? display->drawXBMP(0, 0, icon_width, icon_height, wifi_bits) : display->drawXBMP(0, 0, icon_width, icon_height, wifi_not_connected_bits);
    }
    
#endif
    if (telInfo.hasTelStatus)
    {
      Telescope::ParkState curP = telInfo.getParkState();
      Telescope::TrackState curT = telInfo.getTrackingState();
      Telescope::PierState curPi = telInfo.getPierState();
      if (curP == Telescope::PRK_PARKED)
      {
        display->drawXBMP(x - icon_width, 0, icon_width, icon_height, parked_bits);
        x -= icon_width + 1;
      }
      else if (curP == Telescope::PRK_PARKING)
      {
        display->drawXBMP(x - icon_width, 0, icon_width, icon_height, parking_bits);
        x -= icon_width + 1;
      }
      else if (telInfo.atHome())
      {
        display->drawXBMP(x - icon_width, 0, icon_width, icon_height, home_bits);
        x -= icon_width + 1;
      }
      else
      {
        if (curT == Telescope::TRK_SLEWING)
        {
          display->drawXBMP(x - icon_width, 0, icon_width, icon_height, sleewing_bits);
          x -= icon_width + 1;
        }
        else if (curT == Telescope::TRK_ON)
        {
          display->drawXBMP(x - icon_width, 0, icon_width, icon_height, tracking_S_bits);
          x -= icon_width + 1;
        }
        else if (curT == Telescope::TRK_OFF)
        {
          display->drawXBMP(x - icon_width, 0, icon_width, icon_height, no_tracking_bits);
          x -= icon_width + 1;
        }

        if (curP == Telescope::PRK_FAILED)
        {
          display->drawXBMP(x - icon_width, 0, icon_width, icon_height, parkingFailed_bits);
          x -= icon_width + 1;
        }

        if (curPi == Telescope::PIER_E)
        {
          display->drawXBMP(x - icon_width, 0, icon_width, icon_height, E_bits);
          x -= icon_width + 1;
        }
        else if (curPi == Telescope::PIER_W)
        {
          display->drawXBMP(x - icon_width, 0, icon_width, icon_height, W_bits);
          x -= icon_width + 1;
        }

        
        if (telInfo.align != Telescope::ALI_OFF)
        {
          if (telInfo.aliMode == Telescope::ALIM_ONE)
            display->drawXBMP(x - icon_width, 0, icon_width, icon_height, align1_bits);
          else if (telInfo.aliMode == Telescope::ALIM_TWO)
            display->drawXBMP(x - icon_width, 0, icon_width, icon_height, align2_bits);
          else if (telInfo.aliMode == Telescope::ALIM_THREE)
            display->drawXBMP(x - icon_width, 0, icon_width, icon_height, align3_bits);
          x -= icon_width + 1;
        }

        if (telInfo.isPulseGuiding())
        {
          display->drawXBMP(x - icon_width, 0, icon_width, icon_height, guiding__bits);
          display->setBitmapMode(1);
          if (telInfo.isGuidingN())
          {
            display->drawXBMP(x - icon_width, 0, icon_width, icon_height, guiding_N_bits);
          }
          else if (telInfo.isGuidingS())
          {
            display->drawXBMP(x - icon_width, 0, icon_width, icon_height, guiding_S_bits);
          }
          if (telInfo.isGuidingE())
          {
            display->drawXBMP(x - icon_width, 0, icon_width, icon_height, guiding_E_bits);
          }
          else if (telInfo.isGuidingW())
          {
            display->drawXBMP(x - icon_width, 0, icon_width, icon_height, guiding_W_bits);
          }
          display->setBitmapMode(0);
          x -= icon_width + 1;

        }

      }
      switch (telInfo.getError())
      {
      case Telescope::ERR_MOTOR_FAULT:
        display->drawXBMP(x - icon_width, 0, icon_width, icon_height, ErrMf_bits);
        x -= icon_width + 1;
        break;
      case  Telescope::ERR_ALT:
        display->drawXBMP(x - icon_width, 0, icon_width, icon_height, ErrHo_bits);
        x -= icon_width + 1;
        break;
      case Telescope::ERR_DEC:
        display->drawXBMP(x - icon_width, 0, icon_width, icon_height, ErrDe_bits);
        x -= icon_width + 1;
        break;
      case Telescope::ERR_UNDER_POLE:
        display->drawXBMP(x - icon_width, 0, icon_width, icon_height, ErrUp_bits);
        x -= icon_width + 1;
        break;
      case Telescope::ERR_MERIDIAN:
        display->drawXBMP(x - icon_width, 0, icon_width, icon_height, ErrMe_bits);
        x -= icon_width + 1;
        break;
      default:
        break;
      }

    }

    if (focuserlocked|| telescoplocked)
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

    if (page == 0)
    {

      if (telInfo.hasInfoRa && telInfo.hasInfoDec)
      {
        char Rah[3];
        char Ram[3];
        char Ras[3];
        char decsign[2];
        char decdeg[3];
        char decmin[3];
        char decsec[3];
        memcpy(Rah, telInfo.TempRa, 2);
        Rah[2] = '\0';
        memcpy(Ram, &telInfo.TempRa[3], 2);
        Ram[2] = '\0';
        memcpy(Ras, &telInfo.TempRa[6], 2);
        Ras[2] = '\0';
        memcpy(decsign, telInfo.TempDec, 1);
        decsign[1] = '\0';
        memcpy(decdeg, &telInfo.TempDec[1], 2);
        decdeg[2] = '\0';
        memcpy(decmin, &telInfo.TempDec[4], 2);
        decmin[2] = '\0';
        memcpy(decsec, &telInfo.TempDec[7], 2);
        decsec[2] = '\0';

        u8g2_uint_t y = 36;
        x = u8g2_GetDisplayWidth(u8g2);

        display->drawRA(x, y, Rah, Ram, Ras);
        u8g2_DrawUTF8(u8g2, 0, y, "RA");

        y += line_height + 4;
        u8g2_DrawUTF8(u8g2, 0, y, "Dec");
        display->drawDec(x, y, decsign, decdeg, decmin, decsec);

      }
    }
    else if (page == 1)
    {

      if (telInfo.hasInfoAz && telInfo.hasInfoAlt)
      {
        char Azdeg[4];
        char Azm[3];
        char Azs[3];
        char Altsign[2];
        char Altdeg[3];
        char Altmin[3];
        char Altsec[3];
        memcpy(Azdeg, telInfo.TempAz, 3);
        Azdeg[3] = '\0';
        memcpy(Azm, &telInfo.TempAz[4], 2);
        Azm[2] = '\0';
        memcpy(Azs, &telInfo.TempAz[7], 2);
        Azs[2] = '\0';
        memcpy(Altsign, telInfo.TempAlt, 1);
        Altsign[1] = '\0';
        memcpy(Altdeg, &telInfo.TempAlt[1], 2);
        Altdeg[2] = '\0';
        memcpy(Altmin, &telInfo.TempAlt[4], 2);
        Altmin[2] = '\0';
        memcpy(Altsec, &telInfo.TempAlt[7], 2);
        Altsec[2] = '\0';

        u8g2_uint_t y = 36;
        u8g2_uint_t startpos = u8g2_GetUTF8Width(u8g2, "123456");
        x = startpos;
        x = u8g2_GetDisplayWidth(u8g2);
        u8g2_DrawUTF8(u8g2, 0, y, "Az.");
        display->drawAz(x, y, Azdeg, Azm, Azs);

        y += line_height + 4;
        x = startpos;
        x = u8g2_GetDisplayWidth(u8g2);

        display->drawDec(x, y, Altsign, Altdeg, Altmin, Altsec);
        u8g2_DrawUTF8(u8g2, 0, y, "Alt.");
      }
    }
    else if (page == 2)
    {
      if (telInfo.hasInfoUTC && telInfo.hasInfoSideral)
      {
        char Rah[3];
        char Ram[3];
        char Ras[3];
        u8g2_uint_t y = 36;

        x = u8g2_GetDisplayWidth(u8g2);
        memcpy(Rah, telInfo.TempUTC, 2);
        Rah[2] = '\0';
        memcpy(Ram, &telInfo.TempUTC[3], 2);
        Ram[2] = '\0';
        memcpy(Ras, &telInfo.TempUTC[6], 2);
        Ras[2] = '\0';
        u8g2_DrawUTF8(u8g2, 0, y, "UTC");
        display->drawRA(x, y, Rah, Ram, Ras);

        y += line_height + 4;
        memcpy(Rah, telInfo.TempSideral, 2);
        Rah[2] = '\0';
        memcpy(Ram, &telInfo.TempSideral[3], 2);
        Ram[2] = '\0';
        memcpy(Ras, &telInfo.TempSideral[6], 2);
        Ras[2] = '\0';
        u8g2_DrawUTF8(u8g2, 0, y, "Sideral");
        display->drawRA(x, y, Rah, Ram, Ras);
      }
    }
    else if (page == 3)
    {
      u8g2_uint_t y = 36;
      if (telInfo.hasInfoFocuser)
      {
        char pos[6];
        char spd[4];
        x = u8g2_GetDisplayWidth(u8g2) - u8g2_GetUTF8Width(u8g2, "00000");
        u8g2_DrawUTF8(u8g2, 0, y, "F Position");
        memcpy(pos, &telInfo.TempFocuserStatus[1], 5);
        pos[5] = 0;
        u8g2_DrawUTF8(u8g2, x, y, pos);

        y += line_height + 4;
        x = u8g2_GetDisplayWidth(u8g2) - u8g2_GetUTF8Width(u8g2, "000");
        u8g2_DrawUTF8(u8g2, 0, y, "F Speed");
        memcpy(spd, &telInfo.TempFocuserStatus[7], 3);
        spd[3] = 0;
        u8g2_DrawUTF8(u8g2, x, y, spd);
      }
      else
      {
        u8g2_DrawUTF8(u8g2, 0, y, "Focuser not");
        y += line_height + 4;
        u8g2_DrawUTF8(u8g2, 0, y, "Connected");
      }
    }
    else if (page == 4)
    {
      int idx = telInfo.alignSelectedStar - 1;
      byte cat_letter = Star_letter[idx];
      byte cat_const = Star_constellation[idx];
      u8g2_uint_t y = 36;
      char txt[20];

      if ((telInfo.align - 1) % 3 == 1)
      {
        sprintf(txt, "Slew to Star %u", (telInfo.align - 1) / 3 + 1);
      }
      else if ((telInfo.align - 1) % 3 == 2)
      {
        sprintf(txt, "Recenter Star %u", (telInfo.align - 1) / 3 + 1);
      }
      u8g2_DrawUTF8(u8g2, 0, y, txt);
      y += line_height + 4;
      const uint8_t* myfont = u8g2->font;
      u8g2_SetFont(u8g2, u8g2_font_unifont_t_greek);
      u8g2_DrawGlyph(u8g2, 0, y, 944 + cat_letter);
      u8g2_SetFont(u8g2, myfont);
      u8g2_DrawUTF8(u8g2, 16, y, constellation_txt[cat_const - 1]);
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
  delay(2000);
}

void SmartHandController::drawLoad()
{
  display->firstPage();
  uint8_t x = 0;
  do {
    display->setFont(u8g2_font_helvR14_tr);
    x = (display->getDisplayWidth() - display->getStrWidth("Loading")) / 2;
    display->drawStr(x, display->getDisplayHeight() / 2. - 14, "Loading");
    x = (display->getDisplayWidth() - display->getStrWidth(_version)) / 2;
    display->drawStr(x, display->getDisplayHeight() / 2. + 14, _version);
  } while (display->nextPage());
}

void SmartHandController::drawReady()
{
  display->firstPage();
  uint8_t x = 0;
  do {
    x = (display->getDisplayWidth() - display->getStrWidth("Ready!")) / 2;
    display->drawStr(x, display->getDisplayHeight() / 2, "Ready!");
  } while (display->nextPage());
  delay(500);
}

bool SmartHandController::menuSetStepperGearBox(const uint8_t &axis, unsigned short &worm)
{
  char text[20];
  float stepperGearBox = 10;
  sprintf(text, "Gear box M%u", axis);
  if (display->UserInterfaceInputValueFloat(&buttonPad, text, "Ratio", &stepperGearBox, 1, 100, 5, 0, ""))
  {
    return DisplayMessageLX200(writeTotGearLX200(axis, stepperGearBox * worm), false);
  }
}

bool SmartHandController::menuSetReverse(const uint8_t &axis)
{
  bool reverse;
  if (!DisplayMessageLX200(readReverseLX200(axis, reverse)))
    return false;
  char text[20];
  char * string_list_micro = "Direct\nReversed";
  sprintf(text, "Rotation M%u", axis);
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
  sprintf(text, "Backlash M%u", axis);
  if (display->UserInterfaceInputValueFloat(&buttonPad, text, "", &backlash, 0, 1000, 4, 0, " in seconds"))
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
  sprintf(text, "Gear M%u", axis);
  if (display->UserInterfaceInputValueFloat(&buttonPad, text, "Ratio", &totGear, 1, 60000, 5, 0, ""))
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
  sprintf(text, "Stepper M%u", axis);
  if (display->UserInterfaceInputValueFloat(&buttonPad, text, "", &stepPerRot, 1, 400, 3, 0, " Steps"))
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
  sprintf(text, "Stepper M%u", axis);
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
  sprintf(text, "Low Curr. M%u", axis);
  if (display->UserInterfaceInputValueInteger(&buttonPad, text, "", &lowCurr, 10, 200, 3, "0 mA peak"))
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
  sprintf(text, "High Curr. M%u", axis);
  if (display->UserInterfaceInputValueInteger(&buttonPad, text, "", &highCurr, 10, 200, 3, "0 mA peak"))
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
  char line1[32] = "Slew Settings";
  char line3[32] = "";
  char line4[32] = "";
  if (DisplayMessageLX200(GetLX200(":GXE2#", out, sizeof(out))))
  {
    float acc = atof(&out[0]);
    sprintf(line3, "Acceleration: %.1f", acc);
  }
  if (DisplayMessageLX200(GetLX200(":GX92#", out, sizeof(out))))
  {
    int maxrate = (float)strtol(&out[0], NULL, 10);
    sprintf(line4, "Max Slew: %dx", maxrate);
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
  sprintf(line1, "Motor %u Settings", axis);
  if (DisplayMessageLX200(readReverseLX200(axis, reverse)))
  {
    reverse ? sprintf(line3, "Reversed Rotation") : sprintf(line3, "Direct Rotation");
  }
  if (DisplayMessageLX200(readTotGearLX200(axis, totGear)))
  {
    sprintf(line4, "Ratio: %u", (unsigned int)totGear);
  }

  DisplayLongMessage(line1, NULL, line3, line4, -1);

  line2[0] = 0;
  line3[0] = 0;
  line4[0] = 0;

  if (DisplayMessageLX200(readStepPerRotLX200(axis, stepPerRot)))
  {
    sprintf(line2, "%u Steps per Rot.", (unsigned int)stepPerRot);
  }
  if (DisplayMessageLX200(readMicroLX200(axis, microStep)))
  {
    sprintf(line3, "MicroStep: %u", (unsigned int)pow(2, microStep));
  }
  if (DisplayMessageLX200(readBacklashLX200(axis, backlash)))
  {
    sprintf(line4, "Backlash: %u sec.", (unsigned int)backlash);
  }
  DisplayLongMessage(line1, line2, line3, line4, -1);
  line2[0] = 0;
  line3[0] = 0;
  line4[0] = 0;
  if (DisplayMessageLX200(readLowCurrLX200(axis, lowCurr)))
  {
    sprintf(line3, "Low Curr. %u0 mA", (unsigned int)lowCurr);
  }
  if (DisplayMessageLX200(readHighCurrLX200(axis, highCurr)))
  {
    sprintf(line4, "High Curr. %u0 mA", (unsigned int)highCurr);
  }

  DisplayLongMessage(line1, NULL, line3, line4, -1);
}

void SmartHandController::menuTelAction()
{
  buttonPad.setMenuMode();
  current_selection_L0 = 1;
  while (!exitMenu)
  {
    telInfo.updateTel();
    Telescope::ParkState currentstate = telInfo.getParkState();

    if (currentstate == Telescope::PRK_PARKED)
    {
      const char *string_list_main_ParkedL0 = "Unpark";
      current_selection_L0 = display->UserInterfaceSelectionList(&buttonPad, "Telescope Action", current_selection_L0, string_list_main_ParkedL0);
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
    else if (currentstate == Telescope::PRK_UNPARKED)
    {
      const char *string_list_main_UnParkedL0 = telescoplocked ? "Goto\nSync\nTracking\nSide of Pier\nUnlock" : "Goto\nSync\nTracking\nSide of Pier\nLock";
      current_selection_L0 = display->UserInterfaceSelectionList(&buttonPad, "Telescope Action", current_selection_L0, string_list_main_UnParkedL0);
      switch (current_selection_L0)
      {
      case 0:
        exitMenu = true;
        break;
      case 1:
        menuSyncGoto(false);
        break;
      case 2:
        menuSyncGoto(true);
        break;
      case 3:
        menuTrack();
        break;
      case 4:
        menuPier();
        break;
      case 5:
        telescoplocked = !telescoplocked;
        exitMenu = true;
        break;
      default:
        break;
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
  char * string_list_Speed = "Guide\n1.0x\n4.0x\n16.0x\n64.0x\n0.5 Max\nMax";
  uint8_t selected_speed = display->UserInterfaceSelectionList(&buttonPad, "Set Speed", current_selection_speed, string_list_Speed);
  int speed;
  switch (selected_speed)
  {
  case 0:
    break;
  case 1:
    speed = 0;
    break;
  case 2:
    speed = 2;
    break;
  case 3:
    speed = 4;
    break;
  case 4:
    speed = 5;
    break;
  case 5:
    speed = 7;
    break;
  case 6:
    speed = 8;
    break;
  case 7:
    speed = 9;
    break;
  default:
    break;
  }
  if (selected_speed > 0)
  {
    char cmd[5] = ":Rn#";
    cmd[2] = '0' + speed;
    SetLX200(cmd);
    current_selection_speed = selected_speed;
  }
  buttonPad.setControlerMode();
}

void SmartHandController::menuTrack()
{
  telInfo.updateTel();
  Telescope::TrackState currentstate = telInfo.getTrackingState();
  if (currentstate == Telescope::TRK_ON)
  {
    const char *string_list_tracking = "Stop Tracking";
    current_selection_L1 = display->UserInterfaceSelectionList(&buttonPad, "Tracking State", 0, string_list_tracking);
    switch (current_selection_L1)
    {
    case 1:
      char out[20];
      memset(out, 0, sizeof(out));
      if (SetLX200(":Td#") == LX200VALUESET)
      {
        DisplayMessage("Tracking", "OFF", 500);
        exitMenu = true;
      }
      else
      {
        DisplayMessage("Set State", "Failed", 1000);
      }
      break;
    default:
      break;
    }
  }
  else if (currentstate == Telescope::TRK_OFF)
  {
    const char *string_list_tracking = "Start Tracking";
    current_selection_L1 = display->UserInterfaceSelectionList(&buttonPad, "Tracking State", 0, string_list_tracking);
    switch (current_selection_L1)
    {
    case 1:
      if (SetLX200(":Te#") == LX200VALUESET)
      {
        DisplayMessage("Tracking", "ON", 500);
        exitMenu = true;
      }
      else
      {
        DisplayMessage("Set State", "Failed", 1000);
      }
      break;
    default:
      break;
    }
  }
  else
  {
    DisplayMessage("currently Tracking", "cannot be changed", 1000);
  }
}

void SmartHandController::menuSyncGoto(bool sync)
{
  current_selection_L1 = 1;
  while (!exitMenu)
  {
    const char *string_list_gotoL1 = "Messier\nStar\nSolar System\nHerschel\nCoordinates\nHome\nPark";
    current_selection_L1 = display->UserInterfaceSelectionList(&buttonPad, sync ? "Sync" : "Goto", current_selection_L1, string_list_gotoL1);
    switch (current_selection_L1)
    {
    case 0:
      return;
    case 1:
      menuMessier(sync);
      break;
    case 2:
      menuStar(sync);
      break;
    case 3:
      menuSolarSys(sync);
      break;
    case 4:
      menuHerschel(sync);
      break;
    case 5:
      menuRADec(sync);
      break;
    case 6:
      exitMenu = DisplayMessageLX200(SyncGoHomeLX200(sync), false);
      break;
    case 7:
      exitMenu = DisplayMessageLX200(SyncGoParkLX200(sync), false);
      break;
      break;
    default:
      break;
    }
  }
}

void SmartHandController::menuSolarSys(bool sync)
{
  while (!exitMenu)
  {
    if (current_selection_SolarSys < 1) current_selection_SolarSys = 1;

    const char *string_list_SolarSyst = "Sun\nMercure\nVenus\nMars\nJupiter\nSaturn\nUranus\nNeptun\nMoon";
    current_selection_SolarSys = display->UserInterfaceSelectionList(&buttonPad, sync ? "Sync" : "Goto", current_selection_SolarSys, string_list_SolarSyst);
    if (current_selection_SolarSys == 0)
    {
      return;
    }
    current_selection_SolarSys > 3 ? current_selection_SolarSys : current_selection_SolarSys--;
    exitMenu = DisplayMessageLX200(SyncGotoPlanetLX200(sync, current_selection_SolarSys), false);
  }
}

void SmartHandController::menuHerschel(bool sync)
{
  while (!exitMenu)
  {
    current_selection_Herschel = display->UserInterfaceCatalog(&buttonPad, sync ? "Sync Herschel" : "Goto Herschel", current_selection_Herschel, HERSCHEL);
    if (current_selection_Herschel != 0)
    {
      exitMenu = DisplayMessageLX200(SyncGotoCatLX200(sync, HERSCHEL, current_selection_Herschel - 1), false);
    }
    else
      return;
  }
}

void SmartHandController::menuMessier(bool sync)
{
  while (!exitMenu)
  {
    current_selection_Messier = display->UserInterfaceCatalog(&buttonPad, sync ? "Sync Messier" : "Goto Messier", current_selection_Messier, MESSIER);
    if (current_selection_Messier != 0)
    {
      exitMenu = DisplayMessageLX200(SyncGotoCatLX200(sync, MESSIER, current_selection_Messier - 1), false);
    }
    else
      return;
  }
}

void SmartHandController::menuAlignment()
{
  const char *string_list_AlignmentL2 = "1-Star Align.\n""2-Star Align.\n""3-Star Align.\n""Show Corr.\n""Clear Corr.";
  current_selection_L2 = display->UserInterfaceSelectionList(&buttonPad, "Alignment", 0, string_list_AlignmentL2);
  switch (current_selection_L2)
  {
  case 0:
    return;
  case 1:
    if (SetLX200(":A1#") == LX200VALUESET)
    {
      telInfo.aliMode = Telescope::ALIM_ONE;
      telInfo.align = Telescope::ALI_SELECT_STAR_1;
    }
    else
    {
      DisplayMessage("Alignment", "Failed!", -1);
    }
    break;
  case 2:
    if (SetLX200(":A2#") == LX200VALUESET)
    {
      telInfo.aliMode = Telescope::ALIM_TWO;
      telInfo.align = Telescope::ALI_SELECT_STAR_1;
    }
    else
    {
      DisplayMessage("Alignment", "Failed!", -1);
    }
    break;
  case 3:
    if (SetLX200(":A3#") == LX200VALUESET)
    {
      telInfo.aliMode = Telescope::ALIM_THREE;
      telInfo.align = Telescope::ALI_SELECT_STAR_1;
    }
    else
    {
      DisplayMessage("Alignment", "Failed!", -1);
    }
    break;
  case 4:
    double val1;
    double val2;
    double val3;
    double val4;
    break;
  case 5:
    if (SetLX200(":SX0x#") == LX200VALUESET)
    {
      DisplayMessage("Alignment", "Reset!", -1);
    }
    break;
  default:
    break;
  }
  // Quit Menu
  exitMenu = true;

}

void SmartHandController::menuPier()
{
  telInfo.updateTel();
  uint8_t choice = ((uint8_t)telInfo.getPierState());
  choice = display->UserInterfaceSelectionList(&buttonPad, "Set Side of Pier", choice, "East\nWest");
  bool ok = false;
  if (choice)
  {
    if (choice == 1)
      ok = DisplayMessageLX200(SetLX200(":SmE#"));
    else
      ok = DisplayMessageLX200(SetLX200(":SmW#"));
    if (ok)
    {
      DisplayMessage("Please Sync", "with a Target", 1000);
      menuSyncGoto(true);
      DisplayMessageLX200(SetLX200(":SmN#"));
    }
  }
}

void SmartHandController::menuStar(bool sync)
{
  while (!exitMenu)
  {
    current_selection_Star = display->UserInterfaceCatalog(&buttonPad, sync ? "Sync Star" : "Goto Star", current_selection_Star, STAR);
    if (current_selection_Star != 0)
    {
      exitMenu = DisplayMessageLX200(SyncGotoCatLX200(sync, STAR, current_selection_Star - 1), false);
    }
    else
      return;
  }
}

bool SmartHandController::SelectStarAlign()
{
  buttonPad.setMenuMode();
  bool ok = false;
  telInfo.alignSelectedStar = display->UserInterfaceCatalog(&buttonPad, "select Star", telInfo.alignSelectedStar, STAR);
  if (telInfo.alignSelectedStar != 0)
  {
    ok = DisplayMessageLX200(SyncSelectedStarLX200(telInfo.alignSelectedStar), false);
  }
  buttonPad.setControlerMode();
  return ok;
}

void SmartHandController::menuRADec(bool sync)
{
  if (display->UserInterfaceInputValueRA(&buttonPad, &angleRA))
  {
    uint8_t vr1, vr2, vr3, vd2, vd3;
    short vd1;
    gethms(angleRA, vr1, vr2, vr3);
    if (display->UserInterfaceInputValueDec(&buttonPad, &angleDEC))
    {
      getdms(angleDEC, vd1, vd2, vd3);
      exitMenu = DisplayMessageLX200(SyncGotoLX200(sync, vr1, vr2, vr3, vd1, vd2, vd3));
    }
  }
}

void SmartHandController::menuTelSettings()
{
  buttonPad.setMenuMode();
  current_selection_L1 = 1;
  while (!exitMenu)
  {
    const char *string_list_SettingsL1 = "Display\n"/*"Alignment\n"*/"Date\n""Time\n""Set Park\n""Mount\n""Site\n""Limits\n""Main Unit Info"
#ifdef WIFI_ON
      "\nWifi"
#endif
      ;
    current_selection_L1 = display->UserInterfaceSelectionList(&buttonPad, "Telescope Settings", current_selection_L1, string_list_SettingsL1);
    switch (current_selection_L1)
    {
    case 0:
      exitMenu = true;
    case 1:
      menuDisplay();
      break;
    //case 2:
    //  menuAlignment();
    //  break;
    case 2:
      menuDate();
      break;
    case 3:
      menuUTCTime();
      break;
    case 4:
      DisplayMessageLX200(SetLX200(":hQ#"), false);
      break;
    case 5:
      menuMount();
      break;
    case 6:
      menuSite();
      break;
    case 7:
      menuLimits();
      break;
    case 8:
      menuMainUnitInfo();
      break;
#ifdef WIFI_ON
    case 9:
      menuWifi();
      break;
#endif
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
    const char *string_list_Mount = "Show Settings\n""Predefined\n""Mount type\n""Motor 1\n""Motor 2\n""Set Guide Rate\n""Set Max Rate\nAcceleration";
    current_selection_L2 = display->UserInterfaceSelectionList(&buttonPad, "Mount", current_selection_L2, string_list_Mount);
    switch (current_selection_L2)
    {
    case 0:
      return;
    case 1:
      DisplayMountSettings();
      break;
    case 2:
      menuPredefinedMount();
      break;
    case 3:
      menuMountType();
      break;
    case 4:
      menuMotor(1);
      break;
    case 5:
      menuMotor(2);
      break;
    case 6:
      menuGuideRate();
      break;
    case 7:
      menuMaxRate();
      break;
    case 8:
      menuAcceleration();
        break;
    default:
      break;
    }
  }

}

void SmartHandController::menuMountType()
{

  current_selection_L3 = telInfo.getMount();
  if (current_selection_L3 == 0)
  {
    DisplayLongMessage("Warning!", NULL, "No mount type", "was defined!", -1);
    current_selection_L3 = 1;
  }
  const char *string_list_Mount = "German Equatorial\n""Equatorial Fork"/*\n""Altazimutal\n""Altazimutal Fork"*/;
  current_selection_L3 = display->UserInterfaceSelectionList(&buttonPad, "Mount Type", current_selection_L3, string_list_Mount);
  if (current_selection_L3)
  {

    char out[10];
    sprintf(out, ":S!%u#", current_selection_L3);
    DisplayMessageLX200(SetLX200(out), false);
    delay(1000);
    Serial.end();
    exitMenu = true;
    powerCylceRequired = true;
    Serial.println(out);
  }
}

void SmartHandController::menuPredefinedMount()
{
  current_selection_L3 = 1;
  while (!exitMenu)
  {
    const char *string_list_Mount = "Alt Mount\n""Fornax\n""Knopf\n""Losmandy\n""Sideres85\n""Sky-Watcher\n""Takahashi\n""Vixen";
    current_selection_L3 = display->UserInterfaceSelectionList(&buttonPad, "Mount", current_selection_L3, string_list_Mount);
    switch (current_selection_L3)
    {
    case 0:
      return;
      break;
    case 1:
      menuAltMount();
      break;
    case 2:
      menuFornax();
      break;
    case 3:
      menuKnopf();
      break;
    case 4:
      menuLosmandy();
      break;
    case 5:
      menuSideres();
      break;
    case 6:
      break;
    case 7:
      menuTakahashi();
      break;
    case 8:
      menuVixen();
      break;
    default:
      break;
    }
  }

}

void SmartHandController::menuAltMount()
{
  current_selection_L4 = 1;
  while (!exitMenu)
  {
    const char *string_list_Mount = "Alt 5 Escap\n""Alt 6 Escap\n""Alt 7 Escap P520\n""Alt 5 Berger\n""Alt 6 Berger\n""Alt 7 Berger";
    current_selection_L4 = display->UserInterfaceSelectionList(&buttonPad, "Alt Mount", current_selection_L4, string_list_Mount);

    int gear1, steprot, gear2, currentH, currentL;
    if (current_selection_L4 > 3)
    {
      currentH = 70;
      currentL = 70;
      steprot = 48;
    }
    else
    {
      currentH = 120;
      currentL = 70;
      steprot = 100;
    }
    switch (current_selection_L4)
    {
    case 0:
      return;
    case 1:
      gear1 = 15625;
      gear2 = 16667;
      break;
    case 2:
      gear1 = 13750;
      gear2 = 16667;
      break;
    case 3:
      gear1 = 16875;
      gear2 = 17500;
      break;
    case 4:
      gear1 = 18750;
      gear2 = 18750;
      break;
    case 5:
      gear1 = 20625;
      gear2 = 18750;
      break;
    case 6:
      gear1 = 20250;
      gear2 = 19688;
      break;
    default:
      return;
    }
    writeDefaultMount(true, gear1, true, gear2, steprot, currentL, currentH);

  }
}

//void SmartHandController::menuAP() {}
void SmartHandController::menuFornax()
{
  current_selection_L4 = 1;
  while (!exitMenu)
  {
    const char *string_list_Mount = "Fornax 52\n""Fornax 102";
    current_selection_L4 = display->UserInterfaceSelectionList(&buttonPad, "Fornax Mount", current_selection_L4, string_list_Mount);

    switch (current_selection_L4)
    {
    case 0:
      return;
    case 1:
    case 2:
      writeDefaultMount(true, 864, true, 864, 200, 80, 120);
      break;
    }

  }
}
void SmartHandController::menuKnopf()
{
  current_selection_L4 = 1;
  while (!exitMenu)
  {
    const char *string_list_Mount = "MK70\n""MK70S\n""MK100S\n""MK140S";
    current_selection_L4 = display->UserInterfaceSelectionList(&buttonPad, "Knopf Mount", current_selection_L4, string_list_Mount);
    switch (current_selection_L4)
    {
    case 0:
      return;
    case 1:
      writeDefaultMount(true, 192 * 16, true, 192 * 16, 200, 100, 150);
      break;
    case 2:
      writeDefaultMount(true, 192 * 8, true, 192 * 8, 200, 100, 150);
      break;
    case 3:
      writeDefaultMount(true, 280 * 16, true, 280 * 16, 200, 100, 160);
      break;
    case 4:
      writeDefaultMount(true, 350 * 16, true, 350 * 16, 200, 120, 160);
      break;

    }
  }
}
void SmartHandController::menuLosmandy()
{
  current_selection_L4 = 1;
  while (!exitMenu)
  {
    const char *string_list_Mount = "G8 ESCAP P530\n""G11 ESCAP P530";
    current_selection_L4 = display->UserInterfaceSelectionList(&buttonPad, "Fornax Mount", current_selection_L4, string_list_Mount);

    switch (current_selection_L4)
    {
    case 0:
      return;
    case 1:
      writeDefaultMount(false, 2160, false, 2160, 100, 80, 120);
      break;
    case 2:
      writeDefaultMount(false, 4320, false, 4320, 100, 80, 120);
      break;
    }

  }
}
void SmartHandController::menuSideres()
{
  current_selection_L4 = 1;
  while (!exitMenu)
  {
    const char *string_list_Mount = "Sideres 85\n""Sideres 85 isel";
    current_selection_L4 = display->UserInterfaceSelectionList(&buttonPad, "Sideres Mount", current_selection_L4, string_list_Mount);

    int gear1, steprot1, gear2, steprot2;
    switch (current_selection_L4)
    {
    case 0:
      return;
    case 1:
      gear1 = 4608;
      gear2 = 4608;
      break;
    case 2:
      gear1 = 14400;
      gear2 = 14400;
      break;
    }
    writeDefaultMount(true, gear1, true, gear2, 200, 80, 120);
  }
}
//void SmartHandController::menuSkyWatcher() {}
void SmartHandController::menuTakahashi()
{
  current_selection_L4 = 1;
  while (!exitMenu)
  {
    const char *string_list_Mount = "EM11b kit FS2\n""EM200b kit FS2\n""EM200 kit FS2";
    current_selection_L4 = display->UserInterfaceSelectionList(&buttonPad, "Takahashi Mount", current_selection_L4, string_list_Mount);

    bool reverse1, reverse2;
    int gear1, steprot1, gear2, steprot2;

    switch (current_selection_L4)
    {
    case 0:
      return;
    case 1:
    case 2:
      reverse1 = true;
      reverse2 = false;
      gear1 = 1800;
      gear2 = 1800;
      break;
    case 3:
      reverse1 = true;
      reverse2 = false;
      gear1 = 2000;
      gear2 = 1800;
      break;
    default:
      break;
    }
    writeDefaultMount(reverse1, gear1, reverse2, gear2, 200, 75, 100);

  }

}


void SmartHandController::menuVixen()
{
  current_selection_L4 = 1;
  while (!exitMenu)
  {
    const char *string_list_Mount = "SPHINX SECM3\n""old ALTLUX\n""ATLUX ESCAP\n""SP-DX\n""GP-E\n""GP-2\n""GP-DX\n""GP-D2";
    current_selection_L4 = display->UserInterfaceSelectionList(&buttonPad, "Vixen Mount", current_selection_L4, string_list_Mount);
    int gearbox, steprot, teeth, currentL, currentH;
    int totgear1, totgear2;
    bool valid = false;
    switch (current_selection_L4)
    {
    case 0:
      return;
    case 1:
      gearbox = 10;
      teeth = 180;
      totgear1 = gearbox* teeth;
      totgear2 = totgear1;
      steprot = 200;
      currentH = 100;
      currentH = 50;
      valid = true;
      break;
    case 2:
      totgear1 = 9600;
      totgear2 = 7200;
      steprot = 200;
      currentH = 40;
      currentL = 40;
      valid = true;
      break;
    case 3:
      totgear1 = 9600;
      totgear2 = 7200;
      steprot = 200;
      currentH = 180;
      currentL = 100;
      valid = true;
      break;
    default:
      int teeth = 144;
      valid = menuMotorKit(gearbox, steprot, currentH);
      currentL = 0.5 * currentH;
      totgear1 = gearbox* teeth;
      totgear2 = totgear1;
      break;
    }
    if (valid)
    {
      writeDefaultMount(false, totgear1, false, totgear2, steprot, currentL, currentH);
    }
  }
}

void SmartHandController::writeDefaultMount(const bool& r1, const int& ttgr1, const bool& r2, const int& ttgr2, const int& stprot, const int& cL, const int& cH)
{
  writeReverseLX200(1, r1);
  writeTotGearLX200(1, ttgr1);
  writeStepPerRotLX200(1, stprot);
  writeBacklashLX200(1, 0);
  writeMicroLX200(1, 4);
  writeLowCurrLX200(1, cL);
  writeHighCurrLX200(1, cH);


  writeReverseLX200(2, r2);
  writeTotGearLX200(2, ttgr2);
  writeStepPerRotLX200(2, stprot);
  writeBacklashLX200(2, 0);
  writeMicroLX200(2, 4);
  writeLowCurrLX200(2, cL);
  writeHighCurrLX200(2, cH);
  
  menuMaxRate();
  menuAcceleration();
  DisplayMotorSettings(1);
  DisplayMotorSettings(2);
  SyncGoHomeLX200(true);
  exitMenu = true;
}

bool SmartHandController::menuMotorKit(int& gearBox, int& stepRot, int& current)
{
  const char *string_list_Mount = "SECM3\n""ESCAP P530";
  uint choice = display->UserInterfaceSelectionList(&buttonPad, "Motor Kit", 0, string_list_Mount);
  switch (choice)
  {
  case 0:
    return false;
  case 1:
    gearBox = 10;
    stepRot = 200;
    current = 100;
    break;
  case 2:
    gearBox = 16;
    stepRot = 100;
    current = 100;
    break;
  }
  return true;
}

void SmartHandController::menuMotor(const uint8_t axis)
{
  current_selection_L3 = 1;

  while (current_selection_L3 != 0)
  {
    const char *string_list_Motor = "Show Settings\nRotation\nGear\n""Steps per Rotation\n""Micro Steps\n""Backlash\n""Low Current\n""High Current";
    current_selection_L3 = display->UserInterfaceSelectionList(&buttonPad, axis == 1 ? "Motor 1" : "Motor 2", current_selection_L3, string_list_Motor);
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
    if (display->UserInterfaceInputValueFloat(&buttonPad, "Acceleration", "", &acc, 0.1, 25, 4, 1, " deg."))
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
    if (display->UserInterfaceInputValueFloat(&buttonPad, "Max Rate", "", &maxrate, 32, 1000, 4, 0, ""))
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
    if (display->UserInterfaceInputValueFloat(&buttonPad, "Guide Rate", "", &guiderate, 0.1f, 1.f, 4u, 2u, "x"))
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
    const char *string_list_SiteL2 = "Latitude\n""Longitude\n""Elevation\n""Select Site";
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

void SmartHandController::menuUTCTime()
{
  long value;
  if (DisplayMessageLX200(GetTimeLX200(value)))
  {
    if (display->UserInterfaceInputValueUTCTime(&buttonPad, &value))
    {
      DisplayMessageLX200(SetTimeLX200(value), false);
    }
  }
}

void SmartHandController::menuFocuserAction()
{
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
  current_selection_L2 = 1;
  while (!exitMenu)
  {
    char menustxt[200] = {};
    strcat(menustxt, txt);
    focuserlocked ? strcat(menustxt, "Goto\nSync\nPark\nUnlock") : strcat(menustxt, "Goto\nSync\nPark\nLock");
    const char *string_list_Focuser = &menustxt[0];
    current_selection_L2 = display->UserInterfaceSelectionList(&buttonPad, "Focuser Action", current_selection_L2, string_list_Focuser);
    if (current_selection_L2 == 0)
    {
      exitMenu = true;
      break;
    }
    else if (current_selection_L2 - 1 < idx)
    {
      char cmd[15];
      sprintf(cmd, ":Fg%d#", idxs[current_selection_L2-1]);
      DisplayMessage("Goto", "Position", 1000);
      SetLX200(cmd);
      exitMenu = true;
    }
    else
    {
      switch (current_selection_L2 - idx)
      { 
      case 1:
      {
        if (display->UserInterfaceInputValueFloat(&buttonPad, "Goto Position", "", &FocuserPos, 0, 65535, 5, 0, ""))
        {
          char cmd[15];
          sprintf(cmd, ":FG %05d#", (int)(FocuserPos));
          DisplayMessage("Goto", "Position", 1000);
          SetLX200(cmd);
          exitMenu = true;
        }
        break;
      }
      case 2:
      {
        if (display->UserInterfaceInputValueFloat(&buttonPad, "Sync Position", "", &FocuserPos, 0, 65535, 5, 0, ""))
        {
          char cmd[15];
          sprintf(cmd, ":FS %05d#", (int)(FocuserPos));
          DisplayMessage("Synced", "at Position", 1000);
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
        focuserlocked = !focuserlocked;
        exitMenu = true;
        break;
      default:
        break;
      }
    }
  }
  buttonPad.setControlerMode();
}

void SmartHandController::menuFocuserConfig()
{
  char cmd[50];
  const char *string_list_Focuser = "Display Settings\nPark Position\nMax Position\nMin Speed\nMax Speed\nGoto Acc\nMan. Acc\nDeceleration";
  unsigned int sP, maxP, minS, maxS, cmdAcc, manAcc, manDec;
  float value;
  while (!exitMenu)
  {
    if (DisplayMessageLX200(readFocuserConfig(sP, maxP, minS, maxS, cmdAcc, manAcc, manDec)))
    {
      current_selection_FocuserConfig = display->UserInterfaceSelectionList(&buttonPad, "Focuser Settings", current_selection_FocuserConfig, string_list_Focuser);
      bool ValueSetRequested = false;
      switch (current_selection_FocuserConfig)
      {
      case 0:
        exitMenu = true;
        break;
      case 1:
      {
        char line1[32] = "";
        char line2[32] = "";
        char line3[32] = "";
        char line4[32] = "";
        sprintf(line1, "Focuser Settings");
        sprintf(line3, "Start Pos.: %05u", sP);
        sprintf(line4, "Max  Pos.: %05u", maxP);
        DisplayLongMessage(line1, line2, line3, line4, -1);
        line2[0] = 0;
        sprintf(line3, "min Speed: %03u", minS);
        sprintf(line4, "max Speed: %03u", maxS);
        DisplayLongMessage(line1, line2, line3, line4, -1);
        sprintf(line2, "Acc. cmd.: %03u", cmdAcc);
        sprintf(line3, "Acc. man.: %03u", manAcc);
        sprintf(line4, "Dec. man.: %03u", manDec);
        DisplayLongMessage(line1, line2, line3, line4, -1);
        break;
      }
      case 2:
      {
        value = sP;
        ValueSetRequested = display->UserInterfaceInputValueFloat(&buttonPad, "Park Position", "", &value, 0, 65535, 5, 0, "");
        sprintf(cmd, ":F0 %05d#", (int)(value));
        break;
      }
      case 3:
      {
        value = maxP;
        ValueSetRequested = display->UserInterfaceInputValueFloat(&buttonPad, "Max Position", "", &value, 0, 65535, 5, 0, "");
        sprintf(cmd, ":F1 %05d#", (int)(value));
        break;
      }
      case 4:
      {
        value = minS;
        ValueSetRequested = display->UserInterfaceInputValueFloat(&buttonPad, "Min Speed", "", &value, 1, 999, 5, 0, "");
        sprintf(cmd, ":F2 %03d#", (int)(value));
        break;
      }
      case 5:
      {
        value = maxS;
        ValueSetRequested = display->UserInterfaceInputValueFloat(&buttonPad, "Max Speed", "", &value, 1, 999, 5, 0, "");
        sprintf(cmd, ":F3 %03d#", (int)(value));
        break;
      }
      case 6:
      {
        value = cmdAcc;
        ValueSetRequested = display->UserInterfaceInputValueFloat(&buttonPad, "Acc. for Goto", "", &value, 1, 100, 5, 0, "");
        sprintf(cmd, ":F4 %03d#", (int)(value));
        break;
      }
      case 7:
      {
        value = manAcc;
        ValueSetRequested = display->UserInterfaceInputValueFloat(&buttonPad, "Acc. for Man.", "", &value, 1, 100, 5, 0, "");
        sprintf(cmd, ":F5 %03d#", (int)(value));
        break;
      }
      case 8:
      {
        value = manDec;
        ValueSetRequested = display->UserInterfaceInputValueFloat(&buttonPad, "Dec. for Man.", "", &value, 1, 100, 5, 0, "");
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
  const char *string_list_Focuser = "Display Settings\nResolution\nRotation\nMicroStep\nCurrent";
  unsigned int res, mu, curr;
  bool rev;
  float value;
  while (!exitMenu)
  {
    if (DisplayMessageLX200(readFocuserMotor(rev, mu, res, curr)))
    {
      current_selection_FocuserMotor = display->UserInterfaceSelectionList(&buttonPad, "Focuser Settings", current_selection_FocuserMotor, string_list_Focuser);
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
        sprintf(line1, "Motor Settings");
        sprintf(line3, "Resolution  : %03u", res);
        rev ? sprintf(line4, "Reversed Rotation") : sprintf(line2, "Direct Rotation");
        DisplayLongMessage(line1, line2, line3, line4, -1);
        sprintf(line3, "Micro.      : %03u", (unsigned int)pow(2, mu));
        sprintf(line4, "Current   : %05umA", curr * 10);
        DisplayLongMessage(line1, line2, line3, line4, -1);
        break;
      }
      case 2:
      {
        value = res;
        ValueSetRequested = display->UserInterfaceInputValueFloat(&buttonPad, "Incrementation", "", &value, 1, 512, 5, 0, " micro steps");
        sprintf(cmd, ":F8 %03d#", (int)(value));
        break;
        break;
      }
      case 3:
      {
        char * string_list = "Direct\nReversed";
        uint8_t choice = display->UserInterfaceSelectionList(&buttonPad, "Rotation", (uint8_t)rev + 1, string_list);
        if (choice)
        {
          rev = (bool)(choice - 1);
          sprintf(cmd, ":F7 %01d#", (int)(value));
          ValueSetRequested = true;
        }
        break;
      }
      case 4:
      {
        uint8_t microStep = mu;
        char * string_list_micro = "4\n8\n16\n32\n64\n128";
        uint8_t choice = microStep - 2 + 1;
        choice = display->UserInterfaceSelectionList(&buttonPad, "MicroStep", choice, string_list_micro);
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
        ValueSetRequested = display->UserInterfaceInputValueFloat(&buttonPad, "Current", "", &value, 1, 160, 10, 0, "0 mA");
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
  buttonPad.setMenuMode();
  const char *string_list_Focuser = "Config\nMotor";
  while (!exitMenu)
  {
    current_selection_L2 = display->UserInterfaceSelectionList(&buttonPad, "Focuser Settings", current_selection_L2, string_list_Focuser);
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
    default:
      break;
    }
  }
  buttonPad.setControlerMode();
}

void SmartHandController::menuDisplay()
{
  const char *string_list_Display = "Turn Off\nContrast\nSleep\nDeepSleep";
  current_selection_L2 = 1;
  while (!exitMenu)
  {
    current_selection_L2 = display->UserInterfaceSelectionList(&buttonPad, "Display", current_selection_L2, string_list_Display);
    switch (current_selection_L2)
    {
    case 0:
      return;
    case 1:
      DisplayMessage("Press Shift Key", "to turn on", 1500);
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
      if (display->UserInterfaceInputValueInteger(&buttonPad, "Low Contrast", "after ", &displayT1, 3, 255, 3, "0 sec"))
      {
        EEPROM.write(EEPROM_T1, displayT1);
        EEPROM.commit();
      }
      break;
    }
    case 4:
    {
      if (display->UserInterfaceInputValueInteger(&buttonPad, "Turn display off", "after ", &displayT2, displayT1, 255, 3, "0 sec"))
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

void SmartHandController::menuContrast()
{
  const char *string_list_Display = "Min\nLow\nHigh\nMax";
  current_selection_L3 = 1;

  current_selection_L3 = display->UserInterfaceSelectionList(&buttonPad, "Set Contrast", current_selection_L3, string_list_Display);
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

void SmartHandController::menuDate()
{
  char out[20];
  if (DisplayMessageLX200(GetLX200(":GC#", out, sizeof(out))))
  {
    char* pEnd;
    uint8_t month = strtol(&out[0], &pEnd, 10);
    uint8_t day = strtol(&out[3], &pEnd, 10);
    uint8_t year = strtol(&out[6], &pEnd, 10);
    if (display->UserInterfaceInputValueDate(&buttonPad, "Date", year, month, day))
    {
      sprintf(out, ":SC%02d/%02d/%02d#", month, day, year);
      DisplayMessageLX200(SetLX200(out), false);
    }
  }
}

void SmartHandController::menuLatitude()
{
  int degree, minute;
  if (DisplayMessageLX200(GetLatitudeLX200(degree, minute)))
  {
    long angle = degree * 60;
    degree > 0 ? angle += minute : angle -= minute;
    angle *= 60;
    if (display->UserInterfaceInputValueLatitude(&buttonPad, &angle))
    {
      char cmd[20];
      angle /= 60;
      minute = abs(angle % 60);
      degree = angle / 60;
      sprintf(cmd, ":St%+03d*%02d#", degree, minute);
      DisplayMessageLX200(SetLX200(cmd), false);
    }
  }
}

void SmartHandController::menuLongitude()
{
  int degree, minute;
  if (DisplayMessageLX200(GetLongitudeLX200(degree, minute)))
  {
    long angle = degree * 60;
    degree > 0 ? angle += minute : angle -= minute;
    angle *= 60;
    if (display->UserInterfaceInputValueLongitude(&buttonPad, &angle))
    {
      char cmd[20];
      angle /= 60;
      minute = abs(angle) % 60;
      degree = angle / 60;
      sprintf(cmd, ":Sg%+04d*%02d#", degree, minute);
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
    if (display->UserInterfaceInputValueFloat(&buttonPad, "Site Elevation", "", &alt, -200, 8000, 2, 0, " meters"))
    {
      sprintf(out, ":Se%+04d#", (int)alt);
      DisplayMessageLX200(SetLX200(out), false);
    }
  }
}

void SmartHandController::menuLimits()
{
  const char *string_list_LimitsL2 = "Horizon\n""Overhead\n""Meridian E\n""Meridian W\n""Under Pole";
  current_selection_L3 = 1;
  while (current_selection_L3 != 0)
  {
    current_selection_L3 = display->UserInterfaceSelectionList(&buttonPad, "Limits", current_selection_L3, string_list_LimitsL2);
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
  const char *string_list = "Show Version\nReboot\nReset to factory";
  current_selection_L2 = 1;
  while (current_selection_L2 != 0)
  {
    current_selection_L2 = display->UserInterfaceSelectionList(&buttonPad, "Main Unit Info", 1, string_list);
    switch (current_selection_L2)
    {
    case 0:
      return;
    case 1:
      char out1[20];
      char out2[20];
      if (DisplayMessageLX200(GetLX200(":GVN#", out1, 20)) && DisplayMessageLX200(GetLX200(":GVD#", out2, 20)) )
      { 
        out1[strlen(out1) - 1] = 0;
        out2[strlen(out2) - 1] = 0;
        DisplayMessage(out1, out2 , -1);
      }
      break;
    case 2:
      DisplayMessageLX200(SetLX200(":$!#"), false);
      delay(500);
      powerCylceRequired = true;
      return;
    case 3:
      if (display->UserInterfaceMessage(&buttonPad, "Reset", "To", "Factory?", "NO\nYES") == 2)
      {
        DisplayMessageLX200(SetLX200(":$$#"), false);
        delay(500);
        powerCylceRequired = true;
        return;
      }
      break;
    default:
      break;
    }
  }
}


#ifdef WIFI_ON
void SmartHandController::menuWifi()
{
  const char *string_list = buttonPad.isWifiOn() ? "Turn Wifi off\nShow Password\nSelect Mode\nShow IP\nReset to factory" : "Turn Wifi on\nShow Password\nReset to factory";
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
      powerCylceRequired = true;
      break;
    case 2:
      DisplayMessage("masterPassword is", "password", -1);
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
      DisplayMessage("IP Adress is", iptxt, -1);
      break;
    }
    case 5:
      if (display->UserInterfaceMessage(&buttonPad, "Reset", "To", "Factory?", "NO\nYES")==2)
      {
        EEPROM_writeInt(0, 0);
        EEPROM.commit();
        powerCylceRequired = true;
        exitMenu = true;
        return;
      }
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
    strcat(menustxt, "AccesPoint");
    const char *string_list_WifiMode = &menustxt[0];
    selected_item = display->UserInterfaceSelectionList(&buttonPad, "Wifi Interface", selected_item, string_list_WifiMode);
    if (selected_item == 0)
    {
      return;
    }
    else
    {
      buttonPad.setWifiMode(idxs[selected_item - 1]);
      powerCylceRequired = true;
      exitMenu = true;
    }
  }
}
#endif

void SmartHandController::menuHorizon()
{
  char out[20];
  if (DisplayMessageLX200(GetLX200(":Gh#", out, sizeof(out))))
  {
    float angle = (float)strtol(&out[0], NULL, 10);
    if (display->UserInterfaceInputValueFloat(&buttonPad, "Horizon Limit", "", &angle, -10, 20, 2, 0, " degree"))
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
    if (display->UserInterfaceInputValueFloat(&buttonPad, "Overhead Limit", "", &angle, 60, 91, 2, 0, " degree"))
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
    if (display->UserInterfaceInputValueFloat(&buttonPad, "Max Hour Angle", "+-", &angle, 9, 12, 2, 1, " Hour"))
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
  char text[20];
  sprintf(text, "Meridian Limit X");
  text[15] = east ? 'E' : 'W';

  if (DisplayMessageLX200(GetLX200(cmd, out, sizeof(out))))
  {
    float angle = (float)strtol(&out[0], NULL, 10) / 4.0;
    if (display->UserInterfaceInputValueFloat(&buttonPad, "Meridian Limit", "", &angle, -45, 45, 2, 0, " degree"))
    {
      sprintf(cmd, ":SXEX:%+03d#", (int)(angle*4.0));
      cmd[4] = east ? '9' : 'A';
      DisplayMessageLX200(SetLX200(cmd), false);
    }
  }
}



void SmartHandController::DisplayMessage(const char* txt1, const char* txt2, int duration)
{
  uint8_t x;
  uint8_t y = 40;
  display->firstPage();
  do {
    if (txt2 != NULL)
    {
      y = 50;
      x = (display->getDisplayWidth() - display->getStrWidth(txt2)) / 2;
      display->drawStr(x, y, txt2);
      y = 25;
    }
    x = (display->getDisplayWidth() - display->getStrWidth(txt1)) / 2;
    display->drawStr(x, y, txt1);
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
  display->setFont(u8g2_font_helvR10_tr);
  uint8_t h = 15;
  uint8_t x = 0;
  uint8_t y = h;
  display->firstPage();
  do {
    y = h;
    x = (display->getDisplayWidth() - display->getStrWidth(txt1)) / 2;
    display->drawStr(x, y, txt1);
    y += h;
    if (txt2 != NULL)
    {
      x = 0;
      display->drawStr(x, y, txt2);
    }
    else
    {
      y -= 7;
    }
    y += 15;
    if (txt3 != NULL)
    {
      x = 0;
      display->drawStr(x, y, txt3);
    }

    y += 15;
    if (txt4 != NULL)
    {
      x = 0;
      display->drawStr(x, y, txt4);
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
      sprintf(text1, "LX200 Command");
      sprintf(text2, "has failed!");
    }
    else if (val == LX200SETVALUEFAILED)
    {
      sprintf(text1, "Set Value");
      sprintf(text2, "has failed!");
    }
    else if (val == LX200GETVALUEFAILED)
    {
      sprintf(text1, "Get Value");
      sprintf(text2, "has failed!");
    }
    else if (val == LX200SYNCFAILED)
    {
      sprintf(text1, "Sync");
      sprintf(text2, "has failed!");
    }
    else if (val == LX200SETTARGETFAILED)
    {
      sprintf(text1, "Set Target");
      sprintf(text2, "has failed!");
    }
    else if (val == LX200BELOWHORIZON)
    {
      sprintf(text1, "Target is");
      sprintf(text2, "Below Horizon!");
    }
    else if (val == LX200_ERR_MOTOR_FAULT)
    {
      sprintf(text1, "Telescope Motor");
      sprintf(text2, "Fault!");
    }
    else if (val == LX200_ERR_ALT)
    {
      sprintf(text1, "Telescope is");
      sprintf(text2, "Below Horizon!");
    }
    else if (val == LX200_ERR_LIMIT_SENSE)
    {
      sprintf(text1, "Telescope exceed");
      sprintf(text2, "Sensor limits");
    }
    else if (val == LX200_ERR_DEC)
    {
      sprintf(text1, "Telescope exceed");
      sprintf(text2, "DEC limits");
    }
    else if (val == LX200_ERR_AZM)
    {
      sprintf(text1, "Telescope exceed");
      sprintf(text2, "AZM limits");
    }
    else if (val == LX200_ERR_UNDER_POLE)
    {
      sprintf(text1, "Telescope exceed");
      sprintf(text2, "Under pole limits");
    }
    else if (val == LX200_ERR_MERIDIAN)
    {
      sprintf(text1, "Telescope exceed");
      sprintf(text2, "Meridian limits");
    }
    else if (val == LX200_ERR_SYNC)
    {
      sprintf(text1, "Telescope is");
      sprintf(text2, "Outside Limits");
    }
    else if (val == LX200NOOBJECTSELECTED)
    {
      sprintf(text1, "No Object");
      sprintf(text2, "Selected!");
    }
    else if (val == LX200PARKED)
    {
      sprintf(text1, "Telescope");
      sprintf(text2, "is Parked!");
    }
    else if (val == LX200BUSY)
    {
      sprintf(text1, "Telescope");
      sprintf(text2, "is busy!");
    }
    else if (val == LX200LIMITS)
    {
      sprintf(text1, "Target");
      sprintf(text2, "outside limits");
    }
    else if (val == LX200UNKOWN)
    {
      sprintf(text1, "Unkown");
      sprintf(text2, "Error");
    }
    else if (val == LX200GOPARK_FAILED)
    {
      sprintf(text1, "Telecope");
      sprintf(text2, "Can't Park");
    }
    else if (val == LX200GOHOME_FAILED)
    {
      sprintf(text1, "Telecope");
      sprintf(text2, "Can't go Home");
    }
    else
    {
      sprintf(text1, "Error");
      sprintf(text2, "-1");
    }
    DisplayMessage(text1, text2, -1);
  }
  else if (!silentOk)
  {
    time = 1000;
    if (val == LX200OK)
    {
      sprintf(text1, "LX200 Command");
      sprintf(text2, "Done!");
    }
    else if (val == LX200VALUESET)
    {
      sprintf(text1, "Value");
      sprintf(text2, "Set!");
    }
    else if (val == LX200VALUEGET)
    {
      sprintf(text1, "Value");
      sprintf(text2, "Get!");
    }
    else if (val == LX200SYNCED)
    {
      sprintf(text1, "Telescope");
      sprintf(text2, "Synced!");
    }
    else if (val == LX200GOINGTO)
    {
      sprintf(text1, "Slew to");
      sprintf(text2, "Target");
    }
    else if (val == LX200GOPARK)
    {
      sprintf(text1, "Slew to");
      sprintf(text2, "Park");
    }
    else if (val == LX200GOHOME)
    {
      sprintf(text1, "Slew to");
      sprintf(text2, "Home");
    }
    DisplayMessage(text1, text2, time);
  }
  return isOk(val);
}

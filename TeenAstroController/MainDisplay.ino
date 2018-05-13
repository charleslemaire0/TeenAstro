#define MY_BORDER_SIZE 1

#define icon_width 16
#define icon_height 16

static unsigned char wifi_bits[] U8X8_PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0x80, 0x20, 0x40, 0x4e, 0x00, 0x11,
  0x00, 0x04, 0x00, 0x04, 0x00, 0x04, 0xfe, 0x7f, 0x02, 0x40, 0xda, 0x5f,
  0xda, 0x5f, 0x02, 0x40, 0xfe, 0x7f, 0x00, 0x00 };

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


void update_main(u8g2_t *u8g2, u8g2_uint_t page)
{
  display.setFont(u8g2_font_helvR12_te);
  u8g2_uint_t line_height = u8g2_GetAscent(u8g2) - u8g2_GetDescent(u8g2) + MY_BORDER_SIZE;
  u8g2_uint_t step1 = u8g2_GetUTF8Width(u8g2, "44");
  u8g2_uint_t step2 = u8g2_GetUTF8Width(u8g2, "4") + 1;
  telInfo.connected = true;
  telInfo.updateTel();
  if (telInfo.connected == false)
  {
    return;
  }
  if (telInfo.hasTelStatus && telInfo.align != Telescope::ALI_OFF)
  {
    Telescope::TrackState curT = telInfo.getTrackingState();
    if (curT != Telescope::TRK_SLEWING && (telInfo.align == Telescope::ALI_SLEW_STAR_1 || telInfo.align == Telescope::ALI_SLEW_STAR_2 || telInfo.align == Telescope::ALI_SLEW_STAR_3))
    {
      telInfo.align = static_cast<Telescope::AlignState>(telInfo.align + 1);
    }
    page = 3;
  }
  else if (page == 0)
  {
    telInfo.updateRaDec();
  }
  else if (page == 1)
  {
    telInfo.updateAzAlt();
  }
  else
  {
    telInfo.updateTime();
  }
  u8g2_FirstPage(u8g2);

  do
  {
    u8g2_uint_t x = u8g2_GetDisplayWidth(u8g2);
    int k = 0;
    if (wifiOn)
      display.drawXBMP(0, 0, icon_width, icon_height, wifi_bits);

    if (telInfo.hasTelStatus)
    {
      Telescope::ParkState curP = telInfo.getParkState();
      Telescope::TrackState curT = telInfo.getTrackingState();
      if (curP == Telescope::PRK_PARKED)
      {
        display.drawXBMP(x - icon_width, 0, icon_width, icon_height, parked_bits);
        x -= icon_width + 1;
      }
      else if (curP == Telescope::PRK_PARKING)
      {
        display.drawXBMP(x - icon_width, 0, icon_width, icon_height, parking_bits);
        x -= icon_width + 1;
      }
      else if (telInfo.atHome())
      {
        display.drawXBMP(x - icon_width, 0, icon_width, icon_height, home_bits);
        x -= icon_width + 1;
      }
      else
      {
        if (curT == Telescope::TRK_SLEWING)
        {
          display.drawXBMP(x - icon_width, 0, icon_width, icon_height, sleewing_bits);
          x -= icon_width + 1;
        }
        else if (curT == Telescope::TRK_ON)
        {
          display.drawXBMP(x - icon_width, 0, icon_width, icon_height, tracking_S_bits);
          x -= icon_width + 1;
        }
        else if (curT == Telescope::TRK_OFF)
        {
          display.drawXBMP(x - icon_width, 0, icon_width, icon_height, no_tracking_bits);
          x -= icon_width + 1;
        }

        if (curP == Telescope::PRK_FAILED)
        {
          display.drawXBMP(x - icon_width, 0, icon_width, icon_height, parkingFailed_bits);
          x -= icon_width + 1;
        }
        if (telInfo.hasPierInfo)
        {
          Telescope::PierState CurP = telInfo.getPierState();
          if (CurP == Telescope::PIER_E)
          {
            display.drawXBMP(x - icon_width, 0, icon_width, icon_height, E_bits);
            x -= icon_width + 1;
          }
          else if (CurP == Telescope::PIER_W)
          {
            display.drawXBMP(x - icon_width, 0, icon_width, icon_height, W_bits);
            x -= icon_width + 1;
          }
 
        }
        if (telInfo.align != Telescope::ALI_OFF)
        {
          if (telInfo.aliMode == Telescope::ALIM_ONE)
            display.drawXBMP(x - icon_width, 0, icon_width, icon_height, align1_bits);
          else if (telInfo.aliMode == Telescope::ALIM_TWO)
            display.drawXBMP(x - icon_width, 0, icon_width, icon_height, align2_bits);
          else if (telInfo.aliMode == Telescope::ALIM_THREE)
            display.drawXBMP(x - icon_width, 0, icon_width, icon_height, align3_bits);
          x -= icon_width + 1;
        }

        if (telInfo.isGuiding())
        {
          display.drawXBMP(x - icon_width, 0, icon_width, icon_height, guiding_bits);
          x -= icon_width + 1;
        }

      }
      switch (telInfo.getError())
      {
      case Telescope::ERR_MOTOR_FAULT:
        display.drawXBMP(x - icon_width, 0, icon_width, icon_height, ErrMf_bits);
        x -= icon_width + 1;
        break;
      case  Telescope::ERR_ALT:
        display.drawXBMP(x - icon_width, 0, icon_width, icon_height, ErrHo_bits);
        x -= icon_width + 1;
        break;
      case Telescope::ERR_DEC:
        display.drawXBMP(x - icon_width, 0, icon_width, icon_height, ErrDe_bits);
        x -= icon_width + 1;
        break;
      case Telescope::ERR_UNDER_POLE:
        display.drawXBMP(x - icon_width, 0, icon_width, icon_height, ErrUp_bits);
        x -= icon_width + 1;
        break;
      case Telescope::ERR_MERIDIAN:
        display.drawXBMP(x - icon_width, 0, icon_width, icon_height, ErrMe_bits);
        x -= icon_width + 1;
        break;
      default:
        break;
      }
   
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

        drawRA(u8g2, x, y, Rah, Ram, Ras);
        u8g2_DrawUTF8(u8g2, 0, y, "RA");

        y += line_height + 4;
        u8g2_DrawUTF8(u8g2, 0, y, "Dec");
        drawDec(u8g2, x, y, decsign, decdeg, decmin, decsec);

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
        drawAz(u8g2, x, y, Azdeg, Azm, Azs);

        y += line_height + 4;
        x = startpos;
        x = u8g2_GetDisplayWidth(u8g2);

        drawDec(u8g2, x, y, Altsign, Altdeg, Altmin, Altsec);
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
        drawRA(u8g2, x, y, Rah, Ram, Ras);

        y += line_height + 4;
        memcpy(Rah, telInfo.TempSideral, 2);
        Rah[2] = '\0';
        memcpy(Ram, &telInfo.TempSideral[3], 2);
        Ram[2] = '\0';
        memcpy(Ras, &telInfo.TempSideral[6], 2);
        Ras[2] = '\0';
        u8g2_DrawUTF8(u8g2, 0, y, "Sideral");
        drawRA(u8g2, x, y, Rah, Ram, Ras);
      }
    }
    else if (page == 3)
    {
      int idx = telInfo.alignSelectedStar - 1;
      const byte* cat_letter = NULL;
      const byte*  cat_const = NULL;
      cat_letter = &Star_letter[idx];
      cat_const = &Star_constellation[idx];
      u8g2_uint_t y = 36;
      char txt[20];
      
      if ((telInfo.align-1)%3 == 1)
      {
        sprintf(txt, "Slew to Star %u", (telInfo.align -1) / 3 + 1);
      }
      else if ((telInfo.align - 1) % 3 == 2)
      {
        sprintf(txt, "Recenter Star %u", (telInfo.align - 1) / 3 + 1);
      }
      u8g2_DrawUTF8(u8g2, 0, y, txt);
      y += line_height + 4;
      const uint8_t* myfont = u8g2->font;
      u8g2_SetFont(u8g2, u8g2_font_unifont_t_greek);
      u8g2_DrawGlyph(u8g2, 0, y, 944 + *cat_letter);
      u8g2_SetFont(u8g2, myfont);
      u8g2_DrawUTF8(u8g2, 16, y, constellation_txt[*cat_const - 1]);
    }

  } while (u8g2_NextPage(u8g2));
  lastpageupdate = millis();
}

u8g2_uint_t drawRA(u8g2_t *u8g2, u8g2_uint_t x, u8g2_uint_t y, const char* Rah, const char* Ram, const char* Ras)
{
  u8g2_uint_t step0 = u8g2_GetUTF8Width(u8g2, "00");
  u8g2_uint_t step1 = u8g2_GetUTF8Width(u8g2, "00") + u8g2_GetUTF8Width(u8g2, "0") / 3;
  u8g2_uint_t step2 = u8g2_GetUTF8Width(u8g2, "0") - u8g2_GetUTF8Width(u8g2, "0") / 3;
  if (x == u8g2_GetDisplayWidth(u8g2))
  {
    x -= step0 + 1;
    u8g2_DrawUTF8(u8g2, x, y, Ras);
    x -= step2;
    u8g2_DrawUTF8(u8g2, x, y, ":");
    x -= step1;
    u8g2_DrawUTF8(u8g2, x, y, Ram);
    x -= step2;
    u8g2_DrawUTF8(u8g2, x, y, ":");
    x -= step1;
    u8g2_DrawUTF8(u8g2, x, y, Rah);
  }
  else
  {
    x += u8g2_GetUTF8Width(u8g2, "+") + u8g2_GetUTF8Width(u8g2, "0") / 3;
    u8g2_DrawUTF8(u8g2, x, y, Rah);
    x += step1;
    u8g2_DrawUTF8(u8g2, x, y, ":");
    x += step2;
    u8g2_DrawUTF8(u8g2, x, y, Ram);
    x += step1;
    u8g2_DrawUTF8(u8g2, x, y, ":");
    x += step2;
    x += u8g2_DrawUTF8(u8g2, x, y, Ras);
    return x;
  }

}

u8g2_uint_t drawDec(u8g2_t *u8g2, u8g2_uint_t x, u8g2_uint_t y, const char* decsign, const char* decdeg, const char* decmin, const char* decsec)
{
  char DEGREE_SYMBOL[] = { 0xB0, '\0' };
  u8g2_uint_t step1 = u8g2_GetUTF8Width(u8g2, "00");
  u8g2_uint_t step2 = u8g2_GetUTF8Width(u8g2, "0");
  if (x == u8g2_GetDisplayWidth(u8g2))
  {
    x -= step1 + 1;
    u8g2_DrawUTF8(u8g2, x, y, decsec);
    x -= step2 - u8g2_GetUTF8Width(u8g2, " ") / 2;
    u8g2_DrawUTF8(u8g2, x, y, "'");
    x -= step1 + u8g2_GetUTF8Width(u8g2, " ") / 2;
    u8g2_DrawUTF8(u8g2, x, y, decmin);
    x -= step2;
    u8g2_DrawUTF8(u8g2, x, y, DEGREE_SYMBOL);
    x -= step1;
    u8g2_DrawUTF8(u8g2, x, y, decdeg);
    x -= u8g2_GetUTF8Width(u8g2, "+");
    if (decsign[0] == '-')
    {
      x += u8g2_GetUTF8Width(u8g2, "0") / 3;
      u8g2_DrawUTF8(u8g2, x, y, decsign);
    }
    else
    {
      x -= u8g2_GetUTF8Width(u8g2, "0") / 3;
      u8g2_DrawUTF8(u8g2, x, y, decsign);
    }
  }
  else
  {
    if (decsign[0] == '-')
    {
      x += u8g2_GetUTF8Width(u8g2, "0") / 3;
      u8g2_DrawUTF8(u8g2, x, y, decsign);
    }
    else
    {
      u8g2_DrawUTF8(u8g2, x, y, decsign);
      x += u8g2_GetUTF8Width(u8g2, "0") / 3;
    }
    x += u8g2_GetUTF8Width(u8g2, "+");
    u8g2_DrawUTF8(u8g2, x, y, decdeg);
    x += step1;
    u8g2_DrawUTF8(u8g2, x, y, DEGREE_SYMBOL);
    x += step2;
    u8g2_DrawUTF8(u8g2, x, y, decmin);
    x += step1 + u8g2_GetUTF8Width(u8g2, " ") / 2;
    u8g2_DrawUTF8(u8g2, x, y, "'");
    x += step2 - u8g2_GetUTF8Width(u8g2, " ") / 2;
    x += u8g2_DrawUTF8(u8g2, x, y, decsec);
  }

}

u8g2_uint_t drawAz(u8g2_t *u8g2, u8g2_uint_t x, u8g2_uint_t y, const char* Azdeg, const char* Azmin, const char* Azsec)
{
  char DEGREE_SYMBOL[] = { 0xB0, '\0' };
  u8g2_uint_t step0 = u8g2_GetUTF8Width(u8g2, "000");
  u8g2_uint_t step1 = u8g2_GetUTF8Width(u8g2, "00");
  u8g2_uint_t step2 = u8g2_GetUTF8Width(u8g2, "0");
  if (x == u8g2_GetDisplayWidth(u8g2))
  {
    x -= step1 + 1;
    u8g2_DrawUTF8(u8g2, x, y, Azsec);
    x -= step2 - u8g2_GetUTF8Width(u8g2, " ") / 2;
    u8g2_DrawUTF8(u8g2, x, y, "'");
    x -= step1 + u8g2_GetUTF8Width(u8g2, " ") / 2;
    u8g2_DrawUTF8(u8g2, x, y, Azmin);
    x -= step2;
    u8g2_DrawUTF8(u8g2, x, y, DEGREE_SYMBOL);
    x -= step0;
    u8g2_DrawUTF8(u8g2, x, y, Azdeg);
  }
  else
  {
    u8g2_DrawUTF8(u8g2, x, y, Azdeg);
    x += step1 + u8g2_GetUTF8Width(u8g2, "0");
    u8g2_DrawUTF8(u8g2, x, y, DEGREE_SYMBOL);
    x += step2;
    u8g2_DrawUTF8(u8g2, x, y, Azmin);
    x += step1 + u8g2_GetUTF8Width(u8g2, " ") / 2;
    u8g2_DrawUTF8(u8g2, x, y, "'");
    x += step2 - u8g2_GetUTF8Width(u8g2, " ") / 2;
    x += u8g2_DrawUTF8(u8g2, x, y, Azsec);
  }


}

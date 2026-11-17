#include "SmartController.h"
#include "SHC_text.h"
#include <EEPROM.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Same plates as the web Reticule page, from 1995 on. Arcminutes from the pole.
// dial: 0 = 24 h (12 at the top), 1 = 12 h (0 at the top), 2 = clock every 10 min, 3 = every 15 min.
// flags: south year circles, northern plate only, custom arcminutes.

static const uint8_t RF_SOUTH = 1;
static const uint8_t RF_NORTH = 2;
static const uint8_t RF_CUSTOM = 4;

struct RetPlate {
  char name[16];
  uint8_t dial;
  uint8_t flags;
  int16_t year[8];
  int16_t dash;
  int16_t tick[4];
  int16_t arc10[6];
  int16_t arcS10[4];
  int16_t line10[6];
  int16_t epoch;
};

static const RetPlate kPlates[] PROGMEM = {
  {"Tak 2010-50", 0, RF_SOUTH, {2010, 2030, 2050, 0, 0, 0, 0, 0}, 2030, {0, 0, 0, 0}, {0, 0, 0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0, 0, 0}, 0},
  {"Tak 2000-30", 0, 0, {2000, 2010, 2020, 2030, 0, 0, 0, 0}, 0, {0, 0, 0, 0}, {0, 0, 0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0, 0, 0}, 0},
  {"Konus 00-20", 0, 0, {2000, 2010, 2020, 0, 0, 0, 0, 0}, 0, {0, 0, 0, 0}, {0, 0, 0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0, 0, 0}, 0},
  {"AP RAPAS", 0, RF_SOUTH, {2010, 2020, 2030, 2040, 0, 0, 0, 0}, 0, {0, 0, 0, 0}, {0, 0, 0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0, 0, 0}, 0},
  {"SW 2012-32", 1, RF_NORTH, {2012, 2020, 2028, 0, 0, 0, 0, 0}, 0, {2016, 2024, 2032, 0}, {0, 0, 0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0, 0, 0}, 0},
  {"iOptron", 2, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0, {0, 0, 0, 0}, {360, 400, 440, 0, 0, 0}, {600, 650, 700, 0}, {0, 0, 0, 0, 0, 0}, 0},
  {"iOptron 15m", 3, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0, {0, 0, 0, 0}, {360, 400, 440, 0, 0, 0}, {600, 650, 700, 0}, {0, 0, 0, 0, 0, 0}, 0},
  {"40-60'", 0, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0, {0, 0, 0, 0}, {400, 450, 500, 550, 600, 0}, {0, 0, 0, 0}, {0, 0, 0, 0, 0, 0}, 0},
  {"EXOS-2", 0, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0, {0, 0, 0, 0}, {0, 0, 0, 0, 0, 0}, {0, 0, 0, 0}, {400, 450, 500, 550, 600, 0}, 0},
  {"Epoch 2000", 0, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0, {0, 0, 0, 0}, {440, 0, 0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0, 0, 0}, 0},
  {"Cel. 1995", 0, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0, {0, 0, 0, 0}, {458, 0, 0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0, 0, 0}, 0},
  {"PASILL 95", 0, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0, {0, 0, 0, 0}, {460, 0, 0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0, 0, 0}, 0},
  {"ES 2005", 0, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0, {0, 0, 0, 0}, {0, 0, 0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0, 0, 0}, 2005},
  {"Custom", 0, RF_CUSTOM, {0, 0, 0, 0, 0, 0, 0, 0}, 0, {0, 0, 0, 0}, {0, 0, 0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0, 0, 0}, 0},
};

static const uint8_t kPlateCount = sizeof(kPlates) / sizeof(kPlates[0]);

static uint8_t s_plate = 0;
static uint8_t s_arc = 44;
static bool s_loaded = false;
static double s_lat = 0;
static double s_lonW = 0;
static bool s_siteOk = false;
static unsigned long s_siteMs = 0;

static RetPlate plateAt(uint8_t i)
{
  RetPlate p;
  memcpy_P(&p, &kPlates[i], sizeof(p));
  return p;
}

static int nzCount(const int16_t* a, int cap)
{
  int n = 0;
  while (n < cap && a[n] != 0) n++;
  return n;
}

static void loadReticle()
{
  if (s_loaded) return;
  uint8_t p = EEPROM.read(EEPROM_RETICLE_PLATE);
  uint8_t a = EEPROM.read(EEPROM_RETICLE_ARC);
  if (p < kPlateCount) s_plate = p;
  if (a >= 20 && a <= 80) s_arc = a;
  s_loaded = true;
}

static void saveReticle()
{
  EEPROM.write(EEPROM_RETICLE_PLATE, s_plate);
  EEPROM.write(EEPROM_RETICLE_ARC, s_arc);
  EEPROM.commit();
}

static double norm24(double h)
{
  h = fmod(h, 24.0);
  if (h < 0) h += 24.0;
  return h;
}

static double frac1(double x)
{
  x -= floor(x);
  if (x < 0) x += 1.0;
  return x;
}

static double jdOf(int y, int m, int d, double hf)
{
  int a = (14 - m) / 12;
  int yy = y + 4800 - a;
  int mm = m + 12 * a - 3;
  int jdn = d + (153 * mm + 2) / 5 + 365 * yy + yy / 4 - yy / 100 + yy / 400 - 32045;
  return (double)jdn - 0.5 + hf / 24.0;
}

struct Equ { double ra; double dec; };

static Equ precess(double raH, double decD, double jd)
{
  double s = (jd - 2451545.0) / 36525.0;
  double as = M_PI / 648000.0;
  double z = (2306.2181 * s + 0.30188 * s * s + 0.017998 * s * s * s) * as;
  double ze = (2306.2181 * s + 1.09468 * s * s + 0.018203 * s * s * s) * as;
  double th = (2004.3109 * s - 0.42665 * s * s - 0.041833 * s * s * s) * as;
  double ra = raH * M_PI / 12.0;
  double dec = decD * M_PI / 180.0;
  double A = cos(dec) * sin(ra + z);
  double B = cos(th) * cos(dec) * cos(ra + z) - sin(th) * sin(dec);
  double C = sin(th) * cos(dec) * cos(ra + z) + cos(th) * sin(dec);
  if (C > 1) C = 1;
  if (C < -1) C = -1;
  Equ e;
  e.ra = norm24((atan2(A, B) + ze) * 12.0 / M_PI);
  e.dec = asin(C) * 180.0 / M_PI;
  return e;
}

static double polMin(double dec)
{
  return (90.0 - fabs(dec)) * 60.0;
}

static double gmstHours(double jd)
{
  double d = jd - 2400000.5;
  double n = floor(d);
  double f = d - n;
  double t = (n - 51544.5) / 36525.0;
  return 6.697374558 + 24.0 * f * 1.0027379093
    + (8640184.812866 + (0.093104 - 6.2e-6 * t) * t) * t / 3600.0;
}

static double lstOf(double jd, double lonEast)
{
  return 24.0 * frac1((gmstHours(jd) + lonEast / 15.0) / 24.0);
}

static float yearArc(int year, bool south)
{
  Equ e = precess(south ? 21.146111 : 2.5301944, south ? -88.956389 : 89.2641667, jdOf(year, 1, 1, 0));
  return (float)polMin(e.dec);
}

struct Mk { float arc; uint8_t kind; uint8_t dash; };

static int buildMarks(const RetPlate& p, bool south, Mk* m, int cap)
{
  int n = 0;
  int ny = nzCount(p.year, 8);
  const bool yearSouth = south && (p.flags & RF_SOUTH);
  for (int i = 0; i < ny && n < cap; i++)
  {
    m[n].arc = yearArc(p.year[i], yearSouth);
    m[n].kind = 0;
    m[n].dash = (p.dash != 0 && p.year[i] == p.dash) ? 1 : 0;
    n++;
  }
  int nt = nzCount(p.tick, 4);
  for (int i = 0; i < nt && n < cap; i++)
  {
    m[n].arc = yearArc(p.tick[i], false);
    m[n].kind = 1;
    m[n].dash = 0;
    n++;
  }
  const int16_t* arcs = p.arc10;
  int na = nzCount(p.arc10, 6);
  if (south && nzCount(p.arcS10, 4) > 0)
  {
    arcs = p.arcS10;
    na = nzCount(p.arcS10, 4);
  }
  for (int i = 0; i < na && n < cap; i++)
  {
    m[n].arc = arcs[i] / 10.0f;
    m[n].kind = 0;
    m[n].dash = 0;
    n++;
  }
  int nl = nzCount(p.line10, 6);
  for (int i = 0; i < nl && n < cap; i++)
  {
    m[n].arc = p.line10[i] / 10.0f;
    m[n].kind = 2;
    m[n].dash = 0;
    n++;
  }
  if (p.epoch != 0 && n < cap)
  {
    m[n].arc = yearArc(p.epoch, south);
    m[n].kind = 0;
    m[n].dash = 0;
    n++;
  }
  if ((p.flags & RF_CUSTOM) && n < cap)
  {
    m[n].arc = (float)s_arc;
    m[n].kind = 0;
    m[n].dash = 0;
    n++;
  }
  return n;
}

static int iround(double v)
{
  return (int)(v >= 0 ? v + 0.5 : v - 0.5);
}

static void haPt(int cx, int cy, float r, float ha, int* x, int* y)
{
  double th = ((double)ha * 15.0 + 90.0) * M_PI / 180.0;
  *x = iround((double)cx - (double)r * cos(th));
  *y = iround((double)cy + (double)r * sin(th));
}

static void ring(u8g2_t* u, int cx, int cy, int r)
{
  if (r < 1) return;
  u8g2_DrawCircle(u, cx, cy, r, U8G2_DRAW_ALL);
}

bool SmartHandController::reticuleEditArc()
{
  uint8_t v = s_arc;
  if (!display->UserInterfaceInputValueInteger(&buttonPad, T_PLATE, "", &v, 20, 80, 2, "'"))
    return false;
  s_arc = v;
  saveReticle();
  return true;
}

void SmartHandController::reticulePickPlate()
{
  loadReticle();
  char list[320];
  size_t n = 0;
  list[0] = 0;
  for (uint8_t i = 0; i < kPlateCount; i++)
  {
    RetPlate p = plateAt(i);
    int w = snprintf(list + n, sizeof(list) - n, "%s%s", n ? "\n" : "", p.name);
    if (w < 0 || n + (size_t)w >= sizeof(list)) break;
    n += (size_t)w;
  }
  uint8_t sel = display->UserInterfaceSelectionList(&buttonPad, T_PLATE, s_plate + 1, list);
  if (sel == 0 || sel > kPlateCount) return;
  s_plate = (uint8_t)(sel - 1);
  RetPlate p = plateAt(s_plate);
  if (p.flags & RF_CUSTOM) reticuleEditArc();
  saveReticle();
}

void SmartHandController::drawReticule()
{
  unsigned long now = millis();
  unsigned long wait = s_siteOk ? 30000UL : 5000UL;
  if (s_siteMs == 0 || (now - s_siteMs) >= wait)
  {
    double lat = 0, lon = 0;
    if (m_client->getLatitude(lat) == LX200_VALUEGET && m_client->getLongitude(lon) == LX200_VALUEGET)
    {
      s_lat = lat;
      s_lonW = lon;
      s_siteOk = true;
    }
    s_siteMs = millis();
  }
  ta_MountStatus.updateAllState();

  RetPlate plate = plateAt(s_plate);
  bool south = s_siteOk && s_lat < 0.0;
  bool noStar = south && ((plate.flags & RF_NORTH) || (plate.year[0] != 0 && (plate.flags & RF_SOUTH) == 0));

  bool haveUt = ta_MountStatus.hasInfoUTC();
  double jd = 0;
  if (haveUt)
  {
    double hf = ta_MountStatus.getUtcHour()
      + ta_MountStatus.getUtcMin() / 60.0
      + ta_MountStatus.getUtcSec() / 3600.0;
    jd = jdOf(2000 + ta_MountStatus.getUtcYear(), ta_MountStatus.getUtcMonth(), ta_MountStatus.getUtcDay(), hf);
  }
  bool haveLst = false;
  double lst = 0;
  if (ta_MountStatus.hasInfoSidereal())
  {
    lst = ta_MountStatus.getLstHoursCached();
    haveLst = true;
  }
  else if (haveUt && s_siteOk)
  {
    lst = lstOf(jd, -s_lonW);
    haveLst = true;
  }

  Equ star;
  star.ra = 0;
  star.dec = 89;
  float arc = 0;
  float ha = 0;
  bool drawStar = false;
  if (haveUt && s_siteOk && !noStar)
  {
    star = precess(south ? 21.146111 : 2.5301944, south ? -88.956389 : 89.2641667, jd);
    arc = (float)polMin(star.dec);
    if (haveLst)
    {
      ha = (float)norm24(lst - star.ra);
      drawStar = true;
    }
  }

  Mk marks[40];
  int nMarks = buildMarks(plate, south, marks, 40);
  float fit = 1;
  for (int i = 0; i < nMarks; i++)
    if (marks[i].arc > fit) fit = marks[i].arc;
  if (drawStar && arc > fit) fit = arc;

  const int cx = 30;
  const int cy = 31;
  const float maxR = 28;
  float ppm = maxR / fit;

  char clk[16];
  strcpy(clk, "--:--:--");
  char sub[22];
  sub[0] = 0;
  uint8_t hh = 0, mm = 0, ss = 0, mo = 0, dd = 0, yy = 0;
  if (ta_MountStatus.getLocalTimeCached(hh, mm, ss) && ta_MountStatus.getLocalDateCached(mo, dd, yy))
  {
    snprintf(clk, sizeof(clk), "%02u:%02u:%02u", hh, mm, ss);
    snprintf(sub, sizeof(sub), "%02u/%02u", mo, dd);
  }
  else if (haveUt)
  {
    snprintf(clk, sizeof(clk), "UT %02u:%02u", ta_MountStatus.getUtcHour(), ta_MountStatus.getUtcMin());
    snprintf(sub, sizeof(sub), "%02u/%02u", ta_MountStatus.getUtcMonth(), ta_MountStatus.getUtcDay());
  }

  char info[20];
  if (noStar)
    snprintf(info, sizeof(info), "%s", T_RET_NOSOUTH);
  else if (!s_siteOk)
    snprintf(info, sizeof(info), "%s", T_RET_NOSITE);
  else if (!drawStar)
    snprintf(info, sizeof(info), "--");
  else
    snprintf(info, sizeof(info), "%s %.1f'", south ? "Oct" : "Pol", arc);

  u8g2_t* u = display->getU8g2();
  display->firstPage();
  do
  {
    u8g2_SetClipWindow(u, 0, 0, 62, 64);
    u8g2_DrawPixel(u, cx, cy);
    int lastRing = -1;
    for (int i = 0; i < nMarks; i++)
    {
      float rpx = marks[i].arc * ppm;
      if (marks[i].kind == 0)
      {
        int ri = iround(rpx);
        if (lastRing >= 0 && ri > lastRing - 2 && ri < lastRing + 2) continue;
        lastRing = ri;
        ring(u, cx, cy, ri);
      }
      else if (marks[i].kind == 1)
      {
        static const float hrs[4] = {12, 6, 0, 18};
        for (int t = 0; t < 4; t++)
        {
          int x, y;
          haPt(cx, cy, rpx, hrs[t], &x, &y);
          u8g2_DrawPixel(u, x, y);
        }
      }
      else
      {
        int x, y;
        haPt(cx, cy, rpx, 12, &x, &y);
        u8g2_DrawPixel(u, x, y);
      }
    }
    if (drawStar)
    {
      int sx, sy;
      haPt(cx, cy, arc * ppm, ha, &sx, &sy);
      if (sx >= 0 && sy >= 0)
      {
        u8g2_SetDrawColor(u, 0);
        u8g2_DrawDisc(u, sx, sy, 3, U8G2_DRAW_ALL);
        u8g2_SetDrawColor(u, 1);
        u8g2_DrawDisc(u, sx, sy, 1, U8G2_DRAW_ALL);
      }
    }
    u8g2_SetMaxClipWindow(u);
    u8g2_SetFont(u, u8g2_font_helvR10_te);
    if (u8g2_GetUTF8Width(u, clk) > 64)
      u8g2_SetFont(u, u8g2_font_helvR08_te);
    u8g2_DrawUTF8(u, 64, 12, clk);
    u8g2_SetFont(u, u8g2_font_helvR08_te);
    u8g2_DrawUTF8(u, 64, 26, sub);
    u8g2_DrawUTF8(u, 64, 40, info);
    u8g2_DrawUTF8(u, 64, 52, plate.name);
    u8g2_DrawUTF8(u, 64, 63, plate.dial == 1 ? T_RET_0TOP : T_RET_12TOP);
  }
  while (display->nextPage());
}

void SmartHandController::showReticuleFinder()
{
  loadReticle();
  ta_MountStatus.updateAllState(true);
  const TeenAstroMountStatus::Mount mt = ta_MountStatus.getMount();
  if (mt == TeenAstroMountStatus::MOUNT_TYPE_ALTAZM || mt == TeenAstroMountStatus::MOUNT_TYPE_FORK_ALT)
  {
    DisplayMessage(T_RETICULE, T_EQONLY, 1200);
    return;
  }

  unsigned long lastDraw = 0;
  bool dirty = true;
  for (;;)
  {
    unsigned long now = millis();
    if (dirty || (now - lastDraw) >= 250)
    {
      drawReticule();
      lastDraw = now;
      dirty = false;
    }
    buttonPad.tickButtons();
    yield();
    if (eventbuttons[B_SHIFT] == E_CLICK || eventbuttons[B_f] == E_CLICK)
      break;
    if (eventbuttons[B_NORTH] == E_CLICK)
    {
      s_plate = (uint8_t)((s_plate + 1) % kPlateCount);
      saveReticle();
      dirty = true;
    }
    else if (eventbuttons[B_SOUTH] == E_CLICK)
    {
      s_plate = (uint8_t)((s_plate + kPlateCount - 1) % kPlateCount);
      saveReticle();
      dirty = true;
    }
    else if (eventbuttons[B_EAST] == E_CLICK)
    {
      m_client->reticuleBrightnessAdjust(true);
    }
    else if (eventbuttons[B_WEST] == E_CLICK)
    {
      m_client->reticuleBrightnessAdjust(false);
    }
    else if (eventbuttons[B_F] == E_CLICK)
    {
      RetPlate p = plateAt(s_plate);
      if ((p.flags & RF_CUSTOM) && reticuleEditArc())
        dirty = true;
    }
  }
  display->setFont(u8g2_font_helvR12_te);
}

void SmartHandController::menuReticule()
{
  loadReticle();
  uint8_t selection = 1;
  while (selection != 0)
  {
    selection = display->UserInterfaceSelectionList(&buttonPad, T_RETICULE, selection,
      T_FINDER "\n" T_PLATE "\n" T_BRIGHTER "\n" T_LESSBRIGHT);
    if (selection == 1)
      showReticuleFinder();
    else if (selection == 2)
      reticulePickPlate();
    else if (selection == 3)
      DisplayMessageLX200(m_client->reticuleBrightnessAdjust(true), false);
    else if (selection == 4)
      DisplayMessageLX200(m_client->reticuleBrightnessAdjust(false), false);
  }
}

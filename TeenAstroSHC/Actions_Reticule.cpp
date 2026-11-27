#include "SmartController.h"
#include "SHC_text.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// One generic plate: a circle at the pole star's true distance, 12 at the top.

static double s_lat = 0;
static double s_lonW = 0;
static bool s_siteOk = false;
static unsigned long s_siteMs = 0;

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

static int iround(double v)
{
  return (int)(v >= 0 ? v + 0.5 : v - 0.5);
}

// Local mean time: UT shifted by the site longitude. Longitude is positive west,
// the same sign as :Gg#, so LMT = UT - longitude/15. One degree is four minutes.
static void formatHms(char* out, size_t n, double hours)
{
  double h = norm24(hours);
  int total = (int)floor(h * 3600.0 + 0.5);
  if (total >= 86400) total -= 86400;
  if (total < 0) total += 86400;
  int H = total / 3600;
  int M = (total % 3600) / 60;
  int S = total % 60;
  snprintf(out, n, "%02d:%02d:%02d", H, M, S);
}

static void haPt(int cx, int cy, float r, float ha, int* x, int* y)
{
  double th = ((double)ha * 15.0 + 90.0) * M_PI / 180.0;
  *x = iround((double)cx - (double)r * cos(th));
  *y = iround((double)cy + (double)r * sin(th));
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

  bool south = s_siteOk && s_lat < 0.0;
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

  float arc = 40;
  float ha = 0;
  bool drawStar = false;
  if (haveUt && s_siteOk)
  {
    Equ star = precess(south ? 21.146111 : 2.5301944, south ? -88.956389 : 89.2641667, jd);
    arc = (float)polMin(star.dec);
    if (haveLst)
    {
      ha = (float)norm24(lst - star.ra);
      drawStar = true;
    }
  }

  const int cx = 30;
  const int cy = 31;
  const float maxR = 28;
  float ppm = maxR / (arc > 1 ? arc : 1);

  char clk[16];
  strcpy(clk, "--:--:--");
  char sub[22];
  sub[0] = 0;
  uint8_t hh = 0, mm = 0, ss = 0, mo = 0, dd = 0, yy = 0;
  bool civil = ta_MountStatus.getLocalTimeCached(hh, mm, ss) && ta_MountStatus.getLocalDateCached(mo, dd, yy);
  if (civil)
  {
    snprintf(clk, sizeof(clk), "%02u:%02u:%02u", hh, mm, ss);
    snprintf(sub, sizeof(sub), "%02u/%02u", mo, dd);
  }
  else if (haveUt)
  {
    snprintf(clk, sizeof(clk), "%02u:%02u:%02u", ta_MountStatus.getUtcHour(), ta_MountStatus.getUtcMin(), ta_MountStatus.getUtcSec());
    snprintf(sub, sizeof(sub), "UT %02u/%02u", ta_MountStatus.getUtcMonth(), ta_MountStatus.getUtcDay());
  }
  char lmt[16];
  strcpy(lmt, "--:--:--");
  if (haveUt && s_siteOk)
  {
    double uhf = ta_MountStatus.getUtcHour()
      + ta_MountStatus.getUtcMin() / 60.0
      + ta_MountStatus.getUtcSec() / 3600.0;
    formatHms(lmt, sizeof(lmt), uhf - s_lonW / 15.0);
  }
  char lineLt[24];
  char lineLmt[24];
  snprintf(lineLt, sizeof(lineLt), "%s %s", civil || !haveUt ? T_RET_LT : "UT", clk);
  snprintf(lineLmt, sizeof(lineLmt), "%s %s", T_RET_LMT, lmt);

  char info[20];
  if (!s_siteOk)
    snprintf(info, sizeof(info), "%s", T_RET_NOSITE);
  else if (!drawStar)
    snprintf(info, sizeof(info), "--");
  else
    snprintf(info, sizeof(info), "%s %.1f'", south ? "Oct" : "Pol", arc);

  u8g2_t* u = display->getU8g2();
  u8g2_SetFont(u, u8g2_font_helvR08_te);
  if (u8g2_GetUTF8Width(u, lineLt) > 64 && strlen(lineLt) > 3)
    lineLt[strlen(lineLt) - 3] = 0;
  if (u8g2_GetUTF8Width(u, lineLmt) > 64 && strlen(lineLmt) > 3)
    lineLmt[strlen(lineLmt) - 3] = 0;
  display->firstPage();
  do
  {
    u8g2_SetClipWindow(u, 0, 0, 62, 64);
    u8g2_DrawPixel(u, cx, cy);
    int ri = iround(arc * ppm);
    if (ri > 0)
      u8g2_DrawCircle(u, cx, cy, ri, U8G2_DRAW_ALL);
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
    u8g2_SetFont(u, u8g2_font_helvR08_te);
    u8g2_DrawUTF8(u, 64, 12, lineLt);
    u8g2_DrawUTF8(u, 64, 26, sub);
    u8g2_DrawUTF8(u, 64, 40, lineLmt);
    u8g2_DrawUTF8(u, 64, 52, info);
    u8g2_DrawUTF8(u, 64, 63, T_RET_12TOP);
  }
  while (display->nextPage());
}

void SmartHandController::showReticuleFinder()
{
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
    if (eventbuttons[B_EAST] == E_CLICK)
      m_client->reticuleBrightnessAdjust(true);
    else if (eventbuttons[B_WEST] == E_CLICK)
      m_client->reticuleBrightnessAdjust(false);
  }
  display->setFont(u8g2_font_helvR12_te);
}

void SmartHandController::menuReticule()
{
  uint8_t selection = 1;
  while (selection != 0)
  {
    selection = display->UserInterfaceSelectionList(&buttonPad, T_RETICULE, selection,
      T_FINDER "\n" T_BRIGHTER "\n" T_LESSBRIGHT);
    if (selection == 1)
      showReticuleFinder();
    else if (selection == 2)
      DisplayMessageLX200(m_client->reticuleBrightnessAdjust(true), false);
    else if (selection == 3)
      DisplayMessageLX200(m_client->reticuleBrightnessAdjust(false), false);
  }
}

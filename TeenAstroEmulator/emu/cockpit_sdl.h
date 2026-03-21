/*
 * cockpit_sdl.h -- MainUnit cockpit window (SDL2) with tabbed UI.
 *
 * Provides an exhaustive real-time view of the mount state organized
 * in 7 tabs:  Overview | Park/Home | Tracking | Guiding | Axes |
 *             Alignment | Site/Limits
 *
 * Requires SDL2.  Include after Mount.h / Application.h are visible.
 */
#pragma once

#include <SDL.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <TimeLib.h>

extern Mount mount;
extern siteDefinition localSite;
extern DateTimeTimers rtk;
extern timerLoop tlp;

namespace cockpit {

/* ================================================================== */
/*  Tiny 5×7 bitmap font                                              */
/* ================================================================== */
namespace font5x7 {
    static const int GW = 5, GH = 7;

    static uint8_t col(char c, int ci) {
        struct Gdef { char ch; uint8_t d[5]; };
        static const Gdef table[] = {
            {'0',{0x3E,0x51,0x49,0x45,0x3E}}, {'1',{0x00,0x42,0x7F,0x40,0x00}},
            {'2',{0x42,0x61,0x51,0x49,0x46}}, {'3',{0x21,0x41,0x45,0x4B,0x31}},
            {'4',{0x18,0x14,0x12,0x7F,0x10}}, {'5',{0x27,0x45,0x45,0x45,0x39}},
            {'6',{0x3C,0x4A,0x49,0x49,0x30}}, {'7',{0x01,0x71,0x09,0x05,0x03}},
            {'8',{0x36,0x49,0x49,0x49,0x36}}, {'9',{0x06,0x49,0x49,0x29,0x1E}},
            {'A',{0x7E,0x11,0x11,0x11,0x7E}}, {'B',{0x7F,0x49,0x49,0x49,0x36}},
            {'C',{0x3E,0x41,0x41,0x41,0x22}}, {'D',{0x7F,0x41,0x41,0x22,0x1C}},
            {'E',{0x7F,0x49,0x49,0x49,0x41}}, {'F',{0x7F,0x09,0x09,0x09,0x01}},
            {'G',{0x3E,0x41,0x49,0x49,0x7A}}, {'H',{0x7F,0x08,0x08,0x08,0x7F}},
            {'I',{0x00,0x41,0x7F,0x41,0x00}}, {'J',{0x20,0x40,0x41,0x3F,0x01}},
            {'K',{0x7F,0x08,0x14,0x22,0x41}}, {'L',{0x7F,0x40,0x40,0x40,0x40}},
            {'M',{0x7F,0x02,0x0C,0x02,0x7F}}, {'N',{0x7F,0x02,0x0C,0x20,0x7F}},
            {'O',{0x3E,0x41,0x41,0x41,0x3E}}, {'P',{0x7F,0x09,0x09,0x09,0x06}},
            {'Q',{0x3E,0x41,0x51,0x21,0x5E}}, {'R',{0x7F,0x09,0x19,0x29,0x46}},
            {'S',{0x26,0x49,0x49,0x49,0x32}}, {'T',{0x01,0x01,0x7F,0x01,0x01}},
            {'U',{0x3F,0x40,0x40,0x40,0x3F}}, {'V',{0x1F,0x20,0x40,0x20,0x1F}},
            {'W',{0x3F,0x40,0x30,0x40,0x3F}}, {'X',{0x63,0x14,0x08,0x14,0x63}},
            {'Y',{0x07,0x08,0x70,0x08,0x07}}, {'Z',{0x61,0x51,0x49,0x45,0x43}},
            {'a',{0x20,0x54,0x54,0x54,0x78}}, {'b',{0x7F,0x48,0x44,0x44,0x38}},
            {'c',{0x38,0x44,0x44,0x44,0x20}}, {'d',{0x38,0x44,0x44,0x48,0x7F}},
            {'e',{0x38,0x54,0x54,0x54,0x18}}, {'f',{0x08,0x7E,0x09,0x01,0x02}},
            {'g',{0x0C,0x52,0x52,0x52,0x3E}}, {'h',{0x7F,0x08,0x04,0x04,0x78}},
            {'i',{0x00,0x44,0x7D,0x40,0x00}}, {'j',{0x20,0x40,0x44,0x3D,0x00}},
            {'k',{0x7F,0x10,0x28,0x44,0x00}}, {'l',{0x00,0x41,0x7F,0x40,0x00}},
            {'m',{0x7C,0x04,0x18,0x04,0x78}}, {'n',{0x7C,0x08,0x04,0x04,0x78}},
            {'o',{0x38,0x44,0x44,0x44,0x38}}, {'p',{0x7C,0x14,0x14,0x14,0x08}},
            {'q',{0x08,0x14,0x14,0x18,0x7C}}, {'r',{0x7C,0x08,0x04,0x04,0x08}},
            {'s',{0x48,0x54,0x54,0x54,0x20}}, {'t',{0x04,0x3F,0x44,0x40,0x20}},
            {'u',{0x3C,0x40,0x40,0x20,0x7C}}, {'v',{0x1C,0x20,0x40,0x20,0x1C}},
            {'w',{0x3C,0x40,0x30,0x40,0x3C}}, {'x',{0x44,0x28,0x10,0x28,0x44}},
            {'y',{0x0C,0x50,0x50,0x50,0x3C}}, {'z',{0x44,0x64,0x54,0x4C,0x44}},
            {'+',{0x08,0x08,0x3E,0x08,0x08}}, {'-',{0x08,0x08,0x08,0x08,0x08}},
            {':',{0x00,0x36,0x36,0x00,0x00}}, {'.',{0x00,0x60,0x60,0x00,0x00}},
            {',',{0x00,0x50,0x30,0x00,0x00}},
            {'/',{0x20,0x10,0x08,0x04,0x02}}, {' ',{0,0,0,0,0}},
            {'(',{0x00,0x1C,0x22,0x41,0x00}}, {')',{0x00,0x41,0x22,0x1C,0x00}},
            {'=',{0x14,0x14,0x14,0x14,0x14}},
            {'*',{0x14,0x08,0x3E,0x08,0x14}},
            {'#',{0x14,0x7F,0x14,0x7F,0x14}},
            {'%',{0x23,0x13,0x08,0x64,0x62}},
            {'<',{0x08,0x14,0x22,0x41,0x00}}, {'>',{0x00,0x41,0x22,0x14,0x08}},
            {'[',{0x00,0x7F,0x41,0x41,0x00}}, {']',{0x00,0x41,0x41,0x7F,0x00}},
            {'_',{0x40,0x40,0x40,0x40,0x40}},
            {'|',{0x00,0x00,0x7F,0x00,0x00}},
            {'~',{0x04,0x02,0x04,0x08,0x04}},
            {'@',{0x3E,0x41,0x5D,0x55,0x4E}},
            {'!',{0x00,0x00,0x4F,0x00,0x00}},
            {'?',{0x02,0x01,0x51,0x09,0x06}},
            {'"',{0x00,0x07,0x00,0x07,0x00}},
            {'\'',{0x00,0x00,0x07,0x00,0x00}},
            {'^',{0x04,0x02,0x01,0x02,0x04}},
            {'{',{0x00,0x08,0x36,0x41,0x00}}, {'}',{0x00,0x41,0x36,0x08,0x00}},
        };
        for (int i = 0; i < (int)(sizeof(table)/sizeof(table[0])); i++) {
            if (table[i].ch == c)
                return (ci >= 0 && ci < 5) ? table[i].d[ci] : 0;
        }
        return 0;
    }
}

/* ================================================================== */
/*  Window / rendering state                                          */
/* ================================================================== */
static SDL_Window*   g_win  = nullptr;
static SDL_Renderer* g_ren  = nullptr;

static const int CW = 900, CH = 700;
static const int FONT_SC = 2;
static const int CHAR_W  = font5x7::GW * FONT_SC + FONT_SC;   /* 12 px */
static const int CHAR_H  = font5x7::GH * FONT_SC;             /* 14 px */
static const int LINE_H  = 20;
static const int PAD     = 12;
static const int TAB_H   = 30;

static unsigned long g_lastDraw = 0;

/* Tabs */
enum Tab { TAB_OVERVIEW, TAB_PARKHOME, TAB_TRACKING, TAB_GUIDING, TAB_AXES, TAB_ALIGNMENT, TAB_SITELIMITS, TAB_COUNT };
static const char* g_tabNames[] = { "Overview", "Park/Home", "Tracking", "Guiding", "Axes", "Alignment", "Site/Limits" };
static int g_activeTab = TAB_OVERVIEW;

/* Clickable regions */
struct CBtn { SDL_Rect r; const char* label; int id; };
static CBtn g_buttons[16];
static int  g_nButtons = 0;
static SDL_Rect g_tabRects[TAB_COUNT];

/* ================================================================== */
/*  Colors                                                            */
/* ================================================================== */
struct Col { uint8_t r,g,b; };
static const Col COL_BG        = {0x0F, 0x12, 0x1A};
static const Col COL_TAB_BG    = {0x1A, 0x1F, 0x2E};
static const Col COL_TAB_ACT   = {0x28, 0x3A, 0x55};
static const Col COL_TAB_TXT   = {0x88, 0xAA, 0xCC};
static const Col COL_TAB_ATXT  = {0xDD, 0xEE, 0xFF};
static const Col COL_TITLE     = {0x55, 0xBB, 0xFF};
static const Col COL_SECTION   = {0x44, 0x99, 0xDD};
static const Col COL_LABEL     = {0x77, 0x88, 0x99};
static const Col COL_VALUE     = {0xCC, 0xDD, 0xEE};
static const Col COL_GOOD      = {0x44, 0xDD, 0x88};
static const Col COL_WARN      = {0xFF, 0xCC, 0x33};
static const Col COL_BAD       = {0xFF, 0x55, 0x55};
static const Col COL_DIM       = {0x44, 0x55, 0x66};
static const Col COL_SEP       = {0x2A, 0x33, 0x44};
static const Col COL_BTN_BG    = {0x22, 0x44, 0x66};
static const Col COL_BTN_BR    = {0x44, 0x77, 0xAA};
static const Col COL_BTN_TXT   = {0xDD, 0xEE, 0xFF};

/* ================================================================== */
/*  Drawing primitives                                                */
/* ================================================================== */
static void setCol(Col c, uint8_t a = 0xFF) { SDL_SetRenderDrawColor(g_ren, c.r, c.g, c.b, a); }

static void drawText(int x, int y, const char* text, Col color) {
    SDL_SetRenderDrawColor(g_ren, color.r, color.g, color.b, 0xFF);
    for (int ci = 0; text[ci]; ci++) {
        int gx = x + ci * CHAR_W;
        for (int col = 0; col < font5x7::GW; col++) {
            uint8_t bits = font5x7::col(text[ci], col);
            for (int row = 0; row < font5x7::GH; row++) {
                if ((bits >> row) & 1) {
                    SDL_Rect px = { gx + col*FONT_SC, y + row*FONT_SC, FONT_SC, FONT_SC };
                    SDL_RenderFillRect(g_ren, &px);
                }
            }
        }
    }
}

static int textWidth(const char* t) { return (int)strlen(t) * CHAR_W; }

static void drawSep(int y) {
    setCol(COL_SEP);
    SDL_RenderDrawLine(g_ren, PAD, y, CW - PAD, y);
}

static int drawSection(int y, const char* title) {
    drawText(PAD, y, title, COL_SECTION);
    y += LINE_H;
    drawSep(y - 4);
    return y;
}

static const int VAL_X = PAD + 270;

static int drawKV(int y, const char* label, const char* value, Col valCol = COL_VALUE) {
    drawText(PAD + 8, y, label, COL_LABEL);
    drawText(VAL_X, y, value, valCol);
    return y + LINE_H;
}

static void drawButton(int x, int y, int w, int h, const char* label, int id) {
    int idx = g_nButtons++;
    if (idx >= 16) return;
    g_buttons[idx].r = { x, y, w, h };
    g_buttons[idx].label = label;
    g_buttons[idx].id = id;
    setCol(COL_BTN_BG);
    SDL_Rect r = { x, y, w, h };
    SDL_RenderFillRect(g_ren, &r);
    setCol(COL_BTN_BR);
    SDL_RenderDrawRect(g_ren, &r);
    int tw = textWidth(label);
    drawText(x + (w - tw)/2, y + (h - CHAR_H)/2, label, COL_BTN_TXT);
}

static void drawFilledRect(int x, int y, int w, int h, Col c) {
    setCol(c);
    SDL_Rect r = {x,y,w,h};
    SDL_RenderFillRect(g_ren, &r);
}

/* ================================================================== */
/*  Helper: format angles and numbers                                 */
/* ================================================================== */
static void fmtDeg(char* buf, int sz, double deg) {
    int sign = (deg < 0) ? -1 : 1;
    double ad = fabs(deg);
    int d = (int)ad;
    int m = (int)((ad - d) * 60.0);
    double s = ((ad - d) * 60.0 - m) * 60.0;
    snprintf(buf, sz, "%c%dd %02d' %05.2f\"", sign < 0 ? '-' : '+', d, m, s);
}

static void fmtHMS(char* buf, int sz, double hours) {
    while (hours < 0) hours += 24.0;
    while (hours >= 24.0) hours -= 24.0;
    int h = (int)hours;
    int m = (int)((hours - h) * 60.0);
    double s = ((hours - h) * 60.0 - m) * 60.0;
    snprintf(buf, sz, "%02dh %02dm %05.2fs", h, m, s);
}

/* ================================================================== */
/*  Tab: Overview                                                     */
/* ================================================================== */
static void drawTabOverview(int y0) {
    char buf[128];
    int y = y0;

    /* Title row */
    snprintf(buf, sizeof(buf), "TeenAstro MainUnit v%s  (Emulator)", FirmwareNumber);
    drawText(PAD, y, buf, COL_TITLE);
    y += LINE_H + 4;

    /* ---- Mount Config ---- */
    y = drawSection(y, "Mount Configuration");
    {
        const char* mtNames[] = { "Undefined", "GEM", "Fork", "AltAzm", "Fork-Alt" };
        int mt = (int)mount.config.identity.mountType;
        if (mt < 0 || mt > 4) mt = 0;
        y = drawKV(y, "Mount Type", mtNames[mt]);
    }
    {
        const char* flipNames[] = { "Never", "Align", "Always" };
        int mf = (int)mount.config.identity.meridianFlip;
        if (mf < 0 || mf > 2) mf = 0;
        y = drawKV(y, "Meridian Flip", flipNames[mf]);
    }
    {
        const char* ptNames[] = { "OFF", "RA/DEC", "Alt/Az" };
        int pt = (int)mount.config.peripherals.PushtoStatus;
        if (pt < 0 || pt > 2) pt = 0;
        y = drawKV(y, "Push-To", ptNames[pt]);
    }
    y = drawKV(y, "Focuser", mount.config.peripherals.hasFocuser ? "Connected" : "None",
               mount.config.peripherals.hasFocuser ? COL_GOOD : COL_DIM);
    y = drawKV(y, "GNSS", mount.config.peripherals.hasGNSS ? "Connected" : "None",
               mount.config.peripherals.hasGNSS ? COL_GOOD : COL_DIM);
    y += 4;

    /* ---- Quick Status ---- */
    y = drawSection(y, "Quick Status");
    {
        const char* trackModes[] = { "Star", "Sun", "Moon", "Target" };
        int mi = (int)mount.tracking.sideralMode;
        if (mi < 0 || mi > 3) mi = 0;
        snprintf(buf, sizeof(buf), "%s  (Mode: %s)",
            mount.tracking.sideralTracking ? "ON" : "OFF", trackModes[mi]);
        y = drawKV(y, "Tracking", buf,
            mount.tracking.sideralTracking ? COL_GOOD : COL_WARN);
    }
    {
        const char* parkNames[] = { "Unparked", "Parking...", "Parked" };
        int pi = (int)mount.parkHome.parkStatus;
        if (pi < 0 || pi > 2) pi = 0;
        Col pc = (pi == 2) ? COL_WARN : (pi == 1 ? Col{0xFF,0xAA,0x33} : COL_GOOD);
        y = drawKV(y, "Park Status", parkNames[pi], pc);
    }
    {
        const char* gotoNames[] = { "Idle", "GoTo EQ", "GoTo AltAz" };
        int gi = (int)mount.tracking.gotoState;
        if (gi < 0 || gi > 2) gi = 0;
        y = drawKV(y, "GoTo State", gotoNames[gi],
            gi > 0 ? COL_WARN : COL_VALUE);
    }
    {
        const char* guidNames[] = { "OFF", "Pulse", "ST4", "Recenter", "AtRate" };
        int gi = (int)mount.guiding.GuidingState;
        if (gi < 0 || gi > 4) gi = 0;
        y = drawKV(y, "Guiding", guidNames[gi],
            gi > 0 ? COL_GOOD : COL_DIM);
    }
    {
        const char* errNames[] = { "None", "Motor Fault", "Altitude", "Limit Sense",
                                   "Axis1", "Axis2", "Under Pole", "Meridian" };
        int ei = (int)mount.errors.lastError;
        if (ei < 0 || ei > 7) ei = 0;
        y = drawKV(y, "Last Error", errNames[ei],
            ei > 0 ? COL_BAD : COL_GOOD);
    }
    {
        const char* poleNames[] = { "Not Valid", "Under", "Over" };
        int ps = (int)mount.getPoleSide();
        if (ps < 0 || ps > 2) ps = 0;
        y = drawKV(y, "Pole Side", poleNames[ps]);
    }
    y += 4;

    /* ---- Current Coordinates ---- */
    y = drawSection(y, "Current Coordinates");
    {
        snprintf(buf, sizeof(buf), "%.6f", mount.targetCurrent.newTargetRA);
        y = drawKV(y, "Target RA (deg)", buf);
        snprintf(buf, sizeof(buf), "%.6f", mount.targetCurrent.newTargetDec);
        y = drawKV(y, "Target DEC (deg)", buf);
    }
    {
        snprintf(buf, sizeof(buf), "%.4f", mount.targetCurrent.currentAlt);
        y = drawKV(y, "Current Alt (deg)", buf);
        snprintf(buf, sizeof(buf), "%.4f", mount.targetCurrent.currentAzm);
        y = drawKV(y, "Current Azm (deg)", buf);
    }
    {
        double lst = rtk.LST();
        fmtHMS(buf, sizeof(buf), lst);
        y = drawKV(y, "LST", buf);
    }
    y += 8;

    /* ---- Buttons ---- */
    int bx = PAD;
    int bh = 28, bg = 10;
    drawButton(bx, y, 120, bh, mount.tracking.sideralTracking ? "Track OFF" : "Track ON", 100);
    bx += 120 + bg;
    drawButton(bx, y, 110, bh, mount.isParked() ? "Unpark" : "Park", 101);
    bx += 110 + bg;
    drawButton(bx, y, 80, bh, "Abort", 102);
    bx += 80 + bg;
    drawButton(bx, y, 140, bh, "Stop Guide", 103);
}

/* ================================================================== */
/*  Tab: Park / Home                                                  */
/* ================================================================== */
static void drawTabParkHome(int y0) {
    char buf[128];
    int y = y0;

    y = drawSection(y, "Park Status");
    {
        const char* parkNames[] = { "Unparked", "Parking...", "Parked" };
        int pi = (int)mount.parkHome.parkStatus;
        if (pi < 0 || pi > 2) pi = 0;
        Col pc = (pi == 2) ? COL_GOOD : (pi == 1 ? COL_WARN : COL_VALUE);
        y = drawKV(y, "Status", parkNames[pi], pc);
    }
    y = drawKV(y, "Park Saved", mount.parkHome.parkSaved ? "YES" : "NO",
               mount.parkHome.parkSaved ? COL_GOOD : COL_WARN);
    {
        if (mount.parkHome.parkSaved) {
            long a1 = mount.axes.staA1.pos;
            long a2 = mount.axes.staA2.pos;
            snprintf(buf, sizeof(buf), "A1: %ld steps   A2: %ld steps", a1, a2);
            y = drawKV(y, "Park Position", mount.isParked() ? buf : "(last parked pos in EEPROM)");
        } else {
            y = drawKV(y, "Park Position", "Not saved", COL_DIM);
        }
    }
    {
        const char* blNames[] = { "Init", "Move In", "Move Out", "Done" };
        int bi = (int)mount.parkHome.backlashStatus;
        if (bi < 0 || bi > 3) bi = 0;
        y = drawKV(y, "Backlash Phase", blNames[bi]);
    }
    snprintf(buf, sizeof(buf), "%u ms", mount.parkHome.slewSettleDuration);
    y = drawKV(y, "Settle Duration", buf);
    y = drawKV(y, "Settling", mount.parkHome.settling ? "YES" : "no",
               mount.parkHome.settling ? COL_WARN : COL_DIM);
    y += 8;

    y = drawSection(y, "Home Status");
    y = drawKV(y, "Home Saved", mount.parkHome.homeSaved ? "YES" : "NO",
               mount.parkHome.homeSaved ? COL_GOOD : COL_WARN);
    y = drawKV(y, "At Home", mount.parkHome.atHome ? "YES" : "no",
               mount.parkHome.atHome ? COL_GOOD : COL_DIM);
    y = drawKV(y, "Home Mount", mount.parkHome.homeMount ? "YES" : "no",
               mount.parkHome.homeMount ? COL_GOOD : COL_DIM);
    y += 8;

    y = drawSection(y, "User-Defined Target");
    {
        fmtDeg(buf, sizeof(buf), mount.targetCurrent.newTargetRA);
        y = drawKV(y, "Target RA", buf);
        fmtDeg(buf, sizeof(buf), mount.targetCurrent.newTargetDec);
        y = drawKV(y, "Target DEC", buf);
    }
    {
        fmtDeg(buf, sizeof(buf), mount.targetCurrent.newTargetAlt);
        y = drawKV(y, "Target Alt", buf);
        fmtDeg(buf, sizeof(buf), mount.targetCurrent.newTargetAzm);
        y = drawKV(y, "Target Azm", buf);
    }
    {
        const char* poleNames[] = { "Not Valid", "Under", "Over" };
        int ps = (int)mount.targetCurrent.newTargetPoleSide;
        if (ps < 0 || ps > 2) ps = 0;
        y = drawKV(y, "Target Pole Side", poleNames[ps]);
    }
    y += 8;

    int bx = PAD;
    drawButton(bx, y, 100, 28, mount.isParked() ? "Unpark" : "Park", 101);
}

/* ================================================================== */
/*  Tab: Tracking                                                     */
/* ================================================================== */
static void drawTabTracking(int y0) {
    char buf[128];
    int y = y0;

    y = drawSection(y, "Sidereal Tracking");
    y = drawKV(y, "Enabled", mount.tracking.sideralTracking ? "YES" : "NO",
               mount.tracking.sideralTracking ? COL_GOOD : COL_WARN);
    y = drawKV(y, "Last State", mount.tracking.lastSideralTracking ? "ON" : "OFF", COL_DIM);
    {
        const char* modes[] = { "Star", "Sun", "Moon", "Target" };
        int mi = (int)mount.tracking.sideralMode;
        if (mi < 0 || mi > 3) mi = 0;
        y = drawKV(y, "Mode", modes[mi]);
    }
    snprintf(buf, sizeof(buf), "%.4f", mount.tracking.siderealClockSpeed);
    y = drawKV(y, "Sidereal Clock", buf);
    {
        const char* tcNames[] = { "Unknown", "RA only", "RA+DEC" };
        int tc = (int)mount.tracking.trackComp;
        if (tc < -1 || tc > 2) tc = -1;
        y = drawKV(y, "Compensation", tcNames[tc + 1]);
    }
    y += 4;

    y = drawSection(y, "Tracking Rates");
    snprintf(buf, sizeof(buf), "%.6f arcsec/s", mount.tracking.RequestedTrackingRateHA);
    y = drawKV(y, "Requested HA Rate", buf);
    snprintf(buf, sizeof(buf), "%.6f arcsec/s", mount.tracking.RequestedTrackingRateDEC);
    y = drawKV(y, "Requested DEC Rate", buf);
    snprintf(buf, sizeof(buf), "%ld", mount.tracking.storedTrakingRateRA);
    y = drawKV(y, "Stored RA Rate", buf);
    snprintf(buf, sizeof(buf), "%ld", mount.tracking.storedTrakingRateDEC);
    y = drawKV(y, "Stored DEC Rate", buf);
    y += 4;

    y = drawSection(y, "Effective Axis Rates");
    snprintf(buf, sizeof(buf), "%.6f", mount.axes.staA1.CurrentTrackingRate);
    y = drawKV(y, "A1 Current Rate", buf);
    snprintf(buf, sizeof(buf), "%.6f", mount.axes.staA2.CurrentTrackingRate);
    y = drawKV(y, "A2 Current Rate", buf);
    snprintf(buf, sizeof(buf), "%.6f", mount.axes.staA1.RequestedTrackingRate);
    y = drawKV(y, "A1 Requested Rate", buf);
    snprintf(buf, sizeof(buf), "%.6f", mount.axes.staA2.RequestedTrackingRate);
    y = drawKV(y, "A2 Requested Rate", buf);
    y += 4;

    y = drawSection(y, "GoTo / Spiral");
    {
        const char* gotoNames[] = { "Idle", "GoTo EQ", "GoTo AltAz" };
        int gi = (int)mount.tracking.gotoState;
        if (gi < 0 || gi > 2) gi = 0;
        y = drawKV(y, "GoTo State", gotoNames[gi], gi > 0 ? COL_WARN : COL_VALUE);
    }
    y = drawKV(y, "Abort Slew", mount.tracking.abortSlew ? "ACTIVE" : "no",
               mount.tracking.abortSlew ? COL_BAD : COL_DIM);
    y = drawKV(y, "Spiral Active", mount.tracking.doSpiral ? "YES" : "no",
               mount.tracking.doSpiral ? COL_WARN : COL_DIM);
    snprintf(buf, sizeof(buf), "%.2f deg", mount.tracking.SpiralFOV);
    y = drawKV(y, "Spiral FOV", buf);
    y += 8;

    drawButton(PAD, y, 120, 28, mount.tracking.sideralTracking ? "Track OFF" : "Track ON", 100);
}

/* ================================================================== */
/*  Tab: Guiding                                                      */
/* ================================================================== */
static void drawTabGuiding(int y0) {
    char buf[128];
    int y = y0;

    y = drawSection(y, "Guiding State");
    {
        const char* guidNames[] = { "OFF", "Pulse", "ST4", "Recenter", "AtRate" };
        int gi = (int)mount.guiding.GuidingState;
        if (gi < 0 || gi > 4) gi = 0;
        Col gc = (gi > 0) ? COL_GOOD : COL_DIM;
        y = drawKV(y, "State", guidNames[gi], gc);
    }
    {
        const char* guidNames[] = { "OFF", "Pulse", "ST4", "Recenter", "AtRate" };
        int gi = (int)mount.guiding.lastGuidingState;
        if (gi < 0 || gi > 4) gi = 0;
        y = drawKV(y, "Last State", guidNames[gi], COL_DIM);
    }
    y += 4;

    y = drawSection(y, "Guide Rates");
    {
        const char* rateNames[] = { "RG (0)", "RC (1)", "RM (2)", "RS (3)", "RX (4)" };
        snprintf(buf, sizeof(buf), "%s  (index %d)", rateNames[(int)mount.guiding.activeGuideRate % 5],
            (int)mount.guiding.activeGuideRate);
        y = drawKV(y, "Active Rate", buf);
    }
    snprintf(buf, sizeof(buf), "Index %d", (int)mount.guiding.recenterGuideRate);
    y = drawKV(y, "Recenter Rate", buf);
    snprintf(buf, sizeof(buf), "%.6f", mount.guiding.pulseGuideRate);
    y = drawKV(y, "Pulse Guide Rate", buf);
    for (int i = 0; i < 5; i++) {
        char lbl[32];
        snprintf(lbl, sizeof(lbl), "Rate[%d]", i);
        snprintf(buf, sizeof(buf), "%.6f", mount.guiding.guideRates[i]);
        y = drawKV(y, lbl, buf);
    }
    snprintf(buf, sizeof(buf), "%.4f deg", mount.guiding.DegreesForAcceleration);
    y = drawKV(y, "Accel Distance", buf);
    y += 4;

    y = drawSection(y, "Guide Axis 1 (RA)");
    y = drawKV(y, "Status", mount.guiding.guideA1.isBusy() ? "BUSY" : "Idle",
               mount.guiding.guideA1.isBusy() ? COL_WARN : COL_DIM);
    y = drawKV(y, "Direction", mount.guiding.guideA1.isDirFW() ? "FW" :
               (mount.guiding.guideA1.isDirBW() ? "BW" : "-"), COL_VALUE);
    y = drawKV(y, "Braking", mount.guiding.guideA1.isBraking() ? "YES" : "no",
               mount.guiding.guideA1.isBraking() ? COL_WARN : COL_DIM);
    snprintf(buf, sizeof(buf), "%.6f", mount.guiding.guideA1.absRate);
    y = drawKV(y, "Abs Rate", buf);
    snprintf(buf, sizeof(buf), "%.6f", mount.guiding.guideA1.getRate());
    y = drawKV(y, "Eff Rate", buf);
    snprintf(buf, sizeof(buf), "%lu ms", mount.guiding.guideA1.duration);
    y = drawKV(y, "Duration", buf);
    y += 4;

    y = drawSection(y, "Guide Axis 2 (DEC)");
    y = drawKV(y, "Status", mount.guiding.guideA2.isBusy() ? "BUSY" : "Idle",
               mount.guiding.guideA2.isBusy() ? COL_WARN : COL_DIM);
    y = drawKV(y, "Direction", mount.guiding.guideA2.isDirFW() ? "FW" :
               (mount.guiding.guideA2.isDirBW() ? "BW" : "-"), COL_VALUE);
    y = drawKV(y, "Braking", mount.guiding.guideA2.isBraking() ? "YES" : "no",
               mount.guiding.guideA2.isBraking() ? COL_WARN : COL_DIM);
    snprintf(buf, sizeof(buf), "%.6f", mount.guiding.guideA2.absRate);
    y = drawKV(y, "Abs Rate", buf);
    snprintf(buf, sizeof(buf), "%.6f", mount.guiding.guideA2.getRate());
    y = drawKV(y, "Eff Rate", buf);
    snprintf(buf, sizeof(buf), "%lu ms", mount.guiding.guideA2.duration);
    y = drawKV(y, "Duration", buf);
    y += 8;

    drawButton(PAD, y, 140, 28, "Stop Guide", 103);
}

/* ================================================================== */
/*  Tab: Axes                                                         */
/* ================================================================== */
static void drawTabAxes(int y0) {
    char buf[128];
    int y = y0;

    /* Axis 1 */
    y = drawSection(y, "Axis 1 (RA / Azimuth)");
    snprintf(buf, sizeof(buf), "%ld", mount.axes.staA1.pos);
    y = drawKV(y, "Position (steps)", buf);
    snprintf(buf, sizeof(buf), "%.1f", mount.axes.staA1.target);
    y = drawKV(y, "Target (steps)", buf);
    snprintf(buf, sizeof(buf), "%ld", mount.axes.staA1.deltaTarget);
    y = drawKV(y, "Delta to Target", buf,
        abs(mount.axes.staA1.deltaTarget) > 10 ? COL_WARN : COL_VALUE);
    snprintf(buf, sizeof(buf), "%.4f", mount.axes.staA1.interval_Step_Cur);
    y = drawKV(y, "Step Interval", buf);
    snprintf(buf, sizeof(buf), "%.4f", mount.axes.staA1.interval_Step_Sid);
    y = drawKV(y, "Sidereal Interval", buf);
    snprintf(buf, sizeof(buf), "%.4f", mount.axes.staA1.acc);
    y = drawKV(y, "Acceleration", buf);
    snprintf(buf, sizeof(buf), "%.6f", mount.axes.staA1.fstep);
    y = drawKV(y, "Frac Steps/Tick", buf);
    y = drawKV(y, "Enabled", mount.axes.staA1.enable ? "YES" : "NO",
               mount.axes.staA1.enable ? COL_GOOD : COL_BAD);
    y = drawKV(y, "Fault", mount.axes.staA1.fault ? "FAULT" : "OK",
               mount.axes.staA1.fault ? COL_BAD : COL_GOOD);
    y = drawKV(y, "Direction", mount.axes.staA1.dir ? "FW (+)" : "BW (-)");
    snprintf(buf, sizeof(buf), "%d / %d", mount.axes.staA1.backlash_movedSteps, mount.axes.staA1.backlash_inSteps);
    y = drawKV(y, "Backlash Moved/In", buf);
    y = drawKV(y, "Backlash Active", mount.axes.staA1.backlash_correcting ? "YES" : "no",
               mount.axes.staA1.backlash_correcting ? COL_WARN : COL_DIM);
    y += 4;

    /* Axis 1 geometry */
    y = drawSection(y, "Axis 1 Geometry");
    snprintf(buf, sizeof(buf), "%ld", mount.axes.geoA1.stepsPerRot);
    y = drawKV(y, "Steps/Rotation", buf);
    snprintf(buf, sizeof(buf), "%.4f", mount.axes.geoA1.stepsPerDegree);
    y = drawKV(y, "Steps/Degree", buf);
    snprintf(buf, sizeof(buf), "%.6f", mount.axes.geoA1.stepsPerArcSecond);
    y = drawKV(y, "Steps/ArcSec", buf);
    snprintf(buf, sizeof(buf), "%ld .. %ld", mount.axes.geoA1.minAxis, mount.axes.geoA1.maxAxis);
    y = drawKV(y, "Limits (steps)", buf);
    snprintf(buf, sizeof(buf), "%.2f .. %.2f", mount.axes.geoA1.LimMinAxis, mount.axes.geoA1.LimMaxAxis);
    y = drawKV(y, "Limits (deg)", buf);
    snprintf(buf, sizeof(buf), "%ld", mount.axes.geoA1.poleDef);
    y = drawKV(y, "Pole Def (steps)", buf);
    snprintf(buf, sizeof(buf), "%ld", mount.axes.geoA1.homeDef);
    y = drawKV(y, "Home Def (steps)", buf);
    snprintf(buf, sizeof(buf), "%ld", mount.axes.geoA1.halfRot);
    y = drawKV(y, "Half Rotation", buf);
    y += 4;

    drawSep(y); y += 6;

    /* Axis 2 */
    y = drawSection(y, "Axis 2 (DEC / Altitude)");
    snprintf(buf, sizeof(buf), "%ld", mount.axes.staA2.pos);
    y = drawKV(y, "Position (steps)", buf);
    snprintf(buf, sizeof(buf), "%.1f", mount.axes.staA2.target);
    y = drawKV(y, "Target (steps)", buf);
    snprintf(buf, sizeof(buf), "%ld", mount.axes.staA2.deltaTarget);
    y = drawKV(y, "Delta to Target", buf,
        abs(mount.axes.staA2.deltaTarget) > 10 ? COL_WARN : COL_VALUE);
    snprintf(buf, sizeof(buf), "%.4f", mount.axes.staA2.interval_Step_Cur);
    y = drawKV(y, "Step Interval", buf);
    snprintf(buf, sizeof(buf), "%.4f", mount.axes.staA2.interval_Step_Sid);
    y = drawKV(y, "Sidereal Interval", buf);
    snprintf(buf, sizeof(buf), "%.4f", mount.axes.staA2.acc);
    y = drawKV(y, "Acceleration", buf);
    snprintf(buf, sizeof(buf), "%.6f", mount.axes.staA2.fstep);
    y = drawKV(y, "Frac Steps/Tick", buf);
    y = drawKV(y, "Enabled", mount.axes.staA2.enable ? "YES" : "NO",
               mount.axes.staA2.enable ? COL_GOOD : COL_BAD);
    y = drawKV(y, "Fault", mount.axes.staA2.fault ? "FAULT" : "OK",
               mount.axes.staA2.fault ? COL_BAD : COL_GOOD);
    y = drawKV(y, "Direction", mount.axes.staA2.dir ? "FW (+)" : "BW (-)");
    snprintf(buf, sizeof(buf), "%d / %d", mount.axes.staA2.backlash_movedSteps, mount.axes.staA2.backlash_inSteps);
    y = drawKV(y, "Backlash Moved/In", buf);
    y = drawKV(y, "Backlash Active", mount.axes.staA2.backlash_correcting ? "YES" : "no",
               mount.axes.staA2.backlash_correcting ? COL_WARN : COL_DIM);
    y += 4;

    y = drawSection(y, "Axis 2 Geometry");
    snprintf(buf, sizeof(buf), "%ld", mount.axes.geoA2.stepsPerRot);
    y = drawKV(y, "Steps/Rotation", buf);
    snprintf(buf, sizeof(buf), "%.4f", mount.axes.geoA2.stepsPerDegree);
    y = drawKV(y, "Steps/Degree", buf);
    snprintf(buf, sizeof(buf), "%.6f", mount.axes.geoA2.stepsPerArcSecond);
    y = drawKV(y, "Steps/ArcSec", buf);
    snprintf(buf, sizeof(buf), "%ld .. %ld", mount.axes.geoA2.minAxis, mount.axes.geoA2.maxAxis);
    y = drawKV(y, "Limits (steps)", buf);
    snprintf(buf, sizeof(buf), "%.2f .. %.2f", mount.axes.geoA2.LimMinAxis, mount.axes.geoA2.LimMaxAxis);
    y = drawKV(y, "Limits (deg)", buf);
    snprintf(buf, sizeof(buf), "%ld", mount.axes.geoA2.poleDef);
    y = drawKV(y, "Pole Def (steps)", buf);
    snprintf(buf, sizeof(buf), "%ld", mount.axes.geoA2.homeDef);
    y = drawKV(y, "Home Def (steps)", buf);
    snprintf(buf, sizeof(buf), "%ld", mount.axes.geoA2.halfRot);
    y = drawKV(y, "Half Rotation", buf);

    y += 4;

    /* Motors info */
    y = drawSection(y, "Motor Configuration");
    y = drawKV(y, "Motors Enabled", mount.motorsEncoders.enableMotor ? "YES" : "NO",
               mount.motorsEncoders.enableMotor ? COL_GOOD : COL_BAD);
    snprintf(buf, sizeof(buf), "Gear:%lu  Steps:%u  uStep:%d",
        mount.motorsEncoders.motorA1.gear, mount.motorsEncoders.motorA1.stepRot,
        (int)mount.motorsEncoders.motorA1.micro);
    y = drawKV(y, "A1 Mechanics", buf);
    snprintf(buf, sizeof(buf), "Rev:%s  Silent:%s  Hi:%umA  Lo:%umA",
        mount.motorsEncoders.motorA1.reverse ? "Y" : "N",
        mount.motorsEncoders.motorA1.silent ? "Y" : "N",
        mount.motorsEncoders.motorA1.highCurr, mount.motorsEncoders.motorA1.lowCurr);
    y = drawKV(y, "A1 Settings", buf);
    snprintf(buf, sizeof(buf), "Gear:%lu  Steps:%u  uStep:%d",
        mount.motorsEncoders.motorA2.gear, mount.motorsEncoders.motorA2.stepRot,
        (int)mount.motorsEncoders.motorA2.micro);
    y = drawKV(y, "A2 Mechanics", buf);
    snprintf(buf, sizeof(buf), "Rev:%s  Silent:%s  Hi:%umA  Lo:%umA",
        mount.motorsEncoders.motorA2.reverse ? "Y" : "N",
        mount.motorsEncoders.motorA2.silent ? "Y" : "N",
        mount.motorsEncoders.motorA2.highCurr, mount.motorsEncoders.motorA2.lowCurr);
    y = drawKV(y, "A2 Settings", buf);
    snprintf(buf, sizeof(buf), "A1:%d/%d  A2:%d/%d",
        mount.motorsEncoders.motorA1.backlashAmount, mount.motorsEncoders.motorA1.backlashRate,
        mount.motorsEncoders.motorA2.backlashAmount, mount.motorsEncoders.motorA2.backlashRate);
    y = drawKV(y, "Backlash Amt/Rate", buf);
    y = drawKV(y, "Reboot Pending", mount.motorsEncoders.reboot_unit ? "YES" : "no",
               mount.motorsEncoders.reboot_unit ? COL_BAD : COL_DIM);
}

/* ================================================================== */
/*  Tab: Alignment                                                    */
/* ================================================================== */
static void drawTabAlignment(int y0) {
    char buf[128];
    int y = y0;

    y = drawSection(y, "Alignment Status");
    y = drawKV(y, "Valid", mount.alignment.hasValid ? "YES" : "NO",
               mount.alignment.hasValid ? COL_GOOD : COL_WARN);
    snprintf(buf, sizeof(buf), "%d", (int)mount.alignment.maxAlignNumStar);
    y = drawKV(y, "Max Align Stars", buf);
    y = drawKV(y, "Auto by Sync", mount.alignment.autoAlignmentBySync ? "YES" : "no",
               mount.alignment.autoAlignmentBySync ? COL_GOOD : COL_DIM);
    {
        const char* phaseNames[] = { "Idle", "Select", "Slew", "Recenter" };
        int ap = (int)mount.alignment.alignPhase;
        if (ap < 0 || ap > 3) ap = 0;
        y = drawKV(y, "Phase", phaseNames[ap],
            ap > 0 ? COL_WARN : COL_VALUE);
    }
    snprintf(buf, sizeof(buf), "%d", (int)mount.alignment.alignStarNum);
    y = drawKV(y, "Current Star #", buf);
    y = drawKV(y, "Star Name", strlen(mount.alignment.alignStarName) > 0 ?
               mount.alignment.alignStarName : "(none)");
    y += 4;

    y = drawSection(y, "Coordinate Conversion (CoordConv)");
    y = drawKV(y, "Ready", mount.alignment.conv.isReady() ? "YES" : "NO",
               mount.alignment.conv.isReady() ? COL_GOOD : COL_WARN);
    snprintf(buf, sizeof(buf), "%d", (int)mount.alignment.conv.getRefs());
    y = drawKV(y, "Reference Stars", buf);
    if (mount.alignment.conv.isReady()) {
        snprintf(buf, sizeof(buf), "%.8f rad", mount.alignment.conv.getError());
        y = drawKV(y, "Alignment Error", buf,
            mount.alignment.conv.getError() < 0.001 ? COL_GOOD :
            (mount.alignment.conv.getError() < 0.01 ? COL_WARN : COL_BAD));
    } else {
        y = drawKV(y, "Alignment Error", "N/A (not ready)", COL_DIM);
    }
    y += 4;

    y = drawSection(y, "Transformation Matrix T");
    if (mount.alignment.conv.isReady()) {
        for (int r = 0; r < 3; r++) {
            snprintf(buf, sizeof(buf), "[%+.6f  %+.6f  %+.6f]",
                mount.alignment.conv.T[r][0], mount.alignment.conv.T[r][1], mount.alignment.conv.T[r][2]);
            char lbl[16]; snprintf(lbl, sizeof(lbl), "Row %d", r);
            y = drawKV(y, lbl, buf);
        }
    } else {
        y = drawKV(y, "(matrix)", "Not computed yet", COL_DIM);
    }
    y += 4;

    y = drawSection(y, "Inverse Matrix Tinv");
    if (mount.alignment.conv.isReady()) {
        for (int r = 0; r < 3; r++) {
            snprintf(buf, sizeof(buf), "[%+.6f  %+.6f  %+.6f]",
                mount.alignment.conv.Tinv[r][0], mount.alignment.conv.Tinv[r][1], mount.alignment.conv.Tinv[r][2]);
            char lbl[16]; snprintf(lbl, sizeof(lbl), "Row %d", r);
            y = drawKV(y, lbl, buf);
        }
    } else {
        y = drawKV(y, "(matrix)", "Not computed yet", COL_DIM);
    }
    y += 4;

    y = drawSection(y, "Refraction Settings");
    y = drawKV(y, "For Pole", mount.refraction.forPole ? "ON" : "OFF",
               mount.refraction.forPole ? COL_GOOD : COL_DIM);
    y = drawKV(y, "For GoTo", mount.refraction.forGoto ? "ON" : "OFF",
               mount.refraction.forGoto ? COL_GOOD : COL_DIM);
    y = drawKV(y, "For Tracking", mount.refraction.forTracking ? "ON" : "OFF",
               mount.refraction.forTracking ? COL_GOOD : COL_DIM);
    snprintf(buf, sizeof(buf), "%.1f C", mount.temperature);
    y = drawKV(y, "Temperature", buf);
    snprintf(buf, sizeof(buf), "%.1f kPa", mount.pressure);
    y = drawKV(y, "Pressure", buf);
}

/* ================================================================== */
/*  Tab: Site / Limits                                                */
/* ================================================================== */
static void drawTabSiteLimits(int y0) {
    char buf[128];
    int y = y0;

    y = drawSection(y, "Site Definition");
    y = drawKV(y, "Site Name", localSite.siteName());
    snprintf(buf, sizeof(buf), "Index %d", (int)localSite.siteIndex());
    y = drawKV(y, "Site Index", buf);
    fmtDeg(buf, sizeof(buf), *localSite.latitude());
    y = drawKV(y, "Latitude", buf);
    fmtDeg(buf, sizeof(buf), *localSite.longitude());
    y = drawKV(y, "Longitude", buf);
    snprintf(buf, sizeof(buf), "%d m", (int)*localSite.elevation());
    y = drawKV(y, "Elevation", buf);
    snprintf(buf, sizeof(buf), "%.1f h", *localSite.toff());
    y = drawKV(y, "UTC Offset", buf);
    y = drawKV(y, "Hemisphere", *localSite.latitude() >= 0 ? "North" : "South");
    y += 4;

    y = drawSection(y, "Time");
    {
        double lst = rtk.LST();
        fmtHMS(buf, sizeof(buf), lst);
        y = drawKV(y, "LST", buf);
    }
    {
        unsigned long ts = rtk.getTimeStamp();
        snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d UTC",
            year(ts), month(ts), day(ts), hour(ts), minute(ts), second(ts));
        y = drawKV(y, "UTC Date/Time", buf);
    }
    snprintf(buf, sizeof(buf), "%ld sidereal ticks", rtk.m_lst);
    y = drawKV(y, "Sidereal Timer", buf);
    snprintf(buf, sizeof(buf), "%ld", rtk.m_missedTicks);
    y = drawKV(y, "Missed Ticks", buf, rtk.m_missedTicks > 0 ? COL_WARN : COL_GOOD);
    snprintf(buf, sizeof(buf), "%ld", rtk.m_peakGap);
    y = drawKV(y, "Peak Gap", buf,
        rtk.m_peakGap > 2 ? COL_BAD : (rtk.m_peakGap > 1 ? COL_WARN : COL_GOOD));
    {
        static unsigned long lastTickSnap = 0;
        static unsigned long lastMillisSnap = 0;
        static double ticksPerSec = 0.0;
        unsigned long nowMs = millis();
        unsigned long nowTicks = rtk.m_totalTicks;
        if (nowMs - lastMillisSnap >= 1000) {
            unsigned long dt = nowMs - lastMillisSnap;
            unsigned long dTicks = nowTicks - lastTickSnap;
            if (dt > 0)
                ticksPerSec = (double)dTicks * 1000.0 / (double)dt;
            lastTickSnap = nowTicks;
            lastMillisSnap = nowMs;
        }
        snprintf(buf, sizeof(buf), "%.1f /s", ticksPerSec);
        y = drawKV(y, "Tick Rate", buf,
            ticksPerSec < 90.0 ? COL_BAD : (ticksPerSec < 98.0 ? COL_WARN : COL_GOOD));
    }
    {
        double missRate = 0.0;
        if (rtk.m_totalTicks > 0)
            missRate = (double)rtk.m_missedTicks * 100.0 / (double)rtk.m_totalTicks;
        snprintf(buf, sizeof(buf), "%.2f%%", missRate);
        y = drawKV(y, "Miss Rate", buf,
            missRate > 1.0 ? COL_BAD : (missRate > 0.1 ? COL_WARN : COL_GOOD));
    }
    y += 4;

    y = drawSection(y, "Operational Limits");
    snprintf(buf, sizeof(buf), "%d deg", mount.limits.minAlt);
    y = drawKV(y, "Min Altitude", buf);
    snprintf(buf, sizeof(buf), "%d deg", mount.limits.maxAlt);
    y = drawKV(y, "Max Altitude", buf);
    snprintf(buf, sizeof(buf), "%ld min past", mount.limits.minutesPastMeridianGOTOE);
    y = drawKV(y, "Meridian E GOTO", buf);
    snprintf(buf, sizeof(buf), "%ld min past", mount.limits.minutesPastMeridianGOTOW);
    y = drawKV(y, "Meridian W GOTO", buf);
    snprintf(buf, sizeof(buf), "%.2f deg", mount.limits.underPoleLimitGOTO);
    y = drawKV(y, "Under Pole Limit", buf);
    snprintf(buf, sizeof(buf), "%d deg", mount.limits.distanceFromPoleToKeepTrackingOn);
    y = drawKV(y, "Pole Tracking Dist", buf);
    y += 4;

    y = drawSection(y, "Encoders");
    {
        const char* esNames[] = { "OFF", "60s", "30s", "15s", "8s", "4s", "2s", "Always" };
        int es = (int)mount.motorsEncoders.EncodeSyncMode;
        if (es < 0 || es > 7) es = 0;
        y = drawKV(y, "Sync Mode", esNames[es]);
    }
    y = drawKV(y, "Encoder Enabled", mount.motorsEncoders.enableEncoder ? "YES" : "NO",
               mount.motorsEncoders.enableEncoder ? COL_GOOD : COL_DIM);
    snprintf(buf, sizeof(buf), "%.2f pulse/deg  Rev: %s",
        mount.motorsEncoders.encoderA1.pulsePerDegree,
        mount.motorsEncoders.encoderA1.reverse ? "Y" : "N");
    y = drawKV(y, "Encoder A1", buf);
    snprintf(buf, sizeof(buf), "%.2f pulse/deg  Rev: %s",
        mount.motorsEncoders.encoderA2.pulsePerDegree,
        mount.motorsEncoders.encoderA2.reverse ? "Y" : "N");
    y = drawKV(y, "Encoder A2", buf);
    y += 4;

    y = drawSection(y, "Performance");
    snprintf(buf, sizeof(buf), "%ld us", tlp.getWorstTime());
    y = drawKV(y, "Worst Loop Time", buf,
        tlp.getWorstTime() > 10000 ? COL_BAD :
        (tlp.getWorstTime() > 5000 ? COL_WARN : COL_GOOD));
}

/* ================================================================== */
/*  Scrolling support                                                 */
/* ================================================================== */
static int g_scrollY = 0;
static const int SCROLL_STEP = 40;
static const int CONTENT_Y0 = TAB_H + 8;

/* ================================================================== */
/*  Public API                                                        */
/* ================================================================== */
inline bool cockpit_init() {
    g_win = SDL_CreateWindow("TeenAstro MainUnit Cockpit",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        CW, CH, SDL_WINDOW_SHOWN);
    if (!g_win) return false;
    g_ren = SDL_CreateRenderer(g_win, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!g_ren) return false;
    return true;
}

inline void cockpit_update() {
    if (!g_ren) return;

    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
        if (ev.type == SDL_QUIT) {
            exit(0);
        }
        if (ev.type == SDL_MOUSEWHEEL) {
            g_scrollY -= ev.wheel.y * SCROLL_STEP;
            if (g_scrollY < 0) g_scrollY = 0;
        }
        if (ev.type == SDL_MOUSEBUTTONDOWN && ev.button.button == SDL_BUTTON_LEFT) {
            int mx = ev.button.x, my = ev.button.y;
            /* Tab clicks */
            for (int i = 0; i < TAB_COUNT; i++) {
                SDL_Rect& r = g_tabRects[i];
                if (mx >= r.x && mx < r.x+r.w && my >= r.y && my < r.y+r.h) {
                    g_activeTab = i;
                    g_scrollY = 0;
                }
            }
            /* Button clicks (offset by scroll) */
            for (int i = 0; i < g_nButtons; i++) {
                SDL_Rect& r = g_buttons[i].r;
                if (mx >= r.x && mx < r.x+r.w && my >= r.y && my < r.y+r.h) {
                    switch (g_buttons[i].id) {
                        case 100: mount.setTracking(!mount.tracking.sideralTracking); break;
                        case 101: if (!mount.isParked()) mount.park(); else mount.unpark(); break;
                        case 102: mount.abortSlew(); break;
                        case 103: mount.guiding.stopGuiding(); break;
                    }
                }
            }
        }
    }

    unsigned long now = millis();
    if (now - g_lastDraw < 50) return;
    g_lastDraw = now;

    /* Clear background */
    setCol(COL_BG);
    SDL_RenderClear(g_ren);

    /* ---- Draw tab bar ---- */
    {
        int tx = 0;
        for (int i = 0; i < TAB_COUNT; i++) {
            int tw = textWidth(g_tabNames[i]) + 20;
            g_tabRects[i] = { tx, 0, tw, TAB_H };
            bool active = (i == g_activeTab);
            drawFilledRect(tx, 0, tw, TAB_H, active ? COL_TAB_ACT : COL_TAB_BG);
            /* Bottom border for active tab */
            if (active) {
                setCol(COL_TITLE);
                SDL_RenderDrawLine(g_ren, tx, TAB_H - 1, tx + tw, TAB_H - 1);
            }
            drawText(tx + 10, (TAB_H - CHAR_H) / 2, g_tabNames[i],
                     active ? COL_TAB_ATXT : COL_TAB_TXT);
            tx += tw + 2;
        }
        /* Full separator below tabs */
        setCol(COL_SEP);
        SDL_RenderDrawLine(g_ren, 0, TAB_H, CW, TAB_H);
    }

    /* ---- Clip and scroll content area ---- */
    SDL_Rect clipRect = { 0, CONTENT_Y0, CW, CH - CONTENT_Y0 };
    SDL_RenderSetClipRect(g_ren, &clipRect);

    g_nButtons = 0;
    int contentY = CONTENT_Y0 - g_scrollY;

    switch (g_activeTab) {
        case TAB_OVERVIEW:   drawTabOverview(contentY);   break;
        case TAB_PARKHOME:   drawTabParkHome(contentY);   break;
        case TAB_TRACKING:   drawTabTracking(contentY);   break;
        case TAB_GUIDING:    drawTabGuiding(contentY);    break;
        case TAB_AXES:       drawTabAxes(contentY);       break;
        case TAB_ALIGNMENT:  drawTabAlignment(contentY);  break;
        case TAB_SITELIMITS: drawTabSiteLimits(contentY); break;
    }

    SDL_RenderSetClipRect(g_ren, nullptr);

    /* ---- Scrollbar indicator ---- */
    if (g_scrollY > 0) {
        setCol(COL_DIM);
        for (int i = 0; i < 3; i++) {
            SDL_RenderDrawLine(g_ren, CW/2 - 20 + i*20, CH - 8, CW/2 - 10 + i*20, CH - 4);
            SDL_RenderDrawLine(g_ren, CW/2 - 10 + i*20, CH - 4, CW/2 + i*20, CH - 8);
        }
    }

    SDL_RenderPresent(g_ren);
}

inline void cockpit_quit() {
    if (g_ren) { SDL_DestroyRenderer(g_ren); g_ren = nullptr; }
    if (g_win) { SDL_DestroyWindow(g_win); g_win = nullptr; }
}

} /* namespace cockpit */

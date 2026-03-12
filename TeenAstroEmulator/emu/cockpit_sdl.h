/*
 * cockpit_sdl.h -- MainUnit cockpit window (SDL2).
 *
 * Displays mount state variables and provides clickable buttons for
 * toggling tracking, parking/unparking, aborting slews, and stopping guiding.
 *
 * Requires SDL2.  Include this header from the MainUnit emulator TU
 * (after Mount.h / Application.h are visible).
 */
#pragma once

#include <SDL.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>

extern Mount mount;

namespace cockpit {

/* ------------------------------------------------------------------ */
/*  Tiny 5x7 bitmap font (0-9 A-Z a-z and a few symbols)             */
/* ------------------------------------------------------------------ */
namespace font5x7 {
    static const int GW = 5, GH = 7;

    static uint8_t col(char c, int ci) {
        /* Full ASCII is overkill; cover digits, upper, lower, common symbols. */
        /* Each glyph = 5 bytes, each byte is one column, LSB = top row. */
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
            {'I',{0x00,0x41,0x7F,0x41,0x00}}, {'K',{0x7F,0x08,0x14,0x22,0x41}},
            {'L',{0x7F,0x40,0x40,0x40,0x40}}, {'M',{0x7F,0x02,0x0C,0x02,0x7F}},
            {'N',{0x7F,0x02,0x0C,0x20,0x7F}}, {'O',{0x3E,0x41,0x41,0x41,0x3E}},
            {'P',{0x7F,0x09,0x09,0x09,0x06}}, {'R',{0x7F,0x09,0x19,0x29,0x46}},
            {'S',{0x26,0x49,0x49,0x49,0x32}}, {'T',{0x01,0x01,0x7F,0x01,0x01}},
            {'U',{0x3F,0x40,0x40,0x40,0x3F}}, {'W',{0x3F,0x40,0x30,0x40,0x3F}},
            {'X',{0x63,0x14,0x08,0x14,0x63}}, {'Y',{0x07,0x08,0x70,0x08,0x07}},
            {'Z',{0x61,0x51,0x49,0x45,0x43}},
            {'a',{0x20,0x54,0x54,0x54,0x78}}, {'b',{0x7F,0x48,0x44,0x44,0x38}},
            {'c',{0x38,0x44,0x44,0x44,0x20}}, {'d',{0x38,0x44,0x44,0x48,0x7F}},
            {'e',{0x38,0x54,0x54,0x54,0x18}}, {'f',{0x08,0x7E,0x09,0x01,0x02}},
            {'g',{0x0C,0x52,0x52,0x52,0x3E}}, {'h',{0x7F,0x08,0x04,0x04,0x78}},
            {'i',{0x00,0x44,0x7D,0x40,0x00}}, {'k',{0x7F,0x10,0x28,0x44,0x00}},
            {'l',{0x00,0x41,0x7F,0x40,0x00}}, {'n',{0x7C,0x08,0x04,0x04,0x78}},
            {'o',{0x38,0x44,0x44,0x44,0x38}}, {'p',{0x7C,0x14,0x14,0x14,0x08}},
            {'r',{0x7C,0x08,0x04,0x04,0x08}}, {'s',{0x48,0x54,0x54,0x54,0x20}},
            {'t',{0x04,0x3F,0x44,0x40,0x20}}, {'u',{0x3C,0x40,0x40,0x20,0x7C}},
            {'v',{0x1C,0x20,0x40,0x20,0x1C}}, {'w',{0x3C,0x40,0x30,0x40,0x3C}},
            {'x',{0x44,0x28,0x10,0x28,0x44}}, {'y',{0x0C,0x50,0x50,0x50,0x3C}},
            {'+',{0x08,0x08,0x3E,0x08,0x08}}, {'-',{0x08,0x08,0x08,0x08,0x08}},
            {':',{0x00,0x36,0x36,0x00,0x00}}, {'.',{0x00,0x60,0x60,0x00,0x00}},
            {'/',{0x20,0x10,0x08,0x04,0x02}}, {' ',{0,0,0,0,0}},
            {'(',{0x00,0x1C,0x22,0x41,0x00}}, {')',{0x00,0x41,0x22,0x1C,0x00}},
            {'=',{0x14,0x14,0x14,0x14,0x14}},
            {'Q',{0x3E,0x41,0x51,0x21,0x5E}},
            {'V',{0x1F,0x20,0x40,0x20,0x1F}},
            {'J',{0x20,0x40,0x41,0x3F,0x01}},
        };
        for (int i = 0; i < (int)(sizeof(table)/sizeof(table[0])); i++) {
            if (table[i].ch == c)
                return (ci >= 0 && ci < 5) ? table[i].d[ci] : 0;
        }
        return 0;
    }
}

/* ------------------------------------------------------------------ */
/*  Cockpit state                                                      */
/* ------------------------------------------------------------------ */
static SDL_Window*   g_win  = nullptr;
static SDL_Renderer* g_ren  = nullptr;
static const int CW = 480, CH = 520;
static const int FONT_SC = 2;
static unsigned long g_lastDraw = 0;

/* Clickable button regions */
struct CBtn { SDL_Rect r; const char* label; };
static CBtn g_buttons[4];
static int  g_nButtons = 0;

/* ------------------------------------------------------------------ */
/*  Drawing helpers                                                    */
/* ------------------------------------------------------------------ */
static void drawText(int x, int y, const char* text, uint32_t color) {
    uint8_t cr = (color >> 16) & 0xFF;
    uint8_t cg = (color >> 8) & 0xFF;
    uint8_t cb = color & 0xFF;
    SDL_SetRenderDrawColor(g_ren, cr, cg, cb, 0xFF);
    for (int ci = 0; text[ci]; ci++) {
        int gx = x + ci * (font5x7::GW * FONT_SC + FONT_SC);
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

static void drawButton(int x, int y, int w, int h, const char* label, int idx) {
    g_buttons[idx].r = { x, y, w, h };
    g_buttons[idx].label = label;
    SDL_SetRenderDrawColor(g_ren, 0x33, 0x55, 0x77, 0xFF);
    SDL_RenderFillRect(g_ren, &g_buttons[idx].r);
    SDL_SetRenderDrawColor(g_ren, 0x66, 0x99, 0xCC, 0xFF);
    SDL_RenderDrawRect(g_ren, &g_buttons[idx].r);
    int tw = (int)strlen(label) * (font5x7::GW * FONT_SC + FONT_SC);
    drawText(x + (w - tw)/2, y + (h - font5x7::GH * FONT_SC)/2, label, 0xFFEEEEFF);
}

static void drawRow(int y, const char* label, const char* value) {
    drawText(10, y, label, 0xFF8899AA);
    drawText(200, y, value, 0xFFDDEEFF);
}

/* ------------------------------------------------------------------ */
/*  Public API                                                         */
/* ------------------------------------------------------------------ */
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

    /* Poll SDL events for this window */
    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
        if (ev.type == SDL_QUIT) {
            exit(0);
        }
        if (ev.type == SDL_MOUSEBUTTONDOWN && ev.button.button == SDL_BUTTON_LEFT) {
            int mx = ev.button.x, my = ev.button.y;
            for (int i = 0; i < g_nButtons; i++) {
                SDL_Rect& r = g_buttons[i].r;
                if (mx >= r.x && mx < r.x+r.w && my >= r.y && my < r.y+r.h) {
                    switch (i) {
                        case 0: mount.setTracking(!mount.tracking.sideralTracking); break;
                        case 1: if (!mount.isParked()) mount.park(); else mount.unpark(); break;
                        case 2: mount.abortSlew(); break;
                        case 3: mount.guiding.stopGuiding(); break;
                    }
                }
            }
        }
    }

    /* Throttle redraw to ~20 fps */
    unsigned long now = millis();
    if (now - g_lastDraw < 50) return;
    g_lastDraw = now;

    SDL_SetRenderDrawColor(g_ren, 0x11, 0x14, 0x1A, 0xFF);
    SDL_RenderClear(g_ren);

    int y = 10;
    const int lineH = 22;

    drawText(10, y, "TeenAstro MainUnit Cockpit", 0xFF88CCFF);
    y += lineH + 6;

    /* Separator */
    SDL_SetRenderDrawColor(g_ren, 0x33, 0x44, 0x55, 0xFF);
    SDL_RenderDrawLine(g_ren, 10, y, CW - 10, y);
    y += 8;

    /* --- Tracking --- */
    {
        const char* modes[] = { "Star", "Sun", "Moon", "Target" };
        char val[64];
        int modeIdx = (int)mount.tracking.sideralMode;
        if (modeIdx < 0 || modeIdx > 3) modeIdx = 0;
        snprintf(val, sizeof(val), "%s  Mode: %s",
            mount.tracking.sideralTracking ? "ON" : "OFF",
            modes[modeIdx]);
        drawRow(y, "Tracking:", val); y += lineH;
    }

    /* --- Goto --- */
    {
        const char* gotoNames[] = { "None", "EQ", "AltAz" };
        int gi = (int)mount.tracking.gotoState;
        if (gi < 0 || gi > 2) gi = 0;
        drawRow(y, "Goto:", gotoNames[gi]); y += lineH;
    }

    /* --- Park --- */
    {
        const char* parkNames[] = { "Unparked", "Parking", "Parked" };
        int pi = (int)mount.parkHome.parkStatus;
        if (pi < 0 || pi > 2) pi = 0;
        char val[64];
        snprintf(val, sizeof(val), "%s  Home: %s",
            parkNames[pi], mount.parkHome.atHome ? "YES" : "no");
        drawRow(y, "Park:", val); y += lineH;
    }

    /* --- Guiding --- */
    {
        const char* guidNames[] = { "OFF", "Pulse", "ST4", "Recenter", "AtRate" };
        int gi = (int)mount.guiding.GuidingState;
        if (gi < 0 || gi > 4) gi = 0;
        char val[64];
        snprintf(val, sizeof(val), "%s  Rate: %d",
            guidNames[gi], (int)mount.guiding.activeGuideRate);
        drawRow(y, "Guiding:", val); y += lineH;
    }

    /* --- Error --- */
    {
        const char* errNames[] = { "None", "MotorFault", "Alt", "LimitSense",
                                   "Axis1", "Axis2", "UnderPole", "Meridian" };
        int ei = (int)mount.errors.lastError;
        if (ei < 0 || ei > 7) ei = 0;
        drawRow(y, "Error:", errNames[ei]); y += lineH;
    }

    y += 4;
    SDL_SetRenderDrawColor(g_ren, 0x33, 0x44, 0x55, 0xFF);
    SDL_RenderDrawLine(g_ren, 10, y, CW - 10, y);
    y += 8;

    /* --- Axis 1 --- */
    {
        char val[80];
        long p, t;
        p = mount.axes.staA1.pos;
        t = (long)mount.axes.staA1.target;
        snprintf(val, sizeof(val), "pos: %ld  tgt: %ld", p, t);
        drawRow(y, "Axis1 (RA):", val); y += lineH;
    }

    /* --- Axis 2 --- */
    {
        char val[80];
        long p, t;
        p = mount.axes.staA2.pos;
        t = (long)mount.axes.staA2.target;
        snprintf(val, sizeof(val), "pos: %ld  tgt: %ld", p, t);
        drawRow(y, "Axis2 (DEC):", val); y += lineH;
    }

    /* --- Intervals --- */
    {
        char val[80];
        snprintf(val, sizeof(val), "A1: %.1f  A2: %.1f",
            mount.axes.staA1.interval_Step_Cur,
            mount.axes.staA2.interval_Step_Cur);
        drawRow(y, "Step interval:", val); y += lineH;
    }

    /* --- Delta target --- */
    {
        char val[80];
        snprintf(val, sizeof(val), "A1: %ld  A2: %ld",
            mount.axes.staA1.deltaTarget,
            mount.axes.staA2.deltaTarget);
        drawRow(y, "Delta target:", val); y += lineH;
    }

    y += 8;
    SDL_SetRenderDrawColor(g_ren, 0x33, 0x44, 0x55, 0xFF);
    SDL_RenderDrawLine(g_ren, 10, y, CW - 10, y);
    y += 12;

    /* --- Control buttons --- */
    g_nButtons = 4;
    int bw = 100, bh = 30, bgap = 12;
    int bx = 10;
    drawButton(bx, y, bw, bh, mount.tracking.sideralTracking ? "Track OFF" : "Track ON", 0);
    bx += bw + bgap;
    drawButton(bx, y, bw, bh, mount.isParked() ? "Unpark" : "Park", 1);
    bx += bw + bgap;
    drawButton(bx, y, bw, bh, "Abort", 2);
    bx += bw + bgap;
    drawButton(bx, y, bw, bh, "Stop Guide", 3);

    SDL_RenderPresent(g_ren);
}

inline void cockpit_quit() {
    if (g_ren) { SDL_DestroyRenderer(g_ren); g_ren = nullptr; }
    if (g_win) { SDL_DestroyWindow(g_win); g_win = nullptr; }
}

} /* namespace cockpit */

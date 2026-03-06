/*
 * u8g2_sdl2.h -- U8g2 display backend rendered to an SDL2 window.
 *
 * U8G2_EXT_SDL2 extends U8G2_EXT and uses the real U8g2 C library with
 * no-op I/O callbacks. After each sendBuffer() or nextPage() cycle the
 * internal 128x64 framebuffer is blitted to an SDL2 window at 4x scale.
 *
 * Below the OLED area a button keypad mirrors the physical TeenAstro SHC:
 *
 *            [ N ]
 *       [ E ]     [ W ]
 *            [ S ]
 *  [ f ]    [SH]    [ F ]
 */
#pragma once

#include <u8g2_ext.h>
#include <SDL.h>

/* Exposed by TeenAstroPad_sdl.cpp -- true while the key is held */
extern bool g_padKeyState[7];

/* Tiny 5x7 bitmap font -- upper-case letters + a few symbols, ASCII 32-90.
 * Each glyph is 5 columns of 7 bits (LSB = top row). */
namespace minifont {
    static const uint8_t GLYPH_W = 5;
    static const uint8_t GLYPH_H = 7;
    /* Only define the characters we need: N S E W F f (space) */
    static uint8_t glyph(char c, int col) {
        /* Return the column bitmap for character c (0..4). */
        static const uint8_t font_N[5] = {0x7F,0x02,0x0C,0x20,0x7F};
        static const uint8_t font_S[5] = {0x26,0x49,0x49,0x49,0x32};
        static const uint8_t font_E[5] = {0x7F,0x49,0x49,0x49,0x41};
        static const uint8_t font_W[5] = {0x3F,0x40,0x30,0x40,0x3F};
        static const uint8_t font_F[5] = {0x7F,0x09,0x09,0x09,0x01};
        static const uint8_t font_f[5] = {0x00,0x08,0x7E,0x09,0x02};
        static const uint8_t font_H[5] = {0x7F,0x08,0x08,0x08,0x7F};
        static const uint8_t font_spc[5]={0,0,0,0,0};
        const uint8_t* g = font_spc;
        switch(c) {
            case 'N': g = font_N; break;
            case 'S': g = font_S; break;
            case 'E': g = font_E; break;
            case 'W': g = font_W; break;
            case 'F': g = font_F; break;
            case 'f': g = font_f; break;
            case 'H': g = font_H; break;
        }
        return (col >= 0 && col < 5) ? g[col] : 0;
    }
}

class U8G2_EXT_SDL2 : public U8G2_EXT {
public:
    static constexpr int OLED_W = 128;
    static constexpr int OLED_H = 64;
    static constexpr int SCALE  = 4;
    static constexpr int PAD_H  = 120;  /* height of button keypad area */
    static constexpr int WIN_W  = OLED_W * SCALE;           /* 512 */
    static constexpr int WIN_H  = OLED_H * SCALE + PAD_H;   /* 376 */

    U8G2_EXT_SDL2(const u8g2_cb_t *rotation = U8G2_R0) : U8G2_EXT() {
        u8g2_Setup_ssd1306_i2c_128x64_noname_f(
            &u8g2, rotation, u8x8_byte_empty, u8x8_dummy_cb);
    }

    bool initSDL(const char* title = "TeenAstro SHC") {
        window_ = SDL_CreateWindow(title,
            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            WIN_W, WIN_H,
            SDL_WINDOW_SHOWN);
        if (!window_) return false;

        renderer_ = SDL_CreateRenderer(window_, -1,
            SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        if (!renderer_) return false;

        texture_ = SDL_CreateTexture(renderer_,
            SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
            OLED_W, OLED_H);
        if (!texture_) return false;

        return true;
    }

    void blitToSDL() {
        uint8_t* buf = getBufferPtr();
        if (!buf || !texture_) return;

        uint32_t pixels[OLED_W * OLED_H];
        for (int y = 0; y < OLED_H; y++) {
            for (int x = 0; x < OLED_W; x++) {
                int byteIdx = (y / 8) * OLED_W + x;
                int bitIdx  = y % 8;
                bool on = (buf[byteIdx] >> bitIdx) & 1;
                pixels[y * OLED_W + x] = on ? 0xFFCCDDFF : 0xFF111122;
            }
        }

        SDL_UpdateTexture(texture_, nullptr, pixels, OLED_W * sizeof(uint32_t));

        SDL_SetRenderDrawColor(renderer_, 0x11, 0x11, 0x22, 0xFF);
        SDL_RenderClear(renderer_);

        /* OLED in the top part, rotated 180 degrees */
        SDL_Rect oledDst = { 0, 0, OLED_W * SCALE, OLED_H * SCALE };
        SDL_RenderCopyEx(renderer_, texture_, nullptr, &oledDst, 180.0, nullptr, SDL_FLIP_NONE);

        /* Draw the button keypad below the OLED */
        drawButtonKeypad();

        SDL_RenderPresent(renderer_);
    }

    void sendBuffer() {
        U8G2::sendBuffer();
        blitToSDL();
    }

    uint8_t nextPage() {
        uint8_t r = U8G2::nextPage();
        if (r == 0) blitToSDL();
        return r;
    }

    void destroySDL() {
        if (texture_)  { SDL_DestroyTexture(texture_);   texture_  = nullptr; }
        if (renderer_) { SDL_DestroyRenderer(renderer_); renderer_ = nullptr; }
        if (window_)   { SDL_DestroyWindow(window_);     window_   = nullptr; }
    }

    ~U8G2_EXT_SDL2() { destroySDL(); }

    SDL_Window*   window_   = nullptr;
    SDL_Renderer* renderer_ = nullptr;
    SDL_Texture*  texture_  = nullptr;

private:
    /* Button indices (matching TeenAstroPad.h enum Button) */
    enum { BI_SH=0, BI_N=1, BI_S=2, BI_E=3, BI_W=4, BI_F=5, BI_f=6 };

    void drawButtonKeypad() {
        if (!renderer_) return;

        const int topY = OLED_H * SCALE + 4;
        const int bw = 44, bh = 26, gap = 4;

        const int cx = WIN_W / 2;
        const int cy = topY + bh + gap;

        /* Button rectangles:
         *
         *            [ N ]              row 0
         *       [ E ]     [ W ]         row 1
         *            [ S ]              row 2
         *  [ f ]    [SH]    [ F ]       row 3
         */
        SDL_Rect rects[7];
        /* N: top center */
        rects[BI_N] = { cx - bw/2, topY, bw, bh };
        /* middle row: E left, W right (clean cross arms) */
        rects[BI_E]  = { cx - bw/2 - bw - gap, cy, bw, bh };
        rects[BI_W]  = { cx + bw/2 + gap, cy, bw, bh };
        /* S: bottom center */
        rects[BI_S] = { cx - bw/2, cy + bh + gap, bw, bh };
        /* bottom row: f far-left, SH center, F far-right */
        int bottomY = cy + 2*(bh + gap);
        rects[BI_f]  = { cx - bw/2 - bw - gap, bottomY, bw, bh };
        rects[BI_SH] = { cx - bw/2, bottomY, bw, bh };
        rects[BI_F]  = { cx + bw/2 + gap, bottomY, bw, bh };

        const char* labels[7] = { "SH", "N", "S", "E", "W", "F", "f" };

        for (int i = 0; i < 7; i++) {
            bool pressed = g_padKeyState[i];

            if (pressed) {
                SDL_SetRenderDrawColor(renderer_, 0x44, 0xAA, 0xFF, 0xFF);
            } else {
                SDL_SetRenderDrawColor(renderer_, 0x33, 0x33, 0x44, 0xFF);
            }
            SDL_RenderFillRect(renderer_, &rects[i]);

            /* Border */
            SDL_SetRenderDrawColor(renderer_, 0x66, 0x66, 0x88, 0xFF);
            SDL_RenderDrawRect(renderer_, &rects[i]);

            /* Label */
            drawLabel(rects[i], labels[i], pressed ? 0xFF111122 : 0xFFAABBCC);
        }
    }

    void drawLabel(const SDL_Rect& r, const char* text, uint32_t color) {
        if (!renderer_) return;
        uint8_t cr = (color >> 16) & 0xFF;
        uint8_t cg = (color >> 8) & 0xFF;
        uint8_t cb = color & 0xFF;
        SDL_SetRenderDrawColor(renderer_, cr, cg, cb, 0xFF);

        int len = 0;
        for (const char* p = text; *p; p++) len++;
        const int sc = 2;
        int totalW = len * minifont::GLYPH_W * sc + (len - 1) * sc;
        int totalH = minifont::GLYPH_H * sc;
        int startX = r.x + (r.w - totalW) / 2;
        int startY = r.y + (r.h - totalH) / 2;

        for (int ci = 0; ci < len; ci++) {
            char ch = text[ci];
            int gx = startX + ci * (minifont::GLYPH_W * sc + sc);
            for (int col = 0; col < (int)minifont::GLYPH_W; col++) {
                uint8_t bits = minifont::glyph(ch, col);
                for (int row = 0; row < (int)minifont::GLYPH_H; row++) {
                    if ((bits >> row) & 1) {
                        SDL_Rect px = { gx + col*sc, startY + row*sc, sc, sc };
                        SDL_RenderFillRect(renderer_, &px);
                    }
                }
            }
        }
    }
};

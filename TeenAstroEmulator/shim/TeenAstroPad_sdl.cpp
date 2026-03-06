/*
 * TeenAstroPad_sdl.cpp -- SDL2 keyboard replacement for the TeenAstroPad library.
 *
 * Maps keyboard keys to SHC button events:
 *   Space       -> Shift (B_SHIFT)
 *   8           -> North (B_NORTH)   [Up]
 *   5           -> South (B_SOUTH)   [Down]
 *   4           -> East  (B_EAST)    [Left]
 *   6           -> West  (B_WEST)    [Right]
 *   3           -> Focus+ (B_F)
 *   1           -> Focus- (B_f)
 *
 * This file replaces TeenAstroPad.cpp entirely for the SHC emulator build.
 */

#include <Arduino.h>
#include <TeenAstroPad.h>
#include <SDL.h>

/* Blit the SDL display -- defined in shc_emu.cpp */
extern void _emu_shc_blit();

volatile byte eventbuttons[7] = { E_NONE, E_NONE, E_NONE, E_NONE, E_NONE, E_NONE, E_NONE };

bool g_padKeyState[7] = { false };
static bool (&keyState)[7] = g_padKeyState;

/*
 * Per-button state machine (mimics OneButton):
 *   IDLE  → key down → WAIT (record press time, no event yet)
 *   WAIT  → key up before threshold → emit E_CLICK, go IDLE
 *   WAIT  → threshold elapsed       → emit E_LONGPRESSTART, go LONG
 *   LONG  → repeat timer fires      → emit E_LONGPRESS
 *   LONG  → key up                  → go IDLE (no E_CLICK)
 */
enum BtnState { BTN_IDLE, BTN_WAIT, BTN_LONG };
static BtnState      s_btnState[7]      = {};
static unsigned long s_btnPressTime[7]  = {};

static const unsigned long CLICK_THRESHOLD_MS = 120;

/* Auto-repeat timing for nav buttons 1..4 (N,S,E,W) */
static unsigned long s_lastNavRepeat[4] = { 0, 0, 0, 0 };
static bool          s_navInitial[4]    = { true, true, true, true };
static unsigned long s_initialDelayMs   = 500;
static unsigned long s_repeatMs         = 400;

static int keyToButton(SDL_Keycode k) {
    switch (k) {
        case SDLK_SPACE:   return B_SHIFT;
        case SDLK_8: case SDLK_KP_8: case SDLK_UP:      return B_NORTH;
        case SDLK_5: case SDLK_KP_5: case SDLK_DOWN:    return B_SOUTH;
        case SDLK_4: case SDLK_KP_4: case SDLK_LEFT:    return B_EAST;
        case SDLK_6: case SDLK_KP_6: case SDLK_RIGHT:   return B_WEST;
        case SDLK_3: case SDLK_KP_3:                    return B_F;
        case SDLK_1: case SDLK_KP_1:                    return B_f;
        default:          return -1;
    }
}

/* Physical key position (works with any keyboard layout, e.g. AZERTY) */
static int scancodeToButton(SDL_Scancode s) {
    switch (s) {
        case SDL_SCANCODE_8:  return B_NORTH;
        case SDL_SCANCODE_5:  return B_SOUTH;
        case SDL_SCANCODE_4:  return B_EAST;
        case SDL_SCANCODE_6:  return B_WEST;
        case SDL_SCANCODE_3:  return B_F;
        case SDL_SCANCODE_1:  return B_f;
        default:              return -1;
    }
}

static void applySpeedToRepeat(Pad::ButtonSpeed bs) {
    switch (bs) {
        case Pad::BS_SLOW:   s_initialDelayMs = 700; s_repeatMs = 550; break;
        case Pad::BS_MEDIUM: s_initialDelayMs = 500; s_repeatMs = 400; break;
        case Pad::BS_FAST:   s_initialDelayMs = 350; s_repeatMs = 250; break;
    }
}

void Pad::setup(const int[7], const bool[7], int adress, bool) {
    m_adress = adress;
    m_buttonPressed = false;
    m_shiftPressed = false;
    m_button_speed = BS_MEDIUM;
    readButtonSpeed();
}

void Pad::tickButtons() {
    m_buttonPressed = false;
    for (int k = 0; k < 7; k++)
        eventbuttons[k] = E_NONE;

    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
        if (ev.type == SDL_QUIT) exit(0);

        if (ev.type == SDL_KEYDOWN && !ev.key.repeat) {
            int b = keyToButton(ev.key.keysym.sym);
            if (b < 0) b = scancodeToButton(ev.key.keysym.scancode);
            if (b >= 0) {
                keyState[b] = true;
                s_btnState[b] = BTN_WAIT;
                s_btnPressTime[b] = millis();
            }
        }
        if (ev.type == SDL_KEYUP) {
            int b = keyToButton(ev.key.keysym.sym);
            if (b < 0) b = scancodeToButton(ev.key.keysym.scancode);
            if (b >= 0) {
                if (s_btnState[b] == BTN_WAIT)
                    eventbuttons[b] = E_CLICK;
                keyState[b] = false;
                s_btnState[b] = BTN_IDLE;
            }
        }
    }

    m_shiftPressed = keyState[B_SHIFT];

    unsigned long now = millis();
    for (int k = 0; k < 7; k++) {
        if (eventbuttons[k] != E_NONE) continue;

        if (s_btnState[k] == BTN_WAIT) {
            if (now - s_btnPressTime[k] >= CLICK_THRESHOLD_MS) {
                eventbuttons[k] = E_LONGPRESSTART;
                s_btnState[k] = BTN_LONG;
                if (k >= 1 && k <= 4) {
                    s_lastNavRepeat[k - 1] = now;
                    s_navInitial[k - 1] = true;
                }
            }
        } else if (s_btnState[k] == BTN_LONG) {
            if (k >= 1 && k <= 4) {
                unsigned long threshold = s_navInitial[k - 1] ? s_initialDelayMs : s_repeatMs;
                if (now - s_lastNavRepeat[k - 1] < threshold)
                    continue;
                s_lastNavRepeat[k - 1] = now;
                s_navInitial[k - 1] = false;
            }
            eventbuttons[k] = E_LONGPRESS;
        }
    }

    for (int k = 0; k < 7; k++) {
        if (eventbuttons[k] != E_NONE)
            m_buttonPressed = true;
    }

    for (int k = 1; k < 6; k += 2) {
        if (eventbuttons[k] == eventbuttons[k + 1]) {
            eventbuttons[k] = E_NONE;
            eventbuttons[k + 1] = E_NONE;
        }
    }

    if (!m_buttonPressed)
        SDL_Delay(10);

    _emu_shc_blit();
}

void Pad::setMenuMode() {}
void Pad::setControlerMode() {}
void Pad::attachEvent() {}
Pad::ButtonSpeed Pad::getButtonSpeed() { return m_button_speed; }
void Pad::readButtonSpeed() {
    uint8_t val = 1;
    EEPROM.get(m_adress, val);
    if (val == 0)      m_button_speed = BS_SLOW;
    else if (val == 1)  m_button_speed = BS_MEDIUM;
    else if (val == 2)  m_button_speed = BS_FAST;
    else { m_button_speed = BS_MEDIUM; EEPROM.put(m_adress, (uint8_t)1); }
    applySpeedToRepeat(m_button_speed);
}
void Pad::setButtonSpeed(ButtonSpeed bs) {
    m_button_speed = bs;
    uint8_t val = static_cast<uint8_t>(bs);
    EEPROM.put(m_adress, val);
    EEPROM.commit();
    applySpeedToRepeat(bs);
}
bool Pad::buttonPressed() { return m_buttonPressed; }
bool Pad::shiftPressed() { return m_shiftPressed; }

/* WiFi on/off persisted in EEPROM (address 4 = EEPROM_WifiOn in TeenAstroWifi) */
static const int EEPROM_WIFI_ON = 4;
bool Pad::isWifiOn() {
    return EEPROM.read(EEPROM_WIFI_ON) != 0;
}
bool Pad::isWifiRunning() { return isWifiOn(); }
void Pad::turnWifiOn(bool on) {
    EEPROM.write(EEPROM_WIFI_ON, on ? 255 : 0);
    EEPROM.commit();
}
void Pad::getIP(uint8_t* ip) { memset(ip, 0, 4); }
const char* Pad::getPassword() { return ""; }
bool Pad::setWifiMode(int) { return false; }
int Pad::getWifiMode() { return 0; }
void Pad::getStationName(int, char* SSID) { SSID[0] = 0; }

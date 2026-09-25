/*
 * TeenAstroPad_sdl.cpp -- SDL2 keyboard replacement for the TeenAstroPad library.
 *
 * Faithfully replicates the OneButton FSM used by the real TeenAstroPad.
 * Supports controller mode / menu mode with per-button timing, exactly
 * as in the Release_1.5 firmware.
 *
 * Keyboard mapping:
 *   Space       -> Shift (B_SHIFT)
 *   8 / Up      -> North (B_NORTH)
 *   5 / Down    -> South (B_SOUTH)
 *   4 / Left    -> East  (B_EAST)
 *   6 / Right   -> West  (B_WEST)
 *   3           -> Focus+ (B_F)
 *   1           -> Focus- (B_f)
 */

#include <Arduino.h>
#include <TeenAstroPad.h>
#include <SDL.h>
#include <stdio.h>
#include <string.h>

extern void _emu_shc_blit();

volatile byte eventbuttons[7] = { E_NONE, E_NONE, E_NONE, E_NONE, E_NONE, E_NONE, E_NONE };

bool g_padKeyState[7] = { false };
static bool (&keyState)[7] = g_padKeyState;

/* ------------------------------------------------------------------ */
/*  OneButton-compatible per-button state machine                      */
/* ------------------------------------------------------------------ */

enum OBState { OB_INIT, OB_DOWN, OB_UP, OB_COUNT, OB_PRESS, OB_PRESSEND };

static struct BtnFSM {
    OBState  state       = OB_INIT;
    unsigned long startTime = 0;
    int      nClicks     = 0;
    unsigned long lastLPTime = 0;

    unsigned int debounce_ms  = 50;
    unsigned int click_ms     = 400;
    unsigned int press_ms     = 800;
    unsigned int lp_interval  = 0;

    bool     prevLevel   = false;
    unsigned long dbTime = 0;
    bool     dbLevel     = false;
    bool     lastDbLevel = false;
} s_btn[7];

/* Optional button override written by the screenshot tool.
   Seven integers, one per button: 1 = held, 0 = released.
   While the file exists it replaces the keyboard, so a capture does not
   depend on which window has focus. */
static bool s_ignoreUntilRelease[7] = { false, false, false, false, false, false, false };

static void applyInjectFile() {
    FILE* f = fopen("pad_inject.txt", "r");
    if (!f)
        return;
    int s[7];
    if (fscanf(f, "%d %d %d %d %d %d %d",
               &s[0], &s[1], &s[2], &s[3], &s[4], &s[5], &s[6]) == 7) {
        for (int i = 0; i < 7; i++)
            keyState[i] = s[i] != 0;
    }
    fclose(f);
}

/* One-shot commands in pad_inject.txt, consumed on the next poll:
     click N     -> short press of button N
     menu N      -> Shift held + button N (opens that menu)
   The file is removed so the command is delivered once. */
static void applyInjectCommand() {
    const char* path = "pad_inject.txt";
    FILE* f = fopen(path, "r");
    if (!f)
        return;
    char cmd[16];
    int btn = -1;
    int n = fscanf(f, "%15s %d", cmd, &btn);
    fclose(f);
    if (n != 2 || btn < 0 || btn > 6)
        return;
    bool used = false;
    if (strcmp(cmd, "click") == 0) {
        eventbuttons[btn] = E_CLICK;
        used = true;
    } else if (strcmp(cmd, "menu") == 0) {
        eventbuttons[0] = E_LONGPRESS;
        eventbuttons[btn] = E_CLICK;
        used = true;
    }
    if (!used)
        return;
    remove(path);
}

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

static bool debounce(BtnFSM& b, bool raw, unsigned long now) {
    if (b.lastDbLevel == raw) {
        if (now - b.dbTime >= b.debounce_ms)
            b.dbLevel = raw;
    } else {
        b.dbTime = now;
        b.lastDbLevel = raw;
    }
    return b.dbLevel;
}

static void obTick(int k, bool rawActive, unsigned long now) {
    BtnFSM& b = s_btn[k];
    bool active = debounce(b, rawActive, now);
    unsigned long wait = now - b.startTime;

    switch (b.state) {
    case OB_INIT:
        if (active) {
            b.state = OB_DOWN;
            b.startTime = now;
            b.nClicks = 0;
        }
        break;

    case OB_DOWN:
        if (!active) {
            b.state = OB_UP;
            b.startTime = now;
        } else if (wait > b.press_ms) {
            eventbuttons[k] = E_LONGPRESSTART;
            b.state = OB_PRESS;
            b.lastLPTime = now;
        }
        break;

    case OB_UP:
        b.nClicks++;
        b.state = OB_COUNT;
        break;

    case OB_COUNT:
        if (active) {
            b.state = OB_DOWN;
            b.startTime = now;
        } else if (wait >= b.click_ms || b.nClicks >= 2) {
            if (b.nClicks == 1) {
                eventbuttons[k] = E_CLICK;
            } else if (b.nClicks == 2) {
                eventbuttons[k] = E_DOUBLECLICK;
            }
            b.state = OB_INIT;
            b.nClicks = 0;
            b.startTime = now;
        }
        break;

    case OB_PRESS:
        if (!active) {
            b.state = OB_PRESSEND;
        } else {
            if (now - b.lastLPTime >= b.lp_interval) {
                eventbuttons[k] = E_LONGPRESS;
                b.lastLPTime = now;
            }
        }
        break;

    case OB_PRESSEND:
        eventbuttons[k] = E_LONGPRESSSTOP;
        b.state = OB_INIT;
        b.nClicks = 0;
        b.startTime = now;
        break;

    default:
        b.state = OB_INIT;
        break;
    }
}

/* ------------------------------------------------------------------ */
/*  Timing helpers (matching real TeenAstroPad setMenuMode etc.)       */
/* ------------------------------------------------------------------ */

static int s_tickRef = 30;

static void applyTimings(int tickRef) {
    s_tickRef = tickRef;
}

static Pad::ButtonSpeed s_speed = Pad::BS_MEDIUM;

static int computeTickRef(Pad::ButtonSpeed bs) {
    int tr = 20;
    switch (bs) {
        case Pad::BS_SLOW:   tr = (int)(20 * 2);   break;
        case Pad::BS_MEDIUM: tr = (int)(20 * 1.5);  break;
        case Pad::BS_FAST:   tr = (int)(20 * 1);    break;
    }
    return tr;
}

/* ------------------------------------------------------------------ */
/*  Pad interface                                                      */
/* ------------------------------------------------------------------ */

void Pad::setup(const int[7], const bool[7], int adress, bool) {
    m_adress = adress;
    m_buttonPressed = false;
    m_shiftPressed = false;
    m_button_speed = BS_MEDIUM;
    readButtonSpeed();
    setControlerMode();
}

void Pad::tickButtons() {
    delay(1);
    m_buttonPressed = false;
    m_shiftPressed = false;

    for (int k = 0; k < 7; k++) {
        delay(1);
        eventbuttons[k] = E_NONE;

        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) { extern void emu_shutdown(); emu_shutdown(); }
            if (ev.type == SDL_KEYDOWN && !ev.key.repeat) {
                int b = keyToButton(ev.key.keysym.sym);
                if (b < 0) b = scancodeToButton(ev.key.keysym.scancode);
                if (b >= 0)
                    keyState[b] = true;
            }
            if (ev.type == SDL_KEYUP) {
                int b = keyToButton(ev.key.keysym.sym);
                if (b < 0) b = scancodeToButton(ev.key.keysym.scancode);
                if (b >= 0)
                    keyState[b] = false;
            }
        }

        applyInjectFile();
        bool raw = keyState[k];
        /* A chord that just opened a menu must not also be read as Home. */
        if (s_ignoreUntilRelease[k]) {
            if (!raw)
                s_ignoreUntilRelease[k] = false;
            else
                raw = false;
        }
        obTick(k, raw, millis());
    }

    for (int k = 0; k < 7; k++) {
        if (eventbuttons[k] != E_NONE) {
            m_buttonPressed = true;
            if (k == 0)
                m_shiftPressed = true;
            break;
        }
    }

    for (int k = 1; k < 6; k += 2) {
        if (eventbuttons[k] == eventbuttons[k + 1]) {
            eventbuttons[k] = E_NONE;
            eventbuttons[k + 1] = E_NONE;
        }
    }

    applyInjectCommand();

    _emu_shc_blit();
}

void Pad::setMenuMode() {
    int tr = computeTickRef(m_button_speed);
    for (int k = 0; k < 7; k++) {
        s_btn[k].click_ms    = tr * 4;
        s_btn[k].debounce_ms = tr;
        s_btn[k].press_ms    = tr * 8;
        s_btn[k].state       = OB_INIT;
        s_btn[k].nClicks     = 0;
        if (keyState[k])
            s_ignoreUntilRelease[k] = true;
    }
}

void Pad::setControlerMode() {
    int tr = computeTickRef(m_button_speed);
    s_btn[0].click_ms    = tr * 4;
    s_btn[0].debounce_ms = tr;
    s_btn[0].press_ms    = tr * 8;
    for (int k = 1; k < 7; k++) {
        s_btn[k].click_ms    = 5;
        s_btn[k].debounce_ms = tr;
        s_btn[k].press_ms    = 5;
    }
}

void Pad::attachEvent() {}

Pad::ButtonSpeed Pad::getButtonSpeed() { return m_button_speed; }

void Pad::readButtonSpeed() {
    uint8_t val = 1;
    EEPROM.get(m_adress, val);
    if (val == 0)      m_button_speed = BS_SLOW;
    else if (val == 1)  m_button_speed = BS_MEDIUM;
    else if (val == 2)  m_button_speed = BS_FAST;
    else { m_button_speed = BS_MEDIUM; EEPROM.put(m_adress, (uint8_t)1); }
}

void Pad::setButtonSpeed(ButtonSpeed bs) {
    m_button_speed = bs;
    uint8_t val = static_cast<uint8_t>(bs);
    EEPROM.put(m_adress, val);
    EEPROM.commit();
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

/*
 * shc_emu.cpp -- TeenAstro SHC PC emulator entry point.
 *
 * Runs the real SHC firmware on a desktop PC.
 * - Connects to MainUnit emulator at TCP 127.0.0.1:9998
 * - Renders the 128x64 OLED display via SDL2 at 4x scale
 * - Maps keyboard keys to SHC buttons
 *
 * Build:  pio run -d TeenAstroEmulator -e emu_shc
 * Run:    .pio\build\emu\shc_emu.exe
 */

/* ------------------------------------------------------------------ */
/*  sim:: globals                                                      */
/* ------------------------------------------------------------------ */
namespace sim {
    unsigned long g_micros = 0;
    unsigned long g_millis = 0;
    bool          g_realtime = false;
}

/* Board/platform defines are set via build_flags in platformio.ini:
 *   -DARDUINO=1 -DARDUINO_ESP8266_WEMOS_D1MINI -DARDUINO_ARCH_ESP8266
 *
 * Pin definitions for the emulator (values don't matter, Pad is SDL) */
#ifndef D0
#define D0 0
#define D1 1
#define D2 2
#define D3 3
#define D4 4
#define D5 5
#define D6 6
#define D7 7
#define D8 8
#endif
#ifndef A0
#define A0 0
#endif

/* ------------------------------------------------------------------ */
/*  Shim includes                                                      */
/* ------------------------------------------------------------------ */
#include <ctime>
#include "TcpStream.h"
#include "arduino.h"
#include "EEPROM.h"
#include "esp_shim.h"
#include "u8g2_sdl2.h"

/* TCP client to MainUnit (used by emu_attempt_reconnect and main) */
static TcpClientStream g_tcpToMainUnit;

/* ------------------------------------------------------------------ */
/*  Child-process management (auto-launch MainUnit)                    */
/* ------------------------------------------------------------------ */
#ifdef _WIN32
#  include <windows.h>
static PROCESS_INFORMATION g_muProcInfo = {};
static bool g_muLaunched = false;

static bool launchMainUnit() {
    char myPath[MAX_PATH];
    GetModuleFileNameA(NULL, myPath, MAX_PATH);
    char* lastSlash = strrchr(myPath, '\\');
    if (!lastSlash) return false;
    *(lastSlash + 1) = '\0';
    /* Prefer build-dir name (mainunit_emu.exe), then installed name (TeenAstroMainUnit.exe) */
    char muPath[MAX_PATH];
    strcpy(muPath, myPath);
    strcat(muPath, "mainunit_emu.exe");
    if (GetFileAttributesA(muPath) == INVALID_FILE_ATTRIBUTES) {
        strcpy(muPath, myPath);
        strcat(muPath, "TeenAstroMainUnit.exe");
    }
    if (GetFileAttributesA(muPath) == INVALID_FILE_ATTRIBUTES) return false;
    STARTUPINFOA si = {}; si.cb = sizeof(si);
    if (!CreateProcessA(muPath, NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &g_muProcInfo))
        return false;
    g_muLaunched = true;
    return true;
}

static void killMainUnit() {
    if (g_muLaunched) {
        TerminateProcess(g_muProcInfo.hProcess, 0);
        WaitForSingleObject(g_muProcInfo.hProcess, 2000);
        CloseHandle(g_muProcInfo.hProcess);
        CloseHandle(g_muProcInfo.hThread);
        g_muLaunched = false;
    }
}
#else
static bool launchMainUnit() { return false; }
static void killMainUnit() {}
#endif

void emu_shutdown() {
    killMainUnit();
    SDL_Quit();
    exit(0);
}

/*
 * Called by ESP.reset()/restart() (esp_shim.h) instead of exit(0).
 * Attempts to re-establish the TCP link to the MainUnit and resets the
 * connection failure counter so the SHC firmware continues running.
 */
void emu_attempt_reconnect() {
    extern TeenAstroMountStatus ta_MountStatus;

    printf("[SHC-EMU] Reconnecting to MainUnit...\n");
    fflush(stdout);

    for (int attempt = 0; attempt < 20; attempt++) {
        if (g_tcpToMainUnit.connected()) break;
        if (g_tcpToMainUnit.reconnect()) break;
        printf("[SHC-EMU]   Retry %d/20...\n", attempt + 1);
        fflush(stdout);
        SDL_Delay(500);
    }

    if (g_tcpToMainUnit.connected()) {
        ta_MountStatus.removeLastConnectionFailure();
        ta_MountStatus.removeLastConnectionFailure();
        ta_MountStatus.removeLastConnectionFailure();
        ta_MountStatus.removeLastConnectionFailure();
        ta_MountStatus.removeLastConnectionFailure();
        printf("[SHC-EMU] Reconnected successfully.\n");
    } else {
        printf("[SHC-EMU] Reconnect failed. Exiting.\n");
        killMainUnit();
        SDL_Quit();
        exit(1);
    }
    fflush(stdout);
}

/* ------------------------------------------------------------------ */
/*  SHC firmware includes                                              */
/* ------------------------------------------------------------------ */
#include "SmartConfig.h"
#include "SmartController.h"
#include <TeenAstroMountStatus.h>

/* ------------------------------------------------------------------ */
/*  TCP serial to MainUnit (g_tcpToMainUnit defined above after shims) */
/* ------------------------------------------------------------------ */

/* ------------------------------------------------------------------ */
/*  Global instances (matching TeenAstroSHC.ino)                       */
/* ------------------------------------------------------------------ */
LX200Client*      g_lx200 = nullptr;
SmartHandController HdCrtlr;
TeenAstroMountStatus ta_MountStatus;

/* ------------------------------------------------------------------ */
/*  SDL2 display instance (populated by SmartController::setup)        */
/* ------------------------------------------------------------------ */
U8G2_EXT_SDL2* g_sdlDisplay = nullptr;

/* ------------------------------------------------------------------ */
/*  Blit helper called from delay() in arduino.h during setup/menus    */
/* ------------------------------------------------------------------ */
void _emu_shc_blit() {
    if (g_sdlDisplay) g_sdlDisplay->blitToSDL();
}

/* ------------------------------------------------------------------ */
/*  Automated test suite (--test mode)                                 */
/* ------------------------------------------------------------------ */
static int g_testPass = 0;
static int g_testFail = 0;

#define TEST_OK(name) do { printf("  [PASS] %s\n", name); g_testPass++; } while(0)
#define TEST_FAIL(name, detail) do { printf("  [FAIL] %s -- %s\n", name, detail); g_testFail++; } while(0)
#define TEST_CHECK(cond, name, detail) do { if (cond) TEST_OK(name); else TEST_FAIL(name, detail); } while(0)

static int runAutomatedTests(LX200Client& lx200)
{
    printf("\n========================================\n");
    printf("  TeenAstro Emulator Automated Tests\n");
    printf("========================================\n\n");

    char buf[256];

    printf("[TEST] Mount Info\n");
    TEST_CHECK(lx200.getProductName(buf, sizeof(buf)) == LX200_VALUEGET, "getProductName", "no response");
    printf("         Product: %s\n", buf);
    TEST_CHECK(lx200.getVersionNumber(buf, sizeof(buf)) == LX200_VALUEGET, "getVersionNumber", "no response");
    printf("         Version: %s\n", buf);
    TEST_CHECK(lx200.getBoardVersion(buf, sizeof(buf)) == LX200_VALUEGET, "getBoardVersion", "no response");
    printf("         Board: %s\n", buf);

    printf("\n[TEST] Mount Parameters\n");
    double lat = 0, lon = 0, lst = 0;
    TEST_CHECK(lx200.getLatitude(lat) == LX200_VALUEGET, "getLatitude", "no response");
    printf("         Latitude: %.4f\n", lat);
    TEST_CHECK(lx200.getLongitude(lon) == LX200_VALUEGET, "getLongitude", "no response");
    printf("         Longitude: %.4f\n", lon);
    TEST_CHECK(lx200.getLstT0(lst) == LX200_VALUEGET, "getLstT0", "no response");
    int site = -1;
    TEST_CHECK(lx200.getSite(site) == LX200_VALUEGET, "getSite", "no response");

    /* ============================================================ */
    /*  Settings round-trip (mount is at home — lat/lon allowed)    */
    /* ============================================================ */

    /* ---- 3. Location (Latitude / Longitude) ---- */
    printf("\n[TEST] Location: Latitude / Longitude round-trip\n");
    {
        TEST_CHECK(lx200.setLatitudeDMS(0, 48, 51, 24) == LX200_VALUESET,
                   "setLatitude +48:51:24", "rejected (mount must be at home)");
        double latR = 0;
        TEST_CHECK(lx200.getLatitude(latR) == LX200_VALUEGET, "getLatitude", "no response");
        printf("         Latitude set: +48:51:24 -> read: %.4f\n", latR);
        TEST_CHECK(fabs(latR - 48.8567) < 0.02, "latitude ~48.857", "mismatch");

        TEST_CHECK(lx200.setLongitudeDMS(0, 2, 21, 7) == LX200_VALUESET,
                   "setLongitude +002:21:07", "rejected (mount must be at home)");
        double lonR = 0;
        TEST_CHECK(lx200.getLongitude(lonR) == LX200_VALUEGET, "getLongitude", "no response");
        printf("         Longitude set: +002:21:07 -> read: %.4f\n", lonR);
        TEST_CHECK(fabs(lonR - 2.3519) < 0.02, "longitude ~2.352", "mismatch");
    }

    /* ---- 4. Site Selection ---- */
    printf("\n[TEST] Site selection round-trip\n");
    {
        int siteVal = 2;
        TEST_CHECK(lx200.setSite(siteVal) == LX200_VALUESET, "setSite(2)", "rejected");
        int readSite = -1;
        TEST_CHECK(lx200.getSite(readSite) == LX200_VALUEGET, "getSite", "no response");
        printf("         Site set: 2 -> read: %d\n", readSite);
        TEST_CHECK(readSite == 2, "site == 2", "mismatch");

        siteVal = 1;
        lx200.setSite(siteVal);
        lx200.getSite(readSite);
        TEST_CHECK(readSite == 1, "site back to 1", "mismatch");
    }

    /* ---- 5. Time ---- */
    printf("\n[TEST] Time: setLocalTime\n");
    {
        long setTime = 14L * 3600 + 30 * 60 + 45;
        TEST_CHECK(lx200.setLocalTime(setTime) == LX200_VALUESET,
                   "setLocalTime(14:30:45)", "rejected");
        unsigned int rh = 0, rm = 0, rs = 0;
        LX200RETURN tr = lx200.getLocalTime(rh, rm, rs);
        TEST_CHECK(tr == LX200_VALUEGET, "getLocalTime", "no response");
        printf("         Time set: 14:30:45 -> read: %02d:%02d:%02d\n", rh, rm, rs);
    }

    /* ---- 6. UTC Date ---- */
    printf("\n[TEST] UTC Date: setUTCDateRaw\n");
    {
        TEST_CHECK(lx200.setUTCDateRaw(3, 15, 26) == LX200_VALUESET,
                   "setUTCDate(03/15/26)", "rejected");
        unsigned int day = 0, month = 0, year = 0;
        LX200RETURN dr = lx200.getUTCDate(day, month, year);
        TEST_CHECK(dr == LX200_VALUEGET, "getUTCDate", "no response");
        printf("         Date set: 03/15/26 -> read: %02d/%02d/%04d\n", month, day, year);
    }

    /* ============================================================ */
    /*  Motor configuration round-trip (mount at home, not slewing) */
    /* ============================================================ */

    /* ---- Motor Axis 1 (RA) ---- */
    printf("\n[TEST] Motor Axis 1 (RA) round-trip\n");
    {
        const uint8_t ax = 1;
        float gearOut = 0, stepsOut = 0, blOut = 0;
        uint8_t microOut = 0;
        unsigned int hiCurrOut = 0, loCurrOut = 0;
        bool revOut = false;

        TEST_CHECK(lx200.writeTotGear(ax, 360.0f) == LX200_VALUESET,
                   "writeTotGear(1, 360)", "rejected");
        TEST_CHECK(lx200.readTotGear(ax, gearOut) == LX200_VALUEGET,
                   "readTotGear(1)", "no response");
        printf("         Gear: set 360 -> read %.1f\n", gearOut);
        TEST_CHECK(fabs(gearOut - 360.0f) < 0.1f, "gear == 360", "mismatch");

        TEST_CHECK(lx200.writeStepPerRot(ax, 400.0f) == LX200_VALUESET,
                   "writeStepPerRot(1, 400)", "rejected");
        TEST_CHECK(lx200.readStepPerRot(ax, stepsOut) == LX200_VALUEGET,
                   "readStepPerRot(1)", "no response");
        printf("         StepPerRot: set 400 -> read %.0f\n", stepsOut);
        TEST_CHECK(fabs(stepsOut - 400.0f) < 1.0f, "steps == 400", "mismatch");

        uint8_t microVal = 4;
        TEST_CHECK(lx200.writeMicro(ax, microVal) == LX200_VALUESET,
                   "writeMicro(1, 4)", "rejected");
        TEST_CHECK(lx200.readMicro(ax, microOut) == LX200_VALUEGET,
                   "readMicro(1)", "no response");
        printf("         Micro: set 4 -> read %d\n", (int)microOut);
        TEST_CHECK(microOut == 4, "micro == 4", "mismatch");

        unsigned int hiCurr = 1200;
        TEST_CHECK(lx200.writeHighCurr(ax, hiCurr) == LX200_VALUESET,
                   "writeHighCurr(1, 1200)", "rejected");
        TEST_CHECK(lx200.readHighCurr(ax, hiCurrOut) == LX200_VALUEGET,
                   "readHighCurr(1)", "no response");
        printf("         HighCurr: set 1200 -> read %u\n", hiCurrOut);
        TEST_CHECK(hiCurrOut == 1200, "highCurr == 1200", "mismatch");

        unsigned int loCurr = 600;
        TEST_CHECK(lx200.writeLowCurr(ax, loCurr) == LX200_VALUESET,
                   "writeLowCurr(1, 600)", "rejected");
        TEST_CHECK(lx200.readLowCurr(ax, loCurrOut) == LX200_VALUEGET,
                   "readLowCurr(1)", "no response");
        printf("         LowCurr: set 600 -> read %u\n", loCurrOut);
        TEST_CHECK(loCurrOut == 600, "lowCurr == 600", "mismatch");

        bool revVal = true;
        TEST_CHECK(lx200.writeReverse(ax, revVal) == LX200_VALUESET,
                   "writeReverse(1, true)", "rejected");
        TEST_CHECK(lx200.readReverse(ax, revOut) == LX200_VALUEGET,
                   "readReverse(1)", "no response");
        printf("         Reverse: set true -> read %d\n", (int)revOut);
        TEST_CHECK(revOut == true, "reverse == true", "mismatch");

        revVal = false;
        lx200.writeReverse(ax, revVal);
        lx200.readReverse(ax, revOut);
        TEST_CHECK(revOut == false, "reverse back to false", "mismatch");

        float blVal = 120.0f;
        TEST_CHECK(lx200.writeBacklash(ax, blVal) == LX200_VALUESET,
                   "writeBacklash(1, 120)", "rejected");
        TEST_CHECK(lx200.readBacklash(ax, blOut) == LX200_VALUEGET,
                   "readBacklash(1)", "no response");
        printf("         Backlash: set 120 -> read %.0f\n", blOut);
        TEST_CHECK(fabs(blOut - 120.0f) < 1.0f, "backlash == 120", "mismatch");
    }

    /* ---- Motor Axis 2 (Dec) ---- */
    printf("\n[TEST] Motor Axis 2 (Dec) round-trip\n");
    {
        const uint8_t ax = 2;
        float gearOut = 0, stepsOut = 0;
        uint8_t microOut = 0;
        unsigned int hiCurrOut = 0, loCurrOut = 0;

        TEST_CHECK(lx200.writeTotGear(ax, 180.0f) == LX200_VALUESET,
                   "writeTotGear(2, 180)", "rejected");
        TEST_CHECK(lx200.readTotGear(ax, gearOut) == LX200_VALUEGET,
                   "readTotGear(2)", "no response");
        printf("         Gear: set 180 -> read %.1f\n", gearOut);
        TEST_CHECK(fabs(gearOut - 180.0f) < 0.1f, "gear == 180", "mismatch");

        TEST_CHECK(lx200.writeStepPerRot(ax, 200.0f) == LX200_VALUESET,
                   "writeStepPerRot(2, 200)", "rejected");
        TEST_CHECK(lx200.readStepPerRot(ax, stepsOut) == LX200_VALUEGET,
                   "readStepPerRot(2)", "no response");
        printf("         StepPerRot: set 200 -> read %.0f\n", stepsOut);
        TEST_CHECK(fabs(stepsOut - 200.0f) < 1.0f, "steps == 200", "mismatch");

        uint8_t microVal = 3;
        TEST_CHECK(lx200.writeMicro(ax, microVal) == LX200_VALUESET,
                   "writeMicro(2, 3)", "rejected");
        TEST_CHECK(lx200.readMicro(ax, microOut) == LX200_VALUEGET,
                   "readMicro(2)", "no response");
        printf("         Micro: set 3 -> read %d\n", (int)microOut);
        TEST_CHECK(microOut == 3, "micro == 3", "mismatch");

        unsigned int hiCurr = 800;
        TEST_CHECK(lx200.writeHighCurr(ax, hiCurr) == LX200_VALUESET,
                   "writeHighCurr(2, 800)", "rejected");
        TEST_CHECK(lx200.readHighCurr(ax, hiCurrOut) == LX200_VALUEGET,
                   "readHighCurr(2)", "no response");
        printf("         HighCurr: set 800 -> read %u\n", hiCurrOut);
        TEST_CHECK(hiCurrOut == 800, "highCurr == 800", "mismatch");

        unsigned int loCurr = 400;
        TEST_CHECK(lx200.writeLowCurr(ax, loCurr) == LX200_VALUESET,
                   "writeLowCurr(2, 400)", "rejected");
        TEST_CHECK(lx200.readLowCurr(ax, loCurrOut) == LX200_VALUEGET,
                   "readLowCurr(2)", "no response");
        printf("         LowCurr: set 400 -> read %u\n", loCurrOut);
        TEST_CHECK(loCurrOut == 400, "lowCurr == 400", "mismatch");
    }

    /* ---- Rates & Acceleration ---- */
    printf("\n[TEST] Rates & Acceleration round-trip\n");
    {
        float accOut = 0;
        TEST_CHECK(lx200.setAcceleration(100.0f) == LX200_VALUESET,
                   "setAcceleration(100 -> 10.0 deg)", "rejected");
        TEST_CHECK(lx200.getAcceleration(accOut) == LX200_VALUEGET,
                   "getAcceleration", "no response");
        printf("         Acceleration: set 100 (=10.0 deg) -> read %.1f\n", accOut);
        TEST_CHECK(fabs(accOut - 10.0f) < 0.2f, "acceleration == 10.0", "mismatch");

        int maxRateOut = 0;
        TEST_CHECK(lx200.setMaxRate(800) == LX200_VALUESET,
                   "setMaxRate(800)", "rejected");
        TEST_CHECK(lx200.getMaxRate(maxRateOut) == LX200_VALUEGET,
                   "getMaxRate", "no response");
        printf("         MaxRate: set 800 -> read %d\n", maxRateOut);
        TEST_CHECK(maxRateOut == 800, "maxRate == 800", "mismatch");

        int dbOut = 0;
        TEST_CHECK(lx200.setDeadband(3) == LX200_VALUESET,
                   "setDeadband(3)", "rejected");
        TEST_CHECK(lx200.getDeadband(dbOut) == LX200_VALUEGET,
                   "getDeadband", "no response");
        printf("         Deadband: set 3 -> read %d\n", dbOut);
        TEST_CHECK(dbOut == 3, "deadband == 3", "mismatch");

        float rateOut = 0;
        TEST_CHECK(lx200.setSpeedRate(1, 128.0f) == LX200_VALUESET,
                   "setSpeedRate(1, 128)", "rejected");
        TEST_CHECK(lx200.getSpeedRate(1, rateOut) == LX200_VALUEGET,
                   "getSpeedRate(1)", "no response");
        printf("         SpeedRate[1]: set 128 -> read %.0f\n", rateOut);
        TEST_CHECK(fabs(rateOut - 128.0f) < 1.0f, "rate[1] == 128", "mismatch");

        TEST_CHECK(lx200.setSpeedRate(2, 64.0f) == LX200_VALUESET,
                   "setSpeedRate(2, 64)", "rejected");
        TEST_CHECK(lx200.getSpeedRate(2, rateOut) == LX200_VALUEGET,
                   "getSpeedRate(2)", "no response");
        printf("         SpeedRate[2]: set 64 -> read %.0f\n", rateOut);
        TEST_CHECK(fabs(rateOut - 64.0f) < 1.0f, "rate[2] == 64", "mismatch");
    }

    /* ---- Bulk Config Readback (GXCS) ---- */
    printf("\n[TEST] Bulk Config readback (GXCS)\n");
    {
        ta_MountStatus.updateAllConfig(true);
        bool hasConf = ta_MountStatus.hasConfig();
        TEST_CHECK(hasConf, "updateAllConfig (GXCS)", "decode failed");
        if (hasConf) {
            printf("         Axis1 gear:%.1f step:%u micro:%u hiCurr:%u loCurr:%u\n",
                   ta_MountStatus.getCfgGear(1),
                   (unsigned)ta_MountStatus.getCfgStepRot(1),
                   (unsigned)ta_MountStatus.getCfgMicro(1),
                   (unsigned)ta_MountStatus.getCfgHighCurr(1),
                   (unsigned)ta_MountStatus.getCfgLowCurr(1));
            printf("         Axis2 gear:%.1f step:%u micro:%u hiCurr:%u loCurr:%u\n",
                   ta_MountStatus.getCfgGear(2),
                   (unsigned)ta_MountStatus.getCfgStepRot(2),
                   (unsigned)ta_MountStatus.getCfgMicro(2),
                   (unsigned)ta_MountStatus.getCfgHighCurr(2),
                   (unsigned)ta_MountStatus.getCfgLowCurr(2));
            printf("         Accel:%.1f MaxRate:%u Deadband:%u\n",
                   ta_MountStatus.getCfgAcceleration(),
                   (unsigned)ta_MountStatus.getCfgMaxRate(),
                   (unsigned)ta_MountStatus.getCfgDefaultRate());
        }
    }

    /* ============================================================ */
    /*  Tracking / Status / Focuser                                 */
    /* ============================================================ */

    printf("\n[TEST] Tracking\n");
    TEST_CHECK(lx200.enableTracking(true) == LX200_VALUESET, "enableTracking(on)", "failed");
    TEST_CHECK(lx200.enableTracking(false) == LX200_VALUESET, "enableTracking(off)", "failed");

    printf("\n[TEST] GXAS Bulk Status\n");
    {
        char rawGxas[200] = "";
        LX200RETURN gr = lx200.get(":GXAS#", rawGxas, sizeof(rawGxas));
        printf("         raw GXAS get: ret=%d len=%d\n", (int)gr, (int)strlen(rawGxas));
        if (gr == LX200_VALUEGET) {
            printf("         raw GXAS (first 40): '%.40s'\n", rawGxas);
            printf("         raw GXAS (last  20): '%s'\n",
                   strlen(rawGxas) > 20 ? rawGxas + strlen(rawGxas) - 20 : rawGxas);
        }
    }
    ta_MountStatus.updateAllState(true);
    bool gxasOk = ta_MountStatus.hasInfoMount();
    TEST_CHECK(gxasOk, "updateAllState (GXAS)", "failed");
    if (gxasOk) {
        printf("         hasFocuser: %s\n", ta_MountStatus.hasFocuser() ? "YES" : "NO");
        TEST_CHECK(ta_MountStatus.hasFocuser(), "hasFocuser in GXAS", "not detected");
    }

    printf("\n[TEST] Focuser Config (readFocuserConfig via :FA#)\n");
    unsigned int sP, maxP, minS, maxS, cmdAcc, manAcc, manDec;
    LX200RETURN fr = lx200.readFocuserConfig(sP, maxP, minS, maxS, cmdAcc, manAcc, manDec);
    TEST_CHECK(fr == LX200_VALUEGET, "readFocuserConfig", "decode failed");
    if (fr == LX200_VALUEGET) {
        printf("         startPos=%u maxPos=%u lowSpeed=%u highSpeed=%u\n", sP, maxP, minS, maxS);
        printf("         cmdAcc=%u manAcc=%u manDec=%u\n", cmdAcc, manAcc, manDec);
        TEST_CHECK(maxP > 0, "maxPos > 0", "zero");
    }

    printf("\n[TEST] Focuser Motor (readFocuserMotor via :FA#)\n");
    bool rev; unsigned int micro, incr, curr, steprot;
    fr = lx200.readFocuserMotor(rev, micro, incr, curr, steprot);
    TEST_CHECK(fr == LX200_VALUEGET, "readFocuserMotor", "decode failed");
    if (fr == LX200_VALUEGET)
        printf("         reverse=%d micro=%u resolution=%u current=%u steprot=%u\n", (int)rev, micro, incr, curr, steprot);

    printf("\n[TEST] Focuser Version\n");
    fr = lx200.getFocuserVersion(buf, sizeof(buf));
    TEST_CHECK(fr == LX200_VALUEGET, "getFocuserVersion", "no response");
    if (fr == LX200_VALUEGET) printf("         Version: %s\n", buf);

    printf("\n[TEST] Focuser Set Parameters\n");
    TEST_CHECK(lx200.setFocuserMaxPos(20000) == LX200_VALUESET, "setFocuserMaxPos(20000)", "rejected");
    TEST_CHECK(lx200.setFocuserPark(100) == LX200_VALUESET, "setFocuserPark(100)", "rejected");
    TEST_CHECK(lx200.setFocuserLowSpeed(75) == LX200_VALUESET, "setFocuserLowSpeed(75)", "rejected");
    TEST_CHECK(lx200.setFocuserHighSpeed(300) == LX200_VALUESET, "setFocuserHighSpeed(300)", "rejected");
    fr = lx200.readFocuserConfig(sP, maxP, minS, maxS, cmdAcc, manAcc, manDec);
    TEST_CHECK(fr == LX200_VALUEGET, "re-read after set", "decode failed");
    if (fr == LX200_VALUEGET) {
        TEST_CHECK(sP == 100,    "park pos after set",   "expected 100");
        TEST_CHECK(maxP == 20000, "max pos after set",   "expected 20000");
        TEST_CHECK(minS == 75,    "low speed after set",  "expected 75");
        TEST_CHECK(maxS == 300,   "high speed after set", "expected 300");
    }

    printf("\n[TEST] WiFi\n");
    {
        LX200RETURN wr = lx200.set(":EW1#");
        TEST_CHECK(wr == LX200_VALUESET, "WiFi ON (:EW1#)", "rejected");
        wr = lx200.set(":EW0#");
        TEST_CHECK(wr == LX200_VALUESET, "WiFi OFF (:EW0#)", "rejected");
    }

    /* ============================================================ */
    /*  Goto / Guiding / Slew tests                                 */
    /* ============================================================ */

    /* ---- 14. Position Read ---- */
    printf("\n[TEST] Position Readback\n");
    {
        char raStr[32], decStr[32];
        TEST_CHECK(lx200.getRaStr(raStr, sizeof(raStr)) == LX200_VALUEGET,
                   "getRaStr (:GR#)", "no response");
        printf("         RA:  %s\n", raStr);
        TEST_CHECK(lx200.getDecStr(decStr, sizeof(decStr)) == LX200_VALUEGET,
                   "getDecStr (:GD#)", "no response");
        printf("         Dec: %s\n", decStr);
    }

    /* ---- 21. Sync ---- */
    printf("\n[TEST] Sync (set mount position)\n");
    {
        float syncRa = 6.0f, syncDec = 45.0f;
        LX200RETURN sr = lx200.syncGoto(NAV_SYNC, syncRa, syncDec);
        TEST_CHECK(sr == LX200_SYNCED, "syncGoto(SYNC, 6h, +45d)", "sync failed");
        printf("         Synced to RA=6h Dec=+45d\n");

        ta_MountStatus.updateAllState(true);
        char raStr[32], decStr[32];
        lx200.getRaStr(raStr, sizeof(raStr));
        lx200.getDecStr(decStr, sizeof(decStr));
        printf("         After sync: RA=%s Dec=%s\n", raStr, decStr);
    }

    /* ---- 22. Goto ---- */
    printf("\n[TEST] Goto (slew to target)\n");
    {
        lx200.enableTracking(true);

        float gotoRa = 6.5f, gotoDec = 44.0f;
        LX200RETURN gr = lx200.syncGoto(NAV_GOTO, gotoRa, gotoDec);
        TEST_CHECK(gr == LX200_GOTO_TARGET, "syncGoto(GOTO, 6.5h, +44d)", "goto rejected");
        printf("         Goto command accepted -> slewing to RA=6.5h Dec=+44d\n");

        ta_MountStatus.updateAllState(true);
        auto trk = ta_MountStatus.getTrackingState();
        printf("         Tracking state: %d (2=SLEWING)\n", (int)trk);
        TEST_CHECK(trk == TeenAstroMountStatus::TRK_SLEWING, "mount is slewing", "not slewing after goto");

        printf("         Waiting for slew to complete...\n");
        int polls = 0;
        bool slewDone = false;
        while (polls < 600) {
            SDL_Delay(100);
            ta_MountStatus.updateAllState(true);
            if (ta_MountStatus.getTrackingState() != TeenAstroMountStatus::TRK_SLEWING) {
                slewDone = true;
                break;
            }
            polls++;
        }
        if (!slewDone) lx200.stopSlew();
        TEST_CHECK(slewDone, "slew completed within 60s", "timed out");
        printf("         Slew finished after ~%.1fs\n", polls * 0.1);

        char raStr[32], decStr[32];
        lx200.getRaStr(raStr, sizeof(raStr));
        lx200.getDecStr(decStr, sizeof(decStr));
        printf("         Final position: RA=%s Dec=%s\n", raStr, decStr);

        ta_MountStatus.updateAllState(true);
        trk = ta_MountStatus.getTrackingState();
        TEST_CHECK(trk == TeenAstroMountStatus::TRK_ON, "tracking resumed after slew", "not tracking");
    }

    /* ---- 15. Manual Move ---- */
    printf("\n[TEST] Manual Move (N/S/E/W)\n");
    {
        lx200.setSpeed(1);
        TEST_CHECK(lx200.startMoveNorth() == LX200_VALUESET, "startMoveNorth", "rejected");
        SDL_Delay(200);
        TEST_CHECK(lx200.stopMoveNorth() == LX200_VALUESET, "stopMoveNorth", "rejected");

        TEST_CHECK(lx200.startMoveEast() == LX200_VALUESET, "startMoveEast", "rejected");
        SDL_Delay(200);
        TEST_CHECK(lx200.stopMoveEast() == LX200_VALUESET, "stopMoveEast", "rejected");

        TEST_CHECK(lx200.startMoveSouth() == LX200_VALUESET, "startMoveSouth", "rejected");
        SDL_Delay(200);
        TEST_CHECK(lx200.stopMoveSouth() == LX200_VALUESET, "stopMoveSouth", "rejected");

        TEST_CHECK(lx200.startMoveWest() == LX200_VALUESET, "startMoveWest", "rejected");
        SDL_Delay(200);
        TEST_CHECK(lx200.stopMoveWest() == LX200_VALUESET, "stopMoveWest", "rejected");
    }

    /* ---- 16. Pulse Guide ---- */
    printf("\n[TEST] Pulse Guide (:Mgdnnnn#)\n");
    {
        char reply[32];

        lx200.get(":Mge1000#", reply, sizeof(reply));
        TEST_OK("pulse guide East 1000ms sent");
        SDL_Delay(200);

        lx200.get(":Mgw1000#", reply, sizeof(reply));
        TEST_OK("pulse guide West 1000ms sent");
        SDL_Delay(200);

        lx200.get(":Mgn0500#", reply, sizeof(reply));
        TEST_OK("pulse guide North 500ms sent");
        SDL_Delay(200);

        lx200.get(":Mgs0500#", reply, sizeof(reply));
        TEST_OK("pulse guide South 500ms sent");
        SDL_Delay(200);

        ta_MountStatus.updateAllState(true);
        auto trk = ta_MountStatus.getTrackingState();
        printf("         Tracking state after guides: %d (1=ON)\n", (int)trk);
        TEST_CHECK(trk == TeenAstroMountStatus::TRK_ON,
                   "still tracking after pulse guides", "lost tracking");
    }

    /* ---- 17. Speed Rates ---- */
    printf("\n[TEST] Speed Rates\n");
    {
        for (int i = 0; i <= 4; i++) {
            char name[32];
            sprintf(name, "setSpeed(%d)", i);
            TEST_CHECK(lx200.setSpeed(i) == LX200_VALUESET, name, "rejected");
        }

        float rate = 0;
        LX200RETURN rr = lx200.getSpeedRate(1, rate);
        TEST_CHECK(rr == LX200_VALUEGET, "getSpeedRate(1)", "no response");
        printf("         Speed rate[1]: %.2f\n", rate);
    }

    /* ---- 18. Stop Slew / Emergency Stop ---- */
    printf("\n[TEST] Emergency Stop\n");
    {
        TEST_CHECK(lx200.stopSlew() == LX200_VALUESET, "stopSlew (:Q#)", "rejected");
    }

    /* ---- 19. Second Goto + Stop mid-slew ---- */
    printf("\n[TEST] Goto + Stop mid-slew\n");
    {
        float gotoRa = 8.0f, gotoDec = 30.0f;
        LX200RETURN gr = lx200.syncGoto(NAV_GOTO, gotoRa, gotoDec);
        TEST_CHECK(gr == LX200_GOTO_TARGET, "second goto (8h, +30d)", "rejected");

        SDL_Delay(500);
        ta_MountStatus.updateAllState(true);
        auto trk = ta_MountStatus.getTrackingState();
        printf("         Mid-slew state: %d (2=SLEWING)\n", (int)trk);
        TEST_CHECK(trk == TeenAstroMountStatus::TRK_SLEWING, "slewing in progress", "not slewing");

        TEST_CHECK(lx200.stopSlew() == LX200_VALUESET, "stop mid-slew (:Q#)", "rejected");
        SDL_Delay(500);
        ta_MountStatus.updateAllState(true);
        trk = ta_MountStatus.getTrackingState();
        printf("         After stop: %d (1=ON)\n", (int)trk);
        TEST_CHECK(trk != TeenAstroMountStatus::TRK_SLEWING, "slew stopped", "still slewing");
    }

    /* ---- 20. Alignment Star Name round-trip (:SXAs / :GXAs) ---- */
    printf("\n[TEST] Alignment Star Name round-trip\n");
    {
        const char* testName = "Vega";
        char setCmd[64];
        sprintf(setCmd, ":SXAs,%s#", testName);
        LX200RETURN sr = lx200.set(setCmd);
        TEST_CHECK(sr == LX200_VALUESET, "setAlignStarName(Vega)", "rejected");

        char readName[16] = "";
        LX200RETURN gr = lx200.getAlignStarName(readName, sizeof(readName));
        TEST_CHECK(gr == LX200_VALUEGET, "getAlignStarName", "no response");
        printf("         Star name: set '%s' -> read '%s'\n", testName, readName);
        TEST_CHECK(strcmp(readName, testName) == 0, "starName == Vega", readName);

        const char* testName2 = "Polaris";
        sprintf(setCmd, ":SXAs,%s#", testName2);
        sr = lx200.set(setCmd);
        TEST_CHECK(sr == LX200_VALUESET, "setAlignStarName(Polaris)", "rejected");

        memset(readName, 0, sizeof(readName));
        gr = lx200.getAlignStarName(readName, sizeof(readName));
        TEST_CHECK(gr == LX200_VALUEGET, "getAlignStarName (2)", "no response");
        printf("         Star name: set '%s' -> read '%s'\n", testName2, readName);
        TEST_CHECK(strcmp(readName, testName2) == 0, "starName == Polaris", readName);

        const char* longName = "AlphaCentauri";
        sprintf(setCmd, ":SXAs,%s#", longName);
        sr = lx200.set(setCmd);
        TEST_CHECK(sr == LX200_VALUESET, "setAlignStarName(AlphaCentauri)", "rejected");

        memset(readName, 0, sizeof(readName));
        gr = lx200.getAlignStarName(readName, sizeof(readName));
        TEST_CHECK(gr == LX200_VALUEGET, "getAlignStarName (3)", "no response");
        printf("         Star name: set '%s' -> read '%s'\n", longName, readName);
        TEST_CHECK(strcmp(readName, longName) == 0, "starName == AlphaCentauri", readName);
    }

    /* ---- 21. Two-client alignment: App on WiFi sets name, SHC reads it ---- */
    printf("\n[TEST] Two-client alignment (App via WiFi, SHC reads star name)\n");
    {
        // Enable WiFi port (9999) on the emulator
        LX200RETURN wr = lx200.set(":EW1#");
        TEST_CHECK(wr == LX200_VALUESET, "WiFi ON for two-client test", "rejected");
        SDL_Delay(500);

        // Create a second client (simulating the App) on WiFi port 9999
        TcpClientStream wifiStream;
        bool wifiOk = wifiStream.connect("127.0.0.1", 9999);
        TEST_CHECK(wifiOk, "App connects to WiFi port 9999", "connection failed");

        if (wifiOk) {
            LX200Client appClient(wifiStream, 200);

            // Abort any prior alignment via SHC (clean state)
            lx200.alignAbort();
            SDL_Delay(200);
            ta_MountStatus.updateAllState(true);
            TEST_CHECK(!ta_MountStatus.isAligning(), "idle before two-client test", "still aligning");

            // APP starts alignment (:A0#)
            LX200RETURN ar = appClient.set(":A0#");
            TEST_CHECK(ar == LX200_VALUESET, "App: start alignment (:A0#)", "rejected");
            SDL_Delay(200);

            // SHC polls GXAS — should detect remote alignment
            ta_MountStatus.updateAllState(true);
            uint8_t phase = ta_MountStatus.getMountAlignPhase();
            printf("         SHC sees: phase=%d isAligning=%d isRemoteAlign=%d\n",
                   phase, ta_MountStatus.isAligning(), ta_MountStatus.isRemoteAlign());
            TEST_CHECK(phase == 1, "SHC: alignPhase == SELECT", "wrong phase");
            TEST_CHECK(ta_MountStatus.isRemoteAlign(), "SHC: isRemoteAlign", "not remote");

            // APP sets star name (:SXAs,Vega#)
            ar = appClient.set(":SXAs,Vega#");
            TEST_CHECK(ar == LX200_VALUESET, "App: setAlignStarName(Vega)", "rejected");
            SDL_Delay(100);

            // SHC fetches star name (as it would in the display loop)
            ta_MountStatus.fetchRemoteStarName();
            const char* dispName = ta_MountStatus.getRemoteStarName();
            printf("         SHC remote star name: '%s'\n", dispName);
            TEST_CHECK(strcmp(dispName, "Vega") == 0, "SHC displays 'Vega'", dispName);

            // SHC verifies via direct GXAs query
            char readName[16] = "";
            LX200RETURN gr = lx200.getAlignStarName(readName, sizeof(readName));
            TEST_CHECK(gr == LX200_VALUEGET, "SHC: getAlignStarName", "no response");
            printf("         SHC direct GXAs: '%s'\n", readName);
            TEST_CHECK(strcmp(readName, "Vega") == 0, "SHC GXAs == Vega", readName);

            // APP changes star name to Polaris
            ar = appClient.set(":SXAs,Polaris#");
            TEST_CHECK(ar == LX200_VALUESET, "App: setAlignStarName(Polaris)", "rejected");
            SDL_Delay(100);

            // SHC refetches — should get new name
            ta_MountStatus.fetchRemoteStarName();
            dispName = ta_MountStatus.getRemoteStarName();
            printf("         SHC updated star name: '%s'\n", dispName);
            TEST_CHECK(strcmp(dispName, "Polaris") == 0, "SHC displays 'Polaris'", dispName);

            // Cleanup
            appClient.alignAbort();
            SDL_Delay(200);
            ta_MountStatus.updateAllState(true);
            lx200.set(":EW0#");
            printf("         Cleanup: alignment aborted, WiFi OFF\n");
        }
    }

    printf("\n========================================\n");
    printf("  Results: %d passed, %d failed\n", g_testPass, g_testFail);
    printf("========================================\n\n");
    fflush(stdout);
    return g_testFail > 0 ? 1 : 0;
}

/* ------------------------------------------------------------------ */
/*  Main                                                                */
/* ------------------------------------------------------------------ */
int main(int argc, char** argv)
{
    bool testMode = false;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--test") == 0) testMode = true;
    }

    printf("=== TeenAstro SHC Emulator ===\n");
    if (testMode) printf(">>> TEST MODE <<<\n");
    printf("Connecting to MainUnit at 127.0.0.1:9998...\n");
    fflush(stdout);

    sim::enableRealtime();

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
        printf("SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    /* Launch MainUnit as a child process */
    if (launchMainUnit()) {
        printf("Launched MainUnit emulator (auto)\n");
        SDL_Delay(2000);
    } else {
        printf("MainUnit exe not found next to SHC -- assuming already running.\n");
    }
    fflush(stdout);

    /* Connect to MainUnit */
    int retries = 20;
    while (!g_tcpToMainUnit.connect("127.0.0.1", 9998)) {
        if (--retries <= 0) {
            printf("Failed to connect to MainUnit on port 9998.\n");
            killMainUnit();
            return 1;
        }
        printf("  Retrying connection...\n");
        SDL_Delay(500);
    }

    /* Create LX200 client over TCP */
    g_lx200 = new LX200Client(g_tcpToMainUnit, 100);

    ta_MountStatus.setClient(*g_lx200);
    TeenAstroWifi::setClient(*g_lx200);
    HdCrtlr.setClient(*g_lx200);

    /* Match real SHC version from SmartController.h, suffix -emu */
    const char SHCVersion[] = SHCFirmwareVersionMajor "." SHCFirmwareVersionMinor "." SHCFirmwareVersionPatch "-emu";
    const int pin[7] = { 0, 0, 0, 0, 0, 0, 0 };
    const bool active[7] = { false, false, false, false, false, false, false };

    HdCrtlr.setup(SHCVersion, pin, active, 57600,
                   SmartHandController::OLED::OLED_SSD1306, 1);

    printf("[SHC-EMU] Setup complete.\n");
    fflush(stdout);

    /* Sync MainUnit clock to PC UTC time */
    {
        time_t now = time(nullptr);
        struct tm* utc = gmtime(&now);
        if (utc) {
            g_lx200->setUTCTimeRaw(utc->tm_hour, utc->tm_min, utc->tm_sec);
            g_lx200->setUTCDateRaw(utc->tm_mon + 1, utc->tm_mday, utc->tm_year % 100);
            printf("[SHC-EMU] Synced MainUnit UTC to %04d-%02d-%02d %02d:%02d:%02d\n",
                   utc->tm_year + 1900, utc->tm_mon + 1, utc->tm_mday,
                   utc->tm_hour, utc->tm_min, utc->tm_sec);
            fflush(stdout);
        }
    }

    if (testMode) {
        int rc = runAutomatedTests(*g_lx200);
        killMainUnit();
        SDL_Quit();
        return rc;
    }

    printf("[SHC-EMU] Entering main loop...\n");
    printf("[SHC-EMU] Keys: Space=page, Hold Space+Right=Settings, Hold Space+Left=Goto\n");
    printf("[SHC-EMU]       Hold Space+3=FocuserSettings, Hold Space+1=FocuserAction\n");
    printf("[SHC-EMU]       In menus: Up/Down=scroll, Right(6)=select, Left(4)=back\n");
    fflush(stdout);

    while (true) {
        HdCrtlr.update();
        if (g_sdlDisplay) g_sdlDisplay->blitToSDL();
        SDL_Delay(10);
    }

    emu_shutdown();
    return 0;
}

/*
 * emu_mainunit_main.cpp -- Single-TU build of the TeenAstro MainUnit emulator.
 *
 * Build:  pio run -d TeenAstroEmulator -e emu_mainunit
 * Run:    .pio\build\emu\mainunit_emu.exe
 */

#ifdef EMU_MAINUNIT

/* Include Winsock BEFORE anything that defines INPUT/OUTPUT macros */
#ifdef _WIN32
#  ifndef _WIN32_WINNT
#    define _WIN32_WINNT 0x0601
#  endif
#  include <winsock2.h>
#  include <ws2tcpip.h>
   /* Windows dlgs.h defines lst1..lst16 as dialog-control IDs, which
      collides with the local variable name lst1 in TelTimer.hpp */
#  undef lst1
#  undef lst2
#endif

/* Entry point + globals */
#include "../emu/mainunit_emu.cpp"

/* ---- Library .cpp files (pure math, no hardware) ---- */
#include "../../libraries/TeenAstroLA3/TeenAstroLA3.cpp"
#include "../../libraries/TeenAstroCoord/TeenAstroCoord_EQ.cpp"
#include "../../libraries/TeenAstroCoord/TeenAstroCoord_HO.cpp"
#include "../../libraries/TeenAstroCoord/TeenAstroCoord_IN.cpp"
#include "../../libraries/TeenAstroCoord/TeenAstroCoord_LO.cpp"
#include "../../libraries/TeenAstroCoordConv/TeenAstroCoordConv.cpp"
#include "../../libraries/TeenAstroMath/src/TeenAstroMath.cpp"
/* CommandCodec.cpp has a static atoi2() that clashes with TeenAstroMath's version
   in a single-TU build. Rename it before inclusion. */
#define atoi2 _codec_atoi2
#include "../../libraries/TeenAstroCommandDef/src/CommandCodec.cpp"
#undef atoi2

/* ---- MainUnit firmware .cpp files ---- */
#include "../../TeenAstroMainUnit/Application.cpp"
#include "../../TeenAstroMainUnit/Command.cpp"
#include "../../TeenAstroMainUnit/CommandSerial.cpp"
#include "../../TeenAstroMainUnit/Command_A.cpp"
#include "../../TeenAstroMainUnit/Command_ACK.cpp"
#include "../../TeenAstroMainUnit/Command_B.cpp"
#include "../../TeenAstroMainUnit/Command_C.cpp"
#include "../../TeenAstroMainUnit/Command_D.cpp"
#include "../../TeenAstroMainUnit/Command_dollar.cpp"
#include "../../TeenAstroMainUnit/Command_E.cpp"
#include "../../TeenAstroMainUnit/Command_Focuser.cpp"
#include "../../TeenAstroMainUnit/Command_G.cpp"
#include "../../TeenAstroMainUnit/Command_GNSS.cpp"
/* Command_GX.cpp has static helpers (PrintAtitude etc.) that clash with
   Command_G.cpp in a single-TU build. Rename them. */
#define PrintAtitude _GX_PrintAtitude
#define PrintAzimuth _GX_PrintAzimuth
#define PrintDec     _GX_PrintDec
#define PrintRa      _GX_PrintRa
#include "../../TeenAstroMainUnit/Command_GX.cpp"
#undef PrintAtitude
#undef PrintAzimuth
#undef PrintDec
#undef PrintRa
#include "../../TeenAstroMainUnit/Command_h.cpp"
#include "../../TeenAstroMainUnit/Command_M.cpp"
#include "../../TeenAstroMainUnit/Command_others.cpp"
#include "../../TeenAstroMainUnit/Command_Q.cpp"
#include "../../TeenAstroMainUnit/Command_R.cpp"
#include "../../TeenAstroMainUnit/Command_S.cpp"
#include "../../TeenAstroMainUnit/Command_SX.cpp"
#include "../../TeenAstroMainUnit/Command_T.cpp"
#include "../../TeenAstroMainUnit/Command_U.cpp"
#include "../../TeenAstroMainUnit/Command_W.cpp"
#include "../../TeenAstroMainUnit/EEPROM.cpp"
#include "../../TeenAstroMainUnit/Mount.cpp"
#include "../../TeenAstroMainUnit/MountAxes.cpp"
#include "../../TeenAstroMainUnit/MountAxisLimitManager.cpp"
#include "../../TeenAstroMainUnit/MountGotoSync.cpp"
#include "../../TeenAstroMainUnit/MountGuiding.cpp"
#include "../../TeenAstroMainUnit/MountLimits.cpp"
#include "../../TeenAstroMainUnit/MountMove.cpp"
#include "../../TeenAstroMainUnit/MountMoveTo.cpp"
#include "../../TeenAstroMainUnit/MountParkHomeController.cpp"
#include "../../TeenAstroMainUnit/MountQueriesTracking.cpp"
#include "../../TeenAstroMainUnit/MountST4.cpp"
#include "../../TeenAstroMainUnit/PushTo.cpp"
#include "../../TeenAstroMainUnit/Timer.cpp"
#include "../../TeenAstroMainUnit/ValueToString.cpp"

/* NOTE: TeenAstroMainUnit.cpp is NOT included because it defines
 * setup()/loop()/reboot() which we override in mainunit_emu.cpp. */

#endif /* EMU_MAINUNIT */

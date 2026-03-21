#pragma once
#include <TeenAstroLanguage.h>

#if LANGUAGE == ENGLISH
#include "SHC_text_English.h"
#elif LANGUAGE == FRENCH
#include "SHC_text_French.h"
#elif LANGUAGE == GERMAN
#include "SHC_text_German.h"
#endif

// ============================================================
// Compile-time safety: buffer overflow checks
// ============================================================
// sizeof("str") includes the null terminator, so
// sizeof(S) <= N guarantees S fits in char[N].

// SmartController_Display.cpp
static_assert(sizeof(T_SLEWINGTO " " T_STAR) <= 29, "T_SLEWINGTO+T_STAR exceeds text1[29]");
static_assert(sizeof(T_RECENTER " " T_STAR)  <= 28, "T_RECENTER+T_STAR exceeds text2[28]");

// SmartController.cpp
static_assert(sizeof(T_STAR "#?") <= 10, "T_STAR '#?' exceeds message[10]");

// SmartController_Message.cpp  (text1[32] / text2[32])
static_assert(sizeof(T_LX200COMMAND)           <= 32, "T_LX200COMMAND exceeds text1[32]");
static_assert(sizeof(T_TELESCOPEEXCEED)         <= 32, "T_TELESCOPEEXCEED exceeds text1[32]");
static_assert(sizeof(T_TELESCOPEIS)             <= 32, "T_TELESCOPEIS exceeds text1[32]");
static_assert(sizeof(T_TELESCOPEMOTOR)          <= 32, "T_TELESCOPEMOTOR exceeds text1[32]");
static_assert(sizeof(T_TARGETIS)                <= 32, "T_TARGETIS exceeds text1[32]");
static_assert(sizeof(T_NOOBJECT)                <= 32, "T_NOOBJECT exceeds text1[32]");
static_assert(sizeof(T_TELESCOPE)               <= 32, "T_TELESCOPE exceeds text1[32]");
static_assert(sizeof(T_TUNKOWN)                 <= 32, "T_TUNKOWN exceeds text1[32]");
static_assert(sizeof(T_HASFAILED "!")           <= 32, "T_HASFAILED '!' exceeds text2[32]");
static_assert(sizeof(T_BELOWHORIZON "!")        <= 32, "T_BELOWHORIZON '!' exceeds text2[32]");
static_assert(sizeof(T_ABOVEOVERHEAD "!")       <= 32, "T_ABOVEOVERHEAD '!' exceeds text2[32]");
static_assert(sizeof(T_SENSORLIMIT "!")         <= 32, "T_SENSORLIMIT '!' exceeds text2[32]");
static_assert(sizeof(T_UNDERPOLELIMIT "!")      <= 32, "T_UNDERPOLELIMIT '!' exceeds text2[32]");
static_assert(sizeof(T_MERIDIANLIMIT "!")       <= 32, "T_MERIDIANLIMIT '!' exceeds text2[32]");
static_assert(sizeof(T_OUTSIDELIMITS " A1!")    <= 32, "T_OUTSIDELIMITS ' A1!' exceeds text2[32]");
static_assert(sizeof(T_OUTSIDELIMITS " A2!")    <= 32, "T_OUTSIDELIMITS ' A2!' exceeds text2[32]");
static_assert(sizeof(T_OUTSIDELIMITS "!")       <= 32, "T_OUTSIDELIMITS '!' exceeds text2[32]");
static_assert(sizeof(T_CANTPARK "!")            <= 32, "T_CANTPARK '!' exceeds text2[32]");
static_assert(sizeof(T_CANTGOHOME "!")          <= 32, "T_CANTGOHOME '!' exceeds text2[32]");
static_assert(sizeof(T_SELECTED "!")            <= 32, "T_SELECTED '!' exceeds text2[32]");
static_assert(sizeof(T_SYNCED "!")              <= 32, "T_SYNCED '!' exceeds text2[32]");

// Settings_Motor.cpp  (line1..4[32])
static_assert(sizeof(T_REVERSEDROTATION)        <= 32, "T_REVERSEDROTATION exceeds line[32]");
static_assert(sizeof(T_DIRECTROTATION)          <= 32, "T_DIRECTROTATION exceeds line[32]");

// Settings_Mount.cpp  (line2[32])
static_assert(sizeof(T_SLEWSETTING)             <= 32, "T_SLEWSETTING exceeds line2[32]");

// ============================================================
// Compile-time safety: display width checks
// ============================================================
// The OLED is 128 px wide.  With u8g2_font_helvR12 the average
// glyph width is ~7-8 px, so roughly 17 visible characters fit.
// UTF-8 accented chars take 2 bytes but the same pixel width as
// the base letter, so byte count slightly over-estimates width.
//
// SHC_MAXBYTES_R12 = max sizeof() for a SINGLE LINE rendered in
//   helvR12 (DisplayMessage).  21 bytes ≈ 20 content bytes.
// SHC_MAXBYTES_R10 = max sizeof() for a single line rendered in
//   helvR10 (DisplayLongMessage / SelectionList).
// These are conservative: a string under the limit is guaranteed
// to fit; a string slightly over might still fit depending on the
// actual glyphs.

#define SHC_MAXBYTES_R12  21
#define SHC_MAXBYTES_R10  25

// --- DisplayMessage lines (helvR12, 128 px) ---

// text1 values in DisplayMessageLX200
static_assert(sizeof(T_TELESCOPEEXCEED) <= SHC_MAXBYTES_R12, "T_TELESCOPEEXCEED too wide for 128px (helvR12)");
static_assert(sizeof(T_TELESCOPEMOTOR)  <= SHC_MAXBYTES_R12, "T_TELESCOPEMOTOR too wide for 128px (helvR12)");
static_assert(sizeof(T_TELESCOPEIS)     <= SHC_MAXBYTES_R12, "T_TELESCOPEIS too wide for 128px (helvR12)");
static_assert(sizeof(T_LX200COMMAND)    <= SHC_MAXBYTES_R12, "T_LX200COMMAND too wide for 128px (helvR12)");

// text2 values in DisplayMessageLX200 (with trailing "!")
static_assert(sizeof(T_OUTSIDELIMITS " A1!") <= SHC_MAXBYTES_R12, "T_OUTSIDELIMITS ' A1!' too wide for 128px");
static_assert(sizeof(T_OUTSIDELIMITS " A2!") <= SHC_MAXBYTES_R12, "T_OUTSIDELIMITS ' A2!' too wide for 128px");
static_assert(sizeof(T_OUTSIDELIMITS "!")    <= SHC_MAXBYTES_R12, "T_OUTSIDELIMITS '!' too wide for 128px");
static_assert(sizeof(T_UNDERPOLELIMIT "!")   <= SHC_MAXBYTES_R12, "T_UNDERPOLELIMIT '!' too wide for 128px");
static_assert(sizeof(T_CANTPARK "!")         <= SHC_MAXBYTES_R12, "T_CANTPARK '!' too wide for 128px");
static_assert(sizeof(T_CANTGOHOME "!")       <= SHC_MAXBYTES_R12, "T_CANTGOHOME '!' too wide for 128px");
static_assert(sizeof(T_BELOWHORIZON "!")     <= SHC_MAXBYTES_R12, "T_BELOWHORIZON '!' too wide for 128px");

// other DisplayMessage calls
static_assert(sizeof(T_MOUNTSYNCED)        <= SHC_MAXBYTES_R12, "T_MOUNTSYNCED too wide for 128px (helvR12)");
static_assert(sizeof(T_CURRENTLYTRACKING)  <= SHC_MAXBYTES_R12, "T_CURRENTLYTRACKING too wide for 128px (helvR12)");
static_assert(sizeof(T_CANNOTBECHANGED)    <= SHC_MAXBYTES_R12, "T_CANNOTBECHANGED too wide for 128px (helvR12)");
static_assert(sizeof(T_STARADDED)          <= SHC_MAXBYTES_R12, "T_STARADDED too wide for 128px (helvR12)");
static_assert(sizeof(T_NOT_CONNECTED)      <= SHC_MAXBYTES_R12, "T_NOT_CONNECTED too wide for 128px (helvR12)");

// --- DisplayLongMessage / SelectionList lines (helvR10, 128 px) ---

static_assert(sizeof(T_STEPSPERROT)          <= SHC_MAXBYTES_R10, "T_STEPSPERROT too wide for 128px (helvR10)");
static_assert(sizeof(T_REVERSEDROTATION)     <= SHC_MAXBYTES_R10, "T_REVERSEDROTATION too wide for 128px (helvR10)");
static_assert(sizeof(T_DIRECTROTATION)       <= SHC_MAXBYTES_R10, "T_DIRECTROTATION too wide for 128px (helvR10)");
static_assert(sizeof(T_GERMANEQUATORIAL)     <= SHC_MAXBYTES_R10, "T_GERMANEQUATORIAL too wide for 128px (helvR10)");
static_assert(sizeof(T_RESETTOFACTORY)       <= SHC_MAXBYTES_R10, "T_RESETTOFACTORY too wide for 128px (helvR10)");
static_assert(sizeof(T_SHOWSETTINGS)         <= SHC_MAXBYTES_R10, "T_SHOWSETTINGS too wide for 128px (helvR10)");
static_assert(sizeof(T_TELESCOPESETTINGS)    <= SHC_MAXBYTES_R10, "T_TELESCOPESETTINGS too wide for 128px (helvR10)");
static_assert(sizeof(T_THEMOUNTMUSTBEATHOME1)<= SHC_MAXBYTES_R10, "T_THEMOUNTMUSTBEATHOME1 too wide for 128px (helvR10)");
static_assert(sizeof(T_THEMOUNTMUSTBEATHOME2)<= SHC_MAXBYTES_R10, "T_THEMOUNTMUSTBEATHOME2 too wide for 128px (helvR10)");
static_assert(sizeof(T_SHOWPASSWORD)         <= SHC_MAXBYTES_R10, "T_SHOWPASSWORD too wide for 128px (helvR10)");

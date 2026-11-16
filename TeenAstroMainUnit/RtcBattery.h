#pragma once
/**
 * A missing coin cell does not leave the RTC at a single date.
 *
 * Teensy 3 (Main Unit board 240): VBAT loss sets RTC_SR_TIF. The core then
 * seeds the clock with the firmware build time (__rtc_localtime) and writes
 * 0x5A94C3A5 into the VBAT register file at 0x4003E01C
 * (cores/teensy3/mk20dx128.c). That word is the "VBAT missing" mark. An
 * upload clears it, because the build time is still fresh; the next cold
 * boot with no cell writes it again. The clock the user sees is the build
 * time, not 1 Jan 2019, so the epoch test below does not catch this.
 *
 * Teensy 4: startup.c seeds 2019-01-01 00:00:00 UTC when SNVS_LPCR_SRTC_ENV
 * was clear. The counter then advances with uptime, so a dead cell is that
 * epoch plus the seconds since this boot.
 *
 * Checked once at startup. Setting the clock clears the Teensy 3 mark.
 */
#include <stdint.h>

static constexpr uint32_t kRtcUnsetEpoch = 1546300800u; // 2019-01-01 00:00:00 UTC
static constexpr uint32_t kRtcUnsetSkewSec = 60u;
static constexpr uint32_t kRtcVbatStaleMagic = 0x5A94C3A5u;
static constexpr uint32_t kRtcVbatRegFile = 0x4003E01Cu;

inline bool rtcTimeIsUnset(uint32_t rtcUnix, uint32_t uptimeMs)
{
  if (rtcUnix < kRtcUnsetEpoch)
    return false;
  uint32_t elapsed = uptimeMs / 1000u;
  uint32_t sinceSeed = rtcUnix - kRtcUnsetEpoch;
  uint32_t skew = (sinceSeed > elapsed) ? (sinceSeed - elapsed) : (elapsed - sinceSeed);
  return skew < kRtcUnsetSkewSec;
}

inline bool rtcVbatFileIsStale(uint32_t vbatFileWord)
{
  return vbatFileWord == kRtcVbatStaleMagic;
}

inline bool rtcNeedsTime(uint32_t rtcUnix, uint32_t uptimeMs, uint32_t vbatFileWord)
{
  return rtcVbatFileIsStale(vbatFileWord) || rtcTimeIsUnset(rtcUnix, uptimeMs);
}

inline uint32_t rtcVbatFileWord()
{
#if defined(KINETISK)
  return *(volatile uint32_t *)kRtcVbatRegFile;
#else
  return 0;
#endif
}

inline void rtcVbatClearStale()
{
#if defined(KINETISK)
  *(volatile uint32_t *)kRtcVbatRegFile = 0;
#endif
}

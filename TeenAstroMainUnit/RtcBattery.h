#pragma once
/**
 * Teensy 4 starts the RTC at 2019-01-01 00:00:00 UTC when VBAT did not keep
 * it running (cores/teensy4/startup.c, SNVS_LPCR_SRTC_ENV clear). After that
 * the counter advances with uptime, so a dead or missing cell is the RTC
 * sitting on that epoch plus the seconds since this boot.
 *
 * Checked once at startup. A clock that was actually set is years away.
 */
#include <stdint.h>

static constexpr uint32_t kRtcUnsetEpoch = 1546300800u; // 2019-01-01 00:00:00 UTC
static constexpr uint32_t kRtcUnsetSkewSec = 60u;

inline bool rtcTimeIsUnset(uint32_t rtcUnix, uint32_t uptimeMs)
{
  if (rtcUnix < kRtcUnsetEpoch)
    return false;
  uint32_t elapsed = uptimeMs / 1000u;
  uint32_t sinceSeed = rtcUnix - kRtcUnsetEpoch;
  uint32_t skew = (sinceSeed > elapsed) ? (sinceSeed - elapsed) : (elapsed - sinceSeed);
  return skew < kRtcUnsetSkewSec;
}

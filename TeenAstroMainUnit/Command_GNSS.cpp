/**
 * GNSS commands and helpers: :gs# (full sync), :gt# (time sync).
 * One file per letter (plan). All :gx# TeenAstro specific (not in Meade LX200).
 */
#include "Command.h"

#define GPS_COMPARISON_PERIOD_MS  2000
#define GPS_VALID_TIMEOUT_MS      2000

#define N_GNSS_OBS 12
static double dlat[N_GNSS_OBS];
static double dlng[N_GNSS_OBS];
static double dele[N_GNSS_OBS];
static int numSamples = 0;

void resetDeltaLoc()
{
  for (int i = 0; i < N_GNSS_OBS; i++)
  {
    dlat[i] = 0;
    dlng[i] = 0;
    dele[i] = 0;
  }
  numSamples = 0;
}


void UpdateGnss()
{
#if VERSION == 220
  return;
#endif
  if (mount.config.peripherals.hasGNSS && (mount.isAtHome() || mount.isParked()))
  {
    while (GNSS_Serial.available())
    {
      mount.gnss.encode(GNSS_Serial.read());
    }

    // Once after startup: if still at home and RTC differs from GNSS, sync time silently.
    static bool startupTimeSyncDone = false;
    if (!startupTimeSyncDone && mount.isAtHome() && iSGNSSValid())
    {
      TinyGPSDate d = mount.gnss.date;
      TinyGPSTime t = mount.gnss.time;
      long delta = rtk.GetDeltaUTC(d.year(), d.month(), d.day(),
        t.hour(), t.minute(), t.second());
      if (abs(delta) >= 1)
      {
        rtk.setClock(d.year(), d.month(), d.day(),
          t.hour(), t.minute(), t.second(),
          *localSite.longitude(), 0);
      }
      startupTimeSyncDone = true;
    }
  }
}

bool iSGNSSValid()
{
  if (!mount.config.peripherals.hasGNSS)
  {
    return false;
  }
  bool valid = GNSSTimeIsValid();
  valid &= GNSSLocationIsValid();
  return valid;
}


bool GNSSTimeIsValid()
{
  return mount.gnss.time.isValid() && mount.gnss.time.age() < GPS_VALID_TIMEOUT_MS &&
    mount.gnss.date.isValid() && mount.gnss.date.age() < GPS_VALID_TIMEOUT_MS;
}

bool GNSSLocationIsValid()
{
  return mount.gnss.location.isValid() && mount.gnss.location.age() < GPS_VALID_TIMEOUT_MS &&
    mount.gnss.altitude.isValid() && mount.gnss.altitude.age() < GPS_VALID_TIMEOUT_MS;
}

bool isHdopSmall()
{
  return mount.gnss.hdop.isValid() && mount.gnss.hdop.age() < GPS_VALID_TIMEOUT_MS && mount.gnss.hdop.hdop() < 2.0;
}

bool isTimeSyncWithGNSS()
{
  static unsigned long t1 = 0;
  static bool lastreply = false;
  // Gate on time/date updates — not location (location can stay static while time advances).
  if (millis() - t1 > GPS_COMPARISON_PERIOD_MS &&
      (mount.gnss.time.isUpdated() || mount.gnss.date.isUpdated()))
  {
    TinyGPSDate d = mount.gnss.date;
    TinyGPSTime t = mount.gnss.time;
    long delta = rtk.GetDeltaUTC(d.year(), d.month(), d.day(),
      t.hour(), t.minute(), t.second());
    lastreply = abs(delta) < 5;
    t1 = millis();
  }
  return lastreply;
}


double std_dev(double* val, int nval)
{
  double s = 0.0;
  double m = 0.0;
  double d;
  for (int i = 0; i < nval; i++)
  {
    m += val[i];
  }
  m /= nval;
  for (int i = 0; i < nval; i++)
  {
    d = val[i] - m;
    s += d * d;
  }
  return sqrt(s / nval);
}

double mean(double* val, int nval)
{
  double m = 0.0;
  for (int i = 0; i < nval; i++)
  {
    m += val[i];
  }
  return m / nval;
}

bool isLocationSyncWithGNSS()
{
  static int i = 0;
  static unsigned long t1 = 0;
  static bool lastreply = false;
  if (millis() - t1 > GPS_COMPARISON_PERIOD_MS && mount.gnss.location.isUpdated())
  {
    TinyGPSLocation l = mount.gnss.location;
    TinyGPSAltitude a = mount.gnss.altitude;

    dlng[i] = 3600 * fabs(haRange(*localSite.longitude() - (-l.lng())));
    dlat[i] = 3600 * fabs(*localSite.latitude() - l.lat());
    dele[i] = fabs(*localSite.elevation() - a.meters());
    if (numSamples < N_GNSS_OBS)
      numSamples++;
    int n = min(numSamples, N_GNSS_OBS);
    double dlng_s = max(std_dev(dlng, n), 1.0);
    double dlat_s = max(std_dev(dlat, n), 1.0);
    double dele_s = max(std_dev(dele, n), 20.0);
    lastreply = fabs(dlng[i] - mean(dlng, n)) < 2 * dlng_s && fabs(dlng[i]) < 2;
    lastreply &= fabs(dlat[i] - mean(dlat, n)) < 2 * dlat_s && fabs(dlat[i]) < 2;
    lastreply &= fabs(dele[i] - mean(dele, n)) < 2 * dele_s && fabs(dele[i]) < 30;
    i++;
    if (i == N_GNSS_OBS)
      i = 0;
    t1 = millis();
  }
  return lastreply;
}

// -----------------------------------------------------------------------------
//   g - GNSS  :gs#  full sync  :gt#  time sync
// -----------------------------------------------------------------------------
void Command_GNSS() {
  TinyGPSDate d = mount.gnss.date;
  TinyGPSTime t = mount.gnss.time;
  TinyGPSLocation l = mount.gnss.location;
  TinyGPSAltitude a = mount.gnss.altitude;

  switch (commandState.command[1]) {
  case 's':
    // :gs#  Full GNSS sync (site + time)  TeenAstro specific
    if (iSGNSSValid())
    {
      double lat = l.lat();
      double longi = -l.lng();
      double h = a.meters();
      localSite.setLong(longi);
      localSite.setLat(lat);
      localSite.setElev(h);
      initCelestialPole();
      mount.limits.initLimit();
      mount.initHome();
      mount.syncAtHome();
      initTransformation(true);
      rtk.setClock(d.year(), d.month(), d.day(),
        t.hour(), t.minute(), t.second(),
        *localSite.longitude(), 0);
      resetDeltaLoc();
      replyShortTrue();
    }
    else
      replyShortFalse();
    break;
  case 't':
    // :gt#  Time sync from GNSS  TeenAstro specific
    if (iSGNSSValid())
    {
      rtk.setClock(d.year(), d.month(), d.day(),
        t.hour(), t.minute(), t.second(),
        *localSite.longitude(), 0);
      replyShortTrue();
    }
    else
      replyShortFalse();
    break;
  default:
    replyNothing();
    break;
  }

}

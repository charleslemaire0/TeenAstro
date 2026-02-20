/**
 * GNSS commands and helpers: :gs# (full sync), :gt# (time sync).
 * One file per letter (plan). All :gx# TeenAstro specific (not in Meade LX200).
 */
#include "Command.h"

#define N_GNSS_OBS 3
static double dlat[N_GNSS_OBS];
static double dlng[N_GNSS_OBS];
static double dele[N_GNSS_OBS];

void resetDeltaLoc()
{
  for (int i = 0; i < N_GNSS_OBS; i++)
  {
    dlat[i] = 0;
    dlng[i] = 0;
    dele[i] = 0;
  }
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
  return mount.gnss.time.isValid() && mount.gnss.time.age() < 5000 &&  mount.gnss.date.isValid() && mount.gnss.date.age() < 5000;
}

bool GNSSLocationIsValid()
{
  return mount.gnss.location.isValid() && mount.gnss.location.age() < 5000 &&
    mount.gnss.altitude.isValid() && mount.gnss.altitude.age() < 5000;
}

bool isHdopSmall()
{
  return mount.gnss.hdop.isValid() && mount.gnss.hdop.age() < 5000 && mount.gnss.hdop.hdop() < 2.0;
}

bool isTimeSyncWithGNSS()
{
  static unsigned long t1 = 0;
  static bool lastreply = false;
  if (millis() - t1 > 5000)
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
  for (int i = 0; i < nval; i++)
  {
    m += val[i];
  }
  m /= nval;
  for (int i = 0; i < nval; i++)
  {
    s += pow(val[i] - m, 2);
  }
  return sqrt(s / (nval - 1));
}

bool isLocationSyncWithGNSS()
{
  static int i = 0;
  static unsigned long t1 = 0;
  static bool lastreply = false;
  if (millis() - t1 > 5000)
  {
    TinyGPSLocation l = mount.gnss.location;
    TinyGPSAltitude a = mount.gnss.altitude;

    dlng[i] = 3600*fabs(haRange(*localSite.longitude() - (-l.lng())));
    dlat[i] = 3600*fabs(*localSite.latitude() - l.lat());
    dele[i] = fabs(*localSite.elevation() - a.meters());
    double dlng_s= std_dev(dlng, N_GNSS_OBS);
    double dlat_s = std_dev(dlat, N_GNSS_OBS);
    double dele_s = std_dev(dele, N_GNSS_OBS);
    lastreply = dlng[i] < max(5 * dlng_s, 2);
    lastreply &= dlat[i] < max(5 * dlat_s, 2);
    lastreply &= dele[i] < max(5 * dele_s, 20);
    lastreply &= dlng_s < 2;
    lastreply &= dlat_s < 2;
    lastreply &= dele_s < 20;
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

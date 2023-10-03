#include "Command.h"
// This custom version of delay() ensures that the gps object
// is being "fed".
#define N_GNSS_OBS 3
static double dlat[N_GNSS_OBS];//*localSite.latitude();
static double dlng[N_GNSS_OBS];//*localSite.longitude(); 
static double dele[N_GNSS_OBS];//*localSite.elevation();

void resetDeltaLoc()
{
  for (int i = 0; i < N_GNSS_OBS; i++)
  {
    dlat[i] = 0;
    dlng[i] = 0;
    dele[i] = 0;
  }
}


static void UpdateGnss()
{
#if VERSION == 220
  return;
#endif
  if (hasGNSS && (atHome || parkStatus == PRK_PARKED))
  {
    while (GNSS_Serial.available())
    {
      gps.encode(GNSS_Serial.read());
    }
  }
}

bool iSGNSSValid()
{
  if (!hasGNSS)
  {
    return false;
  }
  bool valid = GNSSTimeIsValid();
  valid &= GNSSLocationIsValid();
  return valid;
}


bool GNSSTimeIsValid()
{
  return gps.time.isValid() && gps.time.age() < 5000 &&  gps.date.isValid() && gps.date.age() < 5000;
}

bool GNSSLocationIsValid()
{
  return gps.location.isValid() && gps.location.age() < 5000 &&
    gps.altitude.isValid() && gps.altitude.age() < 5000;
}

bool isHdopSmall()
{
  return gps.hdop.isValid() && gps.hdop.age() < 5000 && gps.hdop.hdop() < 2.0;
}

bool isTimeSyncWithGNSS()
{
  static unsigned long t1 = 0;
  static bool lastreply = false;
  if (millis() - t1 > 5000)
  {
    TinyGPSDate d = gps.date;
    TinyGPSTime t = gps.time;
    long delta = rtk.GetDeltaUTC(d.year(), d.month(), d.day(),
      t.hour(), t.minute(), t.second());
    lastreply = abs(delta) < 5;
    t1 = millis();
  }
  return lastreply;
}


double std_dev(double* val, int nval)
{
  double s = 0;
  double m = 0;
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
    TinyGPSLocation l = gps.location;
    TinyGPSAltitude a = gps.altitude;

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


    //sprintf(text, "lng= %+01.5f, lat=%+01.5f, ele= %+01.5f\n", dlng[i], dlat[i], dele[i]);
    //Serial.print(text);

    //sprintf(text, "lng_s= %+01.5f, lat_s=%+01.5f, ele_s= %+01.5f\n", dlng_s, dlat_s, dele_s);
    //Serial.print(text);
  }
  return lastreply;
}

void Command_GNSS()
{
  TinyGPSDate d = gps.date;
  TinyGPSTime t = gps.time;
  TinyGPSLocation l = gps.location;
  TinyGPSAltitude a = gps.altitude;

  switch (command[1])
  {
    // :gs# full sync with GNSS
  case 's':
    if (iSGNSSValid())
    {
      double lat = l.lat();
      double longi = -l.lng();
      double h = a.meters();
      localSite.setLong(longi);
      localSite.setLat(lat);
      localSite.setElev(h);
      initCelestialPole();
      initLimit();
      initHome();
      syncAtHome();
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
    // :gt# time sync with GNSS
  case 't':
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
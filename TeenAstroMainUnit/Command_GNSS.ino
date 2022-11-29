#include "Command.h"
// This custom version of delay() ensures that the gps object
// is being "fed".
static void smartDelay(unsigned long ms)
{
#if VERSION == 220
  return;
#endif
  if (!hasGNSS)
  {
    return;
  }
  unsigned long start = millis();
  do
  {
    while (GNSS_Serial.available())
    {
      gps.encode(GNSS_Serial.read());
    }
  } while (millis() - start < ms);
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

bool isLocationSyncWithGNSS()
{
  static unsigned long t1 = 0;
  static bool lastreply = false;
  if (millis() - t1 > 5000)
  {
    TinyGPSLocation l = gps.location;
    TinyGPSAltitude a = gps.altitude;
    double dlng = fabs(degRange(*localSite.longitude() - (-l.lng())));
    double dlat = fabs(*localSite.latitude() - l.lat());
    double dele = fabs(*localSite.elevation() - a.meters());
    lastreply = dlng < 0.01 && dlat < 0.01 && dele < 100;
    t1 = millis();
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
      initHome();
      syncAtHome();
      initTransformation(true);
      rtk.setClock(d.year(), d.month(), d.day(),
        t.hour(), t.minute(), t.second(),
        *localSite.longitude(), 0);
      strcpy(reply, "1");
    }
    else
      strcpy(reply, "0");
    break;
  case 't':
    if (iSGNSSValid())
    {
      rtk.setClock(d.year(), d.month(), d.day(),
        t.hour(), t.minute(), t.second(),
        *localSite.longitude(), 0);
      strcpy(reply, "1");
    }
    else
      strcpy(reply, "0");
    break;
  default:
    strcpy(reply, "0");
    break;
  }

}
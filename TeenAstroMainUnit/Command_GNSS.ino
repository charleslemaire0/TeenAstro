#include "Command.h"
// This custom version of delay() ensures that the gps object
// is being "fed".
static void smartDelay(unsigned long ms)
{
#if VERSION == 220
  return;
#endif
#if VERSION == 230
  return;
#endif
  if (!hasGNSS)
  {
    return;
  }
  unsigned long start = millis();
  do
  {
    while (Serial3.available())
    {
      gps.encode(Serial3.read());
    }
  } while (millis() - start < ms);
}

bool iSGNSSValid()
{
  if (!hasGNSS)
  {
    return false;
  }
  bool valid = gps.date.isValid() && gps.date.age() < 5000;
  valid &= gps.time.isValid() && gps.time.age() < 5000;
  valid &= gps.location.isValid() && gps.location.age() < 5000;
  valid &= gps.altitude.isValid() && gps.altitude.age() < 5000;
  return valid;
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
  case 'S':
    if (iSGNSSValid())
    {
      double lat = l.lat();
      double longi = -l.lng();
      double h = a.meters();
      localSite.setLong(longi, command[1] == 's');
      localSite.setLat(lat, command[1] == 's');
      localSite.setElev(h, command[1] == 's');
      initCelestialPole();
      initHome();
      syncAtHome();
      initTransformation(true);
      rtk.setClock(d.year(), d.month(), d.day(),
        t.hour(), t.minute(), t.second(),
        *localSite.longitude(), 0);
      //char sat[3];
      //sprintf(sat, "%d", gps.satellites);
      //strcpy(reply, sat);
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
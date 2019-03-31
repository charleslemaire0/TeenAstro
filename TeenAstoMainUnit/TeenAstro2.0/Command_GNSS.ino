
// This custom version of delay() ensures that the gps object
// is being "fed".
static void smartDelay(unsigned long ms)
{
#if VERSION == 220
  return;
#endif
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
    if (iSGNSSValid())
    {
      double lat = l.lat();
      double longi = -l.lng();
      double h = a.meters();
      localSite.setLong(longi);
      localSite.setLat(lat);
      localSite.setElev(h);
      initCelestialPole();
      initLat();
      rtk.setClock(d.year(), d.month(), d.day(),
        t.hour(), t.minute(), t.second(),
        *localSite.longitude());
    }
    else
      commandError = true;
    break;
  case 't':
    if (iSGNSSValid())
    {
      rtk.setClock(d.year(), d.month(), d.day(),
        t.hour(), t.minute(), t.second(),
        *localSite.longitude());
    }
    else
      commandError = true;
    break;
  default:
    commandError = true;
    break;
  }

}
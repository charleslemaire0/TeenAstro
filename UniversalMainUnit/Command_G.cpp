/*
 * Command_G
 */
#include "Global.h"
//----------------------------------------------------------------------------------
// :GXnn#  Get TeenAstro value
//         Returns: value
//         All these commands are not part of the LX200 Standard.
void PrintAltitude(double val)
{
  doubleToDms(reply, &val, false, true, highPrecision);
  strcat(reply, "#");
}
void PrintAzimuth(double val)
{
  val = AzRange(val);

  doubleToDms(reply, &val, true, false, highPrecision);
  strcat(reply, "#");
}
void PrintDec(double val)
{
  doubleToDms(reply, &val, false, true, highPrecision);
  strcat(reply, "#");
}
void PrintRa(double val)
{
  doubleToHms(reply, &val, highPrecision);
  strcat(reply, "#");
}


//----------------------------------------------------------------------------------
//   G - Get Telescope Information
void  Command_G()
{
  double f;
  HorCoords horCoords;
  int i;

  switch (command[1])
  {
  case 'A':
    //  :GA#   Get Telescope Altitude, Native LX200 command
    //         Returns: sDD*MM# or sDD*MM'SS# (based on precision setting)
    //         The current scope altitude
    mount.mP->getHorApp(&horCoords);
    if (!doubleToDms(reply, &horCoords.alt, false, true, highPrecision))
      replyLongUnknown();
    else
      strcat(reply, "#");
    break;
  case 'a':
    //  :Ga#   Get Local Time in 12 hour format, Native LX200 command
    //         Returns: HH:MM:SS#
    i = highPrecision;
    highPrecision = true;
    if (!doubleToHms(reply, rtk.getLT(localSite.toff()), highPrecision))
      replyLongUnknown();
    else
      strcat(reply, "#");
    highPrecision = i;
    break;

  case 'C':
    //  :GC#   Get the current date, Native LX200 command
    //         Returns: MM/DD/YY#
    //         The current local calendar date
  {
    int i1, i2, i3, i4, i5;
    rtk.getULDate(i2, i, i1, i3, i4, i5, localSite.toff());
    i2 = i2 % 100;
    sprintf(reply, "%02d/%02d/%02d#", i, i1, i2);
    break;
  }
  case 'c':
    //  :Gc#   Get the current time format, Native LX200 command
    //         Returns: 24#
    //         The current local time format
    strcpy(reply, "24#");
    break;
  case 'D':
  case 'R':
    //  :GD#   Get Telescope Declination, Native LX200 command
    //         Returns: sDD*MM# or sDD*MM'SS# (based on precision setting)
    //  :GR#   Get Telescope RA, Native LX200 command
    //         Returns: HH:MM.T# or HH:MM:SS# (based on precision setting)
    //  :GDL#   Get Telescope Declination, TeenAstro LX200 command
    //         Returns: sDD,VVVVV#
    //  :GRL#   Get Telescope RA, TeenAstro LX200 command
    //         Returns: DDD,VVVVV#
  {
    static double _dec = 0;
    static double _ra = 0;

    mount.mP->getEqu(&_ra, &_dec, localSite.cosLat(), localSite.sinLat(), false);

    if (command[1] == 'R')
    {
      if (command[2] == 'L')
      {
         sprintf(reply, "%08.5f#", _ra);
      }
      else
      {
        PrintRa(_ra / 15.0);
      }
    }
    else if (command[1] == 'D')
    {
      if (command[2] == 'L')
      {
         sprintf(reply, "%08.5f#", _dec);
      }
      else
      {
        PrintDec(_dec);
      }
    }
    break;
  }
  case 'd':
    //  :Gd#   Get Currently Selected Target Declination, Native LX200 command
    //         Returns: sDD*MM# or sDD*MM'SS# (based on precision setting)
    if (!doubleToDms(reply, &newTargetDec, false, true, highPrecision))
      replyLongUnknown();
    else
      strcat(reply, "#");
    break;
  case 'e':
    //  :Ge#   Get Current Site Elevation above see level in meter, TeenAstro LX200 command
    //         Returns: sDDDD#
    sprintf(reply, "%+04d#", *localSite.elevation());
    break;
  case 'f':
    //  :Gf#   Get master sidereal clock (tunable by :T+# and :T-# / reset by :TR#)
    //         Returns: dd#
    char    tmp[10];
    dtostrf(siderealClockSpeed / 256.0 , 0, 0, tmp);    // divided for compatibility
    strcpy(reply, tmp);
    strcat(reply, "#");
    break;
  case 'G':
    //  :GG#   Get UTC offset time, Native LX200 command
    //         Returns: sHH.H#
    //         The number of decimal hours to add to local time to convert it to UTC
    sprintf(reply, "%+05.1f#", *localSite.toff());
    break;
  case 'g':
  {
    //  :Gg#   Get Current Site Longitude, Native LX200 command
    //         Returns: sDDD*MM#
    //  :Ggf#  Returns: sDDD*MM'SS#
    //         The current site Longitude. East Longitudes are negative
    int i = highPrecision;
    highPrecision = command[2] == 'f';
    if (!doubleToDms(reply, localSite.longitude(), true, true, highPrecision))
      replyLongUnknown();
    else
      strcat(reply, "#");
    highPrecision = i;
  }
  break;
  case 'h':
  {
    //  :Gh#   Get Horizon Limit, Native LX200 command
    //         Returns: sDD*#
    //         The minimum elevation of an object above the horizon required for a mount goto
    sprintf(reply, "%+02d*#", limits.minAlt);
  }
  break;

  case 'L':
  {
    //  :GL#   Get Local Time in 24 hour format, Native LX200 command
    //         Returns: HH:MM:SS#
    i = highPrecision;
    highPrecision = true;
    if (!doubleToHms(reply, rtk.getLT(localSite.toff()), highPrecision))
      replyLongUnknown();
    else
      strcat(reply, "#");
    highPrecision = i;
  }
  break;
  //  :GM#   Get Site 1 Name, Native LX200 command
  //  :GN#   Get Site 2 Name, Native LX200 command
  //  :GO#   Get Site 3 Name, Native LX200 command
  //  :GP#   Get Site 4 Name, Native LX200 command
  //         Returns: <string>#
  //         A # terminated string with the name of the requested site.
  case 'M':
  case 'N':
  case 'O':
  case 'P':
  {
    i = command[1] - 'M';
    bool ok = XEEPROM.readString(EE_sites + i * SiteSize + EE_site_name, reply, SiteNameLen);
    ok = ok && reply[0] != 0;
    if (!ok)
    {
      sprintf(reply, "Site %d", i);
      XEEPROM.writeString(EE_sites + i * SiteSize + EE_site_name, reply, SiteNameLen);
    }
    strcat(reply, "#");
    break;
  }
  case 'm':
  {
    //  :Gm#   Gets the meridian pier-side, TeenAstro LX200 command
    //         Returns: E#, W#, N# (none/parked), ?# (Meridian flip in progress)
    //         A # terminated string with the pier side.
    PierSide currentSide = mount.mP->GetPierSide();
    strcpy(reply, "?#");
    switch (currentSide)
    {
      case PIER_EAST: reply[0] = 'E'; break;
      case PIER_WEST: reply[0] = 'W'; break;
      default: reply[0] = '?'; break;
    }
    break;
  }
  case 'n':
    //  :Gn#   Get Current Site name, TeenAstro LX200 command
    //         Returns: <string>#
    sprintf(reply, "%s#", localSite.siteName());
    break;
  case 'o':
    //  :Go#   Get Overhead Limit, TeenAstro LX200 command
    //         Returns: DD*#
    //         The highest elevation above the horizon that the telescope will goto
    sprintf(reply, "%02d*#", limits.maxAlt);
    break;
  case 'r':
    //  :Gr#   Get current/target object RA, Native LX200 command
    //         Returns: HH:MM.T# or HH:MM:SS (based on precision setting)
    f = newTargetRA;
    f /= 15.0;
    if (!doubleToHms(reply, &f, highPrecision))
      replyLongUnknown();
    else
      strcat(reply, "#");
    break;
  case 'S':
    //  :GS#   Get the Sidereal Time, Native LX200 command
    //         Returns: HH:MM:SS#
    //         The Sidereal Time as an ASCII Sexidecimal value in 24 hour format
    //  :GSL#  Get the Sidereal Time, TeenAstro LX200 command
    //         Returns: HH.VVVVVV#
  {
    double f = rtk.LST();
    if (command[2] == 'L')
    {
      sprintf(reply, "%+08.6f#", f);
    }
    else
    {
      doubleToHms(reply, &f, true);
      strcat(reply, "#");
    }
  }
  break;
  case 'T':
    //  :GT#   Get tracking rate, Native LX200 command
    //         Returns: dd.ddddd#
    //         Returns the tracking rate if siderealTracking, 0.0 otherwise
    if (isTracking() && !isSlewing())
    {
      f = RequestedTrackingRateHA;
      f *= 60 * 1.00273790935;
    }
    else
      f = 0.0;

    char    temp[10];
    dtostrf(f, 0, 5, temp);
    strcpy(reply, temp);
    strcat(reply, "#");
    break;
  case 't':
    //  :Gt#   Get Current Site Latitude, Native LX200 command
    //         Returns: sDD*MM#
    //  :Gtf#  Returns: sDD*MM'SS#
    //         The latitude of the current site. Positive for North latitudes
    i = highPrecision;
    highPrecision = command[2] == 'f';
    if (!doubleToDms(reply, localSite.latitude(), false, true, highPrecision))
      replyLongUnknown();
    else
      strcat(reply, "#");
    highPrecision = i;
    break;
  case 'V':
  {
    if (command[3] != (char)0)
    {
      strcpy(reply, "0#");
      return;
    }
    switch (command[2])
    {
    case 'D':
      //  :GVD#   Get Firmware Date, Native LX200 command
      strcpy(reply, FirmwareDate);
      break;
    case 'N':
      //  :GVN#   Get Firmware number, Native LX200 command
      strcpy(reply, FirmwareNumber);
      break;
    case 'P':
      //  :GVP#   Get Firmware Name, Native LX200 command
      strcpy(reply, FirmwareName);
      break;
    case 'p':
      //  :GVp#   Get Firmware SubName
      strcpy(reply, FirmwareSubName);
      break;
    case 'T':
      //  :GVT#   Get Firmware Time, Native LX200 command
      strcpy(reply, FirmwareTime);
      break;
    case 'B':
      //  :GVB#   Get Firmware version, extended LX200 command
      sprintf(reply, "%s", HAL_getBoardVersion());
      break;
    case 'b':
      //  :GVb#   re-implemented for compatibility with SHC that expects a "3" for 5160
      sprintf(reply, "%d", 3);
      break;
    case 'x':
      //  :GVx#   Get Firmware Stepper driver board, extended LX200 command
      sprintf(reply, "%s/%s", Axis1DriverName, Axis2DriverName);
      break;
    default:
      replyLongUnknown();
      break;
    }
    strcat(reply, "#");
  }
  break;

  // :GW# return status of mount
  case 'W':
  {
    switch (mount.mP->type)
    {
      case MOUNT_TYPE_ALTAZM:
      case MOUNT_TYPE_FORK_ALT:
        strcat(reply, "A");
      break;
      case MOUNT_TYPE_FORK:
        strcat(reply, "P");
      break;
      case MOUNT_TYPE_GEM:
        strcat(reply, "G");
      break;
      case MOUNT_UNDEFINED:
      default:
        strcat(reply, "L");
      break;
    }
    if (isTracking())
      strcat(reply, "T");
    else
      strcat(reply, "N");
    if (atHome())
    {
      strcat(reply, "H");
    }
    else if (parkStatus() == PRK_PARKED)
    {
      strcat(reply, "P");
    }
    else if (mount.mP->pm.isReady())
    {
      strcat(reply, "2");
    }
    else
    {
      strcat(reply, "1");
    }
    strcat(reply, "#");
  }
  break;

  case 'X':
    Command_GX();
    break;

  case '(':       // rotation - not implemented
  case ')':
      f = 0.0;
      doubleToDms(reply, &f, true, false, highPrecision);
      strcat(reply, "#");
    break;

  case 'Z':
    //  :GZ#   Get telescope azimuth, Native LX200 command
    //         Returns: DDD*MM# or DDD*MM'SS# (based on precision setting)
    mount.mP->getHorApp(&horCoords);
    f = AzRange(horCoords.az);
    if (!doubleToDms(reply, &f, true, false, highPrecision))
      replyLongUnknown();
    else
      strcat(reply, "#");
    break;
  default:
    replyLongUnknown();
    break;
  }
}

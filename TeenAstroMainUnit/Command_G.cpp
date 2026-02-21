/**
 * G — Standard LX200 Get commands (:Gx#).
 * TeenAstro-specific GX commands live in Command_GX.cpp.
 */
#include "Command.h"
#include "ValueToString.h"

// ----- Reply formatters for coordinates -----
static void PrintAtitude(double& val)
{
  doubleToDms(commandState.reply, &val, false, true, commandState.highPrecision);
  strcat(commandState.reply, "#");
}
static void PrintAzimuth(double& val) {
  val = AzRange(val);
  doubleToDms(commandState.reply, &val, true, false, commandState.highPrecision);
  strcat(commandState.reply, "#");
}

static void PrintDec(double& val) {
  doubleToDms(commandState.reply, &val, false, true, commandState.highPrecision);
  strcat(commandState.reply, "#");
}

static void PrintRa(double& val) {
  doubleToHms(commandState.reply, &val, commandState.highPrecision);
  strcat(commandState.reply, "#");
}

// =============================================================================
//   Command_G  --  :Gx#  Standard LX200 Get commands
// =============================================================================
void Command_G()
{
  int i;

  switch (commandState.command[1])
  {
  case 'A':
  case 'Z':
  case ')':
    //  :GA# :GZ# :G)#  Altitude / Azimuth / Field rotation (horizontal)
  {
    Coord_HO HO_T = mount.getHorTopo();
    if (commandState.command[1] == 'A')
    {
      double f1 = HO_T.Alt() * RAD_TO_DEG;
      PrintAtitude(f1);
    }
    else if (commandState.command[1] == 'Z')
    {
      double f = degRange(HO_T.Az() * RAD_TO_DEG);
      PrintAzimuth(f);
    }
    else if (commandState.command[1] == ')')
    {
      double f1 = HO_T.FrH() * RAD_TO_DEG;
      PrintAzimuth(f1);
    }
  }
  break;
  case 'a':
    //  :Ga#   Get Local Time in 12 hour format
    doubleToHms(commandState.reply, rtk.getLT(localSite.toff()), true);
    strcat(commandState.reply, "#");
    break;
  case 'C':
    //  :GC#   Get the current date (MM/DD/YY#). Reply may be slow if RTC read (getULDate) blocks.
  {
    int i1, i2, i3, i4, i5;
    rtk.getULDate(i2, i, i1, i3, i4, i5, localSite.toff());
    i2 = i2 % 100;
    sprintf(commandState.reply, "%02d/%02d/%02d#", i, i1, i2);
  }
  break;
  case 'c':
    //  :Gc#   Get the current time format
    strcpy(commandState.reply, "24#");
    break;
  case 'D':
  case 'R':
  case '(':
    //  :GD# :GR# :G(#  Dec / RA / Field rotation (EQ)
  {
    Coord_EQ EQ_T = mount.getEqu(*localSite.latitude() * DEG_TO_RAD);
    if (commandState.command[1] == 'R')
    {
      if (commandState.command[2] == 'L')
      {
        double f = EQ_T.Ra(rtk.LST() * HOUR_TO_RAD) * RAD_TO_DEG;
        sprintf(commandState.reply, "%08.5f#", f);
      }
      else
      {
        double f = EQ_T.Ra(rtk.LST() * HOUR_TO_RAD) * RAD_TO_HOUR;
        PrintRa(f);
      }
    }
    else if (commandState.command[1] == 'D')
    {
      double f1 = EQ_T.Dec() * RAD_TO_DEG;
      if (commandState.command[2] == 'L')
      {
        sprintf(commandState.reply, "%+08.5f#", f1);
      }
      else
      {
        PrintDec(f1);
      }
    }
    else if (commandState.command[1] == '(')
    {
      double f1 = EQ_T.FrE() * RAD_TO_DEG;
      PrintAzimuth(f1);
    }
  }
  break;
  case 'd':
    //  :Gd#   Get target Dec
    if (commandState.command[2] == 'L')
    {
      sprintf(commandState.reply, "%+08.5f#", mount.targetCurrent.newTargetDec);
    }
    else
    {
      doubleToDms(commandState.reply, &mount.targetCurrent.newTargetDec, false, true, commandState.highPrecision);
      strcat(commandState.reply, "#");
    }
    break;
  case 'e':
    //  :Ge#   Get Current Site Elevation
    sprintf(commandState.reply, "%+04d#", *localSite.elevation());
    break;
  case 'f':
    //  :Gf#   Get master sidereal clock
  {
    char tmp[10];
    dtostrf(mount.tracking.siderealClockSpeed, 0, 0, tmp);
    strcpy(commandState.reply, tmp);
    strcat(commandState.reply, "#");
  }
  break;
  case 'G':
    //  :GG#   Get UTC offset time
    sprintf(commandState.reply, "%+05.1f#", *localSite.toff());
    break;
  case 'g':
  {
    //  :Gg#   Get Current Site Longitude
    doubleToDms(commandState.reply, localSite.longitude(), true, true, commandState.command[2] == 'f');
    strcat(commandState.reply, "#");
  }
  break;
  case 'h':
  {
    //  :Gh#   Get Horizon Limit
    sprintf(commandState.reply, "%+02d*#", mount.limits.minAlt);
  }
  break;
  case 'L':
  {
    //  :GL#   Get Local Time in 24 hour format
    doubleToHms(commandState.reply, rtk.getLT(localSite.toff()), true);
    strcat(commandState.reply, "#");
  }
  break;
  case 'M':
  case 'N':
  case 'O':
  {
    //  :GM# :GN# :GO#  Get Site 1/2/3 Name
    i = commandState.command[1] - 'M';
    bool ok = XEEPROM.readString(EE_sites + i * SiteSize + EE_site_name, commandState.reply, siteNameLen);
    ok = ok && commandState.reply[0] != 0;
    if (!ok)
    {
      sprintf(commandState.reply, "Site %d", i);
      XEEPROM.writeString(EE_sites + i * SiteSize + EE_site_name, commandState.reply, siteNameLen);
    }
    strcat(commandState.reply, "#");
    break;
  }
  case 'm':
  {
    //  :Gm#   Get meridian pier-side
    PoleSide currentSide = mount.getPoleSide();
    strcpy(commandState.reply, "?#");
    if (currentSide == POLE_UNDER) commandState.reply[0] = mount.isAltAZ() || localSite.northHemisphere() ? 'E' : 'W';
    if (currentSide == POLE_OVER) commandState.reply[0] = mount.isAltAZ() || localSite.northHemisphere() ? 'W' : 'E';
    strcat(commandState.reply, "#");
    break;
  }
  case 'n':
    //  :Gn#   Get Current Site name
    sprintf(commandState.reply, "%s#", localSite.siteName());
    break;
  case 'o':
    //  :Go#   Get Overhead Limit
    sprintf(commandState.reply, "%02d*#", mount.limits.maxAlt);
    break;
  case 'r':
    //  :Gr#   Get target RA
    if (commandState.command[2] == 'L')
    {
      sprintf(commandState.reply, "%+08.5f#", mount.targetCurrent.newTargetRA);
    }
    else
    {
      double f = mount.targetCurrent.newTargetRA;
      f /= 15.0;
      doubleToHms(commandState.reply, &f, commandState.highPrecision);
      strcat(commandState.reply, "#");
    }
    break;
  case 'S':
    //  :GS#   Get the Sidereal Time
  {
    double f = rtk.LST();
    if (commandState.command[2] == 'L')
    {
      sprintf(commandState.reply, "%+08.6f#", f);
    }
    else
    {
      doubleToHms(commandState.reply, &f, true);
      strcat(commandState.reply, "#");
    }
  }
  break;
  case 'T':
    //  :GT#   Get tracking rate
  {
    double f = 0.0;
    if (mount.tracking.sideralTracking && !mount.tracking.movingTo)
    {
      f = mount.tracking.RequestedTrackingRateHA;
      f *= 60 * 1.00273790935;
    }
    char temp[10];
    dtostrf(f, 0, 5, temp);
    strcpy(commandState.reply, temp);
    strcat(commandState.reply, "#");
  }
  break;
  case 't':
    //  :Gt#   Get Current Site Latitude
    doubleToDms(commandState.reply, localSite.latitude(), false, true, commandState.command[2] == 'f');
    strcat(commandState.reply, "#");
    break;
  case 'V':
  {
    switch (commandState.command[2])
    {
    case 'D': strcpy(commandState.reply, FirmwareDate);    strcat(commandState.reply, "#"); break;
    case 'N': strcpy(commandState.reply, FirmwareNumber);  strcat(commandState.reply, "#"); break;
    case 'P': strcpy(commandState.reply, FirmwareName);    strcat(commandState.reply, "#"); break;
    case 'p': strcpy(commandState.reply, FirmwareSubName); strcat(commandState.reply, "#"); break;
    case 'T': strcpy(commandState.reply, FirmwareTime);    strcat(commandState.reply, "#"); break;
    case 'B': sprintf(commandState.reply, "%d#", VERSION); break;
    case 'b': sprintf(commandState.reply, "%d#", AxisDriver); break;
    default:  replyLongUnknow(); break;
    }
  }
  break;
  case 'W':
  {
    switch (mount.config.identity.mountType)
    {
    case MOUNT_TYPE_ALTAZM:
    case MOUNT_TYPE_FORK_ALT: strcat(commandState.reply, "A"); break;
    case MOUNT_TYPE_FORK:     strcat(commandState.reply, "P"); break;
    case MOUNT_TYPE_GEM:      strcat(commandState.reply, "G"); break;
    case MOUNT_UNDEFINED:
    default:                  strcat(commandState.reply, "L"); break;
    }
    mount.tracking.sideralTracking ? strcat(commandState.reply, "T") : strcat(commandState.reply, "N");
    if (mount.isAtHome())                strcat(commandState.reply, "H");
    else if (mount.isParked())           strcat(commandState.reply, "P");
    else if (mount.alignment.hasValid)   strcat(commandState.reply, "2");
    else                                 strcat(commandState.reply, "1");
    strcat(commandState.reply, "#");
  }
  break;
  case 'X':
    Command_GX();
    break;
  default:
    replyLongUnknow();
    break;
  }
}

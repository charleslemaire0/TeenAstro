/**
 * S — Standard LX200 Set commands (:Sx#).
 * TeenAstro-specific SX commands live in Command_SX.cpp.
 */
#include "Command.h"
#include "ValueToString.h"

// =============================================================================
//   Command_S  --  :Sx#  Standard LX200 Set commands
// =============================================================================
void Command_S(Command& process_command)
{
  switch (commandState.command[1])
  {
    //  :S!n#
    //         Set The Mount Type
    //         Return Nothing As it force a reboot
  case '!':
  {
    int i = (int)(commandState.command[2] - '0');
    bool ok = i > 0 && i < 5 && !mount.config.identity.isMountTypeFix;
    if (ok)
    {
      mount.limits.forceResetEELimit();
      XEEPROM.write(getMountAddress(EE_mountType), i);
      mount.motorsEncoders.reboot_unit = true;
    }
    replyValueSetShort(ok);
  }
  break;
  case 'a':
    //  :SasDD*MM#
    //         Set target object altitude to sDD:MM# or sDD:MM:SS# (based on precision setting)
  {
    bool ok = dmsToDouble(&mount.targetCurrent.newTargetAlt, &commandState.command[2], true, commandState.highPrecision);
    replyValueSetShort(ok);
  }
  break;
  case 'B':
    //  :SBn#  Set Baud Rate n for Serial-0
  {
    int i = (int)(commandState.command[2] - '0');
    bool ok = (i >= 0) && (i < 10);
    if (ok)
    {
      if (process_command == COMMAND_SERIAL)
      {
        Serial.print('1');
        delay(20);
        Serial.begin(commandState.baudRate_[i]);
      }
      else
      {
        Serial1.print('1');
        delay(20);
        Serial1.begin(commandState.baudRate_[i]);
      }
    }
    replyValueSetShort(ok);
  }
  break;
  case 'C':
    //  :SCMM/DD/YY#
    //          Change Local Date to MM/DD/YY
  {
    int y, m, d;
    bool ok = dateToYYYYMMDD(&y, &m, &d, &commandState.command[2]);
    if (ok)
    {
      rtk.setClock(y, m, d, hour(), minute(), second(), *localSite.longitude(), 0);
    }
    replyValueSetShort(ok);
  }
  break;
  case 'e':
    //  :SesDDDDD#
    //          Set current sites elevation above see level
  {
    int i;
    bool ok = atoi2(&commandState.command[2], &i) && localSite.setElev(i);
    replyValueSetShort(ok);
  }
  break;
  case 'd':
    //  :SdsDD*MM#
    //  :SdsDD*MM:SS#
    //  :SdLsVV,VVVVV#
    //          Set target object declination
  {
    bool ok = false;
    if (commandState.command[2] == 'L')
    {
      char* conv_end;
      double f = strtod(&commandState.command[3], &conv_end);
      ok = -90 <= f && f <= 90;
      if (ok)
      {
        mount.targetCurrent.newTargetDec = f;
      }
    }
    else
    {
      ok = dmsToDouble(&mount.targetCurrent.newTargetDec, &commandState.command[2], true, commandState.highPrecision);
    }
    replyValueSetShort(ok);
  }
  break;
  case 'g':
    //  :SgsDDD*MM# or :SgDDD*MM# or :SgsDDD:MM:SS# or SgDDD:MM:ss#
    //          Set current sites longitude
  {
    bool ok = false;
    if (mount.isAtHome() || mount.isParked())
    {
      double longi = 0.0;
      int i = (commandState.command[2] == '-') || (commandState.command[2] == '+') ? 1 : 0;
      int j = strlen(&commandState.command[7 + i]) > 1 ? (commandState.command[8 + i] == ':') : 0;
      ok = dmsToDouble(&longi, &commandState.command[2 + i], false, j);
      if (ok)
      {
        if (commandState.command[2] == '-') longi = -longi;
        localSite.setLong(longi);
        rtk.resetLongitude(*localSite.longitude());
      }
    }
    replyValueSetShort(ok);
  }
  break;
  case 'G':
    //  :SG[sHH.H]#
    //            Set the number of hours added to local time to yield UTC
  {
    char* conv_end;
    double f = strtod(&commandState.command[2], &conv_end);
    bool ok = (&commandState.command[2] != conv_end) && (f >= -12 && f <= 12.0);
    if (ok)
    {
      localSite.setToff(f);
    }
    replyValueSetShort(ok);
  }
  break;
  case 'h':
    //  :Sh+DD#
    //          Set the lowest elevation to which the telescope will goTo
  {
    int i;
    bool ok = (commandState.command[2] != 0);
    ok &= !(strlen(&commandState.command[2]) > 3);
    ok &= atoi2(&commandState.command[2], &i);
    ok &= ((i >= -30) && (i <= 30));
    if (ok)
    {
      mount.limits.minAlt = i;
      XEEPROM.update(getMountAddress(EE_minAlt), mount.limits.minAlt + 128);
    }
    replyValueSetShort(ok);
  }
  break;
  case 'L':
    //  :SLHH:MM:SS#
    //          Set the local Time
  {
    int h1, m1, m2, s1;
    bool ok = hmsToHms(&h1, &m1, &m2, &s1, &commandState.command[2], true);
    if (ok)
    {
      rtk.setClock(year(), month(), day(), h1, m1, s1, *localSite.longitude(), *localSite.toff());
    }
    replyValueSetShort(ok);
  }
  break;
  case 'M':
  case 'N':
  case 'O':
    //  :SM<string>#  :SN<string>#  :SO<string>#
    //          Set site name to be <string>, up to 14 characters.
  {
    int i = commandState.command[1] - 'M';
    if (strlen(&commandState.command[2]))
    {
      bool ok = XEEPROM.writeString(EE_sites + i * SiteSize + EE_site_name, &commandState.command[2], siteNameLen);
      replyValueSetShort(ok);
    }
    else
      replyNothing();
  }
  break;
  case 'm':
    //  :Sm#   Sets the meridian pier-side for the next Target
    if ((commandState.command[2] != 0) && (strlen(&commandState.command[2]) < 2))
    {
      if (commandState.command[2] == 'N')
      {
        mount.targetCurrent.newTargetPoleSide = POLE_NOTVALID;
        replyValueSetShort(true);
      }
      else if (commandState.command[2] == 'E')
      {
        mount.targetCurrent.newTargetPoleSide = mount.isAltAZ() || localSite.northHemisphere() ? POLE_UNDER : POLE_OVER;
        replyValueSetShort(true);
      }
      else if (commandState.command[2] == 'W')
      {
        mount.targetCurrent.newTargetPoleSide = mount.isAltAZ() || localSite.northHemisphere() ? POLE_OVER : POLE_UNDER;
        replyValueSetShort(true);
      }
      else replyNothing();
    }
    else replyNothing();
    break;
  case 'n':
  {
    bool ok = strlen(&commandState.command[2]) && localSite.setSiteName(&commandState.command[2]);
    replyValueSetShort(ok);
  }
  break;
  case 'o':
    //  :SoDD#
    //          Set the overhead elevation limit to DD#
  {
    int i;
    bool ok = (commandState.command[2] != 0) && (strlen(&commandState.command[2]) < 3);
    ok &= (atoi2(&commandState.command[2], &i)) && ((i >= 60) && (i <= 91));
    if (ok)
    {
      mount.limits.maxAlt = i;
      XEEPROM.update(getMountAddress(EE_maxAlt), mount.limits.maxAlt);
    }
    replyValueSetShort(ok);
  }
  break;
  case 'r':
    //  :SrHH:MM.T#
    //  :SrHH:MM:SS#
    //  :SrL,VVV.VVVVV#
    //          Set target object RA
  {
    bool ok = false;
    if (commandState.command[2] == 'L')
    {
      char* conv_end;
      double f = strtod(&commandState.command[3], &conv_end);
      ok = 0 <= f && f <= 360;
      if (ok)
      {
        mount.targetCurrent.newTargetRA = f;
      }
    }
    else
    {
      ok = hmsToDouble(&mount.targetCurrent.newTargetRA, &commandState.command[2], commandState.highPrecision);
      if (ok)
      {
        mount.targetCurrent.newTargetRA *= 15.0;
      }
    }
    replyValueSetShort(ok);
  }
  break;
  case 't':
    //  :StsDD*MM# or :StsDD:MM:SS#
    //          Sets the current site latitude
  {
    bool ok = false;
    if (mount.isAtHome() || mount.isParked())
    {
      bool ishighPrecision;
      double f;
      if (strlen(&commandState.command[7]) > 1)
        ishighPrecision = commandState.command[8] == ':';
      else
        ishighPrecision = false;
      ok = dmsToDouble(&f, &commandState.command[2], true, ishighPrecision);
      if (ok)
      {
        localSite.setLat(f);
        initCelestialPole();
        mount.limits.initLimit();
        mount.initHome();
        initTransformation(true);
        if (mount.isAtHome())
        {
          mount.syncAtHome();
        }
        if (mount.isParked())
        {
          mount.syncAtPark();
        }
      }
    }
    replyValueSetShort(ok);
  }
  break;
  case 'T':
    //  :STdd.ddddd#
  {
    bool ok = !mount.tracking.movingTo;
    if (ok)
    {
      char* conv_end;
      double f = strtod(&commandState.command[2], &conv_end);
      ok = (&commandState.command[2] != conv_end) &&
        (((f >= 30.0) && (f < 90.0)) || (abs(f) < 0.1));
      if (ok)
      {
        if (abs(f) < 0.1)
        {
          mount.tracking.sideralTracking = false;
        }
        else
        {
          mount.tracking.RequestedTrackingRateHA = (f / 60.0) / 1.00273790935;
        }
      }
    }
    replyValueSetShort(ok);
  }
  break;
  case 'U':
    // :SU# store current User defined Position
  {
    Coord_EQ EQ_T = mount.getEqu(*localSite.latitude() * DEG_TO_RAD);
    double f = EQ_T.Ra(rtk.LST() * HOUR_TO_RAD) * RAD_TO_HOUR;
    double f1 = EQ_T.Dec() * RAD_TO_DEG;
    XEEPROM.writeFloat(getMountAddress(EE_RA), (float)f);
    XEEPROM.writeFloat(getMountAddress(EE_DEC), (float)f1);
    replyValueSetShort(true);
  }
  break;
  case 'X':
    Command_SX();
    break;
  case 'z':
    //  :SzDDD*MM#
    //          Sets the target Object Azimuth
  {
    bool ok = dmsToDouble(&mount.targetCurrent.newTargetAzm, &commandState.command[2], false, commandState.highPrecision);
    replyValueSetShort(ok);
  }
  break;
  default:
    replyNothing();
    break;
  }
}

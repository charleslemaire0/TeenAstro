#include "ValueToString.h"
#include "Command.h"

//----------------------------------------------------------------------------------
// :GXnn#  Get TeenAstro value
//         Returns: value
//         All these command are not part of the LX200 Standard.
void Command_GX()
{
  int i;
  //  :GXnn#   Get TeenAstro Specific value
  switch (command[2])
  {
  case 'A':
    // :GXAn# Align Model values
  {
    float t11 = 0, t12 = 0, t13 = 0, t21 = 0, t22 = 0, t23 = 0, t31 = 0, t32 = 0, t33 = 0;
    if (hasStarAlignment)
    {
      alignment.getT(t11, t12, t13, t21, t22, t23, t31, t32, t33);
    }
    switch (command[3])
    {
    case '0':
      // :GXA0#
      sprintf(reply, "%f#", t11);
      break;
    case '1':
      // :GXA1#
      sprintf(reply, "%f#", t12);
      break;
    case '2':
      // :GXA2#
      sprintf(reply, "%f#", t13);
      break;
    case '3':
      // :GXA3#
      sprintf(reply, "%f#", t21);
      break;
    case '4':
      // :GXA4#
      sprintf(reply, "%f#", t22);
      break;
    case '5':
      // :GXA5#
      sprintf(reply, "%f#", t23);
      break;
    case '6':
      // :GXA6#
      sprintf(reply, "%f#", t31);
      break;
    case '7':
      // :GXA7#
      sprintf(reply, "%f#", t32);
      break;
    case '8':
      // :GXA8#
      sprintf(reply, "%f#", t33);
      break;
    case 'p':
      apparentPole ? sprintf(reply, "a#") : sprintf(reply, "t#");
      break;
    default:
      strcpy(reply, "0");
    }
    break;
  }
  break;
  case 'D':
    // :GXDnn# for Debug commands
    switch (command[3])
    {
    case'B':
    {
      // :GXDBn# Debug Backlash
      switch (command[4])
      {
      case '0':
        // :GXDB0# Debug inbacklashAxis1
        sprintf(reply, "%d#", (int)backlashA1.correcting);
        break;
      case '1':
        // :GXDB1# Debug inbacklashAxis2
        sprintf(reply, "%d#", (int)backlashA2.correcting);
        break;
      case '2':
        // :GXDB2# Debug Backlash blAxis1
        sprintf(reply, "%d#", (int)backlashA1.movedSteps);
        break;
      case '3':
        // :GXDB3# Debug Backlash blAxis1
        sprintf(reply, "%d#", (int)backlashA2.movedSteps);
        break;
      default:
        strcpy(reply, "0");
        break;
      }
      break;
    }
    case 'R':
    {
      // :GXDRn# Debug Rates
      switch (command[4])
      {
      case '0':
        // :GXDR0# RA Monitored tracking rate
        sprintf(reply, "%ld#", (long)
          ((debugv1 / 53333.3333333333) * 15000));
        break;
      case '1':
        // :GXDR1# RA tracking rate
        sprintf(reply, "%ld#", (long)
          (staA1.az_delta * 1000.0 * 1.00273790935));
        break;
      case '2':
        // :GXDR2# Dec tracking rate
        sprintf(reply, "%ld#", (long)
          (staA2.az_delta * 1000.0 * 1.00273790935));
        break;
      default:
        strcpy(reply, "0");
        break;
      }
      break;
    }
    case 'P':
    {
      // :GXDPn# Debug Position and Target
      long    temp;
      switch (command[4])
      {
      case '0':
        cli();
        temp = staA1.pos;
        sei();
        sprintf(reply, "%ld#", temp);
        break;  // Debug8, HA motor position
      case '1':
        cli();
        temp = staA2.pos;
        sei();
        sprintf(reply, "%ld#", temp);
        break;  // Debug9, Dec motor position
      case '2':
        cli();
        temp = staA1.target;
        sei();
        sprintf(reply, "%ld#", temp);
        break;  // Debug6, HA target position
      case '3':
        cli();
        temp = staA2.target;
        sei();
        sprintf(reply, "%ld#", temp);
        break;  // Debug7, Dec target position
      case '4':
        staA1.updateDeltaTarget();
        sprintf(reply, "%ld#", staA1.deltaTarget);
        break;  // Debug0, true vs. target RA position
      case '5':
        staA2.updateDeltaTarget();
        sprintf(reply, "%ld#", staA2.deltaTarget);
        break;
      default:
        strcpy(reply, "0");
        break;
      }
      break;
    }
    case 'W':
    {
      // :GXDW# Get workload
      if (command[4] == 0)
      {
        sprintf(reply, "%ld%%#", (tlp.getWorstTime() * 100L) / 9970L);
        tlp.resetWorstTime();
      }
      else
        strcpy(reply, "0");
      break;
    }
    break;
    default:
      strcpy(reply, "0");
      break;
    }
    break;
  case 'R':
    // :GXRn# user defined rates
    switch (command[3])
    {

    case 'A':
      // :GXRA# returns the Degrees For Acceleration
      dtostrf(DegreesForAcceleration, 2, 1, reply);
      strcat(reply, "#");
      break;
    case 'B':
      // :GXRB# returns the Backlash Take up Rate
      sprintf(reply, "%ld#", (long)round(BacklashTakeupRate));
      break;
    case 'D':
      sprintf(reply, "%d#", XEEPROM.read(EE_DefaultRate));
      break;
    case '0':
    case '1':
    case '2':
    case '3':
      i = command[3] - '0';
      // :GXRn# return user defined rate
      dtostrf(guideRates[i], 2, 2, reply);
      strcat(reply, "#");
      break;
    case 'X':
      // :GXRX# return Max Slew rate
      sprintf(reply, "%d#", XEEPROM.readInt(EE_maxRate));
      break;
    default:
      strcpy(reply, "0");
      break;
    }
    break;
  case 'L':
    // :GXLn user defined limits
    switch (command[3])
    {
    case 'E':
      // :GXLE# return user defined Meridian East Limit
      sprintf(reply, "%ld#", (long)round(minutesPastMeridianGOTOE));
      break;
    case 'W':
      // :GXLW# return user defined Meridian West Limit
      sprintf(reply, "%ld#", (long)round(minutesPastMeridianGOTOW));
      break;
    case 'U':
      // :GXLU# return user defined Under pole Limit
      sprintf(reply, "%ld#", (long)round(underPoleLimitGOTO * 10));
      break;
    case 'O':
      // :GXLO# return user defined horizon Limit
      // NB: duplicate with :Go#
      sprintf(reply, "%+02d*#", maxAlt);
      break;
    case 'H':
      // :GXLH# return user defined horizon Limit
      // NB: duplicate with :Gh#
      sprintf(reply, "%+02d*#", minAlt);
      break;
    default:
      strcpy(reply, "0");
      break;
    }
    break;
  case 'T':
    // :GXTn# Date/Time definition
    switch (command[3])
    {
    case '0':
      // :GXT0# UTC time
      doubleToHms(reply, rtk.getUT(), true);
      strcat(reply, "#");
      break;
    case '1':
      // :GXT1# UTC date 
    {
      int i1, i2, i3, i4, i5;
      rtk.getUTDate(i, i1, i2, i3, i4, i5);
      i = i % 100;
      sprintf(reply, "%02d/%02d/%02d#", i1, i2, i);
      break;
    }
      // :GXT2# return seconds since 01/01/1970/00:00:00
    case '2':
      unsigned long t = rtk.getTimeStamp();
      sprintf(reply, "%lu#", t);
      break;
    }
    break;
  case 'I':
  {
    // :GXI#   Get telescope Status
    for (i = 0; i < 50; i++)
      reply[i] = ' ';
    i = 0;
    reply[0] = '0' + 2 * movingTo + sideralTracking;
    reply[1] = '0' + sideralMode;
    const char  *parkStatusCh = "pIPF";
    reply[2] = parkStatusCh[parkStatus];  // not [p]arked, parking [I]n-progress, [P]arked, Park [F]ailed
    if (atHome) reply[3] = 'H';
    reply[4] = '0' + activeGuideRate;
    if (doSpiral) reply[5] = '@';
    else if (GuidingState != GuidingOFF)
    {
      reply[5] = 'G';
    }
    if (GuidingState == GuidingPulse || GuidingState == GuidingST4) reply[6] = '*';
    else if (GuidingState == GuidingRecenter) reply[6] = '+';
    if (guideA1.dir == 'e') reply[7] = '>';
    else if (guideA1.dir == 'w') reply[7] = '<';
    else if (guideA1.dir == 'b') reply[7] = 'b';
    if (guideA2.dir == 'n') reply[8] = '^';
    else if (guideA2.dir == 's') reply[8] = '_';
    else if (guideA2.dir == 'b') reply[8] = 'b';
    if (staA1.fault || staA2.fault) reply[9] = 'f';
    if (correct_tracking || isAltAZ())
      reply[10] = 'c';
    reply[11] = hasStarAlignment ? '1' : '0';
    // provide mount type
    if (mountType == MOUNT_TYPE_GEM)
    {
      reply[12] = 'E';
    }
    else if (mountType == MOUNT_TYPE_FORK)
      reply[12] = 'K';
    else if (mountType == MOUNT_TYPE_FORK_ALT)
      reply[12] = 'k';
    else if (mountType == MOUNT_TYPE_ALTAZM)
      reply[12] = 'A';
    else
      reply[12] = 'U';
    PierSide currentSide = GetPierSide();
    if (currentSide == PIER_EAST) reply[13] = 'E';
    if (currentSide == PIER_WEST) reply[13] = 'W';

    char val = 0;
    bitWrite(val, 0, iSGNSSValid());
    bitWrite(val, 1, DecayModeTrack);
    reply[14] = '0' + val;
    reply[15] = '0' + lastError;
    reply[16] = '#';
    reply[17] = 0;
    i = 17;
  }
  break;
  case 'M':
  {
    switch (command[3])
    {
    case 'B':
    {
      if (command[4] == 'D')
      {
        sprintf(reply, "%d#", backlashA2.inSeconds);
      }
      else if (command[4] == 'R')
      {
        sprintf(reply, "%d#", backlashA1.inSeconds);
      }
      else
        strcpy(reply, "0");
    }
    break;
    case 'G':
    {
      if (command[4] == 'D')
      {
        sprintf(reply, "%u#", motorA2.gear);
      }
      else if (command[4] == 'R')
      {
        sprintf(reply, "%u#", motorA1.gear);
      }
      else
        strcpy(reply, "0");
    }
    break;
    case 'S':
    {
      if (command[4] == 'D')
      {
        sprintf(reply, "%u#", motorA2.stepRot);
      }
      else if (command[4] == 'R')
      {
        sprintf(reply, "%u#", motorA1.stepRot);
      }
      else
        strcpy(reply, "0");
    }
    break;
    case 'M':
    {
      if (command[4] == 'D')
      {
        sprintf(reply, "%u#", (unsigned  int)motorA2.micro);
      }
      else if (command[4] == 'R')
      {
        sprintf(reply, "%u#", (unsigned  int)motorA1.micro);
      }
      else
        strcpy(reply, "0");
    }
    break;
    case 'm':
    {
      if (command[4] == 'D')
      {
        sprintf(reply, "%u#", (unsigned  int)motorA2.silent);
      }
      else if (command[4] == 'R')
      {
        sprintf(reply, "%u#", (unsigned  int)motorA1.silent);
      }
      else
        strcpy(reply, "0");
    }
    break;
    case 'R':
    {
      if (command[4] == 'D')
      {
        sprintf(reply, "%u#", (unsigned  int)motorA2.reverse);
      }
      else if (command[4] == 'R')
      {
        sprintf(reply, "%u#", (unsigned  int)motorA1.reverse);
      }
      else
        strcpy(reply, "0");
    }
    break;
    case 'C':
    {
      if (command[4] == 'D')
      {
        sprintf(reply, "%u#", motorA2.highCurr);
      }
      else if (command[4] == 'R')
      {
        sprintf(reply, "%u#", motorA1.highCurr);
      }
      else
        strcpy(reply, "0");
    }
    break;
    case 'c':
    {
      if (command[4] == 'D')
      {
        sprintf(reply, "%u#", motorA2.lowCurr);
      }
      else if (command[4] == 'R')
      {
        sprintf(reply, "%u#", motorA1.lowCurr);
      }
      else
        strcpy(reply, "0");
    }
    break;
    case 'L':
    {
      if ((command[4] == 'D' || command[4] == 'R'))
      {
        if (command[4] == 'D')
        {
          sprintf(reply, "%d#", motorA2.driver.getSG());  
        }
        else if (command[4] == 'R')
        {
          sprintf(reply, "%d#", motorA1.driver.getSG());
  
        }
        else
          strcpy(reply, "0");
      }
      else
        strcpy(reply, "0");
    }
    break;
    case 'I':
    {
      if ((command[4] == 'D' || command[4] == 'R'))
      {
        if (command[4] == 'D')
        {
          sprintf(reply, "%u#", motorA1.driver.getCurrent());
        }
        else if (command[4] == 'R')
        {
          sprintf(reply, "%u#", motorA2.driver.getCurrent());  
        }
        else strcpy(reply, "0");
      }
      else strcpy(reply, "0");
    }
    break;
    default:
      strcpy(reply, "0");
      break;
    }
  }
  break;
  default:
    strcpy(reply, "0");
    break;
  }
}

//----------------------------------------------------------------------------------
//   G - Get Telescope Information
void  Command_G()
{
  double f, f1;
  int i;

  switch (command[1])
  {
  case 'A':
    //  :GA#   Get Telescope Altitude, Native LX200 command
    //         Returns: sDD*MM# or sDD*MM'SS# (based on precision setting)
    //         The current scope altitude
    getHorApp(&f, &f1);
    if (!doubleToDms(reply, &f1, false, true, highPrecision))
      strcpy(reply, "0");
    else
      strcat(reply, "#");
    break;
  case 'a':
    //  :Ga#   Get Local Time in 12 hour format, Native LX200 command
    //         Returns: HH:MM:SS#
    i = highPrecision;
    highPrecision = true;
    if (!doubleToHms(reply, rtk.getLT(localSite.toff()), highPrecision))
      strcpy(reply, "0");
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
  {
    static unsigned long _coord_t = 0;
    static double _dec = 0;
    static double _ra = 0;

    if (millis() - _coord_t < 100)
    {
      f = _ra;
      f1 = _dec;
    }
    else
    {
      getEqu(&f, &f1, localSite.cosLat(), localSite.sinLat(), false);
      f /= 15.0;
      _ra = f;
      _dec = f1;
      _coord_t = millis();
    }
    if (command[1] == 'D')
    {
      if (!doubleToDms(reply, &f1, false, true, highPrecision))
        strcpy(reply, "0");
      else
        strcat(reply, "#");
    }
    else
    {
      if (!doubleToHms(reply, &f, highPrecision))
        strcpy(reply, "0");
      else
        strcat(reply, "#");
    }
    break;
  }
  case 'd':
    //  :Gd#   Get Currently Selected Target Declination, Native LX200 command
    //         Returns: sDD*MM# or sDD*MM'SS# (based on precision setting)
    if (!doubleToDms(reply, &newTargetDec, false, true, highPrecision))
      strcpy(reply, "0");
    else
      strcat(reply, "#");
    break;
  case 'e':
    //  :Ge#   Get Current Site Elevation above see level in meter, TeenAstro LX200 command
    //         Returns: sDDDD#
    sprintf(reply, "%+04d#", *localSite.elevation());
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
    //         The current site Longitude. East Longitudes are negative
    int i = highPrecision;
    highPrecision = false;
    if (!doubleToDms(reply, localSite.longitude(), true, true, highPrecision))
      strcpy(reply, "0");
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
    sprintf(reply, "%+02d*#", minAlt);
  }
  break;

  case 'L':
  {
    //  :GL#   Get Local Time in 24 hour format, Native LX200 command
    //         Returns: HH:MM:SS#
    i = highPrecision;
    highPrecision = true;
    if (!doubleToHms(reply, rtk.getLT(localSite.toff()), highPrecision))
      strcpy(reply, "0");
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
    bool ok = XEEPROM.readString(EE_sites + i * SiteSize + EE_site_name, reply, siteNameLen);
    ok = ok && reply[0] != 0;
    if (!ok)
    {
      sprintf(reply, "Site %d", i);
      XEEPROM.writeString(EE_sites + i * SiteSize + EE_site_name, reply, siteNameLen);
    }
    strcat(reply, "#");
    break;
  }
  case 'm':
  {
    //  :Gm#   Gets the meridian pier-side, TeenAstro LX200 command
    //         Returns: E#, W#, N# (none/parked), ?# (Meridian flip in progress)
    //         A # terminated string with the pier side.
    PierSide currentSide = GetPierSide();
    strcpy(reply, "?#");
    if (currentSide == PIER_EAST) reply[0] = 'E';
    if (currentSide == PIER_WEST) reply[0] = 'W';
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
    sprintf(reply, "%02d*#", maxAlt);
    break;
  case 'r':
    //  :Gr#   Get current/target object RA, Native LX200 command
    //         Returns: HH:MM.T# or HH:MM:SS (based on precision setting)
    f = newTargetRA;
    f /= 15.0;
    if (!doubleToHms(reply, &f, highPrecision))
      strcpy(reply, "0");
    else
      strcat(reply, "#");
    break;
  case 'S':
    //  :GS#   Get the Sidereal Time, Native LX200 command
    //         Returns: HH:MM:SS#
    //         The Sidereal Time as an ASCII Sexidecimal value in 24 hour format
    i = highPrecision;
    highPrecision = true;
    f = rtk.LST();
    if (!doubleToHms(reply, &f, highPrecision))
      strcpy(reply, "0");
    else
      strcat(reply, "#");
    highPrecision = i;
    break;
  case 'T':
    //  :GT#   Get tracking rate, Native LX200 command
    //         Returns: dd.ddddd# (OnStep returns more decimal places than LX200 standard)
    //         Returns the tracking rate if siderealTracking, 0.0 otherwise
    if (sideralTracking && !movingTo)
    {
      f = isAltAZ() ? GetTrackingRate() : staA1.trackingTimerRate;
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
    //         The latitude of the current site. Positive for North latitudes
    i = highPrecision;
    highPrecision = false;
    if (!doubleToDms(reply, localSite.latitude(), false, true, highPrecision))
      strcpy(reply, "0");
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
    case 'T':
      //  :GVT#   Get Firmware Time, Native LX200 command
      strcpy(reply, FirmwareTime);
      break;
    case 'B':
      //  :GVB#   Get Firmware board version, extended LX200 command
      sprintf(reply, "%d", VERSION);
      break;
    case 'b':
      //  :GVb#   Get Firmware Stepper driver board, extended LX200 command
      sprintf(reply, "%d", AxisDriver);
      break;
    default:
      strcpy(reply, "0");
      break;
    }
    strcat(reply, "#");
  }
  break;
  case 'X':
    Command_GX();
    break;
  case 'Z':
    //  :GZ#   Get telescope azimuth, Native LX200 command
    //         Returns: DDD*MM# or DDD*MM'SS# (based on precision setting)
    getHorApp(&f, &f1);
    f = AzRange(f);
    if (!doubleToDms(reply, &f, true, false, highPrecision))
      strcpy(reply, "0");
    else
      strcat(reply, "#");
    break;
  default:
    strcpy(reply, "0");
    break;
  }
}
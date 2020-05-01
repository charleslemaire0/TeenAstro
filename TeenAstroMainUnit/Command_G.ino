//----------------------------------------------------------------------------------
// :GXnn#   Get OnStep value
//         Returns: value
#include "ValueToString.h"

//All these command are not part of the LX200 Standard.
void Command_GX()
{
  //  :GXnn#   Get TeenAstro Specific value
  switch (parameter[0])
  {
  case 'A':
    // :GXAn# Align Model values
  {
    float t11 = 0, t12 = 0, t13 = 0, t21 = 0, t22 = 0, t23 = 0, t31 = 0, t32 = 0, t33 = 0;
    if (hasStarAlignment)
    {
      alignment.getT(t11, t12, t13, t21, t22, t23, t31, t32, t33);
    }
    switch (parameter[1])
    {
    case '0':
      // :GXA0#
      sprintf(reply, "%f", t11);
      quietReply = true;
      break;
    case '1':
      // :GXA1#
      sprintf(reply, "%f", t12);
      quietReply = true;
      break;
    case '2':
      // :GXA2#
      sprintf(reply, "%f", t13);
      quietReply = true;
      break;
    case '3':
      // :GXA3#
      sprintf(reply, "%f", t21);
      quietReply = true;
      break;
    case '4':
      // :GXA4#
      sprintf(reply, "%f", t22);
      quietReply = true;
      break;
    case '5':
      // :GXA5#
      sprintf(reply, "%f", t23);
      quietReply = true;
      break;
    case '6':
      // :GXA6#
      sprintf(reply, "%f", t31);
      quietReply = true;
      break;
    case '7':
      // :GXA7#
      sprintf(reply, "%f", t32);
      quietReply = true;
      break;
    case '8':
      // :GXA8#
      sprintf(reply, "%f", t33);
      quietReply = true;
      break;
    }
    break;
  }
  break;
  case 'D':
    // :GXDnn# for Debug commands
    switch (parameter[1])
    {
    case'B':
    {
      // :GXDBn# Debug Backlash
      switch (parameter[2])
      {
      case '0':
        // :GXDB0# Debug inbacklashAxis1
        sprintf(reply, "%d", (int)inbacklashAxis1);
        quietReply = true;
        break;
      case '1':
        // :GXDB1# Debug inbacklashAxis2
        sprintf(reply, "%d", (int)inbacklashAxis1);
        quietReply = true;
        break;
      case '2':
        // :GXDB2# Debug Backlash blAxis1
        sprintf(reply, "%d", (int)blAxis1);
        quietReply = true;
        break;
      case '3':
        // :GXDB3# Debug Backlash blAxis1
        sprintf(reply, "%d", (int)blAxis2);
        quietReply = true;
        break;
      default:
        commandError = true;
        break;
      }
      break;
    }
    case 'R':
    {
      // :GXDRn# Debug Rates
      switch (parameter[2])
      {

      case '0':
        // :GXDR0# RA Monitored tracking rate
        sprintf(reply, "%ld", (long)
          ((debugv1 / 53333.3333333333) * 15000));
        quietReply = true;
        break;
      case '1':
        // :GXDR1# RA tracking rate
        sprintf(reply, "%ld", (long)
          (az_deltaAxis1 * 1000.0 * 1.00273790935));
        quietReply = true;
        break;
      case '2':
        // :GXDR2# Dec tracking rate
        sprintf(reply, "%ld", (long)
          (az_deltaAxis2 * 1000.0 * 1.00273790935));
        quietReply = true;
        break;
      default:
        commandError = true;
        break;
      }
      break;
    }
    case 'P':
    {
      // :GXDPn# Debug Position and Target
      long    temp;
      switch (parameter[2])
      {
      case '0':
        cli();
        temp = posAxis1;
        sei();
        sprintf(reply, "%ld", temp);
        quietReply = true;
        break;  // Debug8, HA motor position
      case '1':
        cli();
        temp = posAxis2;
        sei();
        sprintf(reply, "%ld", temp);
        quietReply = true;
        break;  // Debug9, Dec motor position
      case '2':
        cli();
        temp = targetAxis1;
        sei();
        sprintf(reply, "%ld", temp);
        quietReply = true;
        break;  // Debug6, HA target position
      case '3':
        cli();
        temp = targetAxis2;
        sei();
        sprintf(reply, "%ld", temp);
        quietReply = true;
        break;  // Debug7, Dec target position
      case '4':
        updateDeltaTargetAxis1();
        sprintf(reply, "%ld", deltaTargetAxis1);
        quietReply = true;
        break;  // Debug0, true vs. target RA position
      case '5':
        updateDeltaTargetAxis2();
        sprintf(reply, "%ld", deltaTargetAxis2);
        quietReply = true;
        break;
      default:
        commandError = true;
        break;
      }
      break;
    }
    case 'W':
    {
      // :GXDW# Get workload
      if (parameter[2] == 0)
      {
        sprintf(reply, "%ld%%", (tlp.getWorstTime() * 100L) / 9970L);
        tlp.resetWorstTime();
        quietReply = true;
      }
      else
        commandError = true;
      break;
    }
    break;
    default:
      commandError = true;
      break;
    }
    break;
  case 'R':
    // :GXRn# user defined rates
    switch (parameter[1])
    {

    case 'A':
      // :GXRA# returns the Degrees For Acceleration
      dtostrf(DegreesForAcceleration, 2, 1, reply);
      quietReply = true;
      break;
    case 'B':
      // :GXRB# returns the Backlash Take up Rate
      sprintf(reply, "%ld", (long)round(BacklashTakeupRate));
      quietReply = true;
      break;
    case 'D':
      sprintf(reply, "%d", XEEPROM.read(EE_DefaultRate));
      quietReply = true;
      break;
    case '0':
    case '1':
    case '2':
    case '3':
      i = parameter[1] - '0';
      // :GXRn# return user defined rate
      dtostrf(guideRates[i], 2, 2, reply);
      quietReply = true;
      break;
    case 'X':
      // :GXRX# return Max Slew rate
      sprintf(reply, "%d", XEEPROM.readInt(EE_maxRate));
      quietReply = true;
      break;

    default:
      commandError = true;
      break;
    }
    break;
  case 'L':
    // :GXLn user defined limits
    switch (parameter[1])
    {
    case 'E':
      // :GXLE# return user defined Meridian East Limit
      sprintf(reply, "%ld", (long)round(minutesPastMeridianGOTOE));
      quietReply = true;
      break;
    case 'W':
      // :GXLW# return user defined Meridian West Limit
      sprintf(reply, "%ld", (long)round(minutesPastMeridianGOTOW));
      quietReply = true;
      break;
    case 'U':
      // :GXLU# return user defined Under pole Limit
      sprintf(reply, "%ld", (long)round(underPoleLimitGOTO * 10));
      quietReply = true;
      break;
    case 'O':
      // :GXLO# return user defined horizon Limit
      // NB: duplicate with :Go#
      sprintf(reply, "%+02d*", maxAlt);
      quietReply = true;
      break;
    case 'H':
      // :GXLH# return user defined horizon Limit
      // NB: duplicate with :Gh#
      sprintf(reply, "%+02d*", minAlt);
      quietReply = true;
      break;
    default:
      commandError = true;
      break;
    }
    break;
  case 'T':
    // :GXTn# Date/Time definition
    switch (parameter[1])
    {
    case '0':
      // :GXT0# UTC time
      doubleToHms(reply, rtk.getUT(), true);
      quietReply = true;
      break;
    case '1':
      // :GXT1# UTC date 
      rtk.getUTDate(i, i1, i2, i3, i4, i5);
      i = i % 100;
      sprintf(reply, "%02d/%02d/%02d", i1, i2, i);
      quietReply = true;
      break;
    case '2':
      // :GXT2# return seconds since 01/01/1970/00:00:00
      // For debug...
      unsigned long t = rtk.getTimeStamp();
      sprintf(reply, "%lu", t);
      quietReply = true;
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
    if (guideDirAxis1 == 'e') reply[7] = '>';
    else if (guideDirAxis1 == 'w') reply[7] = '<';
    else if (guideDirAxis1 == 'b') reply[7] = 'b';
    if (guideDirAxis2 == 'n') reply[8] = '^';
    else if (guideDirAxis2 == 's') reply[8] = '_';
    else if (guideDirAxis2 == 'b') reply[8] = 'b';
    if (faultAxis1 || faultAxis2) reply[9] = 'f';
    if (correct_tracking)
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
    reply[14] = iSGNSSValid() ? '1' : '0';
    reply[15] = '0' + lastError;
    reply[16] = 0;
    i = 17;
    quietReply = true;                                   //         Returns: SS#
  }
  break;
  case 'M':
  {
    switch (parameter[1])
    {
    case 'B':
    {
      if (parameter[2] == 'D')
      {
        sprintf(reply, "%d", backlashAxis2);
        quietReply = true;
      }
      else if (parameter[2] == 'R')
      {
        sprintf(reply, "%d", backlashAxis1);
        quietReply = true;
      }
      else
        commandError = true;
    }
    break;
    case 'G':
    {
      if (parameter[2] == 'D')
      {
        sprintf(reply, "%u", GearAxis2);
        quietReply = true;
      }
      else if (parameter[2] == 'R')
      {
        sprintf(reply, "%u", GearAxis1);
        quietReply = true;
      }
      else
        commandError = true;
    }
    break;
    case 'S':
    {
      if (parameter[2] == 'D')
      {
        sprintf(reply, "%u", StepRotAxis2);
        quietReply = true;
      }
      else if (parameter[2] == 'R')
      {
        sprintf(reply, "%u", StepRotAxis1);
        quietReply = true;
      }
      else
        commandError = true;
    }
    break;
    case 'M':
    {
      if (parameter[2] == 'D')
      {
        sprintf(reply, "%u", (unsigned  int)MicroAxis2);
        quietReply = true;
      }
      else if (parameter[2] == 'R')
      {
        sprintf(reply, "%u", (unsigned  int)MicroAxis1);
        quietReply = true;
      }
      else
        commandError = true;
    }
    break;
    case 'R':
    {
      if (parameter[2] == 'D')
      {
        sprintf(reply, "%u", (unsigned  int)ReverseAxis2);
        quietReply = true;
      }
      else if (parameter[2] == 'R')
      {
        sprintf(reply, "%u", (unsigned  int)ReverseAxis1);
        quietReply = true;
      }
      else
        commandError = true;
    }
    break;
    case 'C':
    {
      if (parameter[2] == 'D')
      {
        sprintf(reply, "%u", HighCurrAxis2);
        quietReply = true;
      }
      else if (parameter[2] == 'R')
      {
        sprintf(reply, "%u", HighCurrAxis1);
        quietReply = true;
      }
      else
        commandError = true;
    }
    break;
    case 'c':
    {
      if (parameter[2] == 'D')
      {
        sprintf(reply, "%u", LowCurrAxis2);
        quietReply = true;
      }
      else if (parameter[2] == 'R')
      {
        sprintf(reply, "%u", LowCurrAxis1);
        quietReply = true;
      }
      else
        commandError = true;
    }
    break;
    case 'L':
    {
      if ((parameter[2] == 'D' || parameter[2] == 'R'))
      {
        if (parameter[2] == 'D')
        {
          sprintf(reply, "%d", motorAxis2.getSG());
          quietReply = true;
        }
        else if (parameter[2] == 'R')
        {
          sprintf(reply, "%d", motorAxis1.getSG());
          quietReply = true;
        }
        else
          commandError = true;
      }
      else
        commandError = true;
    }
    break;
    case 'I':
    {
      if ((parameter[2] == 'D' || parameter[2] == 'R'))
      {
        if (parameter[2] == 'D')
        {
          sprintf(reply, "%u", motorAxis1.getCurrent());
          quietReply = true;
        }
        else if (parameter[2] == 'R')
        {
          sprintf(reply, "%u", motorAxis2.getCurrent());
          quietReply = true;
        }
        else
          commandError = true;
      }
      else
        commandError = true;
    }
    break;
    default:
      commandError = true;
      break;
    }
  }
  break;
  default:
    commandError = true;
    break;
  }
}

//----------------------------------------------------------------------------------
//   G - Get Telescope Information
void  Command_G()
{
  double f, f1;
  int i, i1, i2;
  unsigned long   _coord_t = 0;
  double          _dec = 0;
  double          _ra = 0;
  switch (command[1])
  {
  case 'A':
    //  :GA#   Get Telescope Altitude, Native LX200 command
    //         Returns: sDD*MM# or sDD*MM'SS# (based on precision setting)
    //         The current scope altitude
    getHorApp(&f, &f1);
    if (!doubleToDms(reply, &f1, false, true, highPrecision))
      commandError = true;
    else
      quietReply = true;
    break;
  case 'a':
    //  :Ga#   Get Local Time in 12 hour format, Native LX200 command
    //         Returns: HH:MM:SS#
    i = highPrecision;
    highPrecision = true;
    if (!doubleToHms(reply, rtk.getLT(localSite.toff()), highPrecision))
      commandError = true;
    else
      quietReply = true;
    highPrecision = i;
    break;

  case 'C':
    //  :GC#   Get the current date, Native LX200 command
    //         Returns: MM/DD/YY#
    //         The current local calendar date
    rtk.getULDate(i2, i, i1, i3, i4, i5, localSite.toff());
    i2 = i2 % 100;
    sprintf(reply, "%02d/%02d/%02d", i, i1, i2);
    quietReply = true;
    break;

  case 'c':
    //  :Gc#   Get the current time format, Native LX200 command
    //         Returns: 24#
    //         The current local time format
    strcpy(reply, "24");
    quietReply = true;
    break;

  case 'D':
    //  :GD#   Get Telescope Declination, Native LX200 command
    //         Returns: sDD*MM# or sDD*MM'SS# (based on precision setting)

    if (millis() - _coord_t < 100)
    {
      f = _ra;
      f1 = _dec;
    }
    else
    {
      getEqu(&f, &f1, false);
      f /= 15.0;
      _ra = f;
      _dec = f1;
      _coord_t = millis();
    }

    if (!doubleToDms(reply, &f1, false, true, highPrecision))
      commandError = true;
    else
      quietReply = true;
    break;
  case 'd':
    //  :Gd#   Get Currently Selected Target Declination, Native LX200 command
    //         Returns: sDD*MM# or sDD*MM'SS# (based on precision setting)
    if (!doubleToDms(reply, &newTargetDec, false, true, highPrecision))
      commandError = true;
    else
      quietReply = true;
    break;
  case 'e':
    //  :Ge#   Get Current Site Elevation above see level in meter, TeenAstro LX200 command
    //         Returns: sDDDD#
    sprintf(reply, "%+04d", *localSite.elevation());
    quietReply = true;
    break;
  case 'G':
    //  :GG#   Get UTC offset time, Native LX200 command
    //         Returns: sHH.H#
    //         The number of decimal hours to add to local time to convert it to UTC 
    sprintf(reply, "%+05.1f", *localSite.toff());
    quietReply = true;
    break;
  case 'g':
  {
    //  :Gg#   Get Current Site Longitude, Native LX200 command
    //         Returns: sDDD*MM#
    //         The current site Longitude. East Longitudes are negative
    int i = highPrecision;
    highPrecision = false;
    if (!doubleToDms(reply, localSite.longitude(), true, true, highPrecision))
      commandError = true;
    else
      quietReply = true;
    highPrecision = i;
  }
  break;
  case 'h':
  {
    //  :Gh#   Get Horizon Limit, Native LX200 command
    //         Returns: sDD*#
    //         The minimum elevation of an object above the horizon required for a mount goto
    sprintf(reply, "%+02d*", minAlt);
    quietReply = true;
  }
  break;

  case 'L':
  {
    //  :GL#   Get Local Time in 24 hour format, Native LX200 command
    //         Returns: HH:MM:SS#
    i = highPrecision;
    highPrecision = true;
    if (!doubleToHms(reply, rtk.getLT(localSite.toff()), highPrecision))
      commandError = true;
    else
      quietReply = true;
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
    XEEPROM.readString(EE_sites + i * SiteSize + EE_site_name, reply);
    if (reply[0] == 0)
    {
      sprintf(reply, "Site %d", i);
      XEEPROM.writeString(EE_sites + i * SiteSize + EE_site_name, reply);
    }
    quietReply = true;
    break;
  }
  case 'm':
  {
    //  :Gm#   Gets the meridian pier-side, TeenAstro LX200 command
    //         Returns: E#, W#, N# (none/parked), ?# (Meridian flip in progress)
    //         A # terminated string with the pier side.
    PierSide currentSide = GetPierSide();
    reply[0] = '?';
    reply[1] = 0;
    if (currentSide == PIER_EAST) reply[0] = 'E';
    if (currentSide == PIER_WEST) reply[0] = 'W';
    quietReply = true;
    break;
  }
  case 'n':
    //  :Gn#   Get Current Site name, TeenAstro LX200 command
    //         Returns: <string>#
    sprintf(reply, "%s", localSite.siteName());
    quietReply = true;
    break;
  case 'o':
    //  :Go#   Get Overhead Limit, TeenAstro LX200 command
    //         Returns: DD*#
    //         The highest elevation above the horizon that the telescope will goto
    sprintf(reply, "%02d*", maxAlt);
    quietReply = true;
    break;
  case 'R':
    //  :GR#   Get Telescope RA, Native LX200 command
    //         Returns: HH:MM.T# or HH:MM:SS# (based on precision setting)
    if (parameter[0] == 'E' && parameter[1] == 'F')
    {
      reply[0] = refraction ? '1' : '0';
      reply[1] = 0;
      quietReply = true;
    }
    else
    {
      if (millis() - _coord_t < 100)
      {
        f = _ra;
        f1 = _dec;
      }
      else
      {
        getEqu(&f, &f1, false);
        f /= 15.0;
        _ra = f;
        _dec = f1;
        _coord_t = millis();
      }

      if (!doubleToHms(reply, &f, highPrecision))
        commandError = true;
      else
        quietReply = true;
    }
    break;
  case 'r':
    //  :Gr#   Get current/target object RA, Native LX200 command
    //         Returns: HH:MM.T# or HH:MM:SS (based on precision setting)
    f = newTargetRA;
    f /= 15.0;
    if (!doubleToHms(reply, &f, highPrecision))
      commandError = true;
    else
      quietReply = true;
    break;
  case 'S':
    //  :GS#   Get the Sidereal Time, Native LX200 command
    //         Returns: HH:MM:SS#
    //         The Sidereal Time as an ASCII Sexidecimal value in 24 hour format
    i = highPrecision;
    highPrecision = true;
    f = rtk.LST();
    if (!doubleToHms(reply, &f, highPrecision))
      commandError = true;
    else
      quietReply = true;
    highPrecision = i;
    break;
  case 'T':
    //  :GT#   Get tracking rate, Native LX200 command
    //         Returns: dd.ddddd# (OnStep returns more decimal places than LX200 standard)
    //         Returns the tracking rate if siderealTracking, 0.0 otherwise
    if (sideralTracking && !movingTo)
    {
      f = isAltAZ() ? GetTrackingRate() : trackingTimerRateAxis1;
      f *= 60 * 1.00273790935;
    }
    else
      f = 0.0;

    char    temp[10];
    dtostrf(f, 0, 5, temp);
    strcpy(reply, temp);
    quietReply = true;
    break;
  case 't':
    //  :Gt#   Get Current Site Latitude, Native LX200 command
    //         Returns: sDD*MM#
    //         The latitude of the current site. Positive for North latitudes
    i = highPrecision;
    highPrecision = false;
    if (!doubleToDms(reply, localSite.latitude(), false, true, highPrecision))
      commandError = true;
    else
      quietReply = true;
    highPrecision = i;
    break;
  case 'V':
  {
    if (parameter[1] != (char)0)
    {
      commandError = true;
      quietReply = true;
      return;
    }
    switch (parameter[0])
    {
    case 'D':
      //  :GVD#   Get Firmware Date, Native LX200 command
      strcpy(reply, FirmwareDate);
      break;
    case 'N':
      //  :GVD#   Get Firmware number, Native LX200 command
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
    default:
      commandError = true;
      break;
    }
    quietReply = true;
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
      commandError = true;
    else
      quietReply = true;
    break;
  default:
    commandError = true;
    break;
  }
}
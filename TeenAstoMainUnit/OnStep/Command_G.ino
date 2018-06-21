

//----------------------------------------------------------------------------------
//  :GXE0
void Command_GXE0()
{
  // simple values
  String  c;
#ifdef DEBUG_ON
  c = "1";
#else
  c = "0";
#endif
#if 1//SYNC_ANYWHERE_ON
  c += "1";
#else
  c += "0";
#endif
if (mountType == MOUNT_TYPE_GEM)
  c += "0";
else if (mountType == MOUNT_TYPE_FORK)
  c += "1";
else if (mountType == MOUNT_TYPE_FORK_ALT)
  c += "2";
else if (mountType == MOUNT_TYPE_ALTAZM)
  c += "3";
else
  c += "9";

#if defined(ST4_OFF)
  c += "0";
#elif defined(ST4_ON)
  c += "1";
#elif defined(ST4_PULLUP)
  c += "2";
#else
  c += "0";
#endif
#if defined(ST4_ALTERNATE_PINS_ON)
  c += "1";
#else
  c += "0";
#endif
#if defined(PPS_SENSE_ON)
  c += "1";
#else
  c += "0";
#endif
#if defined(PEC_SENSE_ON)
  c += "1";
#elif defined(PEC_SENSE_PULLUP)
  c += "2";
#else
  c += "0";
#endif
#ifdef LIMIT_SENSE_ON
  c += "1";
#else
  c += "0";
#endif
#ifdef STATUS_LED_PINS_ON
  c += "1";
#else
  c += "0";
#endif
#if defined(STATUS_LED2_PINS_ON)
  c += "1";
#elif defined(STATUS_LED2_PINS)
  c += "2";
#else
  c += "0";
#endif
#ifdef RETICULE_LED_PINS
  c += "1";
#else
  c += "0";
#endif
#if 0 //POWER_SUPPLY_PINS is always OFF
  c += "1";
#else
  c += "0";
#endif
#if defined(AXIS1_DISABLED_HIGH)
  c += "1";
#elif defined(AXIS1_DISABLED_LOW)
  c += "0";
#else
  c += "9";
#endif
#if defined(AXIS2_DISABLED_HIGH)
  c += "1";
#elif defined(AXIS2_DISABLED_LOW)
  c += "0";
#else
  c += "9";
#endif
#if defined(AXIS1_FAULT_LOW)
  c += "0";
#elif defined(AXIS1_FAULT_HIGH)
  c += "1";
#elif defined(AXIS1_FAULT_OFF)
  c += "2";
#endif
#if defined(AXIS2_FAULT_LOW)
  c += "0";
#elif defined(AXIS2_FAULT_HIGH)
  c += "1";
#elif defined(AXIS2_FAULT_OFF)
  c += "2";
#endif
#ifdef TRACK_REFRACTION_RATE_DEFAULT_ON
  c += "1";
#else
  c += "0";
#endif
#ifdef SEPERATE_PULSE_GUIDE_RATE_ON
  c += "1";
#else
  c += "0";
#endif
#if true
  c += "1";
#else
  c += "0";
#endif
  strcpy(reply, (char *)c.c_str());
  quietReply = true;

}

//----------------------------------------------------------------------------------
// :GXnn#   Get OnStep value
//         Returns: value
void Command_GX()
{
  //  :GXnn#   Get OnStep value
  if (parameter[2] != (char)0)
  {
    commandError = true;
    return;
  }
  switch (parameter[0])
  {
  case '0':
    // 0n: Align Model
    switch (parameter[1])
    {

    case '2':
      sprintf(reply, "%ld", (long)(GeoAlign.altCor * 3600.0));
      quietReply = true;
      break;  // altCor

    case '3':
      sprintf(reply, "%ld", (long)(GeoAlign.azmCor * 3600.0));
      quietReply = true;
      break;  // azmCor

    case '4':
      sprintf(reply, "%ld", (long)(GeoAlign.doCor * 3600.0));
      quietReply = true;
      break;  // doCor

    case '5':
      sprintf(reply, "%ld", (long)(GeoAlign.pdCor * 3600.0));
      quietReply = true;
      break;  // pdCor
    }
    break;
  case '8':
    // 8n: Date/Time
    switch (parameter[1])
    {
    case '0':
      i = highPrecision;
      highPrecision = true;
      doubleToHms(reply, rtk.getUT());
      highPrecision = i;
      quietReply = true;
      break;  // UTC time

    case '1':
      rtk.getUTDate(i, i1, i2);
      sprintf(reply, "%d/%d/%d", i1, i2, i);
      quietReply = true;
      break;  // UTC date
    }
    break;
  case '9':
    // 9n: Misc.
    switch (parameter[1])
    {
    case '0':
      dtostrf(guideRates[0], 2,
        2, reply);
      quietReply = true;
      break;  // pulse-guide rate
    case '2':
      sprintf(reply, "%ld", (long)(maxRate / 16L));
      quietReply = true;
      break;  // MaxRate

    case '3':
      sprintf(reply, "%ld", (long)(MaxRate));
      quietReply = true;
      break;  // MaxRate (default)

    case '4':
      if (meridianFlip == MeridianFlipNever)
      {
        sprintf(reply, "%d N", (int)(pierSide));
      }
      else
      {
        sprintf(reply, "%d", (int)(pierSide));
      }

      quietReply = true;
      break;  // pierSide (N if never)

    case '5':
      sprintf(reply, "%i", (int)autoContinue);
      quietReply = true;
      break;  // autoContinue
    }
    break;
  case 'E':
    // En: Get config
    switch (parameter[1])
    {
    case '0':
      Command_GXE0();
      break;
    case '1':
      sprintf(reply, "%ld", (long)MaxRate);
      quietReply = true;
      break;
    case '2':
      dtostrf(DegreesForAcceleration, 2, 1, reply);
      quietReply = true;
      break;
    case '3':
      sprintf(reply, "%ld",
        (long)round(BacklashTakeupRate));
      quietReply = true;
      break;
    case '4':
      sprintf(reply, "%ld",
        (long)round(StepsPerDegreeAxis1));
      quietReply = true;
      break;
    case'5':
      sprintf(reply, "%ld",
        (long)round(StepsPerDegreeAxis2));
      quietReply = true;
      break;
    case '6':
      dtostrf(StepsPerSecondAxis1, 3, 6, reply);
      quietReply = true;
      break;
    case '7':
      quietReply = true;
      break;
    case '8':
      quietReply = true;
      break;
    case '9':
      sprintf(reply, "%ld", (long)round(minutesPastMeridianGOTOE));
      quietReply = true;
      break;
    case 'A':
      sprintf(reply, "%ld", (long)round(minutesPastMeridianGOTOW));
      quietReply = true;
      break;
    case 'B':
      sprintf(reply, "%ld", (long)round(underPoleLimitGOTO*10));
      quietReply = true;
      break;
    case 'C':
      sprintf(reply, "%ld", (long)round(MinDec));
      quietReply = true;
      break;
    case 'D':
      sprintf(reply, "%ld", (long)round(MaxDec));
      quietReply = true;
    default:
      commandError = true;
      break;

    }
    break;
  case 'F':
    long    temp;
    switch (parameter[1])
    {
    case '0':
      cli();
      temp = (long)(posAxis1 - ((long)targetAxis1.part.m));
      sei();
      sprintf(reply, "%ld", temp);
      quietReply = true;
      break;  // Debug0, true vs. target RA position

    case '1':
      cli();
      temp = (long)(posAxis2 - ((long)targetAxis2.part.m));
      sei();
      sprintf(reply, "%ld", temp);
      quietReply = true;
      break;  // Debug1, true vs. target Dec position

              //              case '0': cli(); temp=(long)(((az_Azm1-az_Azm2)*2.0)*1000); sei(); sprintf(reply,"%ld",temp); quietReply=true; break;               // Debug0, true vs. target RA position
              //              case '1': cli(); temp=(long)(az_Azm1); sei(); sprintf(reply,"%ld",temp); quietReply=true; break;                                    // Debug1, true vs. target Dec position
    case '2':
      sprintf(reply, "%ld", (long)
        ((debugv1 / 53333.3333333333) * 15000));
      quietReply = true;
      break;  // Debug2, RA tracking rate

    case '3':
      sprintf(reply, "%ld", (long)
        (az_deltaAxis1 * 1000.0 * 1.00273790935));
      quietReply = true;
      break;  // Debug3, RA refraction tracking rate

    case '4':
      sprintf(reply, "%ld", (long)
        (az_deltaAxis2 * 1000.0 * 1.00273790935));
      quietReply = true;
      break;  // Debug4, Dec refraction tracking rate

    case '5':
      sprintf(reply, "%ld", (long)(ZenithTrackingRate() *
        1000.0 * 1.00273790935));
      quietReply = true;
      break;  // Debug5, Alt RA refraction tracking rate

    case '6':
      cli();
      temp = (long)(targetAxis1.part.m);
      sei();
      sprintf(reply, "%ld", temp);
      quietReply = true;
      break;  // Debug6, HA target position

    case '7':
      cli();
      temp = (long)(targetAxis2.part.m);
      sei();
      sprintf(reply, "%ld", temp);
      quietReply = true;
      break;  // Debug7, Dec target position

    case '8':
      cli();
      temp = (long)(posAxis1);
      sei();
      sprintf(reply, "%ld", temp);
      quietReply = true;
      break;  // Debug8, HA motor position

    case '9':
      cli();
      temp = (long)(posAxis2);
      sei();
      sprintf(reply, "%ld", temp);
      quietReply = true;
      break;  // Debug9, Dec motor position

    case 'A':
      sprintf(reply, "%ld%%", (tlp.getWorstTime() * 100L) / 9970L);
      tlp.resetWorstTime();
      quietReply = true;
      break;  // DebugA, Workload
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
    //  :GA#   Get Telescope Altitude
    //         Returns: sDD*MM# or sDD*MM'SS# (based on precision setting)
    //         The current scope altitude

    getHor(&f, &f1);
    if (!doubleToDms(reply, &f, false, true))
      commandError = true;
    else
      quietReply = true;
    break;
  case 'a':
    //  :Ga#   Get Local Time in 12 hour format
    //         Returns: HH:MM:SS#
    i = highPrecision;
    highPrecision = true;
    if (!doubleToHms(reply, rtk.getUT()))
      commandError = true;
    else
      quietReply = true;
    highPrecision = i;
    break;

  case 'C':
    //  :GC#   Get the current date
    //         Returns: MM/DD/YY#
    //         The current local calendar date
    rtk.getUTDate(i2, i, i1);
    i2 = i2 % 100;
    sprintf(reply, "%02d/%02d/%02d", i, i1, i2);
    quietReply = true;
    break;

  case 'c':
    //  :Gc#   Get the current time format
    //         Returns: 24#
    //         The current local time format
    strcpy(reply, "24");
    quietReply = true;
    break;

  case 'D':
    //  :GD#   Get Telescope Declination
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

    if (!doubleToDms(reply, &f1, false, true))
      commandError = true;
    else
      quietReply = true;
    break;
  case 'd':
    //  :Gd#   Get Currently Selected Target Declination
    //         Returns: sDD*MM# or sDD*MM'SS# (based on precision setting)
    if (!doubleToDms(reply, &newTargetDec, false, true))
      commandError = true;
    else
      quietReply = true;
    break;
  case 'G':
    //  :GG#   Get UTC offset time
    //         Returns: sHH#
    //         The number of decimal hours to add to local time to convert it to UTC 
    sprintf(reply, "+00");
    quietReply = true;
    break;
  case 'g':
  {
    //  :Gg#   Get Current Site Longitude
    //         Returns: sDDD*MM#
    //         The current site Longitude. East Longitudes are negative
    int i = highPrecision;
    highPrecision = false;
    if (!doubleToDms(reply, localSite.longitude(), true, true))
      commandError = true;
    else
      quietReply = true;
    highPrecision = i;
  }
  break;
  case 'h':
  {
    //  :Gh#   Get Horizon Limit
    //         Returns: sDD*#
    //         The minimum elevation of an object above the horizon required for a mount goto
    sprintf(reply, "%+02d*", minAlt);
    quietReply = true;
  }
  break;
  case 'L':
  {
    i = highPrecision;
    highPrecision = true;
    if (!doubleToHms(reply, rtk.getUT()))
      commandError = true;
    else
      quietReply = true;
    highPrecision = i;
  }
  break;
  case 'O':
  {
    if (!(0 < parameter[0] && parameter[0] < 4))
      return;

    i = parameter[0];
    EEPROM_readString(EE_sites + i * 25 + 9, reply);
    if (reply[0] == 0)
    {
      strcat(reply, "None");
    }
    quietReply = true;
  }
  break;
  case 'm':
    //  :Gm#   Gets the meridian pier-side
    //         Returns: E#, W#, N# (none/parked), ?# (Meridian flip in progress)
    //         A # terminated string with the pier side.
    reply[0] = '?';
    reply[1] = 0;
    if (pierSide < PierSideWest) reply[0] = 'E';
    if (pierSide >= PierSideWest) reply[0] = 'W';
    quietReply = true;
    break;
  case 'o':
    //  :Go#   Get Overhead Limit
    //         Returns: DD*#
    //         The highest elevation above the horizon that the telescope will goto
    sprintf(reply, "%02d*", maxAlt);
    quietReply = true;
    break;
  case 'R':
    //  :GR#   Get Telescope RA
    //         Returns: HH:MM.T# or HH:MM:SS# (based on precision setting)
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

    if (!doubleToHms(reply, &f))
      commandError = true;
    else
      quietReply = true;
    break;
  case 'r':
    //  :Gr#   Get current/target object RA
    //         Returns: HH:MM.T# or HH:MM:SS (based on precision setting)
    f = newTargetRA;
    f /= 15.0;
    if (!doubleToHms(reply, &f))
      commandError = true;
    else
      quietReply = true;
    break;
  case 'S':
    //  :GS#   Get the Sidereal Time
    //         Returns: HH:MM:SS#
    //         The Sidereal Time as an ASCII Sexidecimal value in 24 hour format
    i = highPrecision;
    highPrecision = true;
    f = rtk.LST();
    if (!doubleToHms(reply, &f))
      commandError = true;
    else
      quietReply = true;
    highPrecision = i;
    break;
  case 'T':
    //  :GT#   Get tracking rate
    //         Returns: dd.ddddd# (OnStep returns more decimal places than LX200 standard)
    //         Returns the tracking rate if siderealTracking, 0.0 otherwise
    if (trackingState == TrackingON)
    {
      f = mountType == MOUNT_TYPE_ALTAZM ? GetTrackingRate() : trackingTimerRateAxis1 ;
      f *= 60* 1.00273790935;
    }
    else
      f = 0.0;

    char    temp[10];
    dtostrf(f, 0, 5, temp);
    strcpy(reply, temp);
    quietReply = true;
    break;
  case 't':
    //  :Gt#   Get Current Site Latitude
    //         Returns: sDD*MM#
    //         The latitude of the current site. Positive for North latitudes
    i = highPrecision;
    highPrecision = false;
    if (!doubleToDms(reply, localSite.latitude(), false, true))
      commandError = true;
    else
      quietReply = true;
    highPrecision = i;
    break;
  case 'U':
  {
    //  :GU#   Get telescope Status
    for ( i = 0; i<50; i++)
        reply[i] = ' ';
    i = 0;
    if (trackingState != TrackingON) reply[0] = 'n';
    if (trackingState != TrackingMoveTo) reply[1] = 'N';

    const char  *parkStatusCh = "pIPF";
    reply[2] = parkStatusCh[parkStatus];  // not [p]arked, parking [I]n-progress, [P]arked, Park [F]ailed
    if (atHome) reply[3] = 'H';
    if (pps.m_synced) reply[4] = 'S';
    if (GuidingState != GuidingOFF)
    {
      reply[5] = 'G';
      if (GuidingState == GuidingPulse) reply[6] = '*';
      else if (GuidingState == GuidingRecenter) reply[6] = '+';
      if (guideDirAxis1 == 'e') reply[7] = '>';
      else if(guideDirAxis1 == 'w') reply[7] = '<';
      if (guideDirAxis2 == 'n') reply[8] = '^';
      else if (guideDirAxis2 == 's') reply[8] = '_';
    }
    if (faultAxis1 || faultAxis2) reply[9] = 'f';
    if (refraction)
      reply[10] = 'r';
    else
      reply[10] = 's';
    if (onTrack) reply[11] = 't';

    // provide mount type
    if (mountType == MOUNT_TYPE_GEM)
      reply[12] = 'E';
    else if (mountType == MOUNT_TYPE_FORK)
      reply[12] = 'K';
    else if (mountType == MOUNT_TYPE_FORK_ALT)
      reply[12] = 'k';
    else if (mountType == MOUNT_TYPE_ALTAZM)
      reply[12] = 'A';
    else
      reply[12] = 'U';

    reply[13] = '0' + lastError;
    reply[14] = 0;
    i = 15;
    quietReply = true;                                   //         Returns: SS#
  }
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
      strcpy(reply, FirmwareDate);
      break;
    case 'N':
      strcpy(reply, FirmwareNumber);
      break;
    case 'P':
      strcpy(reply, FirmwareName);
      break;
    case 'T':
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
    //  :GZ#   Get telescope azimuth
    //         Returns: DDD*MM# or DDD*MM'SS# (based on precision setting)
    if (command[1] == 'Z')
      getHor(&f, &f1);
    if (!doubleToDms(reply, &f1, true, false))
      commandError = true;
    else
      quietReply = true;
    break;
  default:
    commandError = true;
    break;
  }
}
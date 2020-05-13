#include "Command.h"
#include "ValueToString.h"
//   S - Telescope Set Commands
void Command_SX()
{
//  :SXnnn,VVVVVV...#   Set TeenAstro value
//          Return: 0 on failure
//                  1 on success
  int i;
  switch (command[2])
  {
  case '0':
    // :SXAn# Align Model values
    switch (command[3])
    {
    case '0':
      break;
    case '1':
      break;
    case '2':
      break;
    case '3':
      break;
    case '4':
      break;
    case '5':
      break;
    case 'x':
      //GeoAlign.init();
      //GeoAlign.writeCoe();
      break;
    }
    break;
  case 'R':
    // :SXRn# Rates Settings
    switch (command[3])
    {

    case 'A':
      // :SXRA# Set degree for acceleration
      DegreesForAcceleration = min(max(0.1*(double)strtol(&command[5], NULL, 10), 0.1), 25.0);
      XEEPROM.update(EE_degAcc, (uint8_t)(DegreesForAcceleration * 10));
      SetAcceleration();
      strcpy(reply, "1");
      break;
    case 'D':
    {
      // :SXRD# define default rate
      int val = strtol(&command[5], NULL, 10);
      val = val > 4 || val < 0 ? 3 : val;
      XEEPROM.write(EE_DefaultRate, val);
      strcpy(reply, "1");
      break;
    }
    case '0': // :SXR0,VVV# Set Rate for user defined rates
    case '1': // :SXR1,VVV# Set Rate for user defined rates
    case '2': // :SXR2,VVV# Set Rate for user defined rates
    case '3': // :SXR3,VVV# Set Rate for user defined rates
      if (GuidingState == GuidingOFF)
      {
        i = command[3] - '0';
        int val = strtol(&command[5], NULL, 10);
        val = val > 0 && val < 256 ? val : pow(4, i);
        XEEPROM.write(EE_Rate0 + i, val);
        if (i == 0)
          guideRates[0] = (double)val / 100.;
        else
          guideRates[i] = val;
        if (activeGuideRate == i)
          enableGuideRate(i, true);
        strcpy(reply, "1");
      }
      else strcpy(reply, "0");
      break;
    case 'X':
      // :SXRX,VVVV# Set Rate for max Rate
      XEEPROM.writeInt(EE_maxRate, (int)strtol(&command[5], NULL, 10));
      initMaxRate();
      strcpy(reply, "1");
      break;
    default:
      strcpy(reply, "0");
      break;
    }
  case 'L':
    // user defined limits
    switch (command[3])
    {
    case 'E':
      // :SXLE,VV.V# set user defined Meridian East Limit
      minutesPastMeridianGOTOE = (double)strtol(&command[5], NULL, 10);
      if (minutesPastMeridianGOTOE > 180) minutesPastMeridianGOTOE = 180;
      if (minutesPastMeridianGOTOE < -180) minutesPastMeridianGOTOE = -180;
      XEEPROM.update(EE_dpmE, round((minutesPastMeridianGOTOE*15.0) / 60.0) + 128);
      strcpy(reply, "1");
      break;
    case 'W':
      // :SXLW,VV.V# set user defined Meridian West Limit
      minutesPastMeridianGOTOW = (double)strtol(&command[5], NULL, 10);
      if (minutesPastMeridianGOTOW > 180) minutesPastMeridianGOTOW = 180;
      if (minutesPastMeridianGOTOW < -180) minutesPastMeridianGOTOW = -180;
      XEEPROM.update(EE_dpmW, round((minutesPastMeridianGOTOW*15.0) / 60.0) + 128);
      strcpy(reply, "1");
      break;
    case 'U':
      // :SXLU,VV# set user defined Under Pole Limit
      underPoleLimitGOTO = (double)strtol(&command[5], NULL, 10) / 10;
      if (underPoleLimitGOTO > 12) underPoleLimitGOTO = 12;
      if (underPoleLimitGOTO < 9) underPoleLimitGOTO = 9;
      XEEPROM.update(EE_dup, round(underPoleLimitGOTO*10.0));
      strcpy(reply, "1");
      break;
    case 'H':
      // :GXLH,VV# set user defined horizon Limit
      // NB: duplicate with :Sh#
      if ((atoi2(&command[5], &i)) && ((i >= -30) && (i <= 30)))
      {
        minAlt = i;
        XEEPROM.update(EE_minAlt, minAlt + 128);
        strcpy(reply, "1");
      }
      else
        strcpy(reply, "0");
    case 'O':
      // :GXLO# set user defined horizon Limit
      // NB: duplicate with :So#
      if ((atoi2(&command[5], &i)) && ((i >= 45) && (i <= 91)))
      {
        maxAlt = i;
        XEEPROM.update(EE_maxAlt, maxAlt);
        strcpy(reply, "1");
      }
      else
        strcpy(reply, "0");
    default:
      strcpy(reply, "0");
      break;
    }
  case 'T':
    // :SXTn# Date/Time definition
    switch (command[3])
    {
      //  :SXT0HH:MM:SS#
      //          Return: 0 on failure
      //                  1 on success 
    case '0':
      i = highPrecision;
      highPrecision = true;
      int h1, m1, m2, s1;
      if (!hmsToHms(&h1, &m1, &m2, &s1, &command[4], highPrecision))
        strcpy(reply, "0");
      else
      {
        rtk.setClock(year(), month(), day(), h1, m1, s1, *localSite.longitude(), 0);
        strcpy(reply, "1");
      }
      highPrecision = i;
      break;
    case '1':
      //  :SXT1MM/DD/YY#
      //          Change Local Date to MM/DD/YY
      //          Return: 0 on failure
      //                  1 on success
      int y, m, d;
      if (!dateToYYYYMMDD(&y, &m, &d, &command[4]))
        strcpy(reply, "0");
      else
      {
        rtk.setClock(y, m, d, hour(), minute(), second(), *localSite.longitude(), 0);
        strcpy(reply, "1");
      }
      break;
    case '2':
    {
      //  :SXT2nnnnn#
      //          Define current date from
      //          Return: 0 on failure
      //                  1 on success
      char *pEnd;
      unsigned long t = strtoul(&command[5], &pEnd, 10);
      rtk.SetFromTimeStamp(t);
      strcpy(reply, "1");
      break;
    }
    }
    break;
  case 'M':
    // :SXMnn# Mount Settings
    switch (command[3])
    {
    case 'B':
    {
      // :SXMBn# Set Backlash
      int i;
      if ((atoi2((char *)&command[5], &i)) && ((i >= 0) && (i <= 999)))
      {
        if (command[4] == 'D')
        {
          bl_Axis2.inSeconds = i;
          XEEPROM.writeInt(EE_backlashAxis2, bl_Axis2.inSeconds);
          bl_Axis2.inSteps = (int)round(((double)bl_Axis2.inSeconds * 3600.0) / (double)StepsPerDegreeAxis2);
          bl_Axis2.movedSteps = 0;
          strcpy(reply, "1");
        }
        else if (command[4] == 'R')
        {
          bl_Axis1.inSeconds = i;
          XEEPROM.writeInt(EE_backlashAxis1, bl_Axis1.inSeconds);
          bl_Axis1.inSteps = (int)round(((double)bl_Axis1.inSeconds * 3600.0) / (double)StepsPerDegreeAxis1);
          bl_Axis1.movedSteps = 0;
          strcpy(reply, "1");
        }
        else strcpy(reply, "0");
      }
      else strcpy(reply, "0");
    }
    break;
    case 'G':
    {
      // :SXMGn# Set Gear
      int i;
      if ((command[4] == 'D' || command[4] == 'R')
          && strlen(&command[5]) > 1 && strlen(&command[5]) < 11
          && atoi2(&command[5], &i))
      {
        if (command[4] == 'D')
        {
          double fact = (double)i / GearAxis2;
          cli();
          posAxis2 = fact * posAxis2;
          sei();
          StopAxis2();
          GearAxis2 = (unsigned int)i;
          XEEPROM.writeInt(EE_GearAxis2, i);
        }
        else
        {
          double fact = (double)i / GearAxis1;
          cli();
          posAxis1 = fact * posAxis1;
          sei();
          StopAxis1();
          GearAxis1 = (unsigned int)i;
          XEEPROM.writeInt(EE_GearAxis1, i);
        }
        strcpy(reply, "1");
        unsetPark();
        updateRatios(true);
      }
      else
        strcpy(reply, "0");
    }
    break;
    case 'S':
    {
      // :SXMBn# Set Step per Rotation
      int i;
      if ((command[4] == 'D' || command[4] == 'R')
          && (strlen(&command[5]) > 1) && (strlen(&command[5]) < 11)
          && atoi2((char *)&command[5], &i))
      {
        if (command[4] == 'D')
        {
          double fact = (double)i / StepRotAxis2;
          cli();
          posAxis2 = fact * posAxis2;
          sei();
          StopAxis2();
          StepRotAxis2 = (unsigned int)i;
          XEEPROM.writeInt(EE_StepRotAxis2, i);
        }
        else
        {
          double fact = (double)i / StepRotAxis1;
          cli();
          posAxis1 = fact * posAxis1;
          sei();
          StopAxis1();
          StepRotAxis1 = (unsigned int)i;
          XEEPROM.writeInt(EE_StepRotAxis1, i);
        }
        strcpy(reply, "1");
        unsetPark();
        updateRatios(true);
      }
      else
        strcpy(reply, "0");
    }
    break;
    case 'M':
    {
      // :SXMMn# Set Microstep
      // for example :GRXMMR3# for 1/8 microstep on the first axis 
      int i;
      if ((command[4] == 'D' || command[4] == 'R')
          && strlen(&command[5]) == 1
          && atoi2(&command[5], &i)
          && ((i >= 3) && (i < 9)))
      {
        if (command[4] == 'D')
        {
          double fact = pow(2., i - MicroAxis2);
          cli();
          posAxis2 = fact * posAxis2;
          sei();
          StopAxis2();
          MicroAxis2 = i;
          motorAxis2.setMicrostep(MicroAxis2);
          XEEPROM.write(EE_MicroAxis2, MicroAxis2);
        }
        else
        {
          double fact = pow(2., i - MicroAxis1);
          cli();
          posAxis1 = fact * posAxis1;
          sei();
          StopAxis1();
          MicroAxis1 = i;
          motorAxis1.setMicrostep(MicroAxis1);
          XEEPROM.write(EE_MicroAxis1, MicroAxis1);
        }
        updateRatios(true);
        strcpy(reply, "1");
      }
      else strcpy(reply, "0");
    }
    break;
    case 'R':
    {
      // :SXMRn# Set Reverse rotation
      if ((command[4] == 'D' || command[4] == 'R')
          && strlen(&command[5]) == 1
          && (command[5] == '0' || command[5] == '1'))
      {
        if (command[4] == 'D')
        {
          ReverseAxis2 = command[5] == '1' ? true : false;
          XEEPROM.write(EE_ReverseAxis2, ReverseAxis2);
        }
        else
        {
          ReverseAxis1 = command[5] == '1' ? true : false;
          XEEPROM.write(EE_ReverseAxis1, ReverseAxis1);
        }
        strcpy(reply, "1");
      }
      else
        strcpy(reply, "0");
    }
    break;
    case 'c':
    case 'C':
    {
      // :SXMRn# Set Current
      int i;
      if ((strlen(&command[5]) > 1) && (strlen(&command[5]) < 5)
          && atoi2((char *)&command[5], &i)
          && ((i >= 0) && (i <= 255)))
      {
        if (command[4] == 'D')
        {
          if (command[3] == 'C')
          {
            HighCurrAxis2 = (u_int8_t)i;
            XEEPROM.write(EE_HighCurrAxis2, HighCurrAxis2);
          }
          else
          {
            LowCurrAxis2 = (u_int8_t)i;
            XEEPROM.write(EE_LowCurrAxis2, LowCurrAxis2);
            motorAxis2.setCurrent((unsigned int)LowCurrAxis2 * 10);
          }
        }
        else if (command[4] == 'R')
        {
          if (command[3] == 'C')
          {
            HighCurrAxis1 = (u_int8_t)i;
            XEEPROM.write(EE_HighCurrAxis1, HighCurrAxis1);
          }
          else
          {
            LowCurrAxis1 = (u_int8_t)i;
            XEEPROM.write(EE_LowCurrAxis1, LowCurrAxis1);
            motorAxis1.setCurrent((unsigned int)LowCurrAxis1 * 10);
          }
        }
        strcpy(reply, "1");
      }
      else strcpy(reply, "0");
    }
    break;
    case 'F':
    {
      // :SXMRn# Set Stall guard
      int i;
      if ((command[4] == 'D' || command[4] == 'R')
          && (strlen(&command[5]) > 1) && (strlen(&command[5]) < 5)
          && atoi2((char *)&command[5], &i)
          && ((i >= 0) && (i <= 127)))
      {
        i = i - 64;

        if (command[4] == 'D')
        {
          motorAxis2.setSG(i);
        }
        else
        {
          motorAxis1.setSG(i);
        }
        strcpy(reply, "1");
      }
      else strcpy(reply, "0");
    }
    break;
    }
    break;
  }
}

void Command_S(Command& process_command)
{
  char* conv_end;
  int i;
  double f, f1;
  switch (command[1])
  {
  case '!':
    i = (int)(command[2] - '0');
    if (i > 0 && i < 5)
    {
      XEEPROM.write(EE_mountType, i);
      Serial.end();
      Serial1.end();
      Serial2.end();
      delay(1000);
      _reboot_Teensyduino_();
    }
    else strcpy(reply, "0");
    break;
  case 'a':
    //  :SasDD*MM#
    //         Set target object altitude to sDD:MM# or sDD:MM:SS# (based on precision setting)
    //         Native LX200
    //         Returns:
    //         0 if Object is within slew range, 1 otherwise
    if (dmsToDouble(&newTargetAlt, &command[2], true, highPrecision)) strcpy(reply, "1");
    else strcpy(reply, "1");
    break;
  case 'B':
    //  :SBn#  Set Baud Rate n for Serial-0, where n is an ASCII digit (1..9) with the following interpertation
        //         0=115.2K, 1=56.7K, 2=38.4K, 3=28.8K, 4=19.2K, 5=14.4K, 6=9600, 7=4800, 8=2400, 9=1200
        //         Returns: "1" At the current baud rate and then changes to the new rate for further communication

  {
    i = (int)(command[2] - '0');
    if ((i >= 0) && (i < 10))
    {
      if (process_command == COMMAND_SERIAL)
      {
        Serial.print("1");
        delay(20);
        Serial.begin(baudRate[i]);
      }
      else
      {
        Serial1.print("1");
        delay(20);
        Serial1.begin(baudRate[i]);
      }
      strcpy(reply, "1");
    }
    else strcpy(reply, "0");
  }
  case 'C':
    //  :SCMM/DD/YY#
    //          Change Local Date to MM/DD/YY
    //          Return: 0 on failure
    //                  1 on success
    int y, m, d;
    if (!dateToYYYYMMDD(&y, &m, &d, &command[2])) strcpy(reply, "0");
    else
    {
      rtk.setClock(y, m, d, hour(), minute(), second(), *localSite.longitude(), 0);
      strcpy(reply, "1");
    }
    break;
  case 'e':
    //  :SesDDDDD#
    //          Set current sites elevation above see level
    //          Return: 0 on failure
    //                  1 on success
    if (atoi2(&command[2], &i))
    {
      if (localSite.setElev(i)) strcpy(reply, "1");
      else strcpy(reply, "0");
    }
    else strcpy(reply, "0");
    break;
  case 'd':
    //  :SdsDD*MM#
    //          Set target object declination to sDD*MM or sDD*MM:SS depending on the current precision setting
    //          Return: 0 on failure
    //                  1 on success
    if (dmsToDouble(&newTargetDec, &command[2], true, highPrecision)) strcpy(reply, "1");
    else strcpy(reply, "0");
    break;
  case 'g':
    //  :SgsDDD*MM# or :SgDDD*MM#
    //          Set current sites longitude to sDDD*MM an ASCII position string, East longitudes can be as negative or >180 degrees
    //          Return: 0 on failure
    //                  1 on success
  {
    double longi = 0;
    if ((command[2] == '-') || (command[2] == '+')) i = 1;
    else i = 0;
    if (dmsToDouble(&longi, &command[2 + i], false, false))
    {
      if (command[2] == '-') longi = -longi;
      localSite.setLong(longi);
      rtk.resetLongitude(*localSite.longitude());
      strcpy(reply, "1");
    }
    else strcpy(reply, "0");
  }
  break;
  //  :SG[sHH.H]#
//            Set the number of hours added to local time to yield UTC
//            Return: 0 on failure
//                    1 on success
  case 'G':
  {
    f = strtod(&command[2], &conv_end);
    if ((&command[2] != conv_end) &&
      (f >= -12 && f <= 12.0))
    {
      localSite.setToff(f);
      strcpy(reply, "1");
    }
    else strcpy(reply, "0");
  }
  break;
  case 'h':
    //  :Sh+DD#
    //          Set the lowest elevation to which the telescope will goTo
    //          Return: 0 on failure
    //                  1 on success
  {
    if ((command[2] == 0) || (strlen(&command[2]) > 3))
    {
      strcpy(reply, "0");
      return;
    }

    if ((atoi2(&command[2], &i)) && ((i >= -30) && (i <= 30)))
    {
      minAlt = i;
      XEEPROM.update(EE_minAlt, minAlt + 128);
      strcpy(reply, "1");
    }
    else strcpy(reply, "0");
  }
  break;
  case 'L':
    //  :SLHH:MM:SS#
    //          Set the local Time
    //          Return: 0 on failure
    //                  1 on success  
  {
    i = highPrecision;
    highPrecision = true;
    int h1, m1, m2, s1;
    if (hmsToHms(&h1, &m1, &m2, &s1, &command[2], highPrecision))
    {
      rtk.setClock(year(), month(), day(), h1, m1, s1, *localSite.longitude(), *localSite.toff());
      strcpy(reply, "1");
    }
    else strcpy(reply, "0");
    highPrecision = i;
  }
  break;

  case 'M':
  case 'N':
  case 'O':
  case 'P':
    //  :SM<string>#
    //  :SN<string>#
    //  :SO<string>#
    //  :SP<string>#
    //          Set site name to be <string>, up to 14 characters.
    //          Return: 0 on failure
    //                  1 on success
  {
    i = command[1] - 'M';
    if (strlen(&command[2]) > 14)
      strcpy(reply, "0");
    else
    {
      XEEPROM.writeString(EE_sites + i * SiteSize + EE_site_name, &command[2]);
      strcpy(reply, "1");
    }
  }
  break;
  case 'm':
  {
    if ((command[2] != 0) && (strlen(&command[2]) < 2))
    {
      if (command[2] == 'N')
      {
        newTargetPierSide = PIER_NOTVALID;
        strcpy(reply, "1");
      }
      else if (command[2] == 'E')
      {
        if (GetPierSide() == PIER_WEST)
        {
          newTargetPierSide = PIER_EAST;
          strcpy(reply, "1");
        }
      }
      else if (command[2] == 'W')
      {
        if (GetPierSide() == PIER_EAST)
        {
          newTargetPierSide = PIER_WEST;
          strcpy(reply, "1");
        }
      }
      else strcpy(reply, "0");
    }
    else strcpy(reply, "0");
    break;
  }
  case 'n':
    localSite.setSiteName(&command[2]);
    strcpy(reply, "1");
    break;
  case 'o':
    //  :SoDD#
    //          Set the overhead elevation limit to DD#
    //          Return: 0 on failure
    //                  1 on success

  {
    if ((command[2] != 0) && (strlen(&command[2]) < 3))
    {
      if ((atoi2(&command[2], &i)) && ((i >= 60) && (i <= 91)))
      {
        maxAlt = i;
        maxAlt = maxAlt > 87 && isAltAZ() ? 87 : maxAlt;
        XEEPROM.update(EE_maxAlt, maxAlt);
        strcpy(reply, "1");
      }
      else strcpy(reply, "0");
    }
    else strcpy(reply, "0");
  }
  break;
  case 'r':
    //  :SrHH:MM.T#
    //  :SrHH:MM:SS#
    //          Set target object RA to HH:MM.T or HH:MM:SS (based on precision setting)
    //          Return: 0 on failure
    //                  1 on success

    if (hmsToDouble(&newTargetRA, &command[2], highPrecision))
    {
      newTargetRA *= 15.0;
      strcpy(reply, "1");
    }
    else strcpy(reply, "0");
    break;

  case 't':
    //  :StsDD*MM#
    //          Sets the current site latitude to sDD*MM#
    //          Return: 0 on failure
    //                  1 on success                                                             
  {
    i = highPrecision;
    highPrecision = false;
    double lat = 0;
    if (dmsToDouble(&lat, &command[2], true, highPrecision))
    {
      localSite.setLat(lat);
      initCelestialPole();
      initTransformation(true);
      strcpy(reply, "1");
    }
    else strcpy(reply, "0");
    highPrecision = i;
  }
  break;
  case 'R':
  {
    if (command[2] == 'E' && command[3] == 'F')
    {
      if (command[4] == '1')
      {
        refraction = true;
        XEEPROM.update(EE_refraction, refraction);
        strcpy(reply, "1");
      }
      else if (command[4] == '0')
      {
        refraction = false;
        XEEPROM.update(EE_refraction, refraction);
        strcpy(reply, "1");
      }
      else strcpy(reply, "0");
    }
    else strcpy(reply, "0");
  }
  break;
  case 'T':
    //  :STdd.ddddd#
    //          Return: 0 on failure
    //                  1 on success
  {
    if (!movingTo)
    {
      f = strtod(&command[2], &conv_end);
      if ((&command[2] != conv_end) &&
        (((f >= 30.0) && (f < 90.0)) || (abs(f) < 0.1)))
      {
        if (abs(f) < 0.1)
        {
          sideralTracking = false;
        }
        else
        {
          trackingTimerRateAxis1 = (f / 60.0) / 1.00273790935;
        }
        strcpy(reply, "1");
      }
      else strcpy(reply, "0");
    }
    else strcpy(reply, "0");
  }
  break;
  case 'U':
    // :SU# store current User defined Position
    getEqu(&f, &f1, localSite.cosLat(), localSite.sinLat(), false);
    XEEPROM.writeFloat(EE_RA, (float)f);
    XEEPROM.writeFloat(EE_DEC, (float)f1);
    strcpy(reply, "1");
    break;
  case 'X':
    Command_SX();
    break;
  case 'z':
    //  :SzDDD*MM#
    //          Sets the target Object Azimuth
    //          Return: 0 on failure
    //                  1 on success
    if (dmsToDouble(&newTargetAzm, &command[2], false, highPrecision)) strcpy(reply, "1");
    else strcpy(reply, "1");
    break;
  default:
    strcpy(reply, "0");
    break;
  }
}
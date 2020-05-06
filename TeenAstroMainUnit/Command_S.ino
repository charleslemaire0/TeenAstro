#include "Command.h"
#include "ValueToString.h"
//   S - Telescope Set Commands
void Command_SX()
{
  //  :SXnn,VVVVVV...#   Set TeenAstro value
//          Return: 0 on failure
//                  1 on success
  switch (parameter[0])
  {
  case '0':
    // :SXAn# Align Model values
    switch (parameter[1])
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
    switch (parameter[1])
    {

    case 'A':
      // :SXRA# Set degree for acceleration
      DegreesForAcceleration = min(max(0.1*(double)strtol(&parameter[3], NULL, 10), 0.1), 25.0);
      XEEPROM.update(EE_degAcc, (uint8_t)(DegreesForAcceleration * 10));
      SetAcceleration();
      break;
    case 'D':
    {
      // :SXRD# define default rate
      int val = strtol(&parameter[3], NULL, 10);
      val = val > 4 || val < 0 ? 3 : val;
      XEEPROM.write(EE_DefaultRate, val);
    }
    case '0':
    case '1':
    case '2':
    case '3':
      // :SXRn# Set Rate for user defined rates
      if (GuidingState == GuidingOFF)
      {
        i = parameter[1] - '0';
        int val = strtol(&parameter[3], NULL, 10);
        val = val > 0 && val < 256 ? val : pow(4, i) ;
        XEEPROM.write(EE_Rate0 + i, val);
        if (i == 0)
          guideRates[0] = (double)val / 100.;
        else
          guideRates[i] = val;
        if (activeGuideRate == i)
          enableGuideRate(i, true);
      }
      break;
    case 'X':
      // :SXRX# Set Rate for max Rate
      XEEPROM.writeInt(EE_maxRate, (int)strtol(&parameter[3], NULL, 10));
      initMaxRate();
      break;
    default:
      commandError = true;
      break;
    }
  case 'L':
    // user defined limits
    switch (parameter[1])
    {
    case 'E':
      // :SXLE# set user defined Meridian East Limit
      minutesPastMeridianGOTOE = (double)strtol(&parameter[3], NULL, 10);
      if (minutesPastMeridianGOTOE > 180) minutesPastMeridianGOTOE = 180;
      if (minutesPastMeridianGOTOE < -180) minutesPastMeridianGOTOE = -180;
      XEEPROM.update(EE_dpmE, round((minutesPastMeridianGOTOE*15.0) / 60.0) + 128);
      break;
    case 'W':
      // :SXLW# set user defined Meridian West Limit
      minutesPastMeridianGOTOW = (double)strtol(&parameter[3], NULL, 10);
      if (minutesPastMeridianGOTOW > 180) minutesPastMeridianGOTOW = 180;
      if (minutesPastMeridianGOTOW < -180) minutesPastMeridianGOTOW = -180;
      XEEPROM.update(EE_dpmW, round((minutesPastMeridianGOTOW*15.0) / 60.0) + 128);
      break;
    case 'U':
      // :SXLU# set user defined Under Pole Limit
      underPoleLimitGOTO = (double)strtol(&parameter[3], NULL, 10) / 10;
      if (underPoleLimitGOTO > 12) underPoleLimitGOTO = 12;
      if (underPoleLimitGOTO < 9) underPoleLimitGOTO = 9;
      XEEPROM.update(EE_dup, round(underPoleLimitGOTO*10.0));
      break;
    case 'H':
      // :GXLH# set user defined horizon Limit
      // NB: duplicate with :Sh#
      if ((atoi2(&parameter[3], &i)) && ((i >= -30) && (i <= 30)))
      {
        minAlt = i;
        XEEPROM.update(EE_minAlt, minAlt + 128);
      }
      else
        commandError = true;
    case 'O':
      // :GXLO# set user defined horizon Limit
      // NB: duplicate with :So#
      if ((atoi2(&parameter[3], &i)) && ((i >= 45) && (i <= 91)))
      {
        maxAlt = i;
        XEEPROM.update(EE_maxAlt, maxAlt);
      }
      else
        commandError = true;
    default:
      commandError = true;
      break;
    }
  case 'T':
    // :SXTn# Date/Time definition
    switch (parameter[1])
    {
      //  :SXT0HH:MM:SS#
      //          Return: 0 on failure
      //                  1 on success 
    case '0':
      i = highPrecision;
      highPrecision = true;
      int h1, m1, m2, s1;
      if (!hmsToHms(&h1, &m1, &m2, &s1, &parameter[2], highPrecision))
        commandError = true;
      else
      {
        rtk.setClock(year(), month(), day(), h1, m1, s1, *localSite.longitude(), 0);
      }
      highPrecision = i;
      break;
    case '1':
      //  :SXT1MM/DD/YY#
      //          Change Local Date to MM/DD/YY
      //          Return: 0 on failure
      //                  1 on success
      int y, m, d;
      if (!dateToYYYYMMDD(&y, &m, &d, &parameter[2]))
        commandError = true;
      else
      {
        rtk.setClock(y, m, d, hour(), minute(), second(), *localSite.longitude(), 0);
      }
      break;
    case '2':
    {
      //  :SXT2nnnnn#
      //          Define current date from
      //          Return: 0 on failure
      //                  1 on success
      char *pEnd;
      unsigned long t = strtoul(&parameter[3], &pEnd, 10);
      rtk.SetFromTimeStamp(t);
      break;
    }
    }
    break;
  case 'M':
    // :SXMnn# Mount Settings
    switch (parameter[1])
    {
    case 'B':
    {
      // :SXMBn# Set Backlash
      int i;
      if ((strlen(parameter) > 1) && (strlen(parameter) < 5))
      {
        if ((atoi2((char *)&parameter[3], &i)) && ((i >= 0) && (i <= 999)))
        {
          if (parameter[2] == 'D')
          {
            backlashAxis2 = i;
            XEEPROM.writeInt(EE_backlashAxis2, backlashAxis2);
            StepsBacklashAxis2 = (int)round(((double)backlashAxis2 * 3600.0) / (double)StepsPerDegreeAxis2);
          }
          else if (parameter[2] == 'R')
          {
            backlashAxis1 = i;
            XEEPROM.writeInt(EE_backlashAxis1, backlashAxis1);
            StepsBacklashAxis1 = (int)round(((double)backlashAxis1 * 3600.0) / (double)StepsPerDegreeAxis1);
          }
          else
            commandError = true;
        }
        else
          commandError = true;
      }
    }
    break;
    case 'G':
    {
      // :SXMGn# Set Gear
      int i;
      if ((parameter[2] == 'D' || parameter[2] == 'R')
          && strlen(&parameter[3]) > 1 && strlen(&parameter[3]) < 11
          && atoi2(&parameter[3], &i))
      {
        if (parameter[2] == 'D')
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
        unsetPark();
        updateRatios(true);
      }
      else
        commandError = true;
    }
    break;
    case 'S':
    {
      // :SXMBn# Set Step per Rotation
      int i;
      if ((parameter[2] == 'D' || parameter[2] == 'R')
          && (strlen(&parameter[3]) > 1) && (strlen(&parameter[3]) < 11)
          && atoi2((char *)&parameter[3], &i))
      {
        if (parameter[2] == 'D')
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
        unsetPark();
        updateRatios(true);
      }
      else
        commandError = true;
    }
    break;
    case 'M':
    {
      // :SXMMn# Set Microstep
      // for example :GRXMMR3# for 1/8 microstep on the first axis 
      int i;
      if ((parameter[2] == 'D' || parameter[2] == 'R')
          && strlen(&parameter[3]) == 1
          && atoi2(&parameter[3], &i)
          && ((i >= 3) && (i < 9)))
      {
        if (parameter[2] == 'D')
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
      }
      else
        commandError = true;
    }
    break;
    case 'R':
    {
      // :SXMRn# Set Reverse rotation
      if ((parameter[2] == 'D' || parameter[2] == 'R')
          &&  strlen(&parameter[3]) == 1
          && (parameter[3] == '0' || parameter[3] == '1'))
      {
        if (parameter[2] == 'D')
        {
          ReverseAxis2 = parameter[3] == '1' ? true : false;
          XEEPROM.write(EE_ReverseAxis2, ReverseAxis2);
        }
        else
        {
          ReverseAxis1 = parameter[3] == '1' ? true : false;
          XEEPROM.write(EE_ReverseAxis1, ReverseAxis1);
        }
      }
      else
        commandError = true;
    }
    break;
    case 'c':
    case 'C':
    {
      // :SXMRn# Set Current
      int i;
      if ((strlen(&parameter[3]) > 1) && (strlen(&parameter[3]) < 5)
          && atoi2((char *)&parameter[3], &i)
          && ((i >= 0) && (i <= 255)))
      {
        if (parameter[2] == 'D')
        {
          if (parameter[1] == 'C')
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
        else if (parameter[2] == 'R')
        {
          if (parameter[1] == 'C')
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
      }
      else
        commandError = true;
    }
    break;
    case 'F':
    {
      // :SXMRn# Set Stall guard
      int i;
      if ((parameter[2] == 'D' || parameter[2] == 'R')
          && (strlen(&parameter[3]) > 1) && (strlen(&parameter[3]) < 5)
          && atoi2((char *)&parameter[3], &i)
          && ((i >= 0) && (i <= 127)))
      {
        i = i - 64;

        if (parameter[2] == 'D')
        {
          motorAxis2.setSG(i);
        }
        else
        {
          motorAxis1.setSG(i);
        }
      }
      else
        commandError = true;
    }
    break;
    }
    break;
  }
}

void Command_S(Command& process_command)
{
  char* conv_end;
  switch (command[1])
  {
  case '!':
    i = (int)(parameter[0] - '0');
    if (i > 0 && i < 5)
    {
      XEEPROM.write(EE_mountType, i);
      Serial.end();
      Serial1.end();
      Serial2.end();
      delay(1000);
      _reboot_Teensyduino_();
    }
    else
      commandError = true;
    break;

  case 'a':
    //  :SasDD*MM#
    //         Set target object altitude to sDD:MM# or sDD:MM:SS# (based on precision setting)
    //         Native LX200
    //         Returns:
    //         0 if Object is within slew range, 1 otherwise
    if (!dmsToDouble(&newTargetAlt, parameter, true, highPrecision))
      commandError = true;
    break;


  case 'B':
    //  :SBn#  Set Baud Rate n for Serial-0, where n is an ASCII digit (1..9) with the following interpertation
        //         0=115.2K, 1=56.7K, 2=38.4K, 3=28.8K, 4=19.2K, 5=14.4K, 6=9600, 7=4800, 8=2400, 9=1200
        //         Returns: "1" At the current baud rate and then changes to the new rate for further communication

  {
    i = (int)(parameter[0] - '0');
    if ((i >= 0) && (i < 10))
    {
      if (process_command == COMMAND_SERIAL)
      {
        Serial_print("1");
        delay(20);
        Serial_Init(baudRate[i]);
      }
      else
      {
        Serial1_print("1");
        delay(20);
        Serial1_Init(baudRate[i]);
      }

      quietReply = true;
    }
    else
      commandError = true;
  }


  case 'C':
    //  :SCMM/DD/YY#
    //          Change Local Date to MM/DD/YY
    //          Return: 0 on failure
    //                  1 on success
    int y, m, d;
    if (!dateToYYYYMMDD(&y, &m, &d, parameter))
      commandError = true;
    else
    {
      rtk.setClock(y, m, d, hour(), minute(), second(), *localSite.longitude(), 0);
    }
    break;
  case 'e':
    //  :SesDDDDD#
    //          Set current sites elevation above see level
    //          Return: 0 on failure
    //                  1 on success
    if (atoi2(parameter, &i))
    {
      commandError = !localSite.setElev(i);
    }
    else
      commandError = true;
    break;
  case 'd':
    //  :SdsDD*MM#
    //          Set target object declination to sDD*MM or sDD*MM:SS depending on the current precision setting
    //          Return: 0 on failure
    //                  1 on success
    if (!dmsToDouble(&newTargetDec, parameter, true, highPrecision))
      commandError = true;
    break;
  case 'g':
    //  :SgsDDD*MM# or :SgDDD*MM#
    //          Set current sites longitude to sDDD*MM an ASCII position string, East longitudes can be as negative or >180 degrees
    //          Return: 0 on failure
    //                  1 on success
  {
    double longi = 0;
    if ((parameter[0] == '-') || (parameter[0] == '+'))
      i1 = 1;
    else
      i1 = 0;
    if (!dmsToDouble(&longi, (char *)&parameter[i1], false, false))
      commandError = true;
    else
    {
      if (parameter[0] == '-') longi = -longi;
      localSite.setLong(longi);
      rtk.resetLongitude(*localSite.longitude());
    }
  }
  break;
  //  :SG[sHH.H]#
//            Set the number of hours added to local time to yield UTC
//            Return: 0 on failure
//                    1 on success
  case 'G':
  {
    f = strtod(parameter, &conv_end);
    if ((&parameter[0] != conv_end) &&
      (f >= -12 && f <= 12.0))
    {
      localSite.setToff(f);
    }
    else
      commandError = true;
  }
  break;
  case 'h':
    //  :Sh+DD#
    //          Set the lowest elevation to which the telescope will goTo
    //          Return: 0 on failure
    //                  1 on success
  {
    if ((parameter[0] == 0) || (strlen(parameter) > 3))
      commandError = true;

    if ((atoi2(parameter, &i)) && ((i >= -30) && (i <= 30)))
    {
      minAlt = i;
      XEEPROM.update(EE_minAlt, minAlt + 128);
    }
    else
      commandError = true;
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
    if (!hmsToHms(&h1, &m1, &m2, &s1, parameter, highPrecision))
      commandError = true;
    else
    {
      rtk.setClock(year(), month(), day(), h1, m1, s1, *localSite.longitude(), *localSite.toff());
    }
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
    if (strlen(parameter) > 14)
      commandError = true;
    else
      XEEPROM.writeString(EE_sites + i * SiteSize + EE_site_name, parameter);
  }
  break;
  case 'm':
  {
    if ((parameter[0] != 0) && (strlen(parameter) < 2))
    {
      if (parameter[0] == 'N')
      {
        newTargetPierSide = PIER_NOTVALID;
      }
      else if (parameter[0] == 'E')
      {
        if (GetPierSide() == PIER_WEST)
        {
          newTargetPierSide = PIER_EAST;
        }
      }
      else if (parameter[0] == 'W')
      {
        if (GetPierSide() == PIER_EAST)
        {
          newTargetPierSide = PIER_WEST;
        }
      }
      else
        commandError = true;
    }
    else
      commandError = true;
    break;
  }
  case 'n':
    localSite.setSiteName(parameter);
    break;
  case 'o':
    //  :SoDD#
    //          Set the overhead elevation limit to DD#
    //          Return: 0 on failure
    //                  1 on success

  {
    if ((parameter[0] != 0) && (strlen(parameter) < 3))
    {
      if ((atoi2(parameter, &i)) && ((i >= 60) && (i <= 91)))
      {
        maxAlt = i;
        maxAlt = maxAlt > 87 && isAltAZ() ? 87 : maxAlt;
        XEEPROM.update(EE_maxAlt, maxAlt);
      }
      else
        commandError = true;
    }
    else
      commandError = true;
  }
  break;
  case 'r':
    //  :SrHH:MM.T#
    //  :SrHH:MM:SS#
    //          Set target object RA to HH:MM.T or HH:MM:SS (based on precision setting)
    //          Return: 0 on failure
    //                  1 on success

    if (!hmsToDouble(&newTargetRA, parameter, highPrecision))
      commandError = true;
    else
      newTargetRA *= 15.0;
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
    if (!dmsToDouble(&lat, parameter, true, highPrecision))
    {
      commandError = true;
    }
    else
    {
      localSite.setLat(lat);
      initCelestialPole();
      initTransformation(true);
    }
    highPrecision = i;
  }
  break;
  case 'R':
  {
    if (parameter[0] == 'E' && parameter[1] == 'F')
    {
      if (parameter[2] == '1')
      {
        refraction = true;
        XEEPROM.update(EE_refraction, refraction);
      }
      else if (parameter[2] == '0')
      {
        refraction = false;
        XEEPROM.update(EE_refraction, refraction);
      }
      else
        commandError = true;
    }
    else
      commandError = true;
  }
  break;
  case 'T':
    //  :STdd.ddddd#
    //          Return: 0 on failure
    //                  1 on success
  {
    if (!movingTo)
    {
      f = strtod(parameter, &conv_end);
      if ((&parameter[0] != conv_end) &&
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
      }
      else
        commandError = true;
    }
    else
      commandError = true;
  }
  break;
  case 'U':
    // :SU# store current User defined Position
    getEqu(&f, &f1, localSite.cosLat(), localSite.sinLat(), false);
    XEEPROM.writeFloat(EE_RA, (float)f);
    XEEPROM.writeFloat(EE_DEC, (float)f1);
    break;
  case 'X':
    Command_SX();
    break;
  case 'z':
    //  :SzDDD*MM#
    //          Sets the target Object Azimuth
    //          Return: 0 on failure
    //                  1 on success
    if (!dmsToDouble(&newTargetAzm, parameter, false, highPrecision))
      commandError = true;
    break;
  default:
    commandError = true;
    break;
  }
}
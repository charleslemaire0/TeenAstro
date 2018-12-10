#include "Command.h"
//   S - Telescope Set Commands

void Command_S(Command& process_command)
{
  char* conv_end;
  switch (command[1])
  {
  case '!':
    i = (int)(parameter[0] - '0');
    if (i > 0 && i < 5)
    {
      EEPROM.write(EE_mountType, i);
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
    //         Set target object altitude to sDD*MM# or sDD*MM'SS# (based on precision setting)
    //         Returns:
    //         0 if Object is within slew range, 1 otherwise
    if (!dmsToDouble(&newTargetAlt, parameter, true))
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
        while (Serial_transmit());
        delay(20);
        Serial_Init(baudRate[i]);
      }
      else if (process_command == COMMAND_ETHERNET)
      {
#if defined(W5100_ON)
        Ethernet_print("1");
        while (Ethernet_transmit());
        delay(20);
#endif
      }
      else
      {
        Serial1_print("1");
        while (Serial1_transmit());
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
    //          Change Date to MM/DD/YY
    //          Return: 0 on failure
    //                  1 on success
    int y, m, d;
    if (!dateToYYYYMMDD(&y, &m, &d, parameter))
      commandError = true;
    else
    {
      rtk.setClock(y, m, d, hour(), minute(), second(),*localSite.longitude());
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
    if (!dmsToDouble(&newTargetDec, parameter, true))
      commandError = true;
    break;


  case 'g':
    //  :SgsDDD*MM# or :SgDDD*MM#
    //          Set current sites longitude to sDDD*MM an ASCII position string, East longitudes can be as negative or >180 degrees
    //          Return: 0 on failure
    //                  1 on success
  {
    i = highPrecision;
    highPrecision = false;
    double longi= 0;
    if ((parameter[0] == '-') || (parameter[0] == '+'))
      i1 = 1;
    else
      i1 = 0;
    if (!dmsToDouble(&longi, (char *)&parameter[i1], false))
      commandError = true;
    else
    {
      if (parameter[0] == '-') longi = -longi;
      localSite.setLong(longi);
      rtk.resetLongitude(*localSite.longitude());
    }
    highPrecision = i;
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
      EEPROM.update(EE_minAlt, minAlt + 128);
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
    if (!hmsToHms(&h1,&m1,&m2,&s1, parameter))
      commandError = true;
    else
    {
      rtk.setClock(year(), month(), day(), h1, m1, s1, *localSite.longitude());
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
    //          Set site name to be <string>, up to 15 characters.
    //          Return: 0 on failure
    //                  1 on success
  {
    i = command[1] - 'M';
    if (strlen(parameter) > 15)
      commandError = true;
    else
      EEPROM_writeString(EE_sites + i * SiteSize + 10, parameter);
  }
  break;
  case 'm':
  {
    if ((parameter[0] != 0) && (strlen(parameter) < 2))
    {
      if (parameter[0] == 'N')
      {
        newTargetPierSide = 0;
      }
      else if (parameter[0] == 'E')
      {
        if (pierSide >= PierSideWest)
        {
          newTargetPierSide = PierSideEast;
        }
      }
      else if (parameter[0] == 'W')
      {
        if (pierSide < PierSideWest)
        {
          newTargetPierSide = PierSideWest;
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
        maxAlt = maxAlt > 87 && mountType == MOUNT_TYPE_ALTAZM ? 87 : maxAlt;
        EEPROM.update(EE_maxAlt, maxAlt);
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

    if (!hmsToDouble(&newTargetRA, parameter))
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
    double lat=0;
    if (!dmsToDouble(&lat, parameter, true))
    {
      commandError = true;
    }
    else
    {
      localSite.setLat(lat);
      if (mountType == MOUNT_TYPE_ALTAZM)
      {
        celestialPoleStepAxis2 = fabs(*localSite.latitude()) * StepsPerDegreeAxis2;
        if (*localSite.latitude() < 0)
          celestialPoleStepAxis1 = halfRotAxis1;
        else
          celestialPoleStepAxis1 = 0L;
      }
      else
      {
        if (*localSite.latitude() < 0)
          celestialPoleStepAxis2 = -halfRotAxis2;
        else
          celestialPoleStepAxis2 = halfRotAxis2;
      }

      if (*localSite.latitude() > 0.0)
        HADir = HADirNCPInit;
      else
        HADir = HADirSCPInit;
    }

    highPrecision = i;
  }
  break;


  case 'T':
    //  :STdd.ddddd#
    //          Return: 0 on failure
    //                  1 on success
  {
    if ((trackingState == TrackingON) ||
      (trackingState == TrackingOFF))
    {
      f = strtod(parameter, &conv_end);
      if ((&parameter[0] != conv_end) &&
        (((f >= 30.0) && (f < 90.0)) || (abs(f) < 0.1)))
      {
        if (abs(f) < 0.1)
        {
          trackingState = TrackingOFF;
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


  case 'X':
    //  :SXnn,VVVVVV...#   Set OnStep value
    //          Return: 0 on failure
    //                  1 on success

  {
    if (parameter[0] == '0')
    {                   // 0n: Align Model
      switch (parameter[1])
      {
      //case '0':
      //  indexAxis1 = (double)strtol(&parameter[3], NULL, 10) / 3600.0;
      //  break;  // indexAxis1

      //case '1':
      //  indexAxis2 = (double)strtol(&parameter[3], NULL, 10) / 3600.0;
      //  break;  // indexAxis2

      case '2':
        GeoAlign.altCor = (double)strtol(&parameter[3], NULL,
          10) / 3600.0;
        break;  // altCor

      case '3':
        GeoAlign.azmCor = (double)strtol(&parameter[3], NULL,
          10) / 3600.0;
        break;  // azmCor

      case '4':
        GeoAlign.doCor = (double)strtol(&parameter[3], NULL, 10) / 3600.0;
        break;  // doCor

      case '5':
        GeoAlign.pdCor = (double)strtol(&parameter[3], NULL, 10) / 3600.0;
        break;  // pdCor

      case 'x':
        GeoAlign.init();
        GeoAlign.writeCoe();
        break;
      }
    }
    else if (parameter[0] == '8')
    {
      switch (parameter[1])
      {
      case '2':
      {
        char *pEnd;
        unsigned long t = strtoul(&parameter[3], &pEnd, 10);
        rtk.SetFromTimeStamp(t);
        break;
      }
      }
      break;
    }
    else if (parameter[0] == '9')
    {                   // 9n: Misc.
      switch (parameter[1])
      {
      case '0':
      {
        if (GuidingState == GuidingOFF)
        {
          int val = strtol(&parameter[3], NULL, 10);
          val = val > 255 || val < 0 ? 100 : val;
          EEPROM.write(EE_pulseGuideRate, val);
          guideRates[0] = (double)val / 100.;
          if (activeGuideRate == 0)
            enableGuideRate(0, true);
        }
        break;
      }

      case '2':   // set new acceleration rate
      {
        EEPROM_writeInt(EE_maxRate, (int)strtol(&parameter[3], NULL, 10));
        initMaxRate();
        break;
      }
      default:
        commandError = true;
        break;
      }
    }
    else if (parameter[0] == 'E')
    {
      switch (parameter[1])
      {
      case '1': // Set the MaxRate in sideral Speed
        
        break;
      case '2': // Set degree for acceleration
        DegreesForAcceleration = min(max(0.1*(double)strtol(&parameter[3], NULL, 10),0.1), 25.0);
        EEPROM.update(EE_degAcc, (uint8_t)(DegreesForAcceleration * 10));
        SetAcceleration();
        break;
      case '9': // minutesPastMeridianE 
        minutesPastMeridianGOTOE = (double)strtol(&parameter[3], NULL, 10);
        if (minutesPastMeridianGOTOE > 180) minutesPastMeridianGOTOE = 180;
        if (minutesPastMeridianGOTOE < -180) minutesPastMeridianGOTOE = -180;
        EEPROM.update(EE_dpmE, round((minutesPastMeridianGOTOE*15.0) / 60.0) + 128);
        break;
      case 'A': // minutesPastMeridianW
        minutesPastMeridianGOTOW = (double)strtol(&parameter[3], NULL, 10);
        if (minutesPastMeridianGOTOW > 180) minutesPastMeridianGOTOW = 180;
        if (minutesPastMeridianGOTOW < -180) minutesPastMeridianGOTOW = -180;
        EEPROM.update(EE_dpmW, round((minutesPastMeridianGOTOW*15.0) / 60.0) + 128);
        break;
      case 'B': // minutesPastMeridianW
        underPoleLimitGOTO = (double)strtol(&parameter[3], NULL, 10)/10;
        if (underPoleLimitGOTO > 12) underPoleLimitGOTO = 12;
        if (underPoleLimitGOTO < 9) underPoleLimitGOTO = 9;
        EEPROM.update(EE_dup, round(underPoleLimitGOTO*10.0));
        break;

      default: commandError = true;
      }
    }
    else
      commandError = true;
  }
  break;

  case 'z':
    //  :SzDDD*MM#
    //          Sets the target Object Azimuth
    //          Return: 0 on failure
    //                  1 on success
    if (!dmsToDouble(&newTargetAzm, parameter, false))
      commandError = true;
    break;
  default:
    commandError = true;
    break;
  }
}
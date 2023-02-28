#include "Global.h"

unsigned long   baudRate[10] =
{
  115200, 56700, 38400, 28800, 19200, 14400, 9600, 4800, 2400, 1200
};

//   S - Telescope Set Commands
void Command_SX()
{
  //  :SXnnn,VVVVVV...#   Set TeenAstro value
  //          Return: 0 on failure
  //                  1 on success
  int i;
  long lval;
  bool ok = false;
  switch (command[2])
  {
  case 'A':
    // :SXAn,VVVVVV# Align Model values
  {
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
    case 'c':
      i = command[5] == 'y' ? 1 : (command[5] == 'n' ? 0 : -1);
      if (i != -1)
      {
        ok = 1;
        TrackingCompForAlignment = i;
      }
    }
    if (ok)
      strcpy(reply, "1");
    else
      strcpy(reply, "0");
    break;
  }
  case 'r':
    // :SXrn,V# refraction Settings
  {
    i = command[5] == 'y' ? 1 : (command[5] == 'n' ? 0 : -1);
    switch (command[3])
    {
    case 'p':
      if (i != -1)
      {
        if (doesRefraction.setPole(i))
        {
          initTransformation(true);
          syncAtHome();
        }
        ok = true;
      }
      break;
    case 'g':
      if (i != -1)
      {
        doesRefraction.setGoto(i);
        ok = true;
      }
      break;
    case 't':
      if (i != -1)
      {
        doesRefraction.setTracking(i);
        ok = true;
      }
      break;
    default:
      break;
    }
    if (ok)
      strcpy(reply, "1");
    else
      strcpy(reply, "0");
    break;
  }
  case 'R':
    // :SXRn,VVVV# Rates Settings
    switch (command[3])
    {
    case '0': // :SXR0,VVV# set user defined rate 0
    case '1': // :SXR1,VVV# 
    case '2': // :SXR2,VVV# 
    case '3': // :SXR3,VVV# 
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
    case 'A':
      // :SXRA,VVV# Set degree for acceleration
      mount.DegreesForAcceleration = min(max(0.1 * (double)strtol(&command[5], NULL, 10), 0.1), 25.0);
      XEEPROM.write(EE_degAcc, (uint8_t)(mount.DegreesForAcceleration * 10));
      SetAcceleration();
      strcpy(reply, "1");
      break;
    case 'D':
    {
      // :SXRD,V# define default rate
      int val = strtol(&command[5], NULL, 10);
      val = val > 4 || val < 0 ? 3 : val;
      XEEPROM.write(EE_DefaultRate, val);
      strcpy(reply, "1");
      break;
    }
    case 'X':
      // :SXRX,VVVV# Set Rate for max Rate
      XEEPROM.writeInt(EE_maxRate, (int)strtol(&command[5], NULL, 10));
      initMaxSpeed();
      strcpy(reply, "1");
      break;
    case 'r':
      // :SXRr,VVVVVVVVVV# Set Rate for RA 
      siderealMode = SIDM_TARGET;
      RequestedTrackingRateHA = 1. - (double)strtol(&command[5], NULL, 10) / 10000.0;
      computeTrackingRate(true);
      strcpy(reply, "1");
      break;
    case 'h':
      // :SXRh,VVVVVVVVVV# Set Rate for HA
      siderealMode = SIDM_TARGET;
      RequestedTrackingRateHA = (double)strtol(&command[5], NULL, 10) / 10000.0;
      computeTrackingRate(true);
      strcpy(reply, "1");
      break;
    case 'd':
      // :SXRd,VVVVVVVVVV# Set Rate for DEC
      siderealMode = SIDM_TARGET;
      RequestedTrackingRateDEC = (double)strtol(&command[5], NULL, 10) / 10000.0;
      computeTrackingRate(true);
      strcpy(reply, "1");
      break;
    case 'e':
      // :SXRe,VVVVVVVVVV# Store Rate for RA
      lval = strtol(&command[5], NULL, 10) ;
      storedTrackingRateRA = lval < -50000 || lval > 50000 ? 0 : lval;
      XEEPROM.writeLong(EE_RA_Drift, storedTrackingRateRA);
      strcpy(reply, "1");
      break;
    case 'f':
      // :SXRf,VVVVVVVVVV# Store Rate for DEC
      lval = strtol(&command[5], NULL, 10) ;
      storedTrackingRateDEC = lval < -50000 || lval > 50000 ? 0 : lval;
      XEEPROM.writeLong(EE_DEC_Drift, storedTrackingRateDEC);
      strcpy(reply, "1");
      break;
    default:
      strcpy(reply, "0");
      break;
    }
    break;
  case 'L':
    // user defined limits
    switch (command[3])
    {
    case 'A':
      // :SXLA,VVVV# set user defined minAXIS1 (always negatif)
      i = (int)strtol(&command[5], NULL, 10);
      XEEPROM.writeInt(EE_minAxis1, i);
      initLimitMinAxis1();
      strcpy(reply, "1");
      break;
    case 'B':
      // :SXLB,VVVV# set user defined maxAXIS1 (always positf)
      i = (int)strtol(&command[5], NULL, 10);
      XEEPROM.writeInt(EE_maxAxis1, i);
      initLimitMaxAxis1();
      strcpy(reply, "1");
      break;
    case 'C':
      // :SXLC,VVVV# set user defined minAXIS2 (always positf)
      i = (int)strtol(&command[5], NULL, 10);
      XEEPROM.writeInt(EE_minAxis2, i);
      initLimitMinAxis2();
      strcpy(reply, "1");
      break;
    case 'D':
      // :SXLD,VVVV# set user defined maxAXIS2 (always positf)
      i = (int)strtol(&command[5], NULL, 10);
      XEEPROM.writeInt(EE_maxAxis2, i);
      initLimitMaxAxis2();
      strcpy(reply, "1");
      break;
    case 'E':
      // :SXLE,sVV.V# set user defined Meridian East Limit
      limits.minutesPastMeridianGOTOE = (double)strtol(&command[5], NULL, 10);
      if (limits.minutesPastMeridianGOTOE > 180) limits.minutesPastMeridianGOTOE = 180;
      if (limits.minutesPastMeridianGOTOE < -180) limits.minutesPastMeridianGOTOE = -180;
      XEEPROM.write(EE_dpmE, round((limits.minutesPastMeridianGOTOE * 15.0) / 60.0) + 128);
      strcpy(reply, "1");
      break;
    case 'W':
      // :SXLW,sVV.V# set user defined Meridian West Limit
      limits.minutesPastMeridianGOTOW = (double)strtol(&command[5], NULL, 10);
      if (limits.minutesPastMeridianGOTOW > 180) limits.minutesPastMeridianGOTOW = 180;
      if (limits.minutesPastMeridianGOTOW < -180) limits.minutesPastMeridianGOTOW = -180;
      XEEPROM.write(EE_dpmW, round((limits.minutesPastMeridianGOTOW * 15.0) / 60.0) + 128);
      strcpy(reply, "1");
      break;
    case 'U':
      // :SXLU,VV# set user defined Under Pole Limit
      limits.underPoleLimitGOTO = (double)strtol(&command[5], NULL, 10) / 10;
      if (limits.underPoleLimitGOTO > 12) limits.underPoleLimitGOTO = 12;
      if (limits.underPoleLimitGOTO < 9) limits.underPoleLimitGOTO = 9;
      XEEPROM.write(EE_dup, round(limits.underPoleLimitGOTO * 10.0));
      strcpy(reply, "1");
      break;
    case 'H':
      // :GXLH,sVV# set user defined horizon Limit
      // NB: duplicate with :Sh#
      if ((atoi2(&command[5], &i)) && ((i >= -30) && (i <= 30)))
      {
        limits.minAlt = i;
        XEEPROM.write(EE_minAlt, limits.minAlt + 128);
        strcpy(reply, "1");
      }
      else
        strcpy(reply, "0");
    case 'O':
      // :SXLO,VV.VV# set user defined overhead Limit
      // NB: duplicate with :So#
      if ((atoi2(&command[5], &i)) && ((i >= 45) && (i <= 91)))
      {
        limits.maxAlt = i;
        XEEPROM.write(EE_maxAlt, limits.maxAlt);
        strcpy(reply, "1");
      }
      else
        strcpy(reply, "0");
    default:
      strcpy(reply, "0");
      break;
    }
    break;
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
      char* pEnd;
      unsigned long t = strtoul(&command[5], &pEnd, 10);
      rtk.SetFromTimeStamp(t);
      strcpy(reply, "1");
      break;
    }
    default:
      strcpy(reply, "0");
      break;
    }
    break;
  case 'M':
    // :SXMnn# Mount Settings
    switch (command[3])
    {   
    case 'B':
    {
      // :SXMBn,VVVV# Set Backlash
      int i;
      if ((atoi2((char*)&command[6], &i)) && ((i >= 0) && (i <= 999)))
      {
        if (command[4] == 'D')
        {
          mount.backlashA2.inSeconds = i;
          XEEPROM.writeInt(EE_backlashAxis2, mount.backlashA2.inSeconds);
          mount.backlashA2.inSteps = (int)round(((double)mount.backlashA2.inSeconds * 3600.0) / (double)geoA2.stepsPerDegree);
          mount.backlashA2.movedSteps = 0;
          strcpy(reply, "1");
        }
        else if (command[4] == 'R')
        {
          mount.backlashA1.inSeconds = i;
          XEEPROM.writeInt(EE_backlashAxis1, mount.backlashA1.inSeconds);
          mount.backlashA1.inSteps = (int)round(((double)mount.backlashA1.inSeconds * 3600.0) / (double)geoA1.stepsPerDegree);
          mount.backlashA1.movedSteps = 0;
          strcpy(reply, "1");
        }
        else strcpy(reply, "0");
      }
      else strcpy(reply, "0");
    }
    break;
    case 'G':
    {
      // :SXMGn,VVVV# Set Gear
      int i;
      if ((command[4] == 'D' || command[4] == 'R')
        && strlen(&command[6]) > 1 && strlen(&command[6]) < 11
        && atoi2(&command[6], &i))
      {
        if (command[4] == 'D')
        {
          double fact = (double)i / motorA2.gear;
          mount.mP->StopAxis2();
          motorA2.gear = (unsigned int)i;
          XEEPROM.writeInt(EE_motorA2gear, i);
        }
        else
        {
          double fact = (double)i / motorA1.gear;
          mount.mP->StopAxis1();
          motorA1.gear = (unsigned int)i;
          XEEPROM.writeInt(EE_motorA1gear, i);
        }
        updateRatios(true, true);

        strcpy(reply, "1");
      }
      else
        strcpy(reply, "0");
    }
    break;
    case 'S':
    {
      // :SXMBn,VVVV# Set Step per Rotation
      int i;
      if ((command[4] == 'D' || command[4] == 'R')
        && (strlen(&command[6]) > 1) && (strlen(&command[6]) < 11)
        && atoi2((char*)&command[6], &i))
      {
        if (command[4] == 'D')
        {
          double fact = (double)i / motorA2.stepRot;
          mount.mP->StopAxis2();
          motorA2.stepRot = (unsigned int)i;
          XEEPROM.writeInt(EE_motorA2stepRot, i);
        }
        else
        {
          double fact = (double)i / motorA1.stepRot;
          mount.mP->StopAxis1();
          motorA1.stepRot = (unsigned int)i;
          XEEPROM.writeInt(EE_motorA1stepRot, i);
        }
        updateRatios(true, true);

        strcpy(reply, "1");
      }
      else
        strcpy(reply, "0");
    }
    break;
    case 'M':
    {
      // :SXMMnV# Set Microstep
      // for example :GRXMMR3# for 1/8 microstep on the first axis 
      int i;
      if ((command[4] == 'D' || command[4] == 'R')
        && strlen(&command[6]) == 1
        && atoi2(&command[6], &i)
        && ((i >= 1) && (i < 9)))
      {
        if (command[4] == 'D')
        {
          double fact = pow(2., i - motorA2.micro);
          mount.mP->StopAxis2();
          motorA2.micro = i;
          motorA2.drvP->microsteps(motorA2.micro);;
          XEEPROM.write(EE_motorA2micro, motorA2.micro);
        }
        else
        {
          double fact = pow(2., i - motorA1.micro);
          mount.mP->StopAxis1();
          motorA1.micro = i;
          motorA1.drvP->microsteps(motorA1.micro);
          XEEPROM.write(EE_motorA1micro, motorA1.micro);
        }
        updateRatios(true, false);
        strcpy(reply, "1");
      }
      else strcpy(reply, "0");
    }
    break;
    case 'm':
    {
      // :SXMmnV# Set coolstep Stepper Mode
     // for example :GRXMmR1# for cool step on the first axis 
      int i;
      if ((command[4] == 'D' || command[4] == 'R')
        && strlen(&command[6]) == 1
        && atoi2(&command[6], &i)
        && ((i >= 0) && (i < 2)))
      {
        if (command[4] == 'D')
        {
          XEEPROM.write(EE_motorA2silent, i);
        }
        else
        {
          XEEPROM.write(EE_motorA1silent, i);
        }
        strcpy(reply, "1");
      }
      else strcpy(reply, "0");
    }
    break;
    case 'R':
    {
      // :SXMRnV# Set Reverse rotation
      if ((command[4] == 'D' || command[4] == 'R')
        && strlen(&command[6]) == 1
        && (command[6] == '0' || command[6] == '1'))
      {
        if (command[4] == 'D')
        {
          motorA2.reverse = command[6] == '1' ? true : false;
          XEEPROM.write(EE_motorA2reverse, motorA2.reverse);
        }
        else
        {
          motorA1.reverse = command[6] == '1' ? true : false;
          XEEPROM.write(EE_motorA1reverse, motorA1.reverse);
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
      unsigned int curr = (unsigned int)(strtol(&command[6], NULL, 10)/100)*100;
      if (((curr >= 100) && (curr <= 2800)))
      {
        if (command[4] == 'D')
        {
          if (command[3] == 'C')
          {
            motorA2.highCurr = curr;
            XEEPROM.write(EE_motorA2highCurr, motorA2.highCurr / 100);
          }
          else
          {
            motorA2.lowCurr = curr;
            XEEPROM.write(EE_motorA2lowCurr, motorA2.lowCurr / 100);
            motorA2.drvP->rms_current((unsigned int)motorA2.lowCurr);
          }
        }
        else if (command[4] == 'R')
        {
          if (command[3] == 'C')
          {
            motorA1.highCurr = curr;
            XEEPROM.write(EE_motorA1highCurr, motorA1.highCurr / 100);
          }
          else
          {
            motorA1.lowCurr = curr;
            XEEPROM.write(EE_motorA1lowCurr, motorA1.lowCurr / 100);
            motorA1.drvP->rms_current((unsigned int)motorA1.lowCurr);
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
        && (strlen(&command[6]) > 1) && (strlen(&command[6]) < 5)
        && atoi2((char*)&command[6], &i)
        && ((i >= 0) && (i <= 127)))
      {
        i = i - 64;
#if 0
        if (command[4] == 'D')
        {
          motorA2.drvP->setSG(i);
        }
        else
        {
          motorA1.drvP->setSG(i);
        }
#endif        
        strcpy(reply, "1");
      }
      else strcpy(reply, "0");
    }
    break;
    default:
      strcpy(reply, "0");
      break;
    }
    break;
  default:
    strcpy(reply, "0");
    break;
  }
}

void Command_S(Command& process_command)
{
  char* conv_end;
  int i, j;
  double f, f1;

  switch (command[1])
  {
  case '!':
    i = (int)(command[2] - '0');
    if (i > 0 && i < 5)
    {
      XEEPROM.write(EE_mountType, i);
#if 0
      Serial.end();
      Serial1.end();
      Serial2.end();
      delay(1000);
#endif
#ifdef __arm__      
      _reboot_Teensyduino_();
#endif
#ifdef __ESP32__
      EEPROM.commit();
      ESP.restart();
#endif      
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
    else strcpy(reply, "0");
    break;
  case 'B':
    //  :SBn#  Set Baud Rate n for Serial-0, where n is an ASCII digit (1..9) with the following interpertation
        //         0=115.2K, 1=56.7K, 2=38.4K, 3=28.8K, 4=19.2K, 5=14.4K, 6=9600, 7=4800, 8=2400, 9=1200
        //         Returns: "1" At the current baud rate and then changes to the new rate for further communication
    i = (int)(command[2] - '0');
    if ((i >= 0) && (i < 10))
    {
      if (process_command == COMMAND_SERIAL)
      {
        Serial.print('1');
        delay(20);
        Serial.begin(baudRate[i]);
      }
      else
      {
        Serial1.print('1');
        delay(20);
        Serial1.begin(baudRate[i]);
      }
      strcpy(reply, "1");
    }
    else strcpy(reply, "0");
    break;
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
    //  :SgsDDD*MM# or :SgDDD*MM# or :SgsDDD:MM:SS# or SgDDD:MM:ss#
    //          Set current sites longitude to sDDD*MM an ASCII position string, East longitudes can be as negative or >180 degrees
    //          Return: 0 on failure
    //                  1 on success
  {
    double longi = 0;
    if ((command[2] == '-') || (command[2] == '+')) i = 1;
    else i = 0;
    j = strlen(&command[7 + i]) > 1 ? (command[8 + i] == ':') : 0;
    if (dmsToDouble(&longi, &command[2 + i], false, j))
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
      limits.minAlt = i;
      XEEPROM.write(EE_minAlt, limits.minAlt + 128);
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
    //  :SM<string>#
    //  :SN<string>#
    //  :SO<string>#
    //          Set site name to be <string>, up to 14 characters.
    //          Return: 0 on failure
    //                  1 on success
    i = command[1] - 'M';
    if (strlen(&command[2]))
      XEEPROM.writeString(EE_sites + i * SiteSize + EE_site_name, &command[2], siteNameLen) ?
      strcpy(reply, "1") : strcpy(reply, "0");
    else
      strcpy(reply, "0");
    break;
  case 'm':
    if ((command[2] != 0) && (strlen(&command[2]) < 2))
    {
      if (command[2] == 'N')
      {
        newTargetPierSide = PIER_NOTVALID;
        strcpy(reply, "1");
      }
      else if (command[2] == 'E')
      {
        if (mount.mP->GetPierSide() == PIER_WEST)
        {
          newTargetPierSide = PIER_EAST;
          strcpy(reply, "1");
        }
      }
      else if (command[2] == 'W')
      {
        if (mount.mP->GetPierSide() == PIER_EAST)
        {
          newTargetPierSide = PIER_WEST;
          strcpy(reply, "1");
        }
      }
      else strcpy(reply, "0");
    }
    else strcpy(reply, "0");
    break;
  case 'n':
    if (strlen(&command[2]))
      localSite.setSiteName(&command[2]) ? strcpy(reply, "1") : strcpy(reply, "0");
    else
      strcpy(reply, "0");
    break;
  case 'o':
    //  :SoDD#
    //          Set the overhead elevation limit to DD#
    //          Return: 0 on failure
    //                  1 on success
    if ((command[2] != 0) && (strlen(&command[2]) < 3))
    {
      if ((atoi2(&command[2], &i)) && ((i >= 60) && (i <= 91)))
      {
        limits.maxAlt = i;
        XEEPROM.write(EE_maxAlt, limits.maxAlt);
        strcpy(reply, "1");
      }
      else strcpy(reply, "0");
    }
    else strcpy(reply, "0");
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
    //  :StsDD*MM# or :StsDD:MM:SS#
    //          Sets the current site latitude to sDD*MM#
    //          Return: 0 on failure
    //                  1 on success                                                             
    i = highPrecision;
    if (strlen(&command[7]) > 1)
      highPrecision = command[8] == ':';
    else
      highPrecision = false;
    if (dmsToDouble(&f, &command[2], true, highPrecision))
    {
      localSite.setLat(f);
      initHome();
      initTransformation(true);
      syncAtHome();
      strcpy(reply, "1");
    }
    else strcpy(reply, "0");
    highPrecision = i;
    break;
  case 'T':
    //  :STdd.ddddd#
    //          Return: 0 on failure
    //                  1 on success
    if (!isSlewing())
    {
      f = strtod(&command[2], &conv_end);
      if ((&command[2] != conv_end) &&
        (((f >= 30.0) && (f < 90.0)) || (abs(f) < 0.1)))
      {
        if (abs(f) < 0.1)
        {
          mount.mP->stopTracking();
        }
        else
        {
          RequestedTrackingRateHA = (f / 60.0) / 1.00273790935;
          // todo apply rate
        }
        strcpy(reply, "1");
      }
      else strcpy(reply, "0");
    }
    else strcpy(reply, "0");
    break;
  case 'U':
    // :SU# store current User defined Position
    mount.mP->getEqu(&f, &f1, localSite.cosLat(), localSite.sinLat(), false);
    XEEPROM.writeFloat(EE_RA, (float)f);
    XEEPROM.writeFloat(EE_DEC, (float)f1);
    strcpy(reply, "1");
    break;
  case 'z':
    //  :SzDDD*MM#
    //          Sets the target Object Azimuth
    //          Return: 0 on failure
    //                  1 on success
    if (dmsToDouble(&newTargetAzm, &command[2], false, highPrecision)) strcpy(reply, "1");
    else strcpy(reply, "0");// "1" BUGS ELSEWHERE???
    break;
  default:
    strcpy(reply, "0");
    break;
  }
}
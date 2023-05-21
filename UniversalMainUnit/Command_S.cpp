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
      replyShortTrue();
    else
      replyLongUnknown();
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
      replyShortTrue();
    else
      replyLongUnknown();
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
        XEEPROM.write(getMountAddress(EE_Rate0) + i, val);
        if (i == 0)
          guideRates[0] = (double)val / 100.;
        else
          guideRates[i] = val;
        if (activeGuideRate == i)
          enableGuideRate(i);
        replyShortTrue();
      }
      else replyLongUnknown();
      break;
    case 'A':
      // :SXRA,VVV# Set degree for acceleration
      mount.DegreesForAcceleration = min(max(0.1 * (double)strtol(&command[5], NULL, 10), 0.1), 25.0);
      XEEPROM.write(getMountAddress(EE_degAcc), (uint8_t)(mount.DegreesForAcceleration * 10));
      SetAcceleration();
      replyShortTrue();
      break;
    case 'D':
    {
      // :SXRD,V# define default rate
      int val = strtol(&command[5], NULL, 10);
      val = val > 4 || val < 0 ? 3 : val;
      XEEPROM.write(getMountAddress(EE_DefaultRate), val);
      replyShortTrue();
      break;
    }
    case 'X':
      // :SXRX,VVVV# Set Rate for max Rate
      XEEPROM.writeInt(getMountAddress(EE_maxRate), (int)strtol(&command[5], NULL, 10));
      initMaxSpeed();
      replyShortTrue();
      break;
    case 'r':
      // :SXRr,VVVVVVVVVV# Set Rate for RA 
      siderealMode = SIDM_TARGET;
      RequestedTrackingRateHA = 1. - (double)strtol(&command[5], NULL, 10) / 10000.0;
      computeTrackingRate(true);
      replyShortTrue();
      break;
    case 'h':
      // :SXRh,VVVVVVVVVV# Set Rate for HA
      siderealMode = SIDM_TARGET;
      RequestedTrackingRateHA = (double)strtol(&command[5], NULL, 10) / 10000.0;
      computeTrackingRate(true);
      replyShortTrue();
      break;
    case 'd':
      // :SXRd,VVVVVVVVVV# Set Rate for DEC
      siderealMode = SIDM_TARGET;
      RequestedTrackingRateDEC = (double)strtol(&command[5], NULL, 10) / 10000.0;
      computeTrackingRate(true);
      replyShortTrue();
      break;
    case 'e':
      // :SXRe,VVVVVVVVVV# Store Rate for RA
      lval = strtol(&command[5], NULL, 10) ;
      storedTrackingRateRA = lval < -50000 || lval > 50000 ? 0 : lval;
      XEEPROM.writeLong(getMountAddress(EE_RA_Drift), storedTrackingRateRA);
      replyShortTrue();
      break;
    case 'f':
      // :SXRf,VVVVVVVVVV# Store Rate for DEC
      lval = strtol(&command[5], NULL, 10) ;
      storedTrackingRateDEC = lval < -50000 || lval > 50000 ? 0 : lval;
      XEEPROM.writeLong(getMountAddress(EE_DEC_Drift), storedTrackingRateDEC);
      replyShortTrue();
      break;
    default:
      replyLongUnknown();
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
      XEEPROM.writeInt(getMountAddress(EE_minAxis1), i);
      initLimitMinAxis1();
      replyShortTrue();
      break;
    case 'B':
      // :SXLB,VVVV# set user defined maxAXIS1 (always positf)
      i = (int)strtol(&command[5], NULL, 10);
      XEEPROM.writeInt(getMountAddress(EE_maxAxis1), i);
      initLimitMaxAxis1();
      replyShortTrue();
      break;
    case 'C':
      // :SXLC,VVVV# set user defined minAXIS2 (always positf)
      i = (int)strtol(&command[5], NULL, 10);
      XEEPROM.writeInt(getMountAddress(EE_minAxis2), i);
      initLimitMinAxis2();
      replyShortTrue();
      break;
    case 'D':
      // :SXLD,VVVV# set user defined maxAXIS2 (always positf)
      i = (int)strtol(&command[5], NULL, 10);
      XEEPROM.writeInt(getMountAddress(EE_maxAxis2), i);
      initLimitMaxAxis2();
      replyShortTrue();
      break;
    case 'E':
      // :SXLE,sVV.V# set user defined Meridian East Limit
      limits.minutesPastMeridianGOTOE = (double)strtol(&command[5], NULL, 10);
      if (limits.minutesPastMeridianGOTOE > 180) limits.minutesPastMeridianGOTOE = 180;
      if (limits.minutesPastMeridianGOTOE < -180) limits.minutesPastMeridianGOTOE = -180;
      XEEPROM.write(getMountAddress(EE_dpmE), round((limits.minutesPastMeridianGOTOE * 15.0) / 60.0) + 128);
      replyShortTrue();
      break;
    case 'W':
      // :SXLW,sVV.V# set user defined Meridian West Limit
      limits.minutesPastMeridianGOTOW = (double)strtol(&command[5], NULL, 10);
      if (limits.minutesPastMeridianGOTOW > 180) limits.minutesPastMeridianGOTOW = 180;
      if (limits.minutesPastMeridianGOTOW < -180) limits.minutesPastMeridianGOTOW = -180;
      XEEPROM.write(getMountAddress(EE_dpmW), round((limits.minutesPastMeridianGOTOW * 15.0) / 60.0) + 128);
      replyShortTrue();
      break;
    case 'U':
      // :SXLU,VV# set user defined Under Pole Limit
      limits.underPoleLimitGOTO = (double)strtol(&command[5], NULL, 10) / 10;
      if (limits.underPoleLimitGOTO > 12) limits.underPoleLimitGOTO = 12;
      if (limits.underPoleLimitGOTO < 9) limits.underPoleLimitGOTO = 9;
      XEEPROM.write(getMountAddress(EE_dup), round(limits.underPoleLimitGOTO * 10.0));
      replyShortTrue();
      break;
    case 'H':
      // :GXLH,sVV# set user defined horizon Limit
      // NB: duplicate with :Sh#
      if ((atoi2(&command[5], &i)) && ((i >= -30) && (i <= 30)))
      {
        limits.minAlt = i;
        XEEPROM.write(getMountAddress(EE_minAlt), limits.minAlt + 128);
        replyShortTrue();
      }
      else
      {
        replyLongUnknown();
      }
      break;
    case 'O':
      // :SXLO,VV.VV# set user defined overhead Limit
      // NB: duplicate with :So#
      if ((atoi2(&command[5], &i)) && ((i >= 45) && (i <= 91)))
      {
        limits.maxAlt = i;
        XEEPROM.write(getMountAddress(EE_maxAlt), limits.maxAlt);
        replyShortTrue();
      }
      else
      {
        replyLongUnknown();
      }
    break;
    default:
      replyLongUnknown();
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
        replyLongUnknown();
      else
      {
        rtk.setClock(year(), month(), day(), h1, m1, s1, *localSite.longitude(), 0);
        replyShortTrue();
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
        replyLongUnknown();
      else
      {
        rtk.setClock(y, m, d, hour(), minute(), second(), *localSite.longitude(), 0);
        replyShortTrue();
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
      replyShortTrue();
      break;
    }
    default:
      replyLongUnknown();
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
          XEEPROM.writeInt(getMountAddress(EE_backlashAxis2), mount.backlashA2.inSeconds);
          mount.backlashA2.inSteps = (int)round(((double)mount.backlashA2.inSeconds * 3600.0) / (double)geoA2.stepsPerDegree);
          mount.backlashA2.movedSteps = 0;
          replyShortTrue();
        }
        else if (command[4] == 'R')
        {
          mount.backlashA1.inSeconds = i;
          XEEPROM.writeInt(getMountAddress(EE_backlashAxis1), mount.backlashA1.inSeconds);
          mount.backlashA1.inSteps = (int)round(((double)mount.backlashA1.inSeconds * 3600.0) / (double)geoA1.stepsPerDegree);
          mount.backlashA1.movedSteps = 0;
          replyShortTrue();
        }
        else replyLongUnknown();
      }
      else replyLongUnknown();
    }
    break;
    case 'G':
    {
      // :SXMGn,VVVV# Set Gear
      unsigned long i;
      bool ok = false;
      if ((command[4] == 'D' || command[4] == 'R')
        && strlen(&command[6]) > 1 && strlen(&command[6]) < 11)
      {
        i = strtoul(&command[6], NULL, 10);
        if (command[4] == 'D')
        {
          StopAxis2();
          motorA2.gear = i;
          XEEPROM.writeULong(getMountAddress(EE_motorA2gear), i);
          ok = true;
        }
        else
        {
          StopAxis1();
          motorA1.gear = i;
          XEEPROM.writeULong(getMountAddress(EE_motorA1gear), i);
          ok = true;
        }
      }
      if (ok)
      {
        updateRatios(true, true);
        replyShortTrue();
      }
      else
      {
        replyLongUnknown();
      }
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
          StopAxis2();
          motorA2.stepRot = (unsigned int)i;
          XEEPROM.writeInt(getMountAddress(EE_motorA2stepRot), i);
        }
        else
        {
          StopAxis1();
          motorA1.stepRot = (unsigned int)i;
          XEEPROM.writeInt(getMountAddress(EE_motorA1stepRot), i);
        }
        updateRatios(true, true);
        replyShortTrue();
      }
      else
      {
        replyLongUnknown();
      }
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
          StopAxis2();
          motorA2.micro = i;
          motorA2.setMicrostep(motorA2.micro);;
          XEEPROM.write(getMountAddress(EE_motorA2micro), motorA2.micro);
        }
        else
        {
          StopAxis1();
          motorA1.micro = i;
          motorA1.setMicrostep(motorA1.micro);
          XEEPROM.write(getMountAddress(EE_motorA1micro), motorA1.micro);
        }
        updateRatios(true, false);
        replyShortTrue();
      }
      else 
      {
        replyLongUnknown();
      }
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
          XEEPROM.write(getMountAddress(EE_motorA2silent), i);
        }
        else
        {
          XEEPROM.write(getMountAddress(EE_motorA1silent), i);
        }
        replyShortTrue();
      }
      else 
      {
        replyLongUnknown();
      }
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
          XEEPROM.write(getMountAddress(EE_motorA2reverse), motorA2.reverse);
        }
        else
        {
          motorA1.reverse = command[6] == '1' ? true : false;
          XEEPROM.write(getMountAddress(EE_motorA1reverse), motorA1.reverse);
        }
        replyShortTrue();
      }
      else
      {
        replyLongUnknown();
      }
    }
    break;
    case 'c':
    case 'C':
    {
      // :SXMCRn# Set Current
      unsigned int curr = (unsigned int)(strtol(&command[6], NULL, 10)/100)*100;
      bool ok = false;
      if (((curr >= 100) && (curr <= 2800)))
      {
        if (command[4] == 'D')
        {
          if (command[3] == 'C')
          {
            motorA2.highCurr = curr;
            XEEPROM.write(getMountAddress(EE_motorA2highCurr), motorA2.highCurr / 100);
            ok = true;
          }
          else
          {
            motorA2.lowCurr = curr;
            XEEPROM.write(getMountAddress(EE_motorA2lowCurr), motorA2.lowCurr / 100);
            motorA2.drvP->rms_current((unsigned int)motorA2.lowCurr);
            ok = true;
          }
        }
        else if (command[4] == 'R')
        {
          if (command[3] == 'C')
          {
            motorA1.highCurr = curr;
            XEEPROM.write(getMountAddress(EE_motorA1highCurr), motorA1.highCurr / 100);
            ok = true;
          }
          else
          {
            motorA1.lowCurr = curr;
            XEEPROM.write(getMountAddress(EE_motorA1lowCurr), motorA1.lowCurr / 100);
            motorA1.drvP->rms_current((unsigned int)motorA1.lowCurr);
            ok = true;
          }
        }
        if (ok)
        	replyShortTrue();
        else
        	replyLongUnknown();
      }
      else 
      {
        replyNothing();
      }
    }
    break;
    default:
      replyNothing();
      break;
    }
    break;
  case 'O':
    // :SXO-,VVVV Options
    switch (command[3])
    {
    case 'I':
      // :SXOI,V set Mount index
    {
      if ((atoi2(&command[5], &i)) && ((i >= 0) && (i < maxNumMounts)))
      {
        currentMount = i;
        XEEPROM.write(EE_currentMount, currentMount);
        replyShortTrue();
        reboot_unit = true;
      }
      else
        replyLongUnknown();
    }
    break;
    case 'A':
    case 'B':
    case 'C':
      // :SXON,NNNN set Mount Name
    {

      int i = 0;
      if (command[3] == 'A')
        i = currentMount;
      else if (command[3] == 'C')
        i = 1;
      if (strlen(&command[5]) < MountNameLen + 1)
      {
        memcpy(mountNames[i], &command[5], MountNameLen * sizeof(char));
        XEEPROM.writeString(getMountAddress(EE_mountName, i), mountNames[i], MountNameLen);
        replyShortTrue();
      }
      else
        replyLongUnknown();
      break;
    }
    break;
    }
    break;
  case 'K':
    {
      // :SXK,VVVV# External Clock frequency for TMC5160
      int val = strtol(&command[4], NULL, 10);
      if (val > 10000 && val < 18000)
      {
        mount.clk5160 = val;
        motorA1.mcP->setRatios(mount.clk5160);
        motorA2.mcP->setRatios(mount.clk5160);
        XEEPROM.writeLong(getMountAddress(EE_clk5160), (uint32_t)(mount.clk5160));
        replyShortTrue();
      }
      else
        replyLongUnknown();
    }
    break;
  default:
    replyLongUnknown();
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
      XEEPROM.write(getMountAddress(EE_mountType), i);
      HAL_reboot();
    }
    else replyLongUnknown();
    break;
  case 'a':
    //  :SasDD*MM#
    //         Set target object altitude to sDD:MM# or sDD:MM:SS# (based on precision setting)
    //         Native LX200
    //         Returns:
    //         0 if Object is within slew range, 1 otherwise
    if (dmsToDouble(&newTargetAlt, &command[2], true, highPrecision)) replyShortTrue();
    else replyLongUnknown();
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
      replyShortTrue();
    }
    else replyLongUnknown();
    break;
  case 'C':
    //  :SCMM/DD/YY#
    //          Change Local Date to MM/DD/YY
    //          Return: 0 on failure
    //                  1 on success
    int y, m, d;
    if (!dateToYYYYMMDD(&y, &m, &d, &command[2])) replyLongUnknown();
    else
    {
      rtk.setClock(y, m, d, hour(), minute(), second(), *localSite.longitude(), 0);
      replyShortTrue();
    }
    break;
  case 'e':
    //  :SesDDDDD#
    //          Set current sites elevation above see level
    //          Return: 0 on failure
    //                  1 on success
    if (atoi2(&command[2], &i))
    {
      if (localSite.setElev(i)) replyShortTrue();
      else replyLongUnknown();
    }
    else replyLongUnknown();
    break;
  case 'd':
    //  :SdsDD*MM#
    //          Set target object declination to sDD*MM or sDD*MM:SS depending on the current precision setting
    //          Return: 0 on failure
    //                  1 on success
    if (dmsToDouble(&newTargetDec, &command[2], true, highPrecision)) replyShortTrue();
    else replyLongUnknown();
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
      replyShortTrue();
    }
    else replyLongUnknown();
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
      replyShortTrue();
    }
    else replyLongUnknown();
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
      replyLongUnknown();
      return;
    }

    if ((atoi2(&command[2], &i)) && ((i >= -30) && (i <= 30)))
    {
      limits.minAlt = i;
      XEEPROM.write(getMountAddress(EE_minAlt), limits.minAlt + 128);
      replyShortTrue();
    }
    else replyLongUnknown();
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
      replyShortTrue();
    }
    else replyLongUnknown();
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
    i = command[1] - 'M';
    if (strlen(&command[2]))
      XEEPROM.writeString(EE_sites + i * SiteSize + EE_site_name, &command[2], SiteNameLen) ?
      replyShortTrue() : replyLongUnknown();
    else
      replyLongUnknown();
    break;
  case 'm':
    if ((command[2] != 0) && (strlen(&command[2]) < 2))
    {
      if (command[2] == 'N')
      {
        newTargetPierSide = PIER_NOTVALID;
        replyShortTrue();
      }
      else if (command[2] == 'E')
      {
        if (mount.mP->GetPierSide() == PIER_WEST)
        {
          newTargetPierSide = PIER_EAST;
          replyShortTrue();
        }
      }
      else if (command[2] == 'W')
      {
        if (mount.mP->GetPierSide() == PIER_EAST)
        {
          newTargetPierSide = PIER_WEST;
          replyShortTrue();
        }
      }
      else replyLongUnknown();
    }
    else replyLongUnknown();
    break;
  case 'n':
    if (strlen(&command[2]))
      localSite.setSiteName(&command[2]) ? replyShortTrue() : replyLongUnknown();
    else
      replyLongUnknown();
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
        XEEPROM.write(getMountAddress(EE_maxAlt), limits.maxAlt);
        replyShortTrue();
      }
      else replyLongUnknown();
    }
    else replyLongUnknown();
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
      replyShortTrue();
    }
    else replyLongUnknown();
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
      replyShortTrue();
    }
    else replyLongUnknown();
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
          stopTracking();
        }
        else
        {
          RequestedTrackingRateHA = (f / 60.0) / 1.00273790935;
          // todo apply rate
        }
        replyShortTrue();
      }
      else replyLongUnknown();
    }
    else replyLongUnknown();
    break;
  case 'U':
    // :SU# store current User defined Position
    mount.mP->getEqu(&f, &f1, localSite.cosLat(), localSite.sinLat(), false);
    XEEPROM.writeFloat(getMountAddress(EE_RA), (float)f);
    XEEPROM.writeFloat(getMountAddress(EE_DEC), (float)f1);
    replyShortTrue();
    break;
  case 'X':
  	Command_SX();
  	break;
  case 'z':
    //  :SzDDD*MM#
    //          Sets the target Object Azimuth
    //          Return: 0 on failure
    //                  1 on success
    if (dmsToDouble(&newTargetAzm, &command[2], false, highPrecision)) replyShortTrue();
    else replyLongUnknown();// "1" BUGS ELSEWHERE???
    break;
  default:
    replyLongUnknown();
    break;
  }
}
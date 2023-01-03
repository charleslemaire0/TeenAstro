#include "Command.h"
#include "ValueToString.h"
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
      replyOk();
    else
      replyFailed();
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
      replyOk();
    else
      replyFailed();
    break;
  }
  case 'R':
    // :SXRn,VVVV# Rates Settings
    switch (command[3])
    {
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
          enableGuideRate(i);
        replyOk();
      }
      else replyFailed();
      break;
    case 'A':
      // :SXRA,VVV# Set degree for acceleration
      DegreesForAcceleration = min(max(0.1 * (double)strtol(&command[5], NULL, 10), 0.1), 25.0);
      XEEPROM.update(EE_degAcc, (uint8_t)(DegreesForAcceleration * 10));
      SetAcceleration();
      replyOk();
      break;
    case 'D':
    {
      // :SXRD,V# define default rate
      int val = strtol(&command[5], NULL, 10);
      val = val > 4 || val < 0 ? 3 : val;
      XEEPROM.write(EE_DefaultRate, val);
      replyOk();
      break;
    }
    case 'X':
      // :SXRX,VVVV# Set Rate for max Rate
      XEEPROM.writeInt(EE_maxRate, (int)strtol(&command[5], NULL, 10));
      initMaxRate();
      replyOk();
      break;
    case 'r':
      // :SXRr,VVVVVVVVVV# Set Rate for RA 
      sideralMode = SIDM_TARGET;
      RequestedTrackingRateHA = 1. - (double)strtol(&command[5], NULL, 10) / 10000.0;
      computeTrackingRate(true);
      replyOk();
      break;
    case 'h':
      // :SXRh,VVVVVVVVVV# Set Rate for HA
      sideralMode = SIDM_TARGET;
      RequestedTrackingRateHA = (double)strtol(&command[5], NULL, 10) / 10000.0;
      computeTrackingRate(true);
      replyOk();
      break;
    case 'd':
      // :SXRd,VVVVVVVVVV# Set Rate for DEC
      sideralMode = SIDM_TARGET;
      RequestedTrackingRateDEC = (double)strtol(&command[5], NULL, 10) / 10000.0;
      computeTrackingRate(true);
      replyOk();
      break;
    case 'e':
      // :SXRe,VVVVVVVVVV# Store Rate for RA
      lval = strtol(&command[5], NULL, 10);
      storedTrakingRateRA = lval < -50000 || lval > 50000 ? 0 : lval;
      XEEPROM.writeLong(EE_RA_Drift, storedTrakingRateRA);
      replyOk();
      break;
    case 'f':
      // :SXRf,VVVVVVVVVV# Store Rate for DEC
      lval = strtol(&command[5], NULL, 10);
      storedTrakingRateDEC = lval < -50000 || lval > 50000 ? 0 : lval;
      XEEPROM.writeLong(EE_DEC_Drift, storedTrakingRateDEC);
      replyOk();
      break;
    default:
      replyFailed();
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
      replyOk();
      break;
    case 'B':
      // :SXLB,VVVV# set user defined maxAXIS1 (always positf)
      i = (int)strtol(&command[5], NULL, 10);
      XEEPROM.writeInt(EE_maxAxis1, i);
      initLimitMaxAxis1();
      replyOk();
      break;
    case 'C':
      // :SXLC,VVVV# set user defined minAXIS2 (always positf)
      i = (int)strtol(&command[5], NULL, 10);
      XEEPROM.writeInt(EE_minAxis2, i);
      initLimitMinAxis2();
      replyOk();
      break;
    case 'D':
      // :SXLD,VVVV# set user defined maxAXIS2 (always positf)
      i = (int)strtol(&command[5], NULL, 10);
      XEEPROM.writeInt(EE_maxAxis2, i);
      initLimitMaxAxis2();
      replyOk();
      break;
    case 'E':
      // :SXLE,sVV.V# set user defined Meridian East Limit
      minutesPastMeridianGOTOE = (double)strtol(&command[5], NULL, 10);
      if (minutesPastMeridianGOTOE > 180) minutesPastMeridianGOTOE = 180;
      if (minutesPastMeridianGOTOE < -180) minutesPastMeridianGOTOE = -180;
      XEEPROM.update(EE_dpmE, round((minutesPastMeridianGOTOE * 15.0) / 60.0) + 128);
      replyOk();
      break;
    case 'W':
      // :SXLW,sVV.V# set user defined Meridian West Limit
      minutesPastMeridianGOTOW = (double)strtol(&command[5], NULL, 10);
      if (minutesPastMeridianGOTOW > 180) minutesPastMeridianGOTOW = 180;
      if (minutesPastMeridianGOTOW < -180) minutesPastMeridianGOTOW = -180;
      XEEPROM.update(EE_dpmW, round((minutesPastMeridianGOTOW * 15.0) / 60.0) + 128);
      replyOk();
      break;
    case 'U':
      // :SXLU,VV# set user defined Under Pole Limit
      underPoleLimitGOTO = (double)strtol(&command[5], NULL, 10) / 10;
      if (underPoleLimitGOTO > 12) underPoleLimitGOTO = 12;
      if (underPoleLimitGOTO < 9) underPoleLimitGOTO = 9;
      XEEPROM.update(EE_dup, round(underPoleLimitGOTO * 10.0));
      replyOk();
      break;
    case 'H':
      // :GXLH,sVV# set user defined horizon Limit
      // NB: duplicate with :Sh#
      if ((atoi2(&command[5], &i)) && ((i >= -30) && (i <= 30)))
      {
        minAlt = i;
        XEEPROM.update(EE_minAlt, minAlt + 128);
        replyOk();
      }
      else
        replyFailed();
    case 'O':
      // :SXLO,VV.VV# set user defined overhead Limit
      // NB: duplicate with :So#
      if ((atoi2(&command[5], &i)) && ((i >= 45) && (i <= 91)))
      {
        maxAlt = i;
        XEEPROM.update(EE_maxAlt, maxAlt);
        replyOk();
      }
      else
        replyFailed();
    default:
      replyFailed();
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
        replyFailed();
      else
      {
        rtk.setClock(year(), month(), day(), h1, m1, s1, *localSite.longitude(), 0);
        replyOk();
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
        replyFailed();
      else
      {
        rtk.setClock(y, m, d, hour(), minute(), second(), *localSite.longitude(), 0);
        replyOk();
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
      replyOk();
      break;
    }
    default:
      replyFailed();
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
          backlashA2.inSeconds = i;
          XEEPROM.writeInt(EE_backlashAxis2, backlashA2.inSeconds);
          backlashA2.inSteps = (int)round((double)backlashA2.inSeconds / geoA2.stepsPerArcSecond);
          backlashA2.movedSteps = 0;
          replyOk();
        }
        else if (command[4] == 'R')
        {
          backlashA1.inSeconds = i;
          XEEPROM.writeInt(EE_backlashAxis1, backlashA1.inSeconds);
          backlashA1.inSteps = (int)round((double)backlashA1.inSeconds / geoA1.stepsPerArcSecond);
          backlashA1.movedSteps = 0;
          replyOk();
        }
        else replyFailed();
      }
      else replyFailed();
    }
    break;
    case 'G':
    {
      // :SXMGn,VVVV# Set Gear
      int i;
      bool ok = false;
      if ((command[4] == 'D' || command[4] == 'R')
        && strlen(&command[6]) > 1 && strlen(&command[6]) < 11
        && atoi2(&command[6], &i))
      {
        if (command[4] == 'D')
        {
          if (!motorA2.isGearFix)
          {
            double fact = (double)i / motorA2.gear;
            cli();
            staA2.pos = fact * staA2.pos;
            sei();
            StopAxis2();
            motorA2.gear = (unsigned int)i;
            XEEPROM.writeInt(EE_motorA2gear, i);
            ok = true;
          }
        }
        else
        {
          if (!motorA1.isGearFix)
          {
            double fact = (double)i / motorA1.gear;
            cli();
            staA1.pos = fact * staA1.pos;
            sei();
            StopAxis1();
            motorA1.gear = (unsigned int)i;
            XEEPROM.writeInt(EE_motorA1gear, i);
            ok = true;
          }
        }
      }
      if (ok)
      {
        updateRatios(true, true);
        replyOk();
      }
      else
      {
        replyFailed();
      }
    }
    break;
    case 'S':
    {
      // :SXMBn,VVVV# Set Step per Rotation
      int i;
      bool ok = false;
      if ((command[4] == 'D' || command[4] == 'R')
        && (strlen(&command[6]) > 1) && (strlen(&command[6]) < 11)
        && atoi2((char*)&command[6], &i))
      {
        if (command[4] == 'D')
        {
          if (!motorA2.isStepRotFix)
          {
            double fact = (double)i / motorA2.stepRot;
            cli();
            staA2.pos = fact * staA2.pos;
            sei();
            StopAxis2();
            motorA2.stepRot = (unsigned int)i;
            XEEPROM.writeInt(EE_motorA2stepRot, i);
            ok = true;
          }
        }
        else
        {
          if (!motorA1.isStepRotFix)
          {
            double fact = (double)i / motorA1.stepRot;
            cli();
            staA1.pos = fact * staA1.pos;
            sei();
            StopAxis1();
            motorA1.stepRot = (unsigned int)i;
            XEEPROM.writeInt(EE_motorA1stepRot, i);
            ok = true;
          }
        }
       
      }
      if (ok)
      {
        updateRatios(true, true);
        replyOk();
      }
      else
      {
        replyFailed();
      }
    }
    break;
    case 'M':
    {
      // :SXMMn,V# Set Microstep
      // for example :GRXMMR3# for 1/8 microstep on the first axis 
      int i;
      bool ok = false;
      if ((command[4] == 'D' || command[4] == 'R')
        && strlen(&command[6]) == 1
        && atoi2(&command[6], &i)
        && ((i >= 1) && (i < 9)))
      {
        if (command[4] == 'D')
        { 
          if (!motorA2.isMicroFix)
          {
            double fact = pow(2., i - motorA2.micro);
            cli();
            staA2.pos = fact * staA2.pos;
            sei();
            StopAxis2();
            motorA2.micro = i;
            motorA2.driver.setMicrostep(motorA2.micro);;
            XEEPROM.write(EE_motorA2micro, motorA2.micro);
            ok = true;
          }
        }
        else
        {
          if (!motorA1.isMicroFix)
          {
            double fact = pow(2., i - motorA1.micro);
            cli();
            staA1.pos = fact * staA1.pos;
            sei();
            StopAxis1();
            motorA1.micro = i;
            motorA1.driver.setMicrostep(motorA1.micro);
            XEEPROM.write(EE_motorA1micro, motorA1.micro);
            ok = true;
          }
        }
      }
      if (ok)
      {
        updateRatios(true, false);
        replyOk();
      }
      else
      {
        replyFailed();
      }
    }
    break;
    case 'm':
    {
      // :SXMmn,V# Set coolstep Stepper Mode
     // for example :GRXMmR1# for cool step on the first axis 
      int i;
      bool ok = false;
      if ((command[4] == 'D' || command[4] == 'R')
        && strlen(&command[6]) == 1
        && atoi2(&command[6], &i)
        && ((i >= 0) && (i < 2)))
      {
        if (command[4] == 'D')
        {
          if (!motorA2.isSilentFix)
          {
          //motorA2.driver.setmode(i);
            XEEPROM.write(EE_motorA2silent, i);
            ok = true;
          }

        }
        else
        {
          if (!motorA1.isSilentFix)
          {
          //motorA1.driver.setmode(i);
            XEEPROM.write(EE_motorA1silent, i);
            ok = true;
          }
        }
      }
      ok ? replyOk() : replyFailed();
    }
    break;
    case 'R':
    {
      // :SXMRn,V# Set Reverse rotation
      bool ok = false;
      if ((command[4] == 'D' || command[4] == 'R')
        && strlen(&command[6]) == 1
        && (command[6] == '0' || command[6] == '1'))
      {
        if (command[4] == 'D')
        {
          if (!motorA2.isReverseFix)
          {
            motorA2.reverse = command[6] == '1' ? true : false;
            XEEPROM.write(EE_motorA2reverse, motorA2.reverse);
            ok = true;
          }
        }
        else
        {
          if (!motorA1.isReverseFix)
          {
            motorA1.reverse = command[6] == '1' ? true : false;
            XEEPROM.write(EE_motorA1reverse, motorA1.reverse);
            ok = true;
          }
        }
      }
      ok ? replyOk() : replyFailed();
    }
    break;
    case 'c':
    case 'C':
    {
      // :SXMRn# Set Current
      unsigned int curr = (unsigned int)(strtol(&command[6], NULL, 10) / 100) * 100;
      bool ok = false;
      if (curr >= 100 )
      {
        if (command[4] == 'D' && curr <= motorA2.driver.getMaxCurrent())
        {
          if (command[3] == 'C')
          {
            if (!motorA2.isHighCurrfix)
            {
              motorA2.highCurr = curr;
              XEEPROM.write(EE_motorA2highCurr, motorA2.highCurr / 100);
              ok = true;
            }
          }
          else
          {
            if (!motorA2.isLowCurrfix)
            {
              motorA2.lowCurr = curr;
              XEEPROM.write(EE_motorA2lowCurr, motorA2.lowCurr / 100);
              motorA2.driver.setCurrent((unsigned int)motorA2.lowCurr);
              ok = true;
            }
          }
        }
        else if (command[4] == 'R' && curr <= motorA1.driver.getMaxCurrent())
        {
          if (command[3] == 'C')
          {
            if (!motorA1.isHighCurrfix)
            {
              motorA1.highCurr = curr;
              XEEPROM.write(EE_motorA1highCurr, motorA1.highCurr / 100);
              ok = true;
            }
          }
          else
          {
            if (!motorA1.isLowCurrfix)
            {
              motorA1.lowCurr = curr;
              XEEPROM.write(EE_motorA1lowCurr, motorA1.lowCurr / 100);
              motorA1.driver.setCurrent((unsigned int)motorA1.lowCurr);
              ok = true;
            }
          }
        }
        ok ? replyOk() : replyFailed();
      }
      else replyFailed();
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

        if (command[4] == 'D')
        {
          motorA2.driver.setSG(i);
        }
        else
        {
          motorA1.driver.setSG(i);
        }
        replyOk();
      }
      else replyFailed();
    }
    break;
    default:
      replyFailed();
      break;
    }
    break;
  case 'O':
    // :SXO-,VVVV Options
  default:
    replyFailed();
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
    //  :S!n#
    //         Set The Mount Type
    //         Return Nothing As it force a reboot
  case '!':
    i = (int)(command[2] - '0');
    if (i > 0 && i < 5 && !isMountTypeFix)
    {
      XEEPROM.write(EE_mountType, i);
      if (!atHome)
      {
        replyFailed();
      }
      else
      {
        Serial.end();
        Serial1.end();
        GNSS_Serial.end();
        Focus_Serial.end();
        delay(1000);
#ifdef ARDUINO_TEENSY40 // In fact this code is suitable for Teensy 3.2 also
#define CPU_RESTART_ADDR (uint32_t *)0xE000ED0C
#define CPU_RESTART_VAL 0x5FA0004
#define CPU_RESTART (*CPU_RESTART_ADDR = CPU_RESTART_VAL); 
        CPU_RESTART;
#else
        _reboot_Teensyduino_();
#endif
      }
    }
    else replyFailed();
    break;
  case 'a':
    //  :SasDD*MM#
    //         Set target object altitude to sDD:MM# or sDD:MM:SS# (based on precision setting)
    //         Native LX200
    //         Returns:
    //         0 if Object is within slew range, 1 otherwise
    if (dmsToDouble(&newTargetAlt, &command[2], true, highPrecision)) replyOk();
    else replyFailed();
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
      replyOk();
    }
    else replyFailed();
    break;
  case 'C':
    //  :SCMM/DD/YY#
    //          Change Local Date to MM/DD/YY
    //          Return: 0 on failure
    //                  1 on success
    int y, m, d;
    if (!dateToYYYYMMDD(&y, &m, &d, &command[2])) replyFailed();
    else
    {
      rtk.setClock(y, m, d, hour(), minute(), second(), *localSite.longitude(), 0);
      replyOk();
    }
    break;
  case 'e':
    //  :SesDDDDD#
    //          Set current sites elevation above see level
    //          Return: 0 on failure
    //                  1 on success
    if (atoi2(&command[2], &i))
    {
      if (localSite.setElev(i)) replyOk();
      else replyFailed();
    }
    else replyFailed();
    break;
  case 'd':
    //  :SdsDD*MM#
    //          Set target object declination to sDD*MM or sDD*MM:SS depending on the current precision setting
    //          Return: 0 on failure
    //                  1 on success
    if (dmsToDouble(&newTargetDec, &command[2], true, highPrecision)) replyOk();
    else replyFailed();
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
      replyOk();
    }
    else replyFailed();
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
      replyOk();
    }
    else replyFailed();
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
      replyFailed();
      return;
    }

    if ((atoi2(&command[2], &i)) && ((i >= -30) && (i <= 30)))
    {
      minAlt = i;
      XEEPROM.update(EE_minAlt, minAlt + 128);
      replyOk();
    }
    else replyFailed();
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
      replyOk();
    }
    else replyFailed();
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
      XEEPROM.writeString(EE_sites + i * SiteSize + EE_site_name, &command[2], siteNameLen) ?
      replyOk() : replyFailed();
    else
      replyFailed();
    break;
  case 'm':
    if ((command[2] != 0) && (strlen(&command[2]) < 2))
    {
      if (command[2] == 'N')
      {
        newTargetPierSide = PIER_NOTVALID;
        replyOk();
      }
      else if (command[2] == 'E')
      {
        newTargetPierSide = PIER_EAST;
        replyOk();
      }
      else if (command[2] == 'W')
      {
        newTargetPierSide = PIER_WEST;
        replyOk();
      }
      else replyFailed();
    }
    else replyFailed();
    break;
  case 'n':
    if (strlen(&command[2]))
      localSite.setSiteName(&command[2]) ? replyOk() : replyFailed();
    else
      replyFailed();
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
        maxAlt = i;
        XEEPROM.update(EE_maxAlt, maxAlt);
        replyOk();
      }
      else replyFailed();
    }
    else replyFailed();
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
      replyOk();
    }
    else replyFailed();
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
      initCelestialPole();
      initHome();
      initTransformation(true);
      syncAtHome();
      replyOk();
    }
    else replyFailed();
    highPrecision = i;
    break;
  case 'T':
    //  :STdd.ddddd#
    //          Return: 0 on failure
    //                  1 on success
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
          RequestedTrackingRateHA = (f / 60.0) / 1.00273790935;
          // todo apply rate
        }
        replyOk();
      }
      else replyFailed();
    }
    else replyFailed();
    break;
  case 'U':
    // :SU# store current User defined Position
    getEqu(&f, &f1, localSite.cosLat(), localSite.sinLat(), false);
    XEEPROM.writeFloat(EE_RA, (float)f);
    XEEPROM.writeFloat(EE_DEC, (float)f1);
    replyOk();
    break;
  case 'X':
    Command_SX();
    break;
  case 'z':
    //  :SzDDD*MM#
    //          Sets the target Object Azimuth
    //          Return: 0 on failure
    //                  1 on success
    if (dmsToDouble(&newTargetAzm, &command[2], false, highPrecision)) replyOk();
    else replyFailed();// "1" BUGS ELSEWHERE???
    break;
  default:
    replyFailed();
    break;
  }
}
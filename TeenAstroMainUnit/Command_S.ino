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
    replyValueSetShort(ok);
    break;
  }
  case 'E':
    // :SXEnn# set encoder commands
  {

    switch (command[3])
    {
    case 'O':
      // :SXEO#  set encoder Sync Option
    {
      unsigned int i;
      if (atoui2(&command[5], &i) && i <= (unsigned int)(EncoderSync::ES_ALWAYS))
      {
        ok = true;
        EncodeSyncMode = static_cast<EncoderSync>(i);
        XEEPROM.write(getMountAddress(EE_encoderSync), EncodeSyncMode);
      }
      replyValueSetShort(ok);
    }
    break;
    case 'P':
    {
      // :SXEPn,VVVV# Set pulse per 100deg max
      bool ok = false;
      if ((command[4] == 'D' || command[4] == 'R')
        && (strlen(&command[6]) > 0) && (strlen(&command[6]) < 7)
        )
      {
        char* pEnd;
        unsigned long p = strtoul(&command[6], &pEnd, 10);
        if (p > 0 && p <= 360000)
        {
          if (command[4] == 'D')
          {
            if (!encoderA2.isPulsePerDegreeFix)
            {
              encoderA2.pulsePerDegree = 0.01 * p;
              XEEPROM.writeLong(getMountAddress(EE_encoderA2pulsePerDegree), p);
              ok = true;
            }
          }
          else
          {
            if (!encoderA1.isPulsePerDegreeFix)
            {
              encoderA1.pulsePerDegree = 0.01 * p;
              XEEPROM.writeLong(getMountAddress(EE_encoderA1pulsePerDegree), p);
              ok = true;
            }
          }
        }

      }
      replyValueSetShort(ok);
    }
    break;
    case 'r':
    {
      // :SXErn,V# Set Encoder Reverse rotation
      bool ok = false;
      if ((command[4] == 'D' || command[4] == 'R')
        && strlen(&command[6]) == 1
        && (command[6] == '0' || command[6] == '1'))
      {
        if (command[4] == 'D')
        {
          if (!encoderA2.isReverseFix)
          {
            encoderA2.reverse = command[6] == '1' ? true : false;
            XEEPROM.write(getMountAddress(EE_encoderA2reverse), encoderA2.reverse);
            ok = true;
          }
        }
        else
        {
          if (!encoderA1.isReverseFix)
          {
            encoderA1.reverse = command[6] == '1' ? true : false;
            XEEPROM.write(getMountAddress(EE_encoderA1reverse), encoderA1.reverse);
            ok = true;
          }
        }
      }
      replyValueSetShort(ok);
    }
    break;
    default:
      replyNothing();
      break;
    }
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
    replyValueSetShort(ok);
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
    {
      bool ok = GuidingState == GuidingOFF;
      if (ok)
      {
        i = command[3] - '0';
        int val = strtol(&command[5], NULL, 10);
        val = val > 0 && val < 256 ? val : pow(4, i);
        XEEPROM.write(getMountAddress(EE_Rate0 + i), val);
        if (i == 0)
          guideRates[0] = (double)val / 100.;
        else
          guideRates[i] = val;
        if (activeGuideRate == i)
          enableGuideRate(i);
      }
      replyValueSetShort(ok);
    }
    break;
    case 'A':
      // :SXRA,VVV# Set degree for acceleration
      DegreesForAcceleration = min(max(0.1 * (double)strtol(&command[5], NULL, 10), 0.1), 25.0);
      XEEPROM.update(getMountAddress(EE_degAcc), (uint8_t)(DegreesForAcceleration * 10));
      SetAcceleration();
      replyValueSetShort(true);
      break;
    case 'D':
    {
      // :SXRD,V# define default rate
      int val = strtol(&command[5], NULL, 10);
      val = val > 4 || val < 0 ? 3 : val;
      XEEPROM.write(getMountAddress(EE_DefaultRate), val);
      replyValueSetShort(true);
      break;
    }
    case 'X':
      // :SXRX,VVVV# Set Rate for max Rate
      XEEPROM.writeUShort(getMountAddress(EE_maxRate), (int)strtol(&command[5], NULL, 10));
      initMaxRate();
      replyValueSetShort(true);
      break;
    case 'r':
      // :SXRr,VVVVVVVVVV# Set Rate for RA 
      sideralMode = SIDM_TARGET;
      RequestedTrackingRateHA = (double)(10000l - strtol(&command[5], NULL, 10)) / 10000.;
      computeTrackingRate(true);
      replyValueSetShort(true);
      break;
    case 'h':
      // :SXRh,VVVVVVVVVV# Set Rate for HA
      sideralMode = SIDM_TARGET;
      RequestedTrackingRateHA = (double)strtol(&command[5], NULL, 10) / 10000.0;
      computeTrackingRate(true);
      replyValueSetShort(true);
      break;
    case 'd':
      // :SXRd,VVVVVVVVVV# Set Rate for DEC
      sideralMode = SIDM_TARGET;
      RequestedTrackingRateDEC = (double)strtol(&command[5], NULL, 10) / 10000.0;
      computeTrackingRate(true);
      replyValueSetShort(true);
      break;
    case 'e':
      // :SXRe,VVVVVVVVVV# Store Rate for RA
      lval = strtol(&command[5], NULL, 10);
      storedTrakingRateRA = lval < -50000 || lval > 50000 ? 0 : lval;
      XEEPROM.writeLong(getMountAddress(EE_RA_Drift), storedTrakingRateRA);
      replyValueSetShort(true);
      break;
    case 'f':
      // :SXRf,VVVVVVVVVV# Store Rate for DEC
      lval = strtol(&command[5], NULL, 10);
      storedTrakingRateDEC = lval < -50000 || lval > 50000 ? 0 : lval;
      XEEPROM.writeLong(getMountAddress(EE_DEC_Drift), storedTrakingRateDEC);
      replyValueSetShort(true);
      break;
    default:
      replyNothing();
      break;
    }
    break;
  case 'L':
    // user defined limits
    switch (command[3])
    {
    case 'R':
      // :SXLR# reset user defined axis limit
      reset_EE_Limit();
      break;
    case 'A':
      // :SXLA,VVVV# set user defined minAXIS1
      i = (int)strtol(&command[5], NULL, 10);
      if (i >= 10 * geoA1.LimMinAxis && i < XEEPROM.readShort(getMountAddress(EE_maxAxis1)))
      {
        XEEPROM.writeShort(getMountAddress(EE_minAxis1), i);
        initLimitMinAxis1();
        ok = true;
      }
      replyValueSetShort(ok);
      break;
    case 'B':
      // :SXLB,VVVV# set user defined maxAXIS1
      i = (int)strtol(&command[5], NULL, 10);
      if (i <= 10 * geoA1.LimMaxAxis && i > XEEPROM.readShort(getMountAddress(EE_minAxis1)))
      {
        XEEPROM.writeShort(getMountAddress(EE_maxAxis1), i);
        initLimitMaxAxis1();
        ok = true;
      }
      replyValueSetShort(ok);
      break;
    case 'C':
      // :SXLC,VVVV# set user defined minAXIS2
      i = (int)strtol(&command[5], NULL, 10);
      if (i >= 10 * geoA2.LimMinAxis && i < XEEPROM.readShort(getMountAddress(EE_maxAxis2)))
      {
        XEEPROM.writeShort(getMountAddress(EE_minAxis2), i);
        initLimitMinAxis2();
        ok = true;
      }
      replyValueSetShort(ok);
      break;
    case 'D':
      // :SXLD,VVVV# set user defined maxAXIS2
      i = (int)strtol(&command[5], NULL, 10);
      if (i <= 10 * geoA2.LimMaxAxis && i > XEEPROM.readShort(getMountAddress(EE_minAxis2)))
      {
        XEEPROM.writeShort(getMountAddress(EE_maxAxis2), i);
        initLimitMaxAxis2();
        ok = true;
      }
      replyValueSetShort(ok);
      break;
    case 'E':
      // :SXLE,sVV.V# set user defined Meridian East Limit
      minutesPastMeridianGOTOE = (double)strtol(&command[5], NULL, 10);
      if (minutesPastMeridianGOTOE > 180) minutesPastMeridianGOTOE = 180;
      if (minutesPastMeridianGOTOE < -180) minutesPastMeridianGOTOE = -180;
      XEEPROM.update(getMountAddress(EE_dpmE), round((minutesPastMeridianGOTOE * 15.0) / 60.0) + 128);
      replyValueSetShort(true);
      break;
    case 'W':
      // :SXLW,sVV.V# set user defined Meridian West Limit
      minutesPastMeridianGOTOW = (double)strtol(&command[5], NULL, 10);
      if (minutesPastMeridianGOTOW > 180) minutesPastMeridianGOTOW = 180;
      if (minutesPastMeridianGOTOW < -180) minutesPastMeridianGOTOW = -180;
      XEEPROM.update(getMountAddress(EE_dpmW), round((minutesPastMeridianGOTOW * 15.0) / 60.0) + 128);
      replyValueSetShort(true);
      break;
    case 'U':
      // :SXLU,VV# set user defined Under Pole Limit
      underPoleLimitGOTO = (double)strtol(&command[5], NULL, 10) / 10;
      if (underPoleLimitGOTO > 12) underPoleLimitGOTO = 12;
      if (underPoleLimitGOTO < 9) underPoleLimitGOTO = 9;
      XEEPROM.update(getMountAddress(EE_dup), round(underPoleLimitGOTO * 10.0));
      replyValueSetShort(true);
      break;
    case 'H':
      // :SXLH,sVV# set user defined horizon Limit
      // NB: duplicate with :Sh#
    {
      bool ok = (atoi2(&command[5], &i)) && ((i >= -30) && (i <= 30));
      if (ok)
      {
        minAlt = i;
        XEEPROM.update(getMountAddress(EE_minAlt), minAlt + 128);
      }
      replyValueSetShort(ok);
    }
    break;
    case 'O':
      // :SXLO,VV.VV# set user defined overhead Limit
      // NB: duplicate with :So#
    {
      bool ok = (atoi2(&command[5], &i)) && ((i >= 45) && (i <= 91));
      if (ok)
      {
        maxAlt = i;
        XEEPROM.update(getMountAddress(EE_maxAlt), maxAlt);
      }
      replyValueSetShort(ok);
    }
    break;
    case 'S':
      // :SXLS,sVV# set user defined distance from Pole to keep tracking on for 6 hours after transit, in degrees
    {
      bool ok = (atoi2(&command[5], &i)) && ((i >= 0) && (i <= 181));
      if (ok)
      {
        distanceFromPoleToKeepTrackingOn = i;
        XEEPROM.update(getMountAddress(EE_dpmDistanceFromPole), distanceFromPoleToKeepTrackingOn);
      }
      replyValueSetShort(ok);
    }
    break;
    default:
      replyNothing();
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
    {
      int h1, m1, m2, s1;
      bool ok = hmsToHms(&h1, &m1, &m2, &s1, &command[4], true);
      if (ok)
      {
        rtk.setClock(year(), month(), day(), h1, m1, s1, *localSite.longitude(), 0);
      }
      replyValueSetShort(ok);
    }
    break;
    case '1':
      //  :SXT1MM/DD/YY#
      //          Change Local Date to MM/DD/YY
      //          Return: 0 on failure
      //                  1 on success
    {
      int y, m, d;
      bool ok = dateToYYYYMMDD(&y, &m, &d, &command[4]);
      if (ok)
      {
        rtk.setClock(y, m, d, hour(), minute(), second(), *localSite.longitude(), 0);
      }
      replyValueSetShort(ok);
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
      replyValueSetShort(true);
      break;
    }
    default:
      replyNothing();
      break;
    }
    break;
  case 'M':
    // :SXMnn# Motor Settings
    switch (command[3])
    {
    case 'B':
    {
      // :SXMBn,VVVV# Set Backlash
      int i;
      bool ok = !TelescopeBusy();
      ok &= atoi2((char*)&command[6], &i);
      ok &= ((i >= 0) && (i <= 999));
      if (ok)
      {
        ok = false;
        if (command[4] == 'D')
        {
          motorA2.backlashAmount = i;
          XEEPROM.writeUShort(getMountAddress(EE_motorA2backlashAmount), motorA2.backlashAmount);
          staA2.setBacklash_inSteps(motorA2.backlashAmount, geoA2.stepsPerArcSecond);
          ok = true;
        }
        else if (command[4] == 'R')
        {
          motorA1.backlashAmount = i;
          XEEPROM.writeUShort(getMountAddress(EE_motorA1backlashAmount), motorA1.backlashAmount);
          staA1.setBacklash_inSteps(motorA1.backlashAmount, geoA1.stepsPerArcSecond);
          ok = true;
        }
      }
      replyValueSetShort(ok);
    }
    break;
    case 'b':
    {
      // :SXMbn,VVVV# Set BacklashRate
      int i;
      bool ok = !TelescopeBusy();
      ok &= atoi2((char*)&command[6], &i);
      ok &= ((i >= 16) && (i <= 64));
      if (ok)
      {
        ok = false;
        if (command[4] == 'D')
        {
          motorA2.backlashRate = i;
          XEEPROM.write(getMountAddress(EE_motorA2backlashRate), motorA2.backlashRate);
          staA2.SetBacklash_interval_Step(motorA2.backlashRate);
          ok = true;
        }
        else if (command[4] == 'R')
        {
          motorA1.backlashRate = i;
          XEEPROM.write(getMountAddress(EE_motorA1backlashRate), motorA1.backlashRate);
          staA1.SetBacklash_interval_Step(motorA1.backlashRate);
          ok = true;
        }
      }
      replyValueSetShort(ok);
    }
    break;
    case 'G':
    {
      // :SXMGn,VVVV# Set Gear
      unsigned long i;
      bool ok = !TelescopeBusy();
      ok &= (command[4] == 'D' || command[4] == 'R');
      ok &= strlen(&command[6]) > 1 && strlen(&command[6]) < 11;
      if (ok)
      {
        ok = false;
        i = strtoul(&command[6], NULL, 10);
        if (command[4] == 'D')
        {
          if (!motorA2.isGearFix)
          {
            double fact = (double)(i) / motorA2.gear;
            cli();
            staA2.pos = fact * staA2.pos;
            sei();
            motorA2.gear = i;
            XEEPROM.writeULong(getMountAddress(EE_motorA2gear), i);
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
            motorA1.gear = i;
            XEEPROM.writeULong(getMountAddress(EE_motorA1gear), i);
            ok = true;
          }
        }
      }
      if (ok)
      {
        updateRatios(true, true);
      }
      replyValueSetShort(ok);
    }
    break;
    case 'S':
    {
      // :SXMBn,VVVV# Set Step per Rotation
      int i;
      bool ok = !TelescopeBusy();
      ok &= (command[4] == 'D' || command[4] == 'R');
      ok &= (strlen(&command[6]) > 1) && (strlen(&command[6]) < 11);
      ok &= atoi2((char*)&command[6], &i);
      if (ok)
      {
        ok = false;
        if (command[4] == 'D')
        {
          if (!motorA2.isStepRotFix)
          {
            double fact = (double)i / motorA2.stepRot;
            cli();
            staA2.pos = fact * staA2.pos;
            sei();
            motorA2.stepRot = (unsigned int)i;
            XEEPROM.writeUShort(getMountAddress(EE_motorA2stepRot), i);
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
            motorA1.stepRot = (unsigned int)i;
            XEEPROM.writeUShort(getMountAddress(EE_motorA1stepRot), i);
            ok = true;
          }
        }
      }
      if (ok)
      {
        updateRatios(true, true);
      }
      replyValueSetShort(ok);
    }
    break;
    case 'M':
    {
      // :SXMMn,V# Set Microstep
      // for example :GRXMMR3# for 1/8 microstep on the first axis 
      int i;
      bool ok = !TelescopeBusy();
      ok &= (command[4] == 'D' || command[4] == 'R');
      ok &= strlen(&command[6]) == 1;
      ok &= atoi2(&command[6], &i);
      ok &= (i >= 1 && i < 9);
      if (ok)
      {
        ok = false;
        if (command[4] == 'D')
        {
          if (!motorA2.isMicroFix)
          {
            double fact = pow(2., i - motorA2.micro);
            cli();
            staA2.pos = fact * staA2.pos;
            sei();
            motorA2.micro = i;
            motorA2.driver.setMicrostep(motorA2.micro);;
            XEEPROM.write(getMountAddress(EE_motorA2micro), motorA2.micro);
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
            motorA1.micro = i;
            motorA1.driver.setMicrostep(motorA1.micro);
            XEEPROM.write(getMountAddress(EE_motorA1micro), motorA1.micro);
            ok = true;
          }
        }
      }
      if (ok)
      {
        updateRatios(true, false);
      }
      replyValueSetShort(ok);
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
            XEEPROM.write(getMountAddress(EE_motorA2silent), i);
            ok = true;
          }
        }
        else
        {
          if (!motorA1.isSilentFix)
          {
            //motorA1.driver.setmode(i);
            XEEPROM.write(getMountAddress(EE_motorA1silent), i);
            ok = true;
          }
        }
      }
      replyValueSetShort(ok);
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
            XEEPROM.write(getMountAddress(EE_motorA2reverse), motorA2.reverse);
            ok = true;
          }
        }
        else
        {
          if (!motorA1.isReverseFix)
          {
            motorA1.reverse = command[6] == '1' ? true : false;
            XEEPROM.write(getMountAddress(EE_motorA1reverse), motorA1.reverse);
            ok = true;
          }
        }
      }
      replyValueSetShort(ok);
    }
    break;
    case 'c':
    case 'C':
    {
      // :SXMRn# Set Current
      unsigned int curr = (unsigned int)(strtol(&command[6], NULL, 10) / 100) * 100;
      bool ok = false;
      if (curr >= 100)
      {
        if (command[4] == 'D' && curr <= motorA2.driver.getMaxCurrent())
        {
          if (command[3] == 'C')
          {
            if (!motorA2.isHighCurrfix)
            {
              motorA2.highCurr = curr;
              XEEPROM.write(getMountAddress(EE_motorA2highCurr), motorA2.highCurr / 100);
              ok = true;
            }
          }
          else
          {
            if (!motorA2.isLowCurrfix)
            {
              motorA2.lowCurr = curr;
              XEEPROM.write(getMountAddress(EE_motorA2lowCurr), motorA2.lowCurr / 100);
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
              XEEPROM.write(getMountAddress(EE_motorA1highCurr), motorA1.highCurr / 100);
              ok = true;
            }
          }
          else
          {
            if (!motorA1.isLowCurrfix)
            {
              motorA1.lowCurr = curr;
              XEEPROM.write(getMountAddress(EE_motorA1lowCurr), motorA1.lowCurr / 100);
              motorA1.driver.setCurrent((unsigned int)motorA1.lowCurr);
              ok = true;
            }
          }
        }
        replyValueSetShort(ok);
      }
      else replyNothing();
    }
    break;
    case 'F':
    {
      // :SXMRn# Set Stall guard
      int i;
      bool ok = (command[4] == 'D' || command[4] == 'R')
        && (strlen(&command[6]) > 1) && (strlen(&command[6]) < 5)
        && atoi2((char*)&command[6], &i)
        && ((i >= 0) && (i <= 127));
      if (ok)
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
      }
      replyValueSetShort(ok);
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
      bool ok = (atoi2(&command[5], &i)) && ((i >= 0) && (i < maxNumMount));
      if (ok)
      {
        midx = i;
        XEEPROM.write(EE_currentMount, midx);
        reboot_unit = true;
      }
      replyValueSetShort(ok);
    }
    break;
    case 'A':
    case 'B':
    case 'C':
      // :SXON,NNNN set Mount Name
    {
      int i = 0;
      if (command[3] == 'A')
        i = midx;
      else if (command[3] == 'C')
        i = 1;
      bool ok = strlen(&command[5]) < MountNameLen + 1;
      if (ok)
      {
        memcpy(mountName[i], &command[5], MountNameLen * sizeof(char));
        XEEPROM.writeString(getMountAddress(EE_mountName, i), mountName[i], MountNameLen);
      }
      replyValueSetShort(ok);
      break;
    }
    break;
    }
    break;
  default:
    replyNothing();
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
  {
    i = (int)(command[2] - '0');
    bool ok = i > 0 && i < 5 && !isMountTypeFix;
    if (ok)
    {
      force_reset_EE_Limit();
      XEEPROM.write(getMountAddress(EE_mountType), i);
      reboot_unit = true;
    }
    replyValueSetShort(ok);
  }
  break;
  case 'a':
    //  :SasDD*MM#
    //         Set target object altitude to sDD:MM# or sDD:MM:SS# (based on precision setting)
    //         Native LX200
    //         Returns:
    //         0 if Object is within slew range, 1 otherwise
  {
    bool ok = dmsToDouble(&newTargetAlt, &command[2], true, highPrecision);
    replyValueSetShort(ok);
  }
  break;
  case 'B':
    //  :SBn#  Set Baud Rate n for Serial-0, where n is an ASCII digit (1..9) with the following interpertation
        //         0=115.2K, 1=56.7K, 2=38.4K, 3=28.8K, 4=19.2K, 5=14.4K, 6=9600, 7=4800, 8=2400, 9=1200
        //         Returns: "1" At the current baud rate and then changes to the new rate for further communication
  {
    i = (int)(command[2] - '0');
    bool ok = (i >= 0) && (i < 10);
    if (ok)
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
    }
    replyValueSetShort(ok);
  }
  break;
  case 'C':
    //  :SCMM/DD/YY#
    //          Change Local Date to MM/DD/YY
    //          Return: 0 on failure
    //                  1 on success
  {
    int y, m, d;
    bool ok = dateToYYYYMMDD(&y, &m, &d, &command[2]);
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
    //          Return: 0 on failure
    //                  1 on success
  {
    bool ok = atoi2(&command[2], &i) && localSite.setElev(i);
    replyValueSetShort(ok);
  }
  break;
  case 'd':
    //  :SdsDD*MM#
    //          Set target object declination to sDD*MM or sDD*MM:SS depending on the current precision setting
    //          Return: 0 on failure
    //                  1 on success
  {
    bool ok = dmsToDouble(&newTargetDec, &command[2], true, highPrecision);
    replyValueSetShort(ok);
  }
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
    bool ok = dmsToDouble(&longi, &command[2 + i], false, j);
    if (ok)
    {
      if (command[2] == '-') longi = -longi;
      localSite.setLong(longi);
      rtk.resetLongitude(*localSite.longitude());
    }
    replyValueSetShort(ok);
  }
  break;
  //  :SG[sHH.H]#
//            Set the number of hours added to local time to yield UTC
//            Return: 0 on failure
//                    1 on success
  case 'G':
  {
    f = strtod(&command[2], &conv_end);
    bool ok = (&command[2] != conv_end) && (f >= -12 && f <= 12.0);
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
    //          Return: 0 on failure
    //                  1 on success
  {
    bool ok = (command[2] != 0);
    ok &= !(strlen(&command[2]) > 3);
    ok &= atoi2(&command[2], &i);
    ok &= ((i >= -30) && (i <= 30));
    if (ok)
    {
      minAlt = i;
      XEEPROM.update(getMountAddress(EE_minAlt), minAlt + 128);
    }
    replyValueSetShort(ok);
  }
  break;
  case 'L':
    //  :SLHH:MM:SS#
    //          Set the local Time
    //          Return: 0 on failure
    //                  1 on success  
  {
    int h1, m1, m2, s1;
    bool ok = hmsToHms(&h1, &m1, &m2, &s1, &command[2], true);
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
    //  :SM<string>#
    //  :SN<string>#
    //  :SO<string>#
    //          Set site name to be <string>, up to 14 characters.
    //          Return: 0 on failure
    //                  1 on success
    i = command[1] - 'M';
    if (strlen(&command[2]))
    {
      bool ok = XEEPROM.writeString(EE_sites + i * SiteSize + EE_site_name, &command[2], siteNameLen);
      replyValueSetShort(ok);
    }
    else
      replyNothing();
    break;
  case 'm':
    //  :Sm#   Sets the meridian pier-side for the next Target, TeenAstro LX200 command
    //         Returns: E#, W#, N# (none/parked), ?# (Meridian flip in progress)
    //         A # terminated string with the pier side.
    if ((command[2] != 0) && (strlen(&command[2]) < 2))
    {
      if (command[2] == 'N')
      {
        newTargetPoleSide = POLE_NOTVALID;
        replyValueSetShort(true);
      }
      else if (command[2] == 'E')
      {
        newTargetPoleSide = isAltAZ() || localSite.northHemisphere() ? POLE_UNDER : POLE_OVER;
        replyValueSetShort(true);
      }
      else if (command[2] == 'W')
      {
        newTargetPoleSide = isAltAZ() || localSite.northHemisphere() ? POLE_OVER : POLE_UNDER;
        replyValueSetShort(true);
      }
      else replyNothing();
    }
    else replyNothing();
    break;
  case 'n':
  {
    bool ok = strlen(&command[2]) && localSite.setSiteName(&command[2]);
    replyValueSetShort(ok);
  }
  break;
  case 'o':
    //  :SoDD#
    //          Set the overhead elevation limit to DD#
    //          Return: 0 on failure
    //                  1 on success
  {
    bool ok = (command[2] != 0) && (strlen(&command[2]) < 3);
    ok &= (atoi2(&command[2], &i)) && ((i >= 60) && (i <= 91));
    if (ok)
    {
      maxAlt = i;
      XEEPROM.update(getMountAddress(EE_maxAlt), maxAlt);
    }
    replyValueSetShort(ok);
  }
  break;
  case 'r':
    //  :SrHH:MM.T#
    //  :SrHH:MM:SS#
    //          Set target object RA to HH:MM.T or HH:MM:SS (based on precision setting)
    //          Return: 0 on failure
    //                  1 on success
  {
    bool ok = hmsToDouble(&newTargetRA, &command[2], highPrecision);
    if (ok)
    {
      newTargetRA *= 15.0;
    }
    replyValueSetShort(ok);
  }
  break;
  case 't':
    //  :StsDD*MM# or :StsDD:MM:SS#
    //          Sets the current site latitude to sDD*MM#
    //          Return: 0 on failure
    //                  1 on success     
  {
    bool ishighPrecision;
    if (strlen(&command[7]) > 1)
      ishighPrecision = command[8] == ':';
    else
      ishighPrecision = false;
    bool ok = dmsToDouble(&f, &command[2], true, ishighPrecision);
    if (ok)
    {
      localSite.setLat(f);
      initCelestialPole();
      initLimit();
      initHome();
      initTransformation(true);
      syncAtHome();
    }
    replyValueSetShort(ok);
  }
  break;
  case 'T':
    //  :STdd.ddddd#
    //          Return: 0 on failure
    //                  1 on success
  { bool ok = !movingTo;
  if (ok)
  {
    f = strtod(&command[2], &conv_end);
    bool ok = (&command[2] != conv_end) &&
      (((f >= 30.0) && (f < 90.0)) || (abs(f) < 0.1));
    if (ok)
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
    }
  }
  replyValueSetShort(ok);
  }
  break;
  case 'U':
    // :SU# store current User defined Position
  {
    Coord_EQ EQ_T = getEqu(*localSite.latitude() * DEG_TO_RAD);
    f = EQ_T.Ra(rtk.LST() * HOUR_TO_RAD) * RAD_TO_HOUR;
    f1 = EQ_T.Dec() * RAD_TO_DEG;
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
    //          Return: 0 on failure
    //                  1 on success
  {
    bool ok = dmsToDouble(&newTargetAzm, &command[2], false, highPrecision);
    newTargetAzm = degRange(newTargetAzm + 180.);
    replyValueSetShort(ok);
  }
  break;
  default:
    replyNothing();
    break;
  }
}
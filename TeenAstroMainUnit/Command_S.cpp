/**
 * Set commands: S (LX200 set) and SX (TeenAstro-specific set).
 */
#include "Command.h"
#include "ValueToString.h"

static bool parseYesNo(char c, bool& out) {
  if (c == 'y') { out = true;  return true; }
  if (c == 'n') { out = false; return true; }
  return false;
}

// -----------------------------------------------------------------------------
//   SX - TeenAstro set  :SXnnn,V...#  (0/1 on failure/success)
// -----------------------------------------------------------------------------
void Command_SX() {
  switch (command[2])
  {
  case 'A':
    // :SXAn,VVVVVV# Align Model values
  {
    bool ok = false;
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
    replyValueSetShort(ok);
    break;
  }
  case 'E':
    // :SXEnn# set encoder commands
  {

    switch (command[3])
    {
    case 'E':
      // :SXEE,y#  enable encoders
    {
#if HASEncoder
      bool val;
      bool ok = parseYesNo(command[5], val);
      if (ok && mount.enableEncoder != val)
      {
        mount.enableEncoder = val;
        WriteEEPROMEncoderMotorMode();
        mount.reboot_unit = true;
      }
      replyValueSetShort(ok);
#else
      replyShortFalse();
#endif // HASEncoder
    }
    break;
    case 'O':
      // :SXEO#  set encoder Sync Option
    {
      unsigned int i;
      bool ok = false;
      if (atoui2(&command[5], &i) && i <= (unsigned int)(EncoderSync::ES_ALWAYS))
      {
        ok = true;
        mount.EncodeSyncMode = static_cast<EncoderSync>(i);
        XEEPROM.write(getMountAddress(EE_encoderSync), mount.EncodeSyncMode);
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
            if (!mount.encoderA2.isPulsePerDegreeFix)
            {
              mount.encoderA2.pulsePerDegree = 0.01 * p;
              XEEPROM.writeLong(getMountAddress(EE_encoderA2pulsePerDegree), p);
              ok = true;
            }
          }
          else
          {
            if (!mount.encoderA1.isPulsePerDegreeFix)
            {
              mount.encoderA1.pulsePerDegree = 0.01 * p;
              XEEPROM.writeLong(getMountAddress(EE_encoderA1pulsePerDegree), p);
              ok = true;
            }
          }
          if (ok && !mount.enableMotor)
          {
            updateRatios(true, true);
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
          if (!mount.encoderA2.isReverseFix)
          {
            mount.encoderA2.reverse = command[6] == '1' ? true : false;
            XEEPROM.write(getMountAddress(EE_encoderA2reverse), mount.encoderA2.reverse);
            ok = true;
          }
        }
        else
        {
          if (!mount.encoderA1.isReverseFix)
          {
            mount.encoderA1.reverse = command[6] == '1' ? true : false;
            XEEPROM.write(getMountAddress(EE_encoderA1reverse), mount.encoderA1.reverse);
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
    bool val;
    bool ok = parseYesNo(command[5], val);
    if (ok)
    {
      switch (command[3])
      {
      case 'p':
        if (environment.doesRefraction.setPole(val))
        {
          initTransformation(true);
        }
        ok = true;
        break;
      case 'g':
      {
        environment.doesRefraction.setGoto(val);
        ok = true;
      }
      break;
      case 't':
      {
        environment.doesRefraction.setTracking(val);
        ok = true;
      }
      break;
      default:
        ok = false;
        break;
      }
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
      bool ok = mount.GuidingState == GuidingOFF;
      if (ok)
      {
        int i = command[3] - '0';
        int val = strtol(&command[5], NULL, 10);
        val = val > 0 && val < 256 ? val : pow(4, i);
        XEEPROM.write(getMountAddress(EE_Rate0 + i), val);
        if (i == 0)
          mount.guideRates[0] = (double)val / 100.;
        else
          mount.guideRates[i] = val;
        if (mount.activeGuideRate == i)
          enableGuideRate(i);
      }
      replyValueSetShort(ok);
    }
    break;
    case 'A':
      // :SXRA,VVV# Set degree for acceleration
      mount.DegreesForAcceleration = min(max(0.1 * (double)strtol(&command[5], NULL, 10), 0.1), 25.0);
      XEEPROM.update(getMountAddress(EE_degAcc), (uint8_t)(mount.DegreesForAcceleration * 10));
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
      mount.sideralMode = SIDM_TARGET;
      mount.RequestedTrackingRateHA = (double)(10000l - strtol(&command[5], NULL, 10)) / 10000.;
      computeTrackingRate(true);
      replyValueSetShort(true);
      break;
    case 'h':
      // :SXRh,VVVVVVVVVV# Set Rate for HA
      mount.sideralMode = SIDM_TARGET;
      mount.RequestedTrackingRateHA = (double)strtol(&command[5], NULL, 10) / 10000.0;
      computeTrackingRate(true);
      replyValueSetShort(true);
      break;
    case 'd':
      // :SXRd,VVVVVVVVVV# Set Rate for DEC
      if (mount.trackComp == TC_BOTH)
      {
        mount.sideralMode = SIDM_TARGET;
        mount.RequestedTrackingRateDEC = (double)strtol(&command[5], NULL, 10) / 10000.0;
        computeTrackingRate(true);
        replyValueSetShort(true);
      }
      else
      {
        replyValueSetShort(false);
      }
      break;
    case 'e':
      // :SXRe,VVVVVVVVVV# Store Rate for RA
    {
      long lval = strtol(&command[5], NULL, 10);
      mount.storedTrakingRateRA = lval < -50000 || lval > 50000 ? 0 : lval;
      XEEPROM.writeLong(getMountAddress(EE_RA_Drift), mount.storedTrakingRateRA);
      replyValueSetShort(true);
    }
    break;
    case 'f':
      // :SXRf,VVVVVVVVVV# Store Rate for DEC
    {
      long lval = strtol(&command[5], NULL, 10);
      mount.storedTrakingRateDEC = lval < -50000 || lval > 50000 ? 0 : lval;
      XEEPROM.writeLong(getMountAddress(EE_DEC_Drift), mount.storedTrakingRateDEC);
      replyValueSetShort(true);
    }
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
    {
      bool ok = false;
      int i = (int)strtol(&command[5], NULL, 10);
      if (i >= 10 * mount.geoA1.LimMinAxis && i < XEEPROM.readShort(getMountAddress(EE_maxAxis1)))
      {
        XEEPROM.writeShort(getMountAddress(EE_minAxis1), i);
        initLimitMinAxis1();
        ok = true;
      }
      replyValueSetShort(ok);
    }
    break;
    case 'B':
      // :SXLB,VVVV# set user defined maxAXIS1
    {
      bool ok = false;
      int i = (int)strtol(&command[5], NULL, 10);
      if (i <= 10 * mount.geoA1.LimMaxAxis && i > XEEPROM.readShort(getMountAddress(EE_minAxis1)))
      {
        XEEPROM.writeShort(getMountAddress(EE_maxAxis1), i);
        initLimitMaxAxis1();
        ok = true;
      }
      replyValueSetShort(ok);
    }
    break;
    case 'C':
      // :SXLC,VVVV# set user defined minAXIS2
    {
      bool ok = false;
      int i = (int)strtol(&command[5], NULL, 10);
      if (i >= 10 * mount.geoA2.LimMinAxis && i < XEEPROM.readShort(getMountAddress(EE_maxAxis2)))
      {
        XEEPROM.writeShort(getMountAddress(EE_minAxis2), i);
        initLimitMinAxis2();
        ok = true;
      }
      replyValueSetShort(ok);
    }
    break;
    case 'D':
      // :SXLD,VVVV# set user defined maxAXIS2
    {
      bool ok = false;
      int i = (int)strtol(&command[5], NULL, 10);
      if (i <= 10 * mount.geoA2.LimMaxAxis && i > XEEPROM.readShort(getMountAddress(EE_minAxis2)))
      {
        XEEPROM.writeShort(getMountAddress(EE_maxAxis2), i);
        initLimitMaxAxis2();
        ok = true;
      }
      replyValueSetShort(ok);
    }
    break;
    case 'E':
      // :SXLE,sVV.V# set user defined Meridian East Limit
      mount.minutesPastMeridianGOTOE = (double)strtol(&command[5], NULL, 10);
      if (mount.minutesPastMeridianGOTOE > 180) mount.minutesPastMeridianGOTOE = 180;
      if (mount.minutesPastMeridianGOTOE < -180) mount.minutesPastMeridianGOTOE = -180;
      XEEPROM.update(getMountAddress(EE_dpmE), round((mount.minutesPastMeridianGOTOE * 15.0) / 60.0) + 128);
      replyValueSetShort(true);
      break;
    case 'W':
      // :SXLW,sVV.V# set user defined Meridian West Limit
      mount.minutesPastMeridianGOTOW = (double)strtol(&command[5], NULL, 10);
      if (mount.minutesPastMeridianGOTOW > 180) mount.minutesPastMeridianGOTOW = 180;
      if (mount.minutesPastMeridianGOTOW < -180) mount.minutesPastMeridianGOTOW = -180;
      XEEPROM.update(getMountAddress(EE_dpmW), round((mount.minutesPastMeridianGOTOW * 15.0) / 60.0) + 128);
      replyValueSetShort(true);
      break;
    case 'U':
      // :SXLU,VV# set user defined Under Pole Limit
      mount.underPoleLimitGOTO = (double)strtol(&command[5], NULL, 10) / 10;
      if (mount.underPoleLimitGOTO > 12) mount.underPoleLimitGOTO = 12;
      if (mount.underPoleLimitGOTO < 9) mount.underPoleLimitGOTO = 9;
      XEEPROM.update(getMountAddress(EE_dup), round(mount.underPoleLimitGOTO * 10.0));
      replyValueSetShort(true);
      break;
    case 'H':
      // :SXLH,sVV# set user defined horizon Limit
      // NB: duplicate with :Sh#
    {
      int i;
      bool ok = (atoi2(&command[5], &i)) && ((i >= -30) && (i <= 30));
      if (ok)
      {
        mount.minAlt = i;
        XEEPROM.update(getMountAddress(EE_minAlt), mount.minAlt + 128);
      }
      replyValueSetShort(ok);
    }
    break;
    case 'O':
      // :SXLO,VV# set user defined overhead Limit
      // NB: duplicate with :So#
    {
      int i;
      bool ok = (atoi2(&command[5], &i)) && ((i >= 45) && (i <= 91));
      if (ok)
      {
        mount.maxAlt = i;
        XEEPROM.update(getMountAddress(EE_maxAlt), mount.maxAlt);
      }
      replyValueSetShort(ok);
    }
    break;
    case 'S':
      // :SXLS,sVV# set user defined distance from Pole to keep tracking on for 6 hours after transit, in degrees
    {
      int i;
      bool ok = (atoi2(&command[5], &i)) && ((i >= 0) && (i <= 181));
      if (ok)
      {
        mount.distanceFromPoleToKeepTrackingOn = i;
        XEEPROM.update(getMountAddress(EE_dpmDistanceFromPole), mount.distanceFromPoleToKeepTrackingOn);
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
    case 'E':
      // :SXME,y#  enable motors
    {
#if HASEncoder
      bool val;
      bool ok = parseYesNo(command[5], val);
      if (ok && mount.enableMotor != val)
      {
        mount.enableMotor = val;
        WriteEEPROMEncoderMotorMode();
        mount.reboot_unit = true;
      }
      replyValueSetShort(ok);
#else
      replyShortFalse();
#endif
    }
    break;
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
          mount.motorA2.backlashAmount = i;
          XEEPROM.writeUShort(getMountAddress(EE_motorA2backlashAmount), mount.motorA2.backlashAmount);
          mount.staA2.setBacklash_inSteps(mount.motorA2.backlashAmount, mount.geoA2.stepsPerArcSecond);
          ok = true;
        }
        else if (command[4] == 'R')
        {
          mount.motorA1.backlashAmount = i;
          XEEPROM.writeUShort(getMountAddress(EE_motorA1backlashAmount), mount.motorA1.backlashAmount);
          mount.staA1.setBacklash_inSteps(mount.motorA1.backlashAmount, mount.geoA1.stepsPerArcSecond);
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
          mount.motorA2.backlashRate = i;
          XEEPROM.write(getMountAddress(EE_motorA2backlashRate), mount.motorA2.backlashRate);
          mount.staA2.SetBacklash_interval_Step(mount.motorA2.backlashRate);
          ok = true;
        }
        else if (command[4] == 'R')
        {
          mount.motorA1.backlashRate = i;
          XEEPROM.write(getMountAddress(EE_motorA1backlashRate), mount.motorA1.backlashRate);
          mount.staA1.SetBacklash_interval_Step(mount.motorA1.backlashRate);
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
          if (!mount.motorA2.isGearFix)
          {
            double fact = (double)(i) / mount.motorA2.gear;
            cli();
            mount.staA2.pos = fact * mount.staA2.pos;
            sei();
            mount.motorA2.gear = i;
            XEEPROM.writeULong(getMountAddress(EE_motorA2gear), i);
            ok = true;
          }
        }
        else
        {
          if (!mount.motorA1.isGearFix)
          {
            double fact = (double)i / mount.motorA1.gear;
            cli();
            mount.staA1.pos = fact * mount.staA1.pos;
            sei();
            mount.motorA1.gear = i;
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
      // :SXMSn,VVVV# Set Step per Rotation
      unsigned int i;
      bool ok = !TelescopeBusy();
      ok &= (command[4] == 'D' || command[4] == 'R');
      ok &= (strlen(&command[6]) > 1) && (strlen(&command[6]) < 11);
      ok &= atoui2((char*)&command[6], &i);
      if (ok)
      {
        ok = false;
        if (command[4] == 'D')
        {
          if (!mount.motorA2.isStepRotFix)
          {
            double fact = (double)i / mount.motorA2.stepRot;
            cli();
            mount.staA2.pos = fact * mount.staA2.pos;
            sei();
            mount.motorA2.stepRot = i;
            XEEPROM.writeUShort(getMountAddress(EE_motorA2stepRot), i);
            ok = true;
          }
        }
        else
        {
          if (!mount.motorA1.isStepRotFix)
          {
            double fact = (double)i / mount.motorA1.stepRot;
            cli();
            mount.staA1.pos = fact * mount.staA1.pos;
            sei();
            mount.motorA1.stepRot = i;
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
      // for example :GXMMR3# for 1/8 microstep on the first axis 
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
          if (!mount.motorA2.isMicroFix)
          {
            double fact = pow(2., i - mount.motorA2.micro);
            cli();
            mount.staA2.pos = fact * mount.staA2.pos;
            sei();
            mount.motorA2.micro = i;
            mount.motorA2.driver.setMicrostep(mount.motorA2.micro);;
            XEEPROM.write(getMountAddress(EE_motorA2micro), mount.motorA2.micro);
            ok = true;
          }
        }
        else
        {
          if (!mount.motorA1.isMicroFix)
          {
            double fact = pow(2., i - mount.motorA1.micro);
            cli();
            mount.staA1.pos = fact * mount.staA1.pos;
            sei();
            mount.motorA1.micro = i;
            mount.motorA1.driver.setMicrostep(mount.motorA1.micro);
            XEEPROM.write(getMountAddress(EE_motorA1micro), mount.motorA1.micro);
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
     // for example :GXMmR1# for cool step on the first axis 
      int i;
      bool ok = false;
      if ((command[4] == 'D' || command[4] == 'R')
        && strlen(&command[6]) == 1
        && atoi2(&command[6], &i)
        && ((i >= 0) && (i < 2)))
      {
        if (command[4] == 'D')
        {
          if (!mount.motorA2.isSilentFix)
          {
            //mount.motorA2.driver.setmode(i);
            XEEPROM.write(getMountAddress(EE_motorA2silent), i);
            ok = true;
          }
        }
        else
        {
          if (!mount.motorA1.isSilentFix)
          {
            //mount.motorA1.driver.setmode(i);
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
          if (!mount.motorA2.isReverseFix)
          {
            mount.motorA2.reverse = command[6] == '1' ? true : false;
            XEEPROM.write(getMountAddress(EE_motorA2reverse), mount.motorA2.reverse);
            ok = true;
          }
        }
        else
        {
          if (!mount.motorA1.isReverseFix)
          {
            mount.motorA1.reverse = command[6] == '1' ? true : false;
            XEEPROM.write(getMountAddress(EE_motorA1reverse), mount.motorA1.reverse);
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
      bool ok = false;
      unsigned int curr = (strtoul(&command[6], NULL, 10) / 100) * 100;
      if (curr >= 100)
      {
        if (command[4] == 'D' && curr <= mount.motorA2.driver.getMaxCurrent())
        {
          if (command[3] == 'C')
          {
            if (!mount.motorA2.isHighCurrfix)
            {
              mount.motorA2.highCurr = curr;
              XEEPROM.write(getMountAddress(EE_motorA2highCurr), mount.motorA2.highCurr / 100);
              ok = true;
            }
          }
          else
          {
            if (!mount.motorA2.isLowCurrfix)
            {
              mount.motorA2.lowCurr = curr;
              XEEPROM.write(getMountAddress(EE_motorA2lowCurr), mount.motorA2.lowCurr / 100);
              mount.motorA2.driver.setCurrent(mount.motorA2.lowCurr);
              ok = true;
            }
          }
        }
        else if (command[4] == 'R' && curr <= mount.motorA1.driver.getMaxCurrent())
        {
          if (command[3] == 'C')
          {
            if (!mount.motorA1.isHighCurrfix)
            {
              mount.motorA1.highCurr = curr;
              XEEPROM.write(getMountAddress(EE_motorA1highCurr), mount.motorA1.highCurr / 100);
              ok = true;
            }
          }
          else
          {
            if (!mount.motorA1.isLowCurrfix)
            {
              mount.motorA1.lowCurr = curr;
              XEEPROM.write(getMountAddress(EE_motorA1lowCurr), mount.motorA1.lowCurr / 100);
              mount.motorA1.driver.setCurrent(mount.motorA1.lowCurr);
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
      // :SXMFn# Set Stall guard
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
          mount.motorA2.driver.setSG(i);
        }
        else
        {
          mount.motorA1.driver.setSG(i);
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
      int i;
      bool ok = (atoi2(&command[5], &i)) && ((i >= 0) && (i < maxNumMount));
      if (ok)
      {
        midx = i;
        XEEPROM.write(EE_currentMount, midx);
        mount.reboot_unit = true;
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
      else if (command[3] == 'B')
        i = 0;
      else if (command[3] == 'C')
        i = 1;
      bool ok = strlen(&command[5]) < MountNameLen + 1;
      if (ok)
      {
        memcpy(mount.mountName[i], &command[5], MountNameLen * sizeof(char));
        XEEPROM.writeString(getMountAddress(EE_mountName, i), mount.mountName[i], MountNameLen);
      }
      replyValueSetShort(ok);
    }
    break;
    case 'S':
      // :SXOS,NNN set Mount settle duration in seconds
    {
      unsigned int i;
      bool ok = atoui2((char*)&command[5], &i);
      ok &= i < 20;
      if (ok && mount.slewSettleDuration != i)
      {
        mount.slewSettleDuration = i;
        XEEPROM.write(getMountAddress(EE_SlewSettleDuration), mount.slewSettleDuration);
      }
      replyValueSetShort(ok);
    }
    break;
    default:
      replyNothing();
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
  switch (command[1])
  {
    //  :S!n#
    //         Set The Mount Type
    //         Return Nothing As it force a reboot
  case '!':
  {
    int i = (int)(command[2] - '0');
    bool ok = i > 0 && i < 5 && !mount.isMountTypeFix;
    if (ok)
    {
      force_reset_EE_Limit();
      XEEPROM.write(getMountAddress(EE_mountType), i);
      mount.reboot_unit = true;
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
    bool ok = dmsToDouble(&mount.newTargetAlt, &command[2], true, highPrecision);
    replyValueSetShort(ok);
  }
  break;
  case 'B':
    //  :SBn#  Set Baud Rate n for Serial-0, where n is an ASCII digit (1..9) with the following interpertation
        //         0=115.2K, 1=56.7K, 2=38.4K, 3=28.8K, 4=19.2K, 5=14.4K, 6=9600, 7=4800, 8=2400, 9=1200
        //         Returns: "1" At the current baud rate and then changes to the new rate for further communication
  {
    int i = (int)(command[2] - '0');
    bool ok = (i >= 0) && (i < 10);
    if (ok)
    {
      if (process_command == COMMAND_SERIAL)
      {
        Serial.print('1');
        delay(20);
        Serial.begin(commandState.baudRate_[i]);
      }
      else
      {
        Serial1.print('1');
        delay(20);
        Serial1.begin(commandState.baudRate_[i]);
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
    int i;
    bool ok = atoi2(&command[2], &i) && localSite.setElev(i);
    replyValueSetShort(ok);
  }
  break;
  case 'd':
    //  :SdsDD*MM#
    //  :SdsDD*MM:SS#
    //  :SdLsVV,VVVVV#
    //          Set target object declination to sDD*MM or sDD*MM:SS depending on the current precision setting
    //          Return: 0 on failure
    //                  1 on success
  {
    bool ok = false;
    if (command[2] == 'L')
    {
      char* conv_end;
      double f = strtod(&command[3], &conv_end);
      ok = -90 <= f && f <= 90;
      if (ok)
      {
        mount.newTargetDec = f;
      }
    }
    else
    {
      ok = dmsToDouble(&mount.newTargetDec, &command[2], true, highPrecision);
    }
    replyValueSetShort(ok);
  }
  break;
  case 'g':
    //  :SgsDDD*MM# or :SgDDD*MM# or :SgsDDD:MM:SS# or SgDDD:MM:ss#
    //          Set current sites longitude to sDDD*MM an ASCII position string, East longitudes can be as negative or >180 degrees
    //          Return: 0 on failure
    //                  1 on success
  {
    bool ok = false;
    if (mount.atHome|| mount.parkStatus == PRK_PARKED)
    {
      double longi = 0;
      int i = (command[2] == '-') || (command[2] == '+') ? 1 : 0;
      int j = strlen(&command[7 + i]) > 1 ? (command[8 + i] == ':') : 0;
      ok = dmsToDouble(&longi, &command[2 + i], false, j);
      if (ok)
      {
        if (command[2] == '-') longi = -longi;
        localSite.setLong(longi);
        rtk.resetLongitude(*localSite.longitude());
      }
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
    char* conv_end;
    double f = strtod(&command[2], &conv_end);
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
    int i;
    bool ok = (command[2] != 0);
    ok &= !(strlen(&command[2]) > 3);
    ok &= atoi2(&command[2], &i);
    ok &= ((i >= -30) && (i <= 30));
    if (ok)
    {
      mount.minAlt = i;
      XEEPROM.update(getMountAddress(EE_minAlt), mount.minAlt + 128);
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
  {
    int i = command[1] - 'M';
    if (strlen(&command[2]))
    {
      bool ok = XEEPROM.writeString(EE_sites + i * SiteSize + EE_site_name, &command[2], siteNameLen);
      replyValueSetShort(ok);
    }
    else
      replyNothing();
  }
  break;
  case 'm':
    //  :Sm#   Sets the meridian pier-side for the next Target, TeenAstro LX200 command
    //         Returns: E#, W#, N# (none/parked), ?# (Meridian flip in progress)
    //         A # terminated string with the pier side.
    if ((command[2] != 0) && (strlen(&command[2]) < 2))
    {
      if (command[2] == 'N')
      {
        mount.newTargetPoleSide = POLE_NOTVALID;
        replyValueSetShort(true);
      }
      else if (command[2] == 'E')
      {
        mount.newTargetPoleSide = isAltAZ() || localSite.northHemisphere() ? POLE_UNDER : POLE_OVER;
        replyValueSetShort(true);
      }
      else if (command[2] == 'W')
      {
        mount.newTargetPoleSide = isAltAZ() || localSite.northHemisphere() ? POLE_OVER : POLE_UNDER;
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
    int i;
    bool ok = (command[2] != 0) && (strlen(&command[2]) < 3);
    ok &= (atoi2(&command[2], &i)) && ((i >= 60) && (i <= 91));
    if (ok)
    {
      mount.maxAlt = i;
      XEEPROM.update(getMountAddress(EE_maxAlt), mount.maxAlt);
    }
    replyValueSetShort(ok);
  }
  break;
  case 'r':
    //  :SrHH:MM.T#
    //  :SrHH:MM:SS#
    //  :SrL,VVV.VVVVV#
    //          Set target object RA to HH:MM.T or HH:MM:SS (based on precision setting)
    //          Return: 0 on failure
    //                  1 on success
  {
    bool ok = false;
    if (command[2] == 'L')
    {
      char* conv_end;
      double f = strtod(&command[3], &conv_end);
      ok = 0 <= f && f <= 360;
      if (ok)
      {
        mount.newTargetRA = f;
      }
    }
    else
    {
      ok = hmsToDouble(&mount.newTargetRA, &command[2], highPrecision);
      if (ok)
      {
        mount.newTargetRA *= 15.0;
      }

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
    bool ok = false;
    if (mount.atHome || mount.parkStatus == PRK_PARKED)
    {
      bool ishighPrecision;
      double f;
      if (strlen(&command[7]) > 1)
        ishighPrecision = command[8] == ':';
      else
        ishighPrecision = false;
      ok = dmsToDouble(&f, &command[2], true, ishighPrecision);
      if (ok)
      {
        localSite.setLat(f);
        initCelestialPole();
        initLimit();
        initHome();

        initTransformation(true);
        if (mount.atHome)
        {
          syncAtHome();
        }
        if (mount.parkStatus == PRK_PARKED)
        {
          syncAtPark();
        }
      }
    }
    replyValueSetShort(ok);
  }
  break;
  case 'T':
    //  :STdd.ddddd#
    //          Return: 0 on failure
    //                  1 on success
  {
    bool ok = !mount.movingTo;
    if (ok)
    {
      char* conv_end;
      double f = strtod(&command[2], &conv_end);
      ok = (&command[2] != conv_end) &&
        (((f >= 30.0) && (f < 90.0)) || (abs(f) < 0.1));
      if (ok)
      {
        if (abs(f) < 0.1)
        {
          mount.sideralTracking = false;
        }
        else
        {
          mount.RequestedTrackingRateHA = (f / 60.0) / 1.00273790935;
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
    double f = EQ_T.Ra(rtk.LST() * HOUR_TO_RAD) * RAD_TO_HOUR;
    double f1 = EQ_T.Dec() * RAD_TO_DEG;
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
    bool ok = dmsToDouble(&mount.newTargetAzm, &command[2], false, highPrecision);
    replyValueSetShort(ok);
  }
  break;
  default:
    replyNothing();
    break;
  }
}
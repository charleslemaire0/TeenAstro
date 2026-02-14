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
  switch (commandState.command[2])
  {
  case 'A':
    // :SXAn,VVVVVV# Align Model values
  {
    bool ok = false;
    switch (commandState.command[3])
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

    switch (commandState.command[3])
    {
    case 'E':
      // :SXEE,y#  enable encoders
    {
#if HASEncoder
      bool val;
      bool ok = parseYesNo(commandState.command[5], val);
      if (ok && mount.motorsEncoders.enableEncoder != val)
      {
        mount.motorsEncoders.enableEncoder = val;
        WriteEEPROMEncoderMotorMode();
        mount.motorsEncoders.reboot_unit = true;
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
      if (atoui2(&commandState.command[5], &i) && i <= (unsigned int)(EncoderSync::ES_ALWAYS))
      {
        ok = true;
        mount.motorsEncoders.EncodeSyncMode = static_cast<EncoderSync>(i);
        XEEPROM.write(getMountAddress(EE_encoderSync), mount.motorsEncoders.EncodeSyncMode);
      }
      replyValueSetShort(ok);
    }
    break;
    case 'P':
    {
      // :SXEPn,VVVV# Set pulse per 100deg max
      bool ok = false;
      if ((commandState.command[4] == 'D' || commandState.command[4] == 'R')
        && (strlen(&commandState.command[6]) > 0) && (strlen(&commandState.command[6]) < 7)
        )
      {
        char* pEnd;
        unsigned long p = strtoul(&commandState.command[6], &pEnd, 10);
        if (p > 0 && p <= 360000)
        {
          if (commandState.command[4] == 'D')
          {
            if (!mount.motorsEncoders.encoderA2.isPulsePerDegreeFix)
            {
              mount.motorsEncoders.encoderA2.pulsePerDegree = 0.01 * p;
              XEEPROM.writeLong(getMountAddress(EE_encoderA2pulsePerDegree), p);
              ok = true;
            }
          }
          else
          {
            if (!mount.motorsEncoders.encoderA1.isPulsePerDegreeFix)
            {
              mount.motorsEncoders.encoderA1.pulsePerDegree = 0.01 * p;
              XEEPROM.writeLong(getMountAddress(EE_encoderA1pulsePerDegree), p);
              ok = true;
            }
          }
          if (ok && !mount.motorsEncoders.enableMotor)
          {
            mount.updateRatios(true, true);
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
      if ((commandState.command[4] == 'D' || commandState.command[4] == 'R')
        && strlen(&commandState.command[6]) == 1
        && (commandState.command[6] == '0' || commandState.command[6] == '1'))
      {
        if (commandState.command[4] == 'D')
        {
          if (!mount.motorsEncoders.encoderA2.isReverseFix)
          {
            mount.motorsEncoders.encoderA2.reverse = commandState.command[6] == '1' ? true : false;
            XEEPROM.write(getMountAddress(EE_encoderA2reverse), mount.motorsEncoders.encoderA2.reverse);
            ok = true;
          }
        }
        else
        {
          if (!mount.motorsEncoders.encoderA1.isReverseFix)
          {
            mount.motorsEncoders.encoderA1.reverse = commandState.command[6] == '1' ? true : false;
            XEEPROM.write(getMountAddress(EE_encoderA1reverse), mount.motorsEncoders.encoderA1.reverse);
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
    bool ok = parseYesNo(commandState.command[5], val);
    if (ok)
    {
      switch (commandState.command[3])
      {
      case 'p':
        if (mount.refraction.setPole(val))
        {
          initTransformation(true);
        }
        ok = true;
        break;
      case 'g':
      {
        mount.refraction.setGoto(val);
        ok = true;
      }
      break;
      case 't':
      {
        mount.refraction.setTracking(val);
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
    switch (commandState.command[3])
    {
    case '0': // :SXR0,VVV# Set Rate for user defined rates
    case '1': // :SXR1,VVV# Set Rate for user defined rates
    case '2': // :SXR2,VVV# Set Rate for user defined rates
    case '3': // :SXR3,VVV# Set Rate for user defined rates
    {
      bool ok = mount.guiding.GuidingState == GuidingOFF;
      if (ok)
      {
        int i = commandState.command[3] - '0';
        int val = strtol(&commandState.command[5], NULL, 10);
        val = val > 0 && val < 256 ? val : pow(4, i);
        XEEPROM.write(getMountAddress(EE_Rate0 + i), val);
        if (i == 0)
          mount.guiding.guideRates[0] = (double)val / 100.;
        else
          mount.guiding.guideRates[i] = val;
        if (mount.guiding.activeGuideRate == i)
          mount.enableGuideRate(i);
      }
      replyValueSetShort(ok);
    }
    break;
    case 'A':
      // :SXRA,VVV# Set degree for acceleration
      mount.guiding.DegreesForAcceleration = min(max(0.1 * (double)strtol(&commandState.command[5], NULL, 10), 0.1), 25.0);
      XEEPROM.update(getMountAddress(EE_degAcc), (uint8_t)(mount.guiding.DegreesForAcceleration * 10));
      mount.setAcceleration();
      replyValueSetShort(true);
      break;
    case 'D':
    {
      // :SXRD,V# define default rate
      int val = strtol(&commandState.command[5], NULL, 10);
      val = val > 4 || val < 0 ? 3 : val;
      XEEPROM.write(getMountAddress(EE_DefaultRate), val);
      replyValueSetShort(true);
      break;
    }
    case 'X':
      // :SXRX,VVVV# Set Rate for max Rate
      XEEPROM.writeUShort(getMountAddress(EE_maxRate), (int)strtol(&commandState.command[5], NULL, 10));
      mount.initMaxRate();
      replyValueSetShort(true);
      break;
    case 'r':
      // :SXRr,VVVVVVVVVV# Set Rate for RA 
      mount.tracking.sideralMode = SIDM_TARGET;
      mount.tracking.RequestedTrackingRateHA = (double)(10000l - strtol(&commandState.command[5], NULL, 10)) / 10000.;
      mount.computeTrackingRate(true);
      replyValueSetShort(true);
      break;
    case 'h':
      // :SXRh,VVVVVVVVVV# Set Rate for HA
      mount.tracking.sideralMode = SIDM_TARGET;
      mount.tracking.RequestedTrackingRateHA = (double)strtol(&commandState.command[5], NULL, 10) / 10000.0;
      mount.computeTrackingRate(true);
      replyValueSetShort(true);
      break;
    case 'd':
      // :SXRd,VVVVVVVVVV# Set Rate for DEC
      if (mount.tracking.trackComp == TC_BOTH)
      {
        mount.tracking.sideralMode = SIDM_TARGET;
        mount.tracking.RequestedTrackingRateDEC = (double)strtol(&commandState.command[5], NULL, 10) / 10000.0;
        mount.computeTrackingRate(true);
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
      long lval = strtol(&commandState.command[5], NULL, 10);
      mount.tracking.storedTrakingRateRA = lval < -50000 || lval > 50000 ? 0 : lval;
      XEEPROM.writeLong(getMountAddress(EE_RA_Drift), mount.tracking.storedTrakingRateRA);
      replyValueSetShort(true);
    }
    break;
    case 'f':
      // :SXRf,VVVVVVVVVV# Store Rate for DEC
    {
      long lval = strtol(&commandState.command[5], NULL, 10);
      mount.tracking.storedTrakingRateDEC = lval < -50000 || lval > 50000 ? 0 : lval;
      XEEPROM.writeLong(getMountAddress(EE_DEC_Drift), mount.tracking.storedTrakingRateDEC);
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
    switch (commandState.command[3])
    {
    case 'R':
      // :SXLR# reset user defined axis limit
      mount.limits.resetEELimit();
      break;
    case 'A':
      // :SXLA,VVVV# set user defined minAXIS1
    {
      bool ok = false;
      int i = (int)strtol(&commandState.command[5], NULL, 10);
      if (i >= 10 * mount.axes.geoA1.LimMinAxis && i < XEEPROM.readShort(getMountAddress(EE_maxAxis1)))
      {
        XEEPROM.writeShort(getMountAddress(EE_minAxis1), i);
        mount.limits.initLimitMinAxis1();
        ok = true;
      }
      replyValueSetShort(ok);
    }
    break;
    case 'B':
      // :SXLB,VVVV# set user defined maxAXIS1
    {
      bool ok = false;
      int i = (int)strtol(&commandState.command[5], NULL, 10);
      if (i <= 10 * mount.axes.geoA1.LimMaxAxis && i > XEEPROM.readShort(getMountAddress(EE_minAxis1)))
      {
        XEEPROM.writeShort(getMountAddress(EE_maxAxis1), i);
        mount.limits.initLimitMaxAxis1();
        ok = true;
      }
      replyValueSetShort(ok);
    }
    break;
    case 'C':
      // :SXLC,VVVV# set user defined minAXIS2
    {
      bool ok = false;
      int i = (int)strtol(&commandState.command[5], NULL, 10);
      if (i >= 10 * mount.axes.geoA2.LimMinAxis && i < XEEPROM.readShort(getMountAddress(EE_maxAxis2)))
      {
        XEEPROM.writeShort(getMountAddress(EE_minAxis2), i);
        mount.limits.initLimitMinAxis2();
        ok = true;
      }
      replyValueSetShort(ok);
    }
    break;
    case 'D':
      // :SXLD,VVVV# set user defined maxAXIS2
    {
      bool ok = false;
      int i = (int)strtol(&commandState.command[5], NULL, 10);
      if (i <= 10 * mount.axes.geoA2.LimMaxAxis && i > XEEPROM.readShort(getMountAddress(EE_minAxis2)))
      {
        XEEPROM.writeShort(getMountAddress(EE_maxAxis2), i);
        mount.limits.initLimitMaxAxis2();
        ok = true;
      }
      replyValueSetShort(ok);
    }
    break;
    case 'E':
      // :SXLE,sVV.V# set user defined Meridian East Limit
      mount.limits.minutesPastMeridianGOTOE = (double)strtol(&commandState.command[5], NULL, 10);
      if (mount.limits.minutesPastMeridianGOTOE > 180) mount.limits.minutesPastMeridianGOTOE = 180;
      if (mount.limits.minutesPastMeridianGOTOE < -180) mount.limits.minutesPastMeridianGOTOE = -180;
      XEEPROM.update(getMountAddress(EE_dpmE), round((mount.limits.minutesPastMeridianGOTOE * 15.0) / 60.0) + 128);
      replyValueSetShort(true);
      break;
    case 'W':
      // :SXLW,sVV.V# set user defined Meridian West Limit
      mount.limits.minutesPastMeridianGOTOW = (double)strtol(&commandState.command[5], NULL, 10);
      if (mount.limits.minutesPastMeridianGOTOW > 180) mount.limits.minutesPastMeridianGOTOW = 180;
      if (mount.limits.minutesPastMeridianGOTOW < -180) mount.limits.minutesPastMeridianGOTOW = -180;
      XEEPROM.update(getMountAddress(EE_dpmW), round((mount.limits.minutesPastMeridianGOTOW * 15.0) / 60.0) + 128);
      replyValueSetShort(true);
      break;
    case 'U':
      // :SXLU,VV# set user defined Under Pole Limit
      mount.limits.underPoleLimitGOTO = (double)strtol(&commandState.command[5], NULL, 10) / 10;
      if (mount.limits.underPoleLimitGOTO > 12) mount.limits.underPoleLimitGOTO = 12;
      if (mount.limits.underPoleLimitGOTO < 9) mount.limits.underPoleLimitGOTO = 9;
      XEEPROM.update(getMountAddress(EE_dup), round(mount.limits.underPoleLimitGOTO * 10.0));
      replyValueSetShort(true);
      break;
    case 'H':
      // :SXLH,sVV# set user defined horizon Limit
      // NB: duplicate with :Sh#
    {
      int i;
      bool ok = (atoi2(&commandState.command[5], &i)) && ((i >= -30) && (i <= 30));
      if (ok)
      {
        mount.limits.minAlt = i;
        XEEPROM.update(getMountAddress(EE_minAlt), mount.limits.minAlt + 128);
      }
      replyValueSetShort(ok);
    }
    break;
    case 'O':
      // :SXLO,VV# set user defined overhead Limit
      // NB: duplicate with :So#
    {
      int i;
      bool ok = (atoi2(&commandState.command[5], &i)) && ((i >= 45) && (i <= 91));
      if (ok)
      {
        mount.limits.maxAlt = i;
        XEEPROM.update(getMountAddress(EE_maxAlt), mount.limits.maxAlt);
      }
      replyValueSetShort(ok);
    }
    break;
    case 'S':
      // :SXLS,sVV# set user defined distance from Pole to keep tracking on for 6 hours after transit, in degrees
    {
      int i;
      bool ok = (atoi2(&commandState.command[5], &i)) && ((i >= 0) && (i <= 181));
      if (ok)
      {
        mount.limits.distanceFromPoleToKeepTrackingOn = i;
        XEEPROM.update(getMountAddress(EE_dpmDistanceFromPole), mount.limits.distanceFromPoleToKeepTrackingOn);
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
    switch (commandState.command[3])
    {
      //  :SXT0HH:MM:SS#
      //          Return: 0 on failure
      //                  1 on success 
    case '0':
    {
      int h1, m1, m2, s1;
      bool ok = hmsToHms(&h1, &m1, &m2, &s1, &commandState.command[4], true);
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
      bool ok = dateToYYYYMMDD(&y, &m, &d, &commandState.command[4]);
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
      unsigned long t = strtoul(&commandState.command[5], &pEnd, 10);
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
    switch (commandState.command[3])
    {
    case 'E':
      // :SXME,y#  enable motors
    {
#if HASEncoder
      bool val;
      bool ok = parseYesNo(commandState.command[5], val);
      if (ok && mount.motorsEncoders.enableMotor != val)
      {
        mount.motorsEncoders.enableMotor = val;
        WriteEEPROMEncoderMotorMode();
        mount.motorsEncoders.reboot_unit = true;
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
      bool ok = !mount.isSlewing();
      ok &= atoi2((char*)&commandState.command[6], &i);
      ok &= ((i >= 0) && (i <= 999));
      if (ok)
      {
        ok = false;
        if (commandState.command[4] == 'D')
        {
          mount.motorsEncoders.motorA2.backlashAmount = i;
          XEEPROM.writeUShort(getMountAddress(EE_motorA2backlashAmount), mount.motorsEncoders.motorA2.backlashAmount);
          mount.axes.staA2.setBacklash_inSteps(mount.motorsEncoders.motorA2.backlashAmount, mount.axes.geoA2.stepsPerArcSecond);
          ok = true;
        }
        else if (commandState.command[4] == 'R')
        {
          mount.motorsEncoders.motorA1.backlashAmount = i;
          XEEPROM.writeUShort(getMountAddress(EE_motorA1backlashAmount), mount.motorsEncoders.motorA1.backlashAmount);
          mount.axes.staA1.setBacklash_inSteps(mount.motorsEncoders.motorA1.backlashAmount, mount.axes.geoA1.stepsPerArcSecond);
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
      bool ok = !mount.isSlewing();
      ok &= atoi2((char*)&commandState.command[6], &i);
      ok &= ((i >= 16) && (i <= 64));
      if (ok)
      {
        ok = false;
        if (commandState.command[4] == 'D')
        {
          mount.motorsEncoders.motorA2.backlashRate = i;
          XEEPROM.write(getMountAddress(EE_motorA2backlashRate), mount.motorsEncoders.motorA2.backlashRate);
          mount.axes.staA2.SetBacklash_interval_Step(mount.motorsEncoders.motorA2.backlashRate);
          ok = true;
        }
        else if (commandState.command[4] == 'R')
        {
          mount.motorsEncoders.motorA1.backlashRate = i;
          XEEPROM.write(getMountAddress(EE_motorA1backlashRate), mount.motorsEncoders.motorA1.backlashRate);
          mount.axes.staA1.SetBacklash_interval_Step(mount.motorsEncoders.motorA1.backlashRate);
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
      bool ok = !mount.isSlewing();
      ok &= (commandState.command[4] == 'D' || commandState.command[4] == 'R');
      ok &= strlen(&commandState.command[6]) > 1 && strlen(&commandState.command[6]) < 11;
      if (ok)
      {
        ok = false;
        i = strtoul(&commandState.command[6], NULL, 10);
        if (commandState.command[4] == 'D')
        {
          if (!mount.motorsEncoders.motorA2.isGearFix)
          {
            double fact = (double)(i) / mount.motorsEncoders.motorA2.gear;
            cli();
            mount.axes.staA2.pos = fact * mount.axes.staA2.pos;
            sei();
            mount.motorsEncoders.motorA2.gear = i;
            XEEPROM.writeULong(getMountAddress(EE_motorA2gear), i);
            ok = true;
          }
        }
        else
        {
          if (!mount.motorsEncoders.motorA1.isGearFix)
          {
            double fact = (double)i / mount.motorsEncoders.motorA1.gear;
            cli();
            mount.axes.staA1.pos = fact * mount.axes.staA1.pos;
            sei();
            mount.motorsEncoders.motorA1.gear = i;
            XEEPROM.writeULong(getMountAddress(EE_motorA1gear), i);
            ok = true;
          }
        }
      }
      if (ok)
      {
        mount.updateRatios(true, true);
      }
      replyValueSetShort(ok);
    }
    break;
    case 'S':
    {
      // :SXMSn,VVVV# Set Step per Rotation
      unsigned int i;
      bool ok = !mount.isSlewing();
      ok &= (commandState.command[4] == 'D' || commandState.command[4] == 'R');
      ok &= (strlen(&commandState.command[6]) > 1) && (strlen(&commandState.command[6]) < 11);
      ok &= atoui2((char*)&commandState.command[6], &i);
      if (ok)
      {
        ok = false;
        if (commandState.command[4] == 'D')
        {
          if (!mount.motorsEncoders.motorA2.isStepRotFix)
          {
            double fact = (double)i / mount.motorsEncoders.motorA2.stepRot;
            cli();
            mount.axes.staA2.pos = fact * mount.axes.staA2.pos;
            sei();
            mount.motorsEncoders.motorA2.stepRot = i;
            XEEPROM.writeUShort(getMountAddress(EE_motorA2stepRot), i);
            ok = true;
          }
        }
        else
        {
          if (!mount.motorsEncoders.motorA1.isStepRotFix)
          {
            double fact = (double)i / mount.motorsEncoders.motorA1.stepRot;
            cli();
            mount.axes.staA1.pos = fact * mount.axes.staA1.pos;
            sei();
            mount.motorsEncoders.motorA1.stepRot = i;
            XEEPROM.writeUShort(getMountAddress(EE_motorA1stepRot), i);
            ok = true;
          }
        }
      }
      if (ok)
      {
        mount.updateRatios(true, true);
      }
      replyValueSetShort(ok);
    }
    break;
    case 'M':
    {
      // :SXMMn,V# Set Microstep
      // for example :GXMMR3# for 1/8 microstep on the first axis 
      int i;
      bool ok = !mount.isSlewing();
      ok &= (commandState.command[4] == 'D' || commandState.command[4] == 'R');
      ok &= strlen(&commandState.command[6]) == 1;
      ok &= atoi2(&commandState.command[6], &i);
      ok &= (i >= 1 && i < 9);
      if (ok)
      {
        ok = false;
        if (commandState.command[4] == 'D')
        {
          if (!mount.motorsEncoders.motorA2.isMicroFix)
          {
            double fact = pow(2., i - mount.motorsEncoders.motorA2.micro);
            cli();
            mount.axes.staA2.pos = fact * mount.axes.staA2.pos;
            sei();
            mount.motorsEncoders.motorA2.micro = i;
            mount.motorsEncoders.motorA2.driver.setMicrostep(mount.motorsEncoders.motorA2.micro);;
            XEEPROM.write(getMountAddress(EE_motorA2micro), mount.motorsEncoders.motorA2.micro);
            ok = true;
          }
        }
        else
        {
          if (!mount.motorsEncoders.motorA1.isMicroFix)
          {
            double fact = pow(2., i - mount.motorsEncoders.motorA1.micro);
            cli();
            mount.axes.staA1.pos = fact * mount.axes.staA1.pos;
            sei();
            mount.motorsEncoders.motorA1.micro = i;
            mount.motorsEncoders.motorA1.driver.setMicrostep(mount.motorsEncoders.motorA1.micro);
            XEEPROM.write(getMountAddress(EE_motorA1micro), mount.motorsEncoders.motorA1.micro);
            ok = true;
          }
        }
      }
      if (ok)
      {
        mount.updateRatios(true, false);
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
      if ((commandState.command[4] == 'D' || commandState.command[4] == 'R')
        && strlen(&commandState.command[6]) == 1
        && atoi2(&commandState.command[6], &i)
        && ((i >= 0) && (i < 2)))
      {
        if (commandState.command[4] == 'D')
        {
          if (!mount.motorsEncoders.motorA2.isSilentFix)
          {
            //mount.motorsEncoders.motorA2.driver.setmode(i);
            XEEPROM.write(getMountAddress(EE_motorA2silent), i);
            ok = true;
          }
        }
        else
        {
          if (!mount.motorsEncoders.motorA1.isSilentFix)
          {
            //mount.motorsEncoders.motorA1.driver.setmode(i);
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
      if ((commandState.command[4] == 'D' || commandState.command[4] == 'R')
        && strlen(&commandState.command[6]) == 1
        && (commandState.command[6] == '0' || commandState.command[6] == '1'))
      {
        if (commandState.command[4] == 'D')
        {
          if (!mount.motorsEncoders.motorA2.isReverseFix)
          {
            mount.motorsEncoders.motorA2.reverse = commandState.command[6] == '1' ? true : false;
            XEEPROM.write(getMountAddress(EE_motorA2reverse), mount.motorsEncoders.motorA2.reverse);
            ok = true;
          }
        }
        else
        {
          if (!mount.motorsEncoders.motorA1.isReverseFix)
          {
            mount.motorsEncoders.motorA1.reverse = commandState.command[6] == '1' ? true : false;
            XEEPROM.write(getMountAddress(EE_motorA1reverse), mount.motorsEncoders.motorA1.reverse);
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
      unsigned int curr = (strtoul(&commandState.command[6], NULL, 10) / 100) * 100;
      if (curr >= 100)
      {
        if (commandState.command[4] == 'D' && curr <= mount.motorsEncoders.motorA2.driver.getMaxCurrent())
        {
          if (commandState.command[3] == 'C')
          {
            if (!mount.motorsEncoders.motorA2.isHighCurrfix)
            {
              mount.motorsEncoders.motorA2.highCurr = curr;
              XEEPROM.write(getMountAddress(EE_motorA2highCurr), mount.motorsEncoders.motorA2.highCurr / 100);
              ok = true;
            }
          }
          else
          {
            if (!mount.motorsEncoders.motorA2.isLowCurrfix)
            {
              mount.motorsEncoders.motorA2.lowCurr = curr;
              XEEPROM.write(getMountAddress(EE_motorA2lowCurr), mount.motorsEncoders.motorA2.lowCurr / 100);
              mount.motorsEncoders.motorA2.driver.setCurrent(mount.motorsEncoders.motorA2.lowCurr);
              ok = true;
            }
          }
        }
        else if (commandState.command[4] == 'R' && curr <= mount.motorsEncoders.motorA1.driver.getMaxCurrent())
        {
          if (commandState.command[3] == 'C')
          {
            if (!mount.motorsEncoders.motorA1.isHighCurrfix)
            {
              mount.motorsEncoders.motorA1.highCurr = curr;
              XEEPROM.write(getMountAddress(EE_motorA1highCurr), mount.motorsEncoders.motorA1.highCurr / 100);
              ok = true;
            }
          }
          else
          {
            if (!mount.motorsEncoders.motorA1.isLowCurrfix)
            {
              mount.motorsEncoders.motorA1.lowCurr = curr;
              XEEPROM.write(getMountAddress(EE_motorA1lowCurr), mount.motorsEncoders.motorA1.lowCurr / 100);
              mount.motorsEncoders.motorA1.driver.setCurrent(mount.motorsEncoders.motorA1.lowCurr);
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
      bool ok = (commandState.command[4] == 'D' || commandState.command[4] == 'R')
        && (strlen(&commandState.command[6]) > 1) && (strlen(&commandState.command[6]) < 5)
        && atoi2((char*)&commandState.command[6], &i)
        && ((i >= 0) && (i <= 127));
      if (ok)
      {
        i = i - 64;
        if (commandState.command[4] == 'D')
        {
          mount.motorsEncoders.motorA2.driver.setSG(i);
        }
        else
        {
          mount.motorsEncoders.motorA1.driver.setSG(i);
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
    switch (commandState.command[3])
    {
    case 'I':
      // :SXOI,V set Mount index
    {
      int i;
      bool ok = (atoi2(&commandState.command[5], &i)) && ((i >= 0) && (i < maxNumMount));
      if (ok)
      {
        midx = i;
        XEEPROM.write(EE_currentMount, midx);
        mount.motorsEncoders.reboot_unit = true;
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
      if (commandState.command[3] == 'A')
        i = midx;
      else if (commandState.command[3] == 'B')
        i = 0;
      else if (commandState.command[3] == 'C')
        i = 1;
      bool ok = strlen(&commandState.command[5]) < MountNameLen + 1;
      if (ok)
      {
        memcpy(mount.config.identity.mountName[i], &commandState.command[5], MountNameLen * sizeof(char));
        XEEPROM.writeString(getMountAddress(EE_mountName, i), mount.config.identity.mountName[i], MountNameLen);
      }
      replyValueSetShort(ok);
    }
    break;
    case 'S':
      // :SXOS,NNN set Mount settle duration in seconds
    {
      unsigned int i;
      bool ok = atoui2((char*)&commandState.command[5], &i);
      ok &= i < 20;
      if (ok && mount.parkHome.slewSettleDuration != i)
      {
        mount.parkHome.slewSettleDuration = i;
        XEEPROM.write(getMountAddress(EE_SlewSettleDuration), mount.parkHome.slewSettleDuration);
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

// Intentionally long: central handler for Set (LX200) command variants.
void Command_S(Command& process_command)
{
  switch (commandState.command[1])
  {
    //  :S!n#
    //         Set The Mount Type
    //         Return Nothing As it force a reboot
  case '!':
  {
    int i = (int)(commandState.command[2] - '0');
    bool ok = i > 0 && i < 5 && !mount.config.identity.isMountTypeFix;
    if (ok)
    {
      mount.limits.forceResetEELimit();
      XEEPROM.write(getMountAddress(EE_mountType), i);
      mount.motorsEncoders.reboot_unit = true;
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
    bool ok = dmsToDouble(&mount.targetCurrent.newTargetAlt, &commandState.command[2], true, commandState.highPrecision);
    replyValueSetShort(ok);
  }
  break;
  case 'B':
    //  :SBn#  Set Baud Rate n for Serial-0, where n is an ASCII digit (1..9) with the following interpertation
        //         0=115.2K, 1=56.7K, 2=38.4K, 3=28.8K, 4=19.2K, 5=14.4K, 6=9600, 7=4800, 8=2400, 9=1200
        //         Returns: "1" At the current baud rate and then changes to the new rate for further communication
  {
    int i = (int)(commandState.command[2] - '0');
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
    bool ok = dateToYYYYMMDD(&y, &m, &d, &commandState.command[2]);
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
    bool ok = atoi2(&commandState.command[2], &i) && localSite.setElev(i);
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
    if (commandState.command[2] == 'L')
    {
      char* conv_end;
      double f = strtod(&commandState.command[3], &conv_end);
      ok = -90 <= f && f <= 90;
      if (ok)
      {
        mount.targetCurrent.newTargetDec = f;
      }
    }
    else
    {
      ok = dmsToDouble(&mount.targetCurrent.newTargetDec, &commandState.command[2], true, commandState.highPrecision);
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
    if (mount.isAtHome() || mount.isParked())
    {
      double longi = 0.0;
      int i = (commandState.command[2] == '-') || (commandState.command[2] == '+') ? 1 : 0;
      int j = strlen(&commandState.command[7 + i]) > 1 ? (commandState.command[8 + i] == ':') : 0;
      ok = dmsToDouble(&longi, &commandState.command[2 + i], false, j);
      if (ok)
      {
        if (commandState.command[2] == '-') longi = -longi;
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
    double f = strtod(&commandState.command[2], &conv_end);
    bool ok = (&commandState.command[2] != conv_end) && (f >= -12 && f <= 12.0);
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
    bool ok = (commandState.command[2] != 0);
    ok &= !(strlen(&commandState.command[2]) > 3);
    ok &= atoi2(&commandState.command[2], &i);
    ok &= ((i >= -30) && (i <= 30));
    if (ok)
    {
      mount.limits.minAlt = i;
      XEEPROM.update(getMountAddress(EE_minAlt), mount.limits.minAlt + 128);
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
    bool ok = hmsToHms(&h1, &m1, &m2, &s1, &commandState.command[2], true);
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
    int i = commandState.command[1] - 'M';
    if (strlen(&commandState.command[2]))
    {
      bool ok = XEEPROM.writeString(EE_sites + i * SiteSize + EE_site_name, &commandState.command[2], siteNameLen);
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
    if ((commandState.command[2] != 0) && (strlen(&commandState.command[2]) < 2))
    {
      if (commandState.command[2] == 'N')
      {
        mount.targetCurrent.newTargetPoleSide = POLE_NOTVALID;
        replyValueSetShort(true);
      }
      else if (commandState.command[2] == 'E')
      {
        mount.targetCurrent.newTargetPoleSide = mount.isAltAZ() || localSite.northHemisphere() ? POLE_UNDER : POLE_OVER;
        replyValueSetShort(true);
      }
      else if (commandState.command[2] == 'W')
      {
        mount.targetCurrent.newTargetPoleSide = mount.isAltAZ() || localSite.northHemisphere() ? POLE_OVER : POLE_UNDER;
        replyValueSetShort(true);
      }
      else replyNothing();
    }
    else replyNothing();
    break;
  case 'n':
  {
    bool ok = strlen(&commandState.command[2]) && localSite.setSiteName(&commandState.command[2]);
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
    bool ok = (commandState.command[2] != 0) && (strlen(&commandState.command[2]) < 3);
    ok &= (atoi2(&commandState.command[2], &i)) && ((i >= 60) && (i <= 91));
    if (ok)
    {
      mount.limits.maxAlt = i;
      XEEPROM.update(getMountAddress(EE_maxAlt), mount.limits.maxAlt);
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
    if (commandState.command[2] == 'L')
    {
      char* conv_end;
      double f = strtod(&commandState.command[3], &conv_end);
      ok = 0 <= f && f <= 360;
      if (ok)
      {
        mount.targetCurrent.newTargetRA = f;
      }
    }
    else
    {
      ok = hmsToDouble(&mount.targetCurrent.newTargetRA, &commandState.command[2], commandState.highPrecision);
      if (ok)
      {
        mount.targetCurrent.newTargetRA *= 15.0;
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
    if (mount.isAtHome() || mount.isParked())
    {
      bool ishighPrecision;
      double f;
      if (strlen(&commandState.command[7]) > 1)
        ishighPrecision = commandState.command[8] == ':';
      else
        ishighPrecision = false;
      ok = dmsToDouble(&f, &commandState.command[2], true, ishighPrecision);
      if (ok)
      {
        localSite.setLat(f);
        initCelestialPole();
        mount.limits.initLimit();
        mount.initHome();

        initTransformation(true);
        if (mount.isAtHome())
        {
          mount.syncAtHome();
        }
        if (mount.isParked())
        {
          mount.syncAtPark();
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
    bool ok = !mount.tracking.movingTo;
    if (ok)
    {
      char* conv_end;
      double f = strtod(&commandState.command[2], &conv_end);
      ok = (&commandState.command[2] != conv_end) &&
        (((f >= 30.0) && (f < 90.0)) || (abs(f) < 0.1));
      if (ok)
      {
        if (abs(f) < 0.1)
        {
          mount.tracking.sideralTracking = false;
        }
        else
        {
          mount.tracking.RequestedTrackingRateHA = (f / 60.0) / 1.00273790935;
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
    Coord_EQ EQ_T = mount.getEqu(*localSite.latitude() * DEG_TO_RAD);
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
    bool ok = dmsToDouble(&mount.targetCurrent.newTargetAzm, &commandState.command[2], false, commandState.highPrecision);
    replyValueSetShort(ok);
  }
  break;
  default:
    replyNothing();
    break;
  }
}
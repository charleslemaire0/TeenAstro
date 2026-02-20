/**
 * SX — TeenAstro-specific set commands (:SXnnn,V#).
 * Split from Command_S.cpp for clarity.  Called from Command_S() via case 'X'.
 */
#include "Command.h"
#include "CommandHelpers.h"
#include "ValueToString.h"

static bool parseYesNo(char c, bool& out) {
  if (c == 'y') { out = true;  return true; }
  if (c == 'n') { out = false; return true; }
  return false;
}

// =============================================================================
//   SX Sub-handlers  (static -- called only from Command_SX)
// =============================================================================

// ---- SX Alignment  :SXAn,V# ------------------------------------------------
static void Command_SX_Alignment()
{
  bool ok = false;
  switch (commandState.command[3])
  {
  case '0': break;
  case '1': break;
  case '2': break;
  case '3': break;
  case '4': break;
  case '5': break;
  case 'x':
    //GeoAlign.init();
    //GeoAlign.writeCoe();
    break;
  }
  replyValueSetShort(ok);
}

// ---- SX Encoders  :SXEn# ---------------------------------------------------
static void Command_SX_Encoders()
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
    AxisRef* ax = selectAxis(commandState.command[4]);
    if (ax && (strlen(&commandState.command[6]) > 0) && (strlen(&commandState.command[6]) < 7))
    {
      char* pEnd;
      unsigned long p = strtoul(&commandState.command[6], &pEnd, 10);
      if (p > 0 && p <= 360000 && !ax->encoder.isPulsePerDegreeFix)
      {
        ax->encoder.pulsePerDegree = 0.01 * p;
        XEEPROM.writeLong(getMountAddress(ax->eePulsePerDegree), p);
        ok = true;
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
    AxisRef* ax = selectAxis(commandState.command[4]);
    if (ax && strlen(&commandState.command[6]) == 1
      && (commandState.command[6] == '0' || commandState.command[6] == '1')
      && !ax->encoder.isReverseFix)
    {
      ax->encoder.reverse = commandState.command[6] == '1';
      XEEPROM.write(getMountAddress(ax->eeEncoderReverse), ax->encoder.reverse);
      ok = true;
    }
    replyValueSetShort(ok);
  }
  break;
  default:
    replyNothing();
    break;
  }
}

// ---- SX Refraction  :SXrn,V# -----------------------------------------------
static void Command_SX_Refraction()
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
      mount.refraction.setGoto(val);
      ok = true;
      break;
    case 't':
      mount.refraction.setTracking(val);
      ok = true;
      break;
    default:
      ok = false;
      break;
    }
  }
  replyValueSetShort(ok);
}

// ---- SX Rates  :SXRn,V# ----------------------------------------------------
static void Command_SX_Rates()
{
  switch (commandState.command[3])
  {
  case '0': // :SXR0,VVV# Set Rate for user defined rates
  case '1': // :SXR1,VVV#
  case '2': // :SXR2,VVV#
  case '3': // :SXR3,VVV#
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
}

// ---- SX Limits  :SXLn,V# ---------------------------------------------------
static void Command_SX_Limits()
{
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
    // :SXLS,sVV# set user defined distance from Pole
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
}

// ---- SX Time  :SXTn# -------------------------------------------------------
static void Command_SX_Time()
{
  switch (commandState.command[3])
  {
  case '0':
    //  :SXT0HH:MM:SS#
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
}

// ---- SX Motors  :SXMn,V# ---------------------------------------------------
static void Command_SX_Motors()
{
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
      AxisRef* ax = selectAxis(commandState.command[4]);
      ok = (ax != nullptr);
      if (ax)
      {
        ax->motor.backlashAmount = i;
        XEEPROM.writeUShort(getMountAddress(ax->eeBacklashAmount), ax->motor.backlashAmount);
        ax->stepper.setBacklash_inSteps(ax->motor.backlashAmount, ax->geo.stepsPerArcSecond);
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
      AxisRef* ax = selectAxis(commandState.command[4]);
      ok = (ax != nullptr);
      if (ax)
      {
        ax->motor.backlashRate = i;
        XEEPROM.write(getMountAddress(ax->eeBacklashRate), ax->motor.backlashRate);
        ax->stepper.SetBacklash_interval_Step(ax->motor.backlashRate);
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
    AxisRef* ax = ok ? selectAxis(commandState.command[4]) : nullptr;
    ok &= (ax != nullptr);
    ok &= strlen(&commandState.command[6]) > 1 && strlen(&commandState.command[6]) < 11;
    if (ok)
    {
      ok = false;
      i = strtoul(&commandState.command[6], NULL, 10);
      if (!ax->motor.isGearFix)
      {
        double fact = (double)(i) / ax->motor.gear;
        cli();
        ax->stepper.pos = fact * ax->stepper.pos;
        sei();
        ax->motor.gear = i;
        XEEPROM.writeULong(getMountAddress(ax->eeGear), i);
        ok = true;
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
    AxisRef* ax = ok ? selectAxis(commandState.command[4]) : nullptr;
    ok &= (ax != nullptr);
    ok &= (strlen(&commandState.command[6]) > 1) && (strlen(&commandState.command[6]) < 11);
    ok &= atoui2((char*)&commandState.command[6], &i);
    if (ok)
    {
      ok = false;
      if (!ax->motor.isStepRotFix)
      {
        double fact = (double)i / ax->motor.stepRot;
        cli();
        ax->stepper.pos = fact * ax->stepper.pos;
        sei();
        ax->motor.stepRot = i;
        XEEPROM.writeUShort(getMountAddress(ax->eeStepRot), i);
        ok = true;
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
    int i;
    bool ok = !mount.isSlewing();
    AxisRef* ax = ok ? selectAxis(commandState.command[4]) : nullptr;
    ok &= (ax != nullptr);
    ok &= strlen(&commandState.command[6]) == 1;
    ok &= atoi2(&commandState.command[6], &i);
    ok &= (i >= 1 && i < 9);
    if (ok)
    {
      ok = false;
      if (!ax->motor.isMicroFix)
      {
        double fact = pow(2., i - ax->motor.micro);
        cli();
        ax->stepper.pos = fact * ax->stepper.pos;
        sei();
        ax->motor.micro = i;
        ax->motor.driver.setMicrostep(ax->motor.micro);
        XEEPROM.write(getMountAddress(ax->eeMicro), ax->motor.micro);
        ok = true;
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
    int i;
    bool ok = false;
    AxisRef* ax = selectAxis(commandState.command[4]);
    if (ax && strlen(&commandState.command[6]) == 1
      && atoi2(&commandState.command[6], &i)
      && ((i >= 0) && (i < 2))
      && !ax->motor.isSilentFix)
    {
      XEEPROM.write(getMountAddress(ax->eeSilent), i);
      ok = true;
    }
    replyValueSetShort(ok);
  }
  break;
  case 'R':
  {
    // :SXMRn,V# Set Reverse rotation
    bool ok = false;
    AxisRef* ax = selectAxis(commandState.command[4]);
    if (ax && strlen(&commandState.command[6]) == 1
      && (commandState.command[6] == '0' || commandState.command[6] == '1')
      && !ax->motor.isReverseFix)
    {
      ax->motor.reverse = commandState.command[6] == '1';
      XEEPROM.write(getMountAddress(ax->eeReverse), ax->motor.reverse);
      ok = true;
    }
    replyValueSetShort(ok);
  }
  break;
  case 'c':
  case 'C':
  {
    // :SXMCn,V# / :SXMcn,V# Set Current (high / low)
    bool ok = false;
    unsigned int curr = (strtoul(&commandState.command[6], NULL, 10) / 100) * 100;
    if (curr >= 100)
    {
      AxisRef* ax = selectAxis(commandState.command[4]);
      if (ax && curr <= ax->motor.driver.getMaxCurrent())
      {
        if (commandState.command[3] == 'C')
        {
          if (!ax->motor.isHighCurrfix)
          {
            ax->motor.highCurr = curr;
            XEEPROM.write(getMountAddress(ax->eeHighCurr), ax->motor.highCurr / 100);
            ok = true;
          }
        }
        else
        {
          if (!ax->motor.isLowCurrfix)
          {
            ax->motor.lowCurr = curr;
            XEEPROM.write(getMountAddress(ax->eeLowCurr), ax->motor.lowCurr / 100);
            ax->motor.driver.setCurrent(ax->motor.lowCurr);
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
    AxisRef* ax = selectAxis(commandState.command[4]);
    bool ok = (ax != nullptr)
      && (strlen(&commandState.command[6]) > 1) && (strlen(&commandState.command[6]) < 5)
      && atoi2((char*)&commandState.command[6], &i)
      && ((i >= 0) && (i <= 127));
    if (ok)
    {
      i = i - 64;
      ax->motor.driver.setSG(i);
    }
    replyValueSetShort(ok);
  }
  break;
  default:
    replyNothing();
    break;
  }
}

// ---- SX Options  :SXOn,V# --------------------------------------------------
static void Command_SX_Options()
{
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
}

// =============================================================================
//   Command_SX  --  :SXnn#  dispatch to sub-handlers
// =============================================================================
void Command_SX() {
  switch (commandState.command[2])
  {
  case 'A': Command_SX_Alignment();  break;
  case 'E': Command_SX_Encoders();   break;
  case 'r': Command_SX_Refraction(); break;
  case 'R': Command_SX_Rates();      break;
  case 'L': Command_SX_Limits();     break;
  case 'T': Command_SX_Time();       break;
  case 'M': Command_SX_Motors();     break;
  case 'O': Command_SX_Options();    break;
  default:  replyNothing();          break;
  }
}

/** EEPROM and mount init: AutoinitEEPROM, initMount, initTransformation, motors/encoders. */
#include "MainUnit.h"
#include "Site.hpp"
#include "TeenAstoCustomizations.h"
#include "EEPROM_address.h"

uint8_t midx;
siteDefinition localSite;  // single definition — declared extern in Site.hpp

int getMountAddress(int address)
{
  return (int)EE_Mounts + (int)MountSize * (int)midx + address;
}

int getMountAddress(int address, int idx)
{
  return (int)EE_Mounts + (int)MountSize * (int)idx + address;
}

static const float pulsePerDegreedefault = 15.f;
static const EncoderSync EncoderSyncDefault = EncoderSync::ES_OFF;

void AutoinitEEPROM()
{
  long thisAutoInitKey = XEEPROM.readLong(EE_autoInitKey);
  if (thisAutoInitKey != initKey)
  {
    for (int i = 0; i < XEEPROM.length(); i++)
    {
      XEEPROM.write(i, 0);
    }
    // init the site information, lat/long/tz/name
    localSite.initdefault();
    writeDefaultMounts();

    // finally, stop the init from happening again
    XEEPROM.writeLong(EE_autoInitKey, initKey);
  }
}

void writeDefaultMounts()
{
  for (uint8_t i = 0; i < maxNumMount; i++)
  {
    midx = i;
    writeDefaultMount();
  }
  midx = 0;
}

void writeDefaultMount()
{
  writeDefaultMountName(midx);
  XEEPROM.write(getMountAddress(EE_mountType), MOUNT_TYPE_GEM);
  // init the min and max altitude
  mount.limits.minAlt = -10;
  mount.limits.maxAlt = 91;
  XEEPROM.write(getMountAddress(EE_minAlt), mount.limits.minAlt + 128);
  XEEPROM.write(getMountAddress(EE_maxAlt), mount.limits.maxAlt);
  XEEPROM.write(getMountAddress(EE_dpmE), 0);
  XEEPROM.write(getMountAddress(EE_dpmW), 0);
  XEEPROM.write(getMountAddress(EE_dup), (12 - 9) * 15);
  XEEPROM.write(getMountAddress(EE_dpmDistanceFromPole), 181);
  mount.limits.forceResetEELimit();


  // init the Park/Home status
  XEEPROM.write(getMountAddress(EE_parkSaved), false);
  XEEPROM.write(getMountAddress(EE_homeSaved), false);
  XEEPROM.write(getMountAddress(EE_parkStatus), PRK_UNPARKED);

  // init the Rate
  XEEPROM.write(getMountAddress(EE_Rate0), DefaultR0 * 100);
  XEEPROM.write(getMountAddress(EE_Rate1), DefaultR1);
  XEEPROM.write(getMountAddress(EE_Rate2), DefaultR2);
  XEEPROM.write(getMountAddress(EE_Rate3), DefaultR3);

  // init the default recentering speed
  XEEPROM.write(getMountAddress(EE_DefaultRate), 3);

  // init the default maxRate
  XEEPROM.writeUShort(getMountAddress(EE_maxRate), DefaultR4);

  // init degree for acceleration 1°
  XEEPROM.write(getMountAddress(EE_degAcc), (uint8_t)(1 * 10));

  // init the sidereal tracking rate, use this once - then issue the T+ and T- commands to fine tune
  // 1/16uS resolution timer, ticks per sidereal second
  XEEPROM.writeLong(getMountAddress(EE_siderealClockSpeed), mastersiderealClockSpeed * 16);

  // the transformation is not valid
  XEEPROM.write(getMountAddress(EE_Tvalid), 0);

  // reset flag for Tracking Correction
  XEEPROM.write(getMountAddress(EE_TC_Axis), 0);

  XEEPROM.writeLong(getMountAddress(EE_RA_Drift), 0);
  XEEPROM.writeLong(getMountAddress(EE_DEC_Drift), 0);

  XEEPROM.write(getMountAddress(EE_SlewSettleDuration), 0);

  XEEPROM.write(getMountAddress(EE_enableEncoderMotor), 2);
  writeDefaultEEPROMmotor();
  mount.refraction.writeDefaultToEEPROM();
}

void writeDefaultMountName(int i)
{
  sprintf(mount.config.identity.mountName[i], "Mount %d", i);
  XEEPROM.writeString(getMountAddress(EE_mountName), mount.config.identity.mountName[i], MountNameLen);
}

void initMount()
{
  long lval = 0;
  byte val = 0;


  midx = XEEPROM.read(EE_currentMount);
  if (midx > maxNumMount - 1)
  {
    writeDefaultMounts();
  }

  for (int i = 0; i < maxNumMount; i++)
  {
    bool ok = XEEPROM.readString(getMountAddress(EE_mountName, i), mount.config.identity.mountName[i], MountNameLen);
    if (!ok || strlen(mount.config.identity.mountName[i]) == 0)
    {
      writeDefaultMountName(i);
    }
  }

#ifdef D_mountType
  mount.config.identity.mountType = static_cast<MountType>(D_mountType);
  mount.config.identity.isMountTypeFix = true;
#else
  val = XEEPROM.read(getMountAddress(EE_mountType));
  if (val < 1 || val >  4)
  {
    XEEPROM.write(getMountAddress(EE_mountType), MOUNT_TYPE_GEM);
    mount.config.identity.mountType = MOUNT_TYPE_GEM;
  }
  else
  {
    mount.config.identity.mountType = static_cast<MountType>(val);
  }
  mount.config.identity.isMountTypeFix = false;
#endif
  if (mount.config.identity.mountType == MOUNT_TYPE_GEM)
    mount.config.identity.meridianFlip = FLIP_ALWAYS;
  else if (mount.config.identity.mountType == MOUNT_TYPE_FORK)
    mount.config.identity.meridianFlip = FLIP_NEVER;
  else if (mount.config.identity.mountType == MOUNT_TYPE_FORK_ALT)
    mount.config.identity.meridianFlip = FLIP_NEVER;
  else if (mount.config.identity.mountType == MOUNT_TYPE_ALTAZM)
    mount.config.identity.meridianFlip = FLIP_NEVER;
  // align
  if (mount.config.identity.mountType == MOUNT_TYPE_GEM)
    mount.alignment.maxAlignNumStar = 3;
  else if (mount.config.identity.mountType == MOUNT_TYPE_FORK)
    mount.alignment.maxAlignNumStar = 3;
  else if (mount.config.identity.mountType == MOUNT_TYPE_FORK_ALT)
    mount.alignment.maxAlignNumStar = 1;
  else if (mount.config.identity.mountType == MOUNT_TYPE_ALTAZM)
    mount.alignment.maxAlignNumStar = 3;
  mount.guiding.DegreesForAcceleration = 0.1 * EEPROM.read(getMountAddress(EE_degAcc));
  if (mount.guiding.DegreesForAcceleration == 0 || mount.guiding.DegreesForAcceleration > 25)
  {
    mount.guiding.DegreesForAcceleration = 3.0;
    XEEPROM.write(getMountAddress(EE_degAcc), (uint8_t)(mount.guiding.DegreesForAcceleration * 10));
  }
  // get the min. and max altitude
  mount.limits.minAlt = XEEPROM.read(getMountAddress(EE_minAlt)) - 128;
  mount.limits.maxAlt = XEEPROM.read(getMountAddress(EE_maxAlt));
  mount.limits.distanceFromPoleToKeepTrackingOn = XEEPROM.read(getMountAddress(EE_dpmDistanceFromPole));
  mount.limits.minutesPastMeridianGOTOE = round(((EEPROM.read(getMountAddress(EE_dpmE)) - 128) * 60.0) / 15.0);
  if (abs(mount.limits.minutesPastMeridianGOTOE) > 180)
    mount.limits.minutesPastMeridianGOTOE = 60;
  mount.limits.minutesPastMeridianGOTOW = round(((EEPROM.read(getMountAddress(EE_dpmW)) - 128) * 60.0) / 15.0);
  if (abs(mount.limits.minutesPastMeridianGOTOW) > 180)
    mount.limits.minutesPastMeridianGOTOW = 60;
  mount.limits.underPoleLimitGOTO = (double)EEPROM.read(getMountAddress(EE_dup)) / 10;
  if (mount.limits.underPoleLimitGOTO < 9 || mount.limits.underPoleLimitGOTO>12)
    mount.limits.underPoleLimitGOTO = 12;

  // initialize some fixed-point values
  mount.axes.staA1.fstep = 0.0;
  mount.axes.staA2.fstep = 0.0;

  mount.axes.staA1.target = mount.axes.geoA1.quaterRot;
  mount.axes.staA2.target = mount.axes.geoA2.quaterRot;
  mount.axes.staA1.fstep = mount.axes.geoA1.stepsPerCentiSecond;
  // Tracking and rate control
  if (mount.isAltAZ())
  {
    mount.tracking.trackComp = TC_BOTH;
  }
  else
  {
    val = XEEPROM.read(getMountAddress(EE_TC_Axis));
    mount.tracking.trackComp = val < 1 || val >  2 ? TC_RA : static_cast<TrackingCompensation>(val);
  }
  lval = XEEPROM.read(getMountAddress(EE_RA_Drift));
  mount.tracking.storedTrakingRateRA = lval < -50000 || lval > 50000 ? 0 : lval;
  lval = XEEPROM.read(getMountAddress(EE_DEC_Drift));
  mount.tracking.storedTrakingRateDEC = lval < -50000 || lval > 50000 ? 0 : lval;

  mount.refraction.readFromEEPROM();

  // get the site information from EEPROM
  localSite.ReadCurrentSiteDefinition();

  ReadEEPROMEncoderMotorMode();

  initencoder();
  initmotor(false);
  mount.syncEwithT();

#ifndef keepTrackingOnWhenFarFromPole
  mount.limits.distanceFromPoleToKeepTrackingOn = 181;
#endif
  
  val = XEEPROM.read(getMountAddress(EE_SlewSettleDuration));
  if (val > 20)
  {
    val = 0;
    XEEPROM.write(getMountAddress(EE_SlewSettleDuration), val);
  }
  mount.parkHome.slewSettleDuration = val;

}

void initTransformation(bool reset)
{
  float t11 = 0.f, t12 = 0.f, t13 = 0.f, t21 = 0.f, t22 = 0.f, t23 = 0.f, t31 = 0.f, t32 = 0.f, t33 = 0.f;
  mount.alignment.hasValid = false;
  mount.alignment.conv.clean();
  byte TvalidFromEEPROM = XEEPROM.read(getMountAddress(EE_Tvalid));

  if (TvalidFromEEPROM == 1 && reset)
  {
    XEEPROM.write(getMountAddress(EE_Tvalid), 0);
  }
  if (TvalidFromEEPROM == 1 && !reset)
  {
    t11 = XEEPROM.readFloat(getMountAddress(EE_T11));
    t12 = XEEPROM.readFloat(getMountAddress(EE_T12));
    t13 = XEEPROM.readFloat(getMountAddress(EE_T13));
    t21 = XEEPROM.readFloat(getMountAddress(EE_T21));
    t22 = XEEPROM.readFloat(getMountAddress(EE_T22));
    t23 = XEEPROM.readFloat(getMountAddress(EE_T23));
    t31 = XEEPROM.readFloat(getMountAddress(EE_T31));
    t32 = XEEPROM.readFloat(getMountAddress(EE_T32));
    t33 = XEEPROM.readFloat(getMountAddress(EE_T33));
    mount.alignment.conv.setT(t11, t12, t13, t21, t22, t23, t31, t32, t33);
    mount.alignment.conv.setTinvFromT();
    mount.alignment.hasValid = true;
  }
  else
  {
    if (mount.isAltAZ())
    {
      double rot = localSite.northHemisphere() ? 0 : M_PI;
      mount.alignment.conv.addReference(0, 0, rot, 0);
      mount.alignment.conv.addReference(M_PI_2, 0, rot + M_PI_2, 0);
      mount.alignment.conv.calculateThirdReference();
    }
    else
    {
      double Lat = *localSite.latitude() * DEG_TO_RAD;
      double sign = localSite.northHemisphere() ? 1 : -1;
      if (mount.refraction.forPole && abs(*localSite.latitude()) > 10)
      {
        Lat = abs(Lat);
        LA3::Topocentric2Apparent(Lat, mount.refrOptForPole());
        Lat *= sign;
      }
      if (mount.config.identity.mountType == MOUNT_TYPE_GEM)
      {
        Coord_HO HO1 = Coord_HO(0, 45 * DEG_TO_RAD, 90 * DEG_TO_RAD, false);
        Coord_EQ EQ1 = HO1.To_Coord_EQ(Lat);
        Coord_IN IN1 = Coord_IN(0, sign * EQ1.Dec(), sign * EQ1.Ha() - M_PI_2);

        Coord_HO HO2 = Coord_HO(0, 45 * DEG_TO_RAD, 270 * DEG_TO_RAD, false);
        Coord_EQ EQ2 = HO2.To_Coord_EQ(Lat);
        Coord_IN IN2 = Coord_IN(0, sign * EQ2.Dec(), sign * EQ2.Ha() - M_PI_2);

        mount.alignment.conv.addReference(HO1.direct_Az_S(), HO1.Alt(), IN1.Axis1_direct(), IN1.Axis2());
        mount.alignment.conv.addReference(HO2.direct_Az_S(), HO2.Alt(), IN2.Axis1_direct(), IN2.Axis2());
      }
      else
      {
        Coord_HO HO1 = Coord_HO(0, 45 * DEG_TO_RAD, 90 * DEG_TO_RAD, false);
        Coord_EQ EQ1 = HO1.To_Coord_EQ(Lat);
        Coord_IN IN1 = Coord_IN(0, sign * EQ1.Dec(), sign * EQ1.Ha());

        Coord_HO HO2 = Coord_HO(0, 45 * DEG_TO_RAD, 270 * DEG_TO_RAD, false);
        Coord_EQ EQ2 = HO2.To_Coord_EQ(Lat);
        Coord_IN IN2 = Coord_IN(0, sign * EQ2.Dec(), sign * EQ2.Ha());

        mount.alignment.conv.addReference(HO1.direct_Az_S(), HO1.Alt(), IN1.Axis1_direct(), IN1.Axis2());
        mount.alignment.conv.addReference(HO2.direct_Az_S(), HO2.Alt(), IN2.Axis1_direct(), IN2.Axis2());
      }


      mount.alignment.conv.calculateThirdReference();
    }
  }
}

void saveAlignModel()
{
  float t11 = 0.f, t12 = 0.f, t13 = 0.f, t21 = 0.f, t22 = 0.f, t23 = 0.f, t31 = 0.f, t32 = 0.f, t33 = 0.f;
  XEEPROM.write(getMountAddress(EE_Tvalid), mount.alignment.hasValid);
  if (mount.alignment.hasValid)
  {
    mount.alignment.conv.getT(t11, t12, t13, t21, t22, t23, t31, t32, t33);
  }
  XEEPROM.writeFloat(getMountAddress(EE_T11), t11);
  XEEPROM.writeFloat(getMountAddress(EE_T12), t12);
  XEEPROM.writeFloat(getMountAddress(EE_T13), t13);
  XEEPROM.writeFloat(getMountAddress(EE_T21), t21);
  XEEPROM.writeFloat(getMountAddress(EE_T22), t22);
  XEEPROM.writeFloat(getMountAddress(EE_T23), t23);
  XEEPROM.writeFloat(getMountAddress(EE_T31), t31);
  XEEPROM.writeFloat(getMountAddress(EE_T32), t32);
  XEEPROM.writeFloat(getMountAddress(EE_T33), t33);
}

void initCelestialPole()
{
  mount.axes.geoA1.poleDef = 0L;
  mount.axes.geoA2.poleDef = mount.axes.geoA2.quaterRot;
  if (mount.isAltAZ())
  {
    mount.axes.geoA1.LimMinAxis = -180;
    mount.axes.geoA1.LimMaxAxis = 180;
    mount.axes.geoA2.LimMinAxis = -20;
    mount.axes.geoA2.LimMaxAxis = 90;
  }
  else
  {
    mount.axes.geoA1.LimMinAxis = -180;
    mount.axes.geoA1.LimMaxAxis = 180;
    mount.axes.geoA2.LimMinAxis = -90;
    mount.axes.geoA2.LimMaxAxis = 270;
  }
}

void initmotor(bool deleteAlignment)
{
  readEEPROMmotor();
  mount.updateRatios(deleteAlignment, false);
  mount.motorsEncoders.motorA1.initMotor(static_cast<Driver::MOTORDRIVER>(AxisDriver), Axis1EnablePin, Axis1CSPin, Axis1DirPin, Axis1StepPin);
  mount.motorsEncoders.motorA2.initMotor(static_cast<Driver::MOTORDRIVER>(AxisDriver), Axis2EnablePin, Axis2CSPin, Axis2DirPin, Axis2StepPin);
  readEEPROMmotorCurrent();
  mount.motorsEncoders.motorA1.driver.setCurrent((unsigned int)mount.motorsEncoders.motorA1.lowCurr);
  mount.motorsEncoders.motorA2.driver.setCurrent((unsigned int)mount.motorsEncoders.motorA2.lowCurr);
}

void ReadEEPROMEncoderMotorMode()
{
#if HASEncoder
  byte val = XEEPROM.read(getMountAddress(EE_enableEncoderMotor));
  mount.motorsEncoders.enableMotor = bitRead(val, 0);
  mount.motorsEncoders.enableEncoder = bitRead(val, 1);
#else
  mount.motorsEncoders.enableEncoder = false;
  mount.motorsEncoders.enableMotor = true;
#endif // HASEncoder
}
void WriteEEPROMEncoderMotorMode()
{
#if HASEncoder
  byte val = 0;
  bitWrite(val, 0, mount.motorsEncoders.enableMotor);
  bitWrite(val, 1, mount.motorsEncoders.enableEncoder);
  XEEPROM.write(getMountAddress(EE_enableEncoderMotor), val);
#endif // HASEncoder
}

void initencoder()
{
#if HASEncoder
  if (mount.motorsEncoders.enableEncoder)
  {
    readEEPROMencoder();
    mount.motorsEncoders.encoderA1.init(EA1A, EA1B);
    mount.motorsEncoders.encoderA2.init(EA2A, EA2B);
  }
#endif
}

void readEEPROMmotorCurrent()
{
#ifdef D_motorA1lowCurr
  mount.motorsEncoders.motorA1.lowCurr = D_motorA1lowCurr;
  mount.motorsEncoders.motorA1.isLowCurrfix = true;
#else
  mount.motorsEncoders.motorA1.lowCurr = (unsigned int)XEEPROM.read(getMountAddress(EE_motorA1lowCurr)) * 100;
  if (mount.motorsEncoders.motorA1.lowCurr > mount.motorsEncoders.motorA1.driver.getMaxCurrent() || mount.motorsEncoders.motorA1.lowCurr < 200u)
  {
    mount.motorsEncoders.motorA1.lowCurr = 1000u;
    XEEPROM.write(getMountAddress(EE_motorA1lowCurr), 10u);
  }
  mount.motorsEncoders.motorA1.isLowCurrfix = false;
#endif 

#ifdef D_motorA1highCurr
  mount.motorsEncoders.motorA1.highCurr = D_motorA1highCurr;
  mount.motorsEncoders.motorA1.isHighCurrfix = true;
#else
  mount.motorsEncoders.motorA1.highCurr = (unsigned int)XEEPROM.read(getMountAddress(EE_motorA1highCurr)) * 100;
  if (mount.motorsEncoders.motorA1.highCurr > mount.motorsEncoders.motorA1.driver.getMaxCurrent() || mount.motorsEncoders.motorA1.highCurr < 200u)
  {
    mount.motorsEncoders.motorA1.highCurr = 1000u;
    XEEPROM.write(getMountAddress(EE_motorA1highCurr), 10u);
  }
  mount.motorsEncoders.motorA1.isHighCurrfix = false;
#endif


#ifdef D_motorA2lowCurr
  mount.motorsEncoders.motorA2.lowCurr = D_motorA2lowCurr;
  mount.motorsEncoders.motorA2.isLowCurrfix = true;
#else
  mount.motorsEncoders.motorA2.lowCurr = (unsigned int)XEEPROM.read(getMountAddress(EE_motorA2lowCurr)) * 100;
  if (mount.motorsEncoders.motorA2.lowCurr > mount.motorsEncoders.motorA2.driver.getMaxCurrent() || mount.motorsEncoders.motorA2.lowCurr < 200u)
  {
    mount.motorsEncoders.motorA2.lowCurr = 1000u;
    XEEPROM.write(getMountAddress(EE_motorA2lowCurr), 10u);
  }
  mount.motorsEncoders.motorA2.isLowCurrfix = false;
#endif 

#ifdef D_motorA2highCurr
  mount.motorsEncoders.motorA2.highCurr = D_motorA2highCurr;
  mount.motorsEncoders.motorA2.isHighCurrfix = true;
#else
  mount.motorsEncoders.motorA2.highCurr = (unsigned int)XEEPROM.read(getMountAddress(EE_motorA2highCurr)) * 100;
  if (mount.motorsEncoders.motorA2.highCurr > mount.motorsEncoders.motorA2.driver.getMaxCurrent() || mount.motorsEncoders.motorA2.highCurr < 200u)
  {
    mount.motorsEncoders.motorA2.highCurr = 1000u;
    XEEPROM.write(getMountAddress(EE_motorA2highCurr), 10u);
  }
  mount.motorsEncoders.motorA2.isHighCurrfix = false;
#endif

}

void readEEPROMmotor()
{
  mount.motorsEncoders.motorA1.backlashAmount = XEEPROM.readUShort(getMountAddress(EE_motorA1backlashAmount));
  if (mount.motorsEncoders.motorA1.backlashAmount > 999 || mount.motorsEncoders.motorA1.backlashAmount < 0)
  {
    mount.motorsEncoders.motorA1.backlashAmount = 0;
    XEEPROM.writeUShort(getMountAddress(EE_motorA1backlashAmount), 0);
  }

  mount.motorsEncoders.motorA2.backlashAmount = XEEPROM.readUShort(getMountAddress(EE_motorA2backlashAmount));
  if (mount.motorsEncoders.motorA2.backlashAmount > 999 || mount.motorsEncoders.motorA2.backlashAmount < 0)
  {
    mount.motorsEncoders.motorA2.backlashAmount = 0;
    XEEPROM.writeUShort(getMountAddress(EE_motorA2backlashAmount), 0);
  }

  mount.motorsEncoders.motorA1.backlashRate = XEEPROM.read(getMountAddress(EE_motorA1backlashRate));
  if (mount.motorsEncoders.motorA1.backlashRate < 16 || mount.motorsEncoders.motorA1.backlashRate > 64)
  {
    mount.motorsEncoders.motorA1.backlashRate = 16;
    XEEPROM.write(getMountAddress(EE_motorA1backlashAmount), 16);
  }

  mount.motorsEncoders.motorA2.backlashRate = XEEPROM.read(getMountAddress(EE_motorA2backlashRate));
  if (mount.motorsEncoders.motorA2.backlashRate < 16 || mount.motorsEncoders.motorA2.backlashRate > 64)
  {
    mount.motorsEncoders.motorA2.backlashRate = 16;
    XEEPROM.write(getMountAddress(EE_motorA2backlashAmount), 16);
  }

  //AXIS 1
#ifdef D_motorA1gear
  mount.motorsEncoders.motorA1.gear = D_motorA1gear;
  mount.motorsEncoders.motorA1.isGearFix = true;
#else
  mount.motorsEncoders.motorA1.gear = XEEPROM.readULong(getMountAddress(EE_motorA1gear));
  mount.motorsEncoders.motorA1.isGearFix = false;
#endif

#ifdef D_motorA1stepRot
  mount.motorsEncoders.motorA1.stepRot = D_motorA1stepRot;
  mount.motorsEncoders.motorA1.isStepRotFix = true;
#else
  mount.motorsEncoders.motorA1.stepRot = XEEPROM.readUShort(getMountAddress(EE_motorA1stepRot));
  mount.motorsEncoders.motorA1.isStepRotFix = false;
#endif 

#ifdef D_motorA1micro
  mount.motorsEncoders.motorA1.micro = D_motorA1micro;
  mount.motorsEncoders.motorA1.isMicroFix = true;
#else
  mount.motorsEncoders.motorA1.micro = XEEPROM.read(getMountAddress(EE_motorA1micro));
  if (mount.motorsEncoders.motorA1.micro > 8 || mount.motorsEncoders.motorA1.micro < 1)
  {
    mount.motorsEncoders.motorA1.micro = 4;
    XEEPROM.update(getMountAddress(EE_motorA1micro), 4u);
  }
  mount.motorsEncoders.motorA1.isMicroFix = false;
#endif

#ifdef D_motorA1reverse
  mount.motorsEncoders.motorA1.reverse = D_motorA1reverse;
  mount.motorsEncoders.motorA1.isReverseFix = true;
#else
  mount.motorsEncoders.motorA1.reverse = XEEPROM.read(getMountAddress(EE_motorA1reverse));
  mount.motorsEncoders.motorA1.isReverseFix = false;
#endif 



#ifdef D_motorA1silent
  mount.motorsEncoders.motorA1.silent = D_motorA1silent;
  mount.motorsEncoders.motorA1.isSilentFix = true;
#else
  mount.motorsEncoders.motorA1.silent = XEEPROM.read(getMountAddress(EE_motorA1silent));
  mount.motorsEncoders.motorA1.isSilentFix = false;
#endif

  //AXIS 2
#ifdef D_motorA2gear
  mount.motorsEncoders.motorA2.gear = D_motorA2gear;
  mount.motorsEncoders.motorA2.isGearFix = true;
#else
  mount.motorsEncoders.motorA2.gear = XEEPROM.readULong(getMountAddress(EE_motorA2gear));
  mount.motorsEncoders.motorA2.isGearFix = false;
#endif

#ifdef D_motorA2stepRot
  mount.motorsEncoders.motorA2.stepRot = D_motorA2stepRot;
  mount.motorsEncoders.motorA2.isStepRotFix = true;
#else
  mount.motorsEncoders.motorA2.stepRot = XEEPROM.readUShort(getMountAddress(EE_motorA2stepRot));
  mount.motorsEncoders.motorA2.isStepRotFix = false;
#endif 

#ifdef D_motorA2micro
  mount.motorsEncoders.motorA2.micro = D_motorA2micro;
  mount.motorsEncoders.motorA2.isMicroFix = true;
#else
  mount.motorsEncoders.motorA2.micro = XEEPROM.read(getMountAddress(EE_motorA2micro));
  if (mount.motorsEncoders.motorA2.micro > 8 || mount.motorsEncoders.motorA2.micro < 1)
  {
    mount.motorsEncoders.motorA2.micro = 4;
    XEEPROM.update(getMountAddress(EE_motorA2micro), 4u);
  }
  mount.motorsEncoders.motorA2.isMicroFix = false;
#endif

#ifdef D_motorA2reverse
  mount.motorsEncoders.motorA2.reverse = D_motorA2reverse;
  mount.motorsEncoders.motorA2.isReverseFix = true;
#else
  mount.motorsEncoders.motorA2.reverse = XEEPROM.read(getMountAddress(EE_motorA2reverse));
  mount.motorsEncoders.motorA2.isReverseFix = false;
#endif 


#ifdef D_motorA2silent
  mount.motorsEncoders.motorA2.silent = D_motorA2silent;
  mount.motorsEncoders.motorA2.isSilentFix = true;
#else
  mount.motorsEncoders.motorA2.silent = XEEPROM.read(getMountAddress(EE_motorA2silent));
  mount.motorsEncoders.motorA2.isSilentFix = false;
#endif

}

void writeDefaultEEPROMmotor()
{
  // init (clear) the backlash amounts
  XEEPROM.writeUShort(getMountAddress(EE_motorA1backlashAmount), 0);
  XEEPROM.write(getMountAddress(EE_motorA1backlashRate), 16);
  XEEPROM.writeULong(getMountAddress(EE_motorA1gear), 1800 * 1000);
  XEEPROM.writeUShort(getMountAddress(EE_motorA1stepRot), 200);
  XEEPROM.write(getMountAddress(EE_motorA1micro), 4);
  XEEPROM.write(getMountAddress(EE_motorA1reverse), 0);
  XEEPROM.write(getMountAddress(EE_motorA1highCurr), 10);
  XEEPROM.write(getMountAddress(EE_motorA1lowCurr), 10);
  XEEPROM.write(getMountAddress(EE_motorA1silent), 0);

  XEEPROM.writeUShort(getMountAddress(EE_motorA1backlashAmount), 0);
  XEEPROM.write(getMountAddress(EE_motorA2backlashRate), 16);
  XEEPROM.writeULong(getMountAddress(EE_motorA2gear), 1800 * 1000);
  XEEPROM.writeUShort(getMountAddress(EE_motorA2stepRot), 200);
  XEEPROM.write(getMountAddress(EE_motorA2micro), 4);
  XEEPROM.write(getMountAddress(EE_motorA2reverse), 0);
  XEEPROM.write(getMountAddress(EE_motorA2highCurr), 10);
  XEEPROM.write(getMountAddress(EE_motorA2lowCurr), 10);
  XEEPROM.write(getMountAddress(EE_motorA2silent), 0);
}



void readEEPROMencoder()
{

  //EncoderSync
  int val = XEEPROM.read(getMountAddress(EE_encoderSync));
  mount.motorsEncoders.EncodeSyncMode = static_cast<EncoderSync>(val);

  //AXIS 1

#ifdef D_encoderA1pulsePerDegree
  mount.motorsEncoders.encoderA1.pulseRot = D_encoderA1pulseRot;
  mount.motorsEncoders.encoderA1.isStepRotFix = true;
#else
  mount.motorsEncoders.encoderA1.pulsePerDegree = 0.01 * XEEPROM.readLong(getMountAddress(EE_encoderA1pulsePerDegree));
  if (mount.motorsEncoders.encoderA1.pulsePerDegree <= 0 || mount.motorsEncoders.encoderA1.pulsePerDegree > 3600)
  {
    XEEPROM.writeLong(getMountAddress(EE_encoderA1pulsePerDegree), 100 * pulsePerDegreedefault);
    mount.motorsEncoders.encoderA1.pulsePerDegree = pulsePerDegreedefault;
  }
  mount.motorsEncoders.encoderA1.isPulsePerDegreeFix = false;
#endif 


#ifdef D_encoderA1reverse
  mount.motorsEncoders.encoderA1.reverse = D_encoderA1reverse;
  mount.motorsEncoders.encoderA1.isReverseFix = true;
#else
  mount.motorsEncoders.encoderA1.reverse = XEEPROM.read(getMountAddress(EE_encoderA1reverse));
  mount.motorsEncoders.encoderA1.isReverseFix = false;
#endif 

  //AXIS 2

#ifdef D_encoderA2pulsePerDegree
  mount.motorsEncoders.encoderA2.pulseRot = D_encoderA2pulseRot;
  mount.motorsEncoders.encoderA2.isStepRotFix = true;
#else
  mount.motorsEncoders.encoderA2.pulsePerDegree = 0.01 * XEEPROM.readLong(getMountAddress(EE_encoderA2pulsePerDegree));
  if (mount.motorsEncoders.encoderA2.pulsePerDegree <= 0 || mount.motorsEncoders.encoderA2.pulsePerDegree > 3600)
  {
    XEEPROM.writeLong(getMountAddress(EE_encoderA2pulsePerDegree), 100 * pulsePerDegreedefault);
    mount.motorsEncoders.encoderA2.pulsePerDegree = pulsePerDegreedefault;
  }
  mount.motorsEncoders.encoderA2.isPulsePerDegreeFix = false;
#endif 

#ifdef D_encoderA2reverse
  mount.motorsEncoders.encoderA2.reverse = D_encoderA2reverse;
  mount.motorsEncoders.encoderA2.isReverseFix = true;
#else
  mount.motorsEncoders.encoderA2.reverse = XEEPROM.read(getMountAddress(EE_encoderA2reverse));
  mount.motorsEncoders.encoderA2.isReverseFix = false;
#endif 

}

void writeDefaultEEPROMencoder()
{
  XEEPROM.write(getMountAddress(EE_encoderSync), EncoderSyncDefault);
  XEEPROM.writeLong(getMountAddress(EE_encoderA1pulsePerDegree), 100 * pulsePerDegreedefault);
  XEEPROM.write(getMountAddress(EE_encoderA1reverse), 0);
  XEEPROM.writeLong(getMountAddress(EE_encoderA2pulsePerDegree), 100 * pulsePerDegreedefault);
  XEEPROM.write(getMountAddress(EE_encoderA2reverse), 0);

}

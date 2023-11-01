#include "TeenAstoCustomizations.h"

static const float pulsePerDegreedefault = 15;
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
  minAlt = -10;
  maxAlt = 91;
  XEEPROM.write(getMountAddress(EE_minAlt), minAlt + 128);
  XEEPROM.write(getMountAddress(EE_maxAlt), maxAlt);
  XEEPROM.write(getMountAddress(EE_dpmE), 0);
  XEEPROM.write(getMountAddress(EE_dpmW), 0);
  XEEPROM.write(getMountAddress(EE_dup), (12 - 9) * 15);
  XEEPROM.write(getMountAddress(EE_dpmDistanceFromPole), 181);
  force_reset_EE_Limit();


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

  // reset flag for Apparent Pole
  writeDefaultEEPROMmotor();
  doesRefraction.writeDefaultToEEPROM();
}

void writeDefaultMountName(int i)
{
  sprintf(mountName[i], "Mount %d", i);
  XEEPROM.writeString(getMountAddress(EE_mountName), mountName[i], MountNameLen);
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
    bool ok = XEEPROM.readString(getMountAddress(EE_mountName, i), mountName[i], MountNameLen);
    if (!ok || strlen(mountName[i]) == 0)
    {
      writeDefaultMountName(i);
    }
  }

#ifdef D_mountType
  mountType = static_cast<Mount>(D_mountType);
  isMountTypeFix = true;
#else
  val = XEEPROM.read(getMountAddress(EE_mountType));
  if (val < 1 || val >  4)
  {
    XEEPROM.write(getMountAddress(EE_mountType), MOUNT_TYPE_GEM);
    mountType = MOUNT_TYPE_GEM;
  }
  else
  {
    mountType = static_cast<Mount>(val);
  }
  isMountTypeFix = false;
#endif
  if (mountType == MOUNT_TYPE_GEM)
    meridianFlip = FLIP_ALWAYS;
  else if (mountType == MOUNT_TYPE_FORK)
    meridianFlip = FLIP_NEVER;
  else if (mountType == MOUNT_TYPE_FORK_ALT)
    meridianFlip = FLIP_NEVER;
  else if (mountType == MOUNT_TYPE_ALTAZM)
    meridianFlip = FLIP_NEVER;
  // align
  if (mountType == MOUNT_TYPE_GEM)
    maxAlignNumStar = 3;
  else if (mountType == MOUNT_TYPE_FORK)
    maxAlignNumStar = 3;
  else if (mountType == MOUNT_TYPE_FORK_ALT)
    maxAlignNumStar = 1;
  else if (mountType == MOUNT_TYPE_ALTAZM)
    maxAlignNumStar = 3;
  DegreesForAcceleration = 0.1 * EEPROM.read(getMountAddress(EE_degAcc));
  if (DegreesForAcceleration == 0 || DegreesForAcceleration > 25)
  {
    DegreesForAcceleration = 3.0;
    XEEPROM.write(getMountAddress(EE_degAcc), (uint8_t)(DegreesForAcceleration * 10));
  }
  // get the min. and max altitude
  minAlt = XEEPROM.read(getMountAddress(EE_minAlt)) - 128;
  maxAlt = XEEPROM.read(getMountAddress(EE_maxAlt));
  distanceFromPoleToKeepTrackingOn = XEEPROM.read(getMountAddress(EE_dpmDistanceFromPole));
  minutesPastMeridianGOTOE = round(((EEPROM.read(getMountAddress(EE_dpmE)) - 128) * 60.0) / 15.0);
  if (abs(minutesPastMeridianGOTOE) > 180)
    minutesPastMeridianGOTOE = 60;
  minutesPastMeridianGOTOW = round(((EEPROM.read(getMountAddress(EE_dpmW)) - 128) * 60.0) / 15.0);
  if (abs(minutesPastMeridianGOTOW) > 180)
    minutesPastMeridianGOTOW = 60;
  underPoleLimitGOTO = (double)EEPROM.read(getMountAddress(EE_dup)) / 10;
  if (underPoleLimitGOTO < 9 || underPoleLimitGOTO>12)
    underPoleLimitGOTO = 12;

  // initialize some fixed-point values
  //guideA1.amount = 0;
  //guideA2.amount = 0;

  staA1.fstep = 0;
  staA2.fstep = 0;

  staA1.target = geoA1.quaterRot;
  staA2.target = geoA2.quaterRot;
  staA1.fstep = geoA1.stepsPerCentiSecond;
  // Tracking and rate control
  val = XEEPROM.read(getMountAddress(EE_TC_Axis));
  tc = val < 0 || val >  2 ? TC_NONE : static_cast<TrackingCompensation>(val);
  lval = XEEPROM.read(getMountAddress(EE_RA_Drift));
  storedTrakingRateRA = lval < -50000 || lval > 50000 ? 0 : lval;
  lval = XEEPROM.read(getMountAddress(EE_DEC_Drift));
  storedTrakingRateDEC = lval < -50000 || lval > 50000 ? 0 : lval;

  doesRefraction.readFromEEPROM();

  // get the site information from EEPROM
  localSite.ReadCurrentSiteDefinition();
  initmotor(false);
  initencoder();
  syncEwithT();

#ifndef keepTrackingOnWhenFarFromPole
  distanceFromPoleToKeepTrackingOn = 181;
#endif

}

void initTransformation(bool reset)
{
  float t11 = 0, t12 = 0, t13 = 0, t21 = 0, t22 = 0, t23 = 0, t31 = 0, t32 = 0, t33 = 0;
  hasStarAlignment = false;
  alignment.clean();
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
    alignment.setT(t11, t12, t13, t21, t22, t23, t31, t32, t33);
    alignment.setTinvFromT();
    hasStarAlignment = true;
  }
  else
  {
    if (isAltAZ())
    {
      double rot = localSite.northHemisphere() ? 0 : M_PI;
      alignment.addReference(0, 0, rot, 0);
      alignment.addReference(0, M_PI_2, rot, M_PI_2);
      alignment.calculateThirdReference();
    }
    else
    {
      double Lat = *localSite.latitude() * DEG_TO_RAD;
      double sign = localSite.northHemisphere() ? 1 : -1;
      if (doesRefraction.forPole && abs(*localSite.latitude()) > 10)
      {
        Lat = abs(Lat);
        LA3::Topocentric2Apparent(Lat, RefrOptForPole());
        Lat *= sign;
      }
      if (mountType == MOUNT_TYPE_GEM)
      {
        Coord_HO HO1 = Coord_HO(0, 45 * DEG_TO_RAD, 90 * DEG_TO_RAD, false);
        Coord_EQ EQ1 = HO1.To_Coord_EQ(Lat);
        Coord_IN IN1 = Coord_IN(0, sign * EQ1.Dec(), sign * EQ1.Ha() - M_PI_2);

        Coord_HO HO2 = Coord_HO(0, 45 * DEG_TO_RAD, 270 * DEG_TO_RAD, false);
        Coord_EQ EQ2 = HO2.To_Coord_EQ(Lat);
        Coord_IN IN2 = Coord_IN(0, sign * EQ2.Dec(), sign * EQ2.Ha() - M_PI_2);
        alignment.addReference(HO1.Az(), HO1.Alt(), IN1.Axis1(), IN1.Axis2());
        alignment.addReference(HO2.Az(), HO2.Alt(), IN2.Axis1(), IN2.Axis2());
      }
      else
      {
        Coord_HO HO1 = Coord_HO(0, 45 * DEG_TO_RAD, 90 * DEG_TO_RAD, false);
        Coord_EQ EQ1 = HO1.To_Coord_EQ(Lat);
        Coord_IN IN1 = Coord_IN(0, sign * EQ1.Dec(), sign * EQ1.Ha());

        Coord_HO HO2 = Coord_HO(0, 45 * DEG_TO_RAD, 270 * DEG_TO_RAD, false);
        Coord_EQ EQ2 = HO2.To_Coord_EQ(Lat);
        Coord_IN IN2 = Coord_IN(0, sign * EQ2.Dec(), sign * EQ2.Ha());
        alignment.addReference(HO1.Az(), HO1.Alt(), IN1.Axis1(), IN1.Axis2());
        alignment.addReference(HO2.Az(), HO2.Alt(), IN2.Axis1(), IN2.Axis2());
      }


      alignment.calculateThirdReference();
    }
  }
}

void initCelestialPole()
{
  geoA1.poleDef = 0L;
  geoA2.poleDef = geoA2.quaterRot;
  if (isAltAZ())
  {
    geoA1.LimMinAxis = -180;
    geoA1.LimMaxAxis = 180;
    geoA2.LimMinAxis = -20;
    geoA2.LimMaxAxis = 90;
  }
  else
  {
    geoA1.LimMinAxis = -180;
    geoA1.LimMaxAxis = 180;
    geoA2.LimMinAxis = -90;
    geoA2.LimMaxAxis = 270;
  }
}

void initmotor(bool deleteAlignment)
{
  readEEPROMmotor();
  updateRatios(deleteAlignment, false);
  motorA1.initMotor(static_cast<Driver::MOTORDRIVER>(AxisDriver), Axis1EnablePin, Axis1CSPin, Axis1DirPin, Axis1StepPin);
  motorA2.initMotor(static_cast<Driver::MOTORDRIVER>(AxisDriver), Axis2EnablePin, Axis2CSPin, Axis2DirPin, Axis2StepPin);
  readEEPROMmotorCurrent();
  motorA1.driver.setCurrent((unsigned int)motorA1.lowCurr);
  motorA2.driver.setCurrent((unsigned int)motorA2.lowCurr);
}

void initencoder()
{
  readEEPROMencoder();
#if HASEncoder
  encoderA1.init(EA1A, EA1B);
  encoderA2.init(EA2A, EA2B);
#endif
}

void readEEPROMmotorCurrent()
{
#ifdef D_motorA1lowCurr
  motorA1.lowCurr = D_motorA1lowCurr;
  motorA1.isLowCurrfix = true;
#else
  motorA1.lowCurr = (unsigned int)XEEPROM.read(getMountAddress(EE_motorA1lowCurr)) * 100;
  if (motorA1.lowCurr > motorA1.driver.getMaxCurrent() || motorA1.lowCurr < 200u)
  {
    motorA1.lowCurr = 1000u;
    XEEPROM.write(getMountAddress(EE_motorA1lowCurr), 10u);
  }
  motorA1.isLowCurrfix = false;
#endif 

#ifdef D_motorA1highCurr
  motorA1.highCurr = D_motorA1highCurr;
  motorA1.isHighCurrfix = true;
#else
  motorA1.highCurr = (unsigned int)XEEPROM.read(getMountAddress(EE_motorA1highCurr)) * 100;
  if (motorA1.highCurr > motorA1.driver.getMaxCurrent() || motorA1.highCurr < 200u)
  {
    motorA1.highCurr = 1000u;
    XEEPROM.write(getMountAddress(EE_motorA1highCurr), 10u);
  }
  motorA1.isHighCurrfix = false;
#endif


#ifdef D_motorA2lowCurr
  motorA2.lowCurr = D_motorA2lowCurr;
  motorA2.isLowCurrfix = true;
#else
  motorA2.lowCurr = (unsigned int)XEEPROM.read(getMountAddress(EE_motorA2lowCurr)) * 100;
  if (motorA2.lowCurr > motorA2.driver.getMaxCurrent() || motorA2.lowCurr < 200u)
  {
    motorA2.lowCurr = 1000u;
    XEEPROM.write(getMountAddress(EE_motorA2lowCurr), 10u);
  }
  motorA2.isLowCurrfix = false;
#endif 

#ifdef D_motorA2highCurr
  motorA2.highCurr = D_motorA2highCurr;
  motorA2.isHighCurrfix = true;
#else
  motorA2.highCurr = (unsigned int)XEEPROM.read(getMountAddress(EE_motorA2highCurr)) * 100;
  if (motorA2.highCurr > motorA2.driver.getMaxCurrent() || motorA2.highCurr < 200u)
  {
    motorA2.highCurr = 1000u;
    XEEPROM.write(getMountAddress(EE_motorA2highCurr), 10u);
  }
  motorA2.isHighCurrfix = false;
#endif

}

void readEEPROMmotor()
{
  motorA1.backlashAmount = XEEPROM.readUShort(getMountAddress(EE_motorA1backlashAmount));
  if (motorA1.backlashAmount > 999 || motorA1.backlashAmount < 0)
  {
    motorA1.backlashAmount = 0;
    XEEPROM.writeUShort(getMountAddress(EE_motorA1backlashAmount), 0);
  }

  motorA2.backlashAmount = XEEPROM.readUShort(getMountAddress(EE_motorA2backlashAmount));
  if (motorA2.backlashAmount > 999 || motorA2.backlashAmount < 0)
  {
    motorA2.backlashAmount = 0;
    XEEPROM.writeUShort(getMountAddress(EE_motorA2backlashAmount), 0);
  }

  motorA1.backlashRate = XEEPROM.read(getMountAddress(EE_motorA1backlashRate));
  if (motorA1.backlashRate < 16 || motorA1.backlashRate > 64)
  {
    motorA1.backlashRate = 16;
    XEEPROM.write(getMountAddress(EE_motorA1backlashAmount), 16);
  }

  motorA2.backlashRate = XEEPROM.read(getMountAddress(EE_motorA2backlashRate));
  if (motorA2.backlashRate < 16 || motorA2.backlashRate > 64)
  {
    motorA2.backlashRate = 16;
    XEEPROM.write(getMountAddress(EE_motorA2backlashAmount), 16);
  }

  //AXIS 1
#ifdef D_motorA1gear
  motorA1.gear = D_motorA1gear;
  motorA1.isGearFix = true;
#else
  motorA1.gear = XEEPROM.readULong(getMountAddress(EE_motorA1gear));
  motorA1.isGearFix = false;
#endif

#ifdef D_motorA1stepRot
  motorA1.stepRot = D_motorA1stepRot;
  motorA1.isStepRotFix = true;
#else
  motorA1.stepRot = XEEPROM.readUShort(getMountAddress(EE_motorA1stepRot));
  motorA1.isStepRotFix = false;
#endif 

#ifdef D_motorA1micro
  motorA1.micro = D_motorA1micro;
  motorA1.isMicroFix = true;
#else
  motorA1.micro = XEEPROM.read(getMountAddress(EE_motorA1micro));
  if (motorA1.micro > 8 || motorA1.micro < 1)
  {
    motorA1.micro = 4;
    XEEPROM.update(getMountAddress(EE_motorA1micro), 4u);
  }
  motorA1.isMicroFix = false;
#endif

#ifdef D_motorA1reverse
  motorA1.reverse = D_motorA1reverse;
  motorA1.isReverseFix = true;
#else
  motorA1.reverse = XEEPROM.read(getMountAddress(EE_motorA1reverse));
  motorA1.isReverseFix = false;
#endif 



#ifdef D_motorA1silent
  motorA1.silent = D_motorA1silent;
  motorA1.isSilentFix = true;
#else
  motorA1.silent = XEEPROM.read(getMountAddress(EE_motorA1silent));
  motorA1.isSilentFix = false;
#endif

  //AXIS 2
#ifdef D_motorA2gear
  motorA2.gear = D_motorA2gear;
  motorA2.isGearFix = true;
#else
  motorA2.gear = XEEPROM.readULong(getMountAddress(EE_motorA2gear));
  motorA2.isGearFix = false;
#endif

#ifdef D_motorA2stepRot
  motorA2.stepRot = D_motorA2stepRot;
  motorA2.isStepRotFix = true;
#else
  motorA2.stepRot = XEEPROM.readUShort(getMountAddress(EE_motorA2stepRot));
  motorA2.isStepRotFix = false;
#endif 

#ifdef D_motorA2micro
  motorA2.micro = D_motorA2micro;
  motorA2.isMicroFix = true;
#else
  motorA2.micro = XEEPROM.read(getMountAddress(EE_motorA2micro));
  if (motorA2.micro > 8 || motorA2.micro < 1)
  {
    motorA2.micro = 4;
    XEEPROM.update(getMountAddress(EE_motorA2micro), 4u);
  }
  motorA2.isMicroFix = false;
#endif

#ifdef D_motorA2reverse
  motorA2.reverse = D_motorA2reverse;
  motorA2.isReverseFix = true;
#else
  motorA2.reverse = XEEPROM.read(getMountAddress(EE_motorA2reverse));
  motorA2.isReverseFix = false;
#endif 


#ifdef D_motorA2silent
  motorA2.silent = D_motorA2silent;
  motorA2.isSilentFix = true;
#else
  motorA2.silent = XEEPROM.read(getMountAddress(EE_motorA2silent));
  motorA2.isSilentFix = false;
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
  EncodeSyncMode = static_cast<EncoderSync>(val);

  //AXIS 1

#ifdef D_encoderA1pulsePerDegree
  encoderA1.pulseRot = D_encoderA1pulseRot;
  encoderA1.isStepRotFix = true;
#else
  encoderA1.pulsePerDegree = 0.01 * XEEPROM.readLong(getMountAddress(EE_encoderA1pulsePerDegree));
  if (encoderA1.pulsePerDegree <= 0 || encoderA1.pulsePerDegree > 3600)
  {
    XEEPROM.writeLong(getMountAddress(EE_encoderA1pulsePerDegree), 100 * pulsePerDegreedefault);
    encoderA1.pulsePerDegree = pulsePerDegreedefault;
  }
  encoderA1.isPulsePerDegreeFix = false;
#endif 


#ifdef D_encoderA1reverse
  encoderA1.reverse = D_encoderA1reverse;
  encoderA1.isReverseFix = true;
#else
  encoderA1.reverse = XEEPROM.read(getMountAddress(EE_encoderA1reverse));
  encoderA1.isReverseFix = false;
#endif 

  //AXIS 2

#ifdef D_encoderA2pulsePerDegree
  encoderA2.pulseRot = D_encoderA2pulseRot;
  encoderA2.isStepRotFix = true;
#else
  encoderA2.pulsePerDegree = 0.01 * XEEPROM.readLong(getMountAddress(EE_encoderA2pulsePerDegree));
  if (encoderA2.pulsePerDegree <= 0 || encoderA2.pulsePerDegree > 3600)
  {
    XEEPROM.writeLong(getMountAddress(EE_encoderA2pulsePerDegree), 100 * pulsePerDegreedefault);
    encoderA2.pulsePerDegree = pulsePerDegreedefault;
  }
  encoderA2.isPulsePerDegreeFix = false;
#endif 

#ifdef D_encoderA2reverse
  encoderA2.reverse = D_encoderA2reverse;
  encoderA2.isReverseFix = true;
#else
  encoderA2.reverse = XEEPROM.read(getMountAddress(EE_encoderA2reverse));
  encoderA2.isReverseFix = false;
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
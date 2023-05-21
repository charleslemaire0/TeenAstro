// EEPROM automatic initialization
#include "Global.h"


void writeDefaultMountName(int i)
{
  snprintf(mountNames[i], MountNameLen, "Mount %d", i);
  XEEPROM.writeString(getMountAddress(EE_mountName), mountNames[i], MountNameLen);
}

void writeDefaultMount()
{
  writeDefaultMountName(currentMount);
  XEEPROM.write(getMountAddress(EE_mountType), MOUNT_TYPE_GEM);
  // init the min and max altitude
  int minAlt = -10;
  int maxAlt = 91;
  XEEPROM.write(getMountAddress(EE_minAlt), minAlt + 128);
  XEEPROM.write(getMountAddress(EE_maxAlt), maxAlt);
  XEEPROM.write(getMountAddress(EE_dpmE), 0);
  XEEPROM.write(getMountAddress(EE_dpmW), 0);
  XEEPROM.write(getMountAddress(EE_dup), (12 - 9) * 15);
  XEEPROM.writeInt(getMountAddress(EE_minAxis1), 3600);
  XEEPROM.writeInt(getMountAddress(EE_maxAxis1), 3600);
  XEEPROM.writeInt(getMountAddress(EE_minAxis2), 3600);
  XEEPROM.writeInt(getMountAddress(EE_maxAxis2), 3600);

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
  XEEPROM.writeInt(getMountAddress(EE_maxRate), DefaultR4);

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



void writeDefaultMounts()
{
  for (uint8_t i = 0; i < maxNumMounts; i++)
  {
    currentMount = i;
    writeDefaultMount();
  }
  currentMount = 0;
}



void initMount()
{
  long lval = 0;

  currentMount = XEEPROM.read(EE_currentMount);

  if (currentMount > maxNumMounts - 1)
  {
    writeDefaultMounts();
  }
  for (int i = 0; i < maxNumMounts; i++)
  {
    bool ok = XEEPROM.readString(getMountAddress(EE_mountName, i), mountNames[i], MountNameLen);
    if (!ok || strlen(mountNames[i]) == 0)
    {
      writeDefaultMountName(i);
    }
  }

  byte val = XEEPROM.read(getMountAddress(EE_mountType));

  switch(val)
  {
    case 1:
      mount.init(MOUNT_TYPE_GEM);
    break;
    case 2:
      mount.init(MOUNT_TYPE_FORK);
    break;
    case 3:
      mount.init(MOUNT_TYPE_ALTAZM);
    break;
    case 4:
      mount.init(MOUNT_TYPE_FORK_ALT);
    break;
    default:
      mount.init(MOUNT_TYPE_GEM);
  }
  mount.DegreesForAcceleration = 0.1*EEPROM.read(getMountAddress(EE_degAcc));
  if (mount.DegreesForAcceleration == 0 || mount.DegreesForAcceleration > 25)
  {
    mount.DegreesForAcceleration = 3.0;
    XEEPROM.write(EE_degAcc, (uint8_t)(mount.DegreesForAcceleration * 10));
  }
  // get the min. and max altitude
  limits.minAlt = XEEPROM.read(getMountAddress(EE_minAlt)) - 128;
  limits.maxAlt = XEEPROM.read(getMountAddress(EE_maxAlt));
  limits.minutesPastMeridianGOTOE = round(((XEEPROM.read(getMountAddress(EE_dpmE)) - 128)*60.0) / 15.0);
  if (abs(limits.minutesPastMeridianGOTOE) > 180)
    limits.minutesPastMeridianGOTOE = 60;
  limits.minutesPastMeridianGOTOW = round(((XEEPROM.read(getMountAddress(EE_dpmW)) - 128)*60.0) / 15.0);
  if (abs(limits.minutesPastMeridianGOTOW) > 180)
    limits.minutesPastMeridianGOTOW = 60;
  limits.underPoleLimitGOTO = (double)XEEPROM.read(getMountAddress(EE_dup)) / 10;
  if (limits.underPoleLimitGOTO < 9 || limits.underPoleLimitGOTO>12)
    limits.underPoleLimitGOTO = 12;
  
  // initialize some fixed-point values
  guideA1.amount = 0;
  guideA2.amount = 0;

  // Tracking and rate control
  val = XEEPROM.read(EE_TC_Axis);
//  tc = val < 0 || val >  2 ? TC_NONE : static_cast<TrackingCompensation>(val);
  lval = XEEPROM.read(EE_RA_Drift);
  storedTrackingRateRA  = lval < -50000 || lval > 50000? 0 :lval;
  lval = XEEPROM.read(EE_DEC_Drift);
  storedTrackingRateDEC = lval < -50000 || lval > 50000 ? 0 : lval;
  mount.clk5160 = XEEPROM.readLong(getMountAddress(EE_clk5160));
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
    if (isAltAz())
    {
      if (*localSite.latitude() > 0)
      {
        alignment.addReferenceDeg(0, 0, 180, 0);
        alignment.addReferenceDeg(0, 90, 180, 90);
      }
      else
      {
        alignment.addReferenceDeg(0, 0, 0, 0);
        alignment.addReferenceDeg(0, 90, 0, 90);
      }
      alignment.calculateThirdReference();
    }
    else
    {
      double ha, dec;
      double cosLat = *localSite.cosLat();
      double sinLat = *localSite.sinLat();
      if (doesRefraction.forPole && abs(*localSite.latitude() > 10))
      {
        double val = abs(*localSite.latitude());
        Topocentric2Apparent(&val);
        if (*localSite.latitude() < 0)
          val = -val;
        cosLat = cos(val / Rad);
        sinLat = sin(val / Rad);
      }
      HorTopoToEqu(90., 45., &ha, &dec, &cosLat, &sinLat);
      alignment.addReferenceDeg(90., 45., ha, dec);
      HorTopoToEqu(270., 45., &ha, &dec, &cosLat, &sinLat);
      alignment.addReferenceDeg(270., 45., ha, dec);
      alignment.calculateThirdReference();
    }
  }
}


void readEEPROMmotor()
{
  mount.backlashA1.inSeconds = XEEPROM.readInt(getMountAddress(EE_backlashAxis1));
  mount.backlashA1.movedSteps = 0;
  motorA1.gear = XEEPROM.readULong(getMountAddress(EE_motorA1gear));
  motorA1.stepRot = XEEPROM.readInt(getMountAddress(EE_motorA1stepRot));

  motorA1.micro = XEEPROM.read(getMountAddress(EE_motorA1micro));
  if (motorA1.micro > 8 || motorA1.micro < 1)
  {
    motorA1.micro = 4;
    XEEPROM.write(getMountAddress(EE_motorA1micro), 4u);
  }

  motorA1.reverse = XEEPROM.read(getMountAddress(EE_motorA1reverse));

  motorA1.lowCurr = (unsigned int)XEEPROM.read(getMountAddress(EE_motorA1lowCurr)) * 100;
  if (motorA1.lowCurr > 2800u || motorA1.lowCurr < 200u)
  {
    motorA1.lowCurr = 1000u;
    XEEPROM.write(EE_motorA1lowCurr, 10u);
  }

  motorA1.highCurr = (unsigned int)XEEPROM.read(getMountAddress(EE_motorA1highCurr)) * 100;
  if (motorA1.highCurr > 2800u || motorA1.highCurr < 200u)
  {
    motorA1.highCurr = 1000u;
    XEEPROM.write(getMountAddress(EE_motorA1highCurr), 10u);
  }

  motorA1.silent = XEEPROM.read(getMountAddress(EE_motorA1silent));
  mount.backlashA2.inSeconds = XEEPROM.readInt(getMountAddress(EE_backlashAxis2));
  mount.backlashA2.movedSteps = 0;
  motorA2.gear = XEEPROM.readULong(getMountAddress(EE_motorA2gear));
  motorA2.stepRot = XEEPROM.readInt(getMountAddress(EE_motorA2stepRot));
  motorA2.micro = XEEPROM.read(getMountAddress(EE_motorA2micro));

  // do not allow microsteps higher than 256 (2^8)
  if (motorA2.micro > 8 || motorA2.micro < 1)
  {
    motorA2.micro = 4;
    XEEPROM.write(EE_motorA2micro, 4);
  }
  motorA2.reverse = XEEPROM.read(getMountAddress(EE_motorA2reverse));

  motorA2.lowCurr = (unsigned int)XEEPROM.read(getMountAddress(EE_motorA2lowCurr)) * 100;
  if (motorA2.lowCurr > 2800u || motorA2.lowCurr < 200u)
  {
    motorA2.lowCurr = 1000u;
    XEEPROM.write(getMountAddress(EE_motorA2lowCurr), 10u);
  }

  motorA2.highCurr = (unsigned int)XEEPROM.read(getMountAddress(EE_motorA2highCurr)) * 100;
  if (motorA2.highCurr > 2800u || motorA2.highCurr < 200u)
  {
    motorA2.highCurr = 1000u;
    XEEPROM.write(getMountAddress(EE_motorA2highCurr), 10u);
  }

  motorA2.silent = XEEPROM.read(getMountAddress(EE_motorA2silent));
}


void writeDefaultEEPROMmotor()
{
  // init (clear) the backlash amounts
  XEEPROM.writeInt(getMountAddress(EE_backlashAxis1), 0);
  XEEPROM.writeLong(getMountAddress(EE_motorA1gear), 1800 * 1000);
  XEEPROM.writeInt(getMountAddress(EE_motorA1stepRot), 200);
  XEEPROM.write(getMountAddress(EE_motorA1micro), 4);
  XEEPROM.write(getMountAddress(EE_motorA1reverse), 0);
  XEEPROM.write(getMountAddress(EE_motorA1highCurr), 10);
  XEEPROM.write(getMountAddress(EE_motorA1lowCurr), 10);
  XEEPROM.write(getMountAddress(EE_motorA1silent), 0);

  XEEPROM.writeInt(getMountAddress(EE_backlashAxis2), 0);
  XEEPROM.writeInt(getMountAddress(EE_motorA2gear), 1800);
  XEEPROM.writeInt(getMountAddress(EE_motorA2stepRot), 200);
  XEEPROM.write(getMountAddress(EE_motorA2micro), 4);
  XEEPROM.write(getMountAddress(EE_motorA2reverse), 0);
  XEEPROM.write(getMountAddress(EE_motorA2highCurr), 10);
  XEEPROM.write(getMountAddress(EE_motorA2lowCurr), 10);
  XEEPROM.write(getMountAddress(EE_motorA2silent), 0);
}



void EEPROM_AutoInit()
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

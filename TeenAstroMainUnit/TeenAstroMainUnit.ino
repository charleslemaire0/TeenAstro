/*
 * Title       TeenAstro
 * by          Howard Dutton, Charles Lemaire, Markus Noga, Francois Desvall�e
 *
 * Copyright (C) 2012 to 2016 On-Step by Howard Dutton
 * Copyright (C) 2016 to 2022 TeenAstro by Charles Lemaire, Markus Noga, Francois Desvall�e
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *
 *
 * Revision History, see GitHub
 *
 *
 * Author: Howard Dutton, Charles Lemaire
 *
 * Description
 *
 * Arduino Stepper motor controller for Telescop mounts
 * with LX200 derived command set.
 *
 */
#include "Global.h"

void setup()
{

  pinMode(LEDPin, OUTPUT);
  for (int k = 0; k < 20; k++)
  {
    digitalWrite(LEDPin, HIGH);
    delay(10);
    digitalWrite(LEDPin, LOW);
    delay(50);
  }

  // EEPROM automatic initialization
  long thisAutoInitKey = XEEPROM.readLong(EE_autoInitKey);
  if (thisAutoInitKey != initKey)
  {
    for (int i = 0; i < XEEPROM.length(); i++)
    {
      XEEPROM.write(i, 0);
    }
    // init the site information, lat/long/tz/name
    localSite.initdefault();
    XEEPROM.write(EE_mountType, MOUNT_TYPE_GEM);
    // init the min and max altitude
    minAlt = -10;
    maxAlt = 91;
    XEEPROM.write(EE_minAlt, minAlt + 128);
    XEEPROM.write(EE_maxAlt, maxAlt);
    XEEPROM.write(EE_dpmE, 0);
    XEEPROM.write(EE_dpmW, 0);
    XEEPROM.write(EE_dup, (12 - 9) * 15);
    XEEPROM.writeInt(EE_minAxis1, 3600);
    XEEPROM.writeInt(EE_maxAxis1, 3600);
    XEEPROM.writeInt(EE_minAxis2, 3600);
    XEEPROM.writeInt(EE_maxAxis2, 3600);
    writeDefaultEEPROMmotor();

    // init the Park status
    XEEPROM.write(EE_parkSaved, false);
    XEEPROM.write(EE_homeSaved, false);
    XEEPROM.write(EE_parkStatus, PRK_UNPARKED);

    // init the  rate
    XEEPROM.write(EE_Rate0, 100);
    XEEPROM.write(EE_Rate1, 4);
    XEEPROM.write(EE_Rate2, 16);
    XEEPROM.write(EE_Rate3, 64);

    // init the default maxRate
    XEEPROM.writeInt(EE_maxRate, 200);


    // init degree for acceleration
    XEEPROM.write(EE_degAcc, (uint8_t)(DegreesForAcceleration * 10));

    // init the sidereal tracking rate, use this once - then issue the T+ and T- commands to fine tune
    // 1/16uS resolution timer, ticks per sidereal second
    XEEPROM.writeLong(EE_siderealClockSpeed, siderealClockSpeed*16);

    // the transformation is not valid
    XEEPROM.write(EE_Tvalid, 0);
    // reset flag for Tracking Correction
    XEEPROM.write(EE_TC_Axis, 0);

    XEEPROM.writeLong(EE_RA_Drift, 0);
    XEEPROM.writeLong(EE_DEC_Drift, 0);

    // reset flag for Apparent Pole
    doesRefraction.resetEEPROM();
    // finally, stop the init from happening again
    XEEPROM.writeLong(EE_autoInitKey, initKey);
  }
  // get the site information from EEPROM
  localSite.ReadCurrentSiteDefinition();
  doesRefraction.readFromEEPROM();
  initmount();
  initmotor(false);


  // init the date and time January 1, 2013. 0 hours LMT
  setSyncProvider(rtk.getTime);
  setSyncInterval(1);
  setTime(rtk.getTime());



  // light reticule LED
#ifdef RETICULE_LED_PINS
  pinMode(RETICULE_LED_PINS, OUTPUT);
  analogWrite(RETICULE_LED_PINS, reticuleBrightness);
#endif

  // ST4 interface
  pinMode(ST4RAw, INPUT);
  pinMode(ST4RAe, INPUT);
  pinMode(ST4DEn, INPUT);
  pinMode(ST4DEs, INPUT);

  // inputs for stepper drivers fault signal
#ifndef AXIS1_FAULT_OFF
#if defined(__arm__) && defined(TEENSYDUINO) && defined(ALTERNATE_PINMAP_ON)
#ifdef AXIS1_FAULT_LOW
  pinMode(Axis1_FAULT, INPUT_PULLUP);
#endif
#ifdef AXIS1_FAULT_HIGH
  pinMode(Axis1_FAULT, INPUT_PULLDOWN);
#endif
#else
  pinMode(Axis1_FAULT, INPUT);
#endif
#endif
#ifndef AXIS2_FAULT_OFF
#if defined(__arm__) && defined(TEENSYDUINO) && defined(ALTERNATE_PINMAP_ON)
#ifdef AXIS2_FAULT_LOW
  pinMode(Axis2_FAULT, INPUT_PULLUP);
#endif
#ifdef AXIS1_FAULT_HIGH
  pinMode(Axis2_FAULT, INPUT_PULLDOWN);
#endif
#else
  pinMode(Axis2_FAULT, INPUT);
#endif
#endif

  // disable the stepper drivers for now, if the enable lines are connected
  enable_Axis(false);

  // automatic mode switching before/after slews, initialize micro-step mode
  DecayModeTracking();

  // this sets the sidereal clock speed, controls the tracking speed so that the mount moves precisely with the stars
  siderealClockSpeed = (double)XEEPROM.readLong(EE_siderealClockSpeed)/16.0;
  updateSideral();
  beginTimers();

  // get ready for serial communications
  Serial.begin(BAUD);
  S_USB.attach_Stream((Stream *)&Serial, COMMAND_SERIAL);
  Serial1.begin(57600);
  S_SHC.attach_Stream((Stream *)&Serial1, COMMAND_SERIAL1);

  Focus_Serial.setRX(FocuserRX);
  Focus_Serial.setTX(FocuserTX);
  Focus_Serial.begin(56000);
  Focus_Serial.setTimeout(10);

  //GNSS connection
#if VERSION != 220
  GNSS_Serial.begin(9600);
#endif

  rtk.resetLongitude(*localSite.longitude());
  // get the Park status
  if (!iniAtPark())
  {
    syncAtHome();
  }

  // get the pulse-guide rate
  int val = EEPROM.read(EE_Rate0);
  guideRates[0] = val > 0 ? (float)val / 100 : DefaultR0;
  val = EEPROM.read(EE_Rate1);
  guideRates[1] = val > 0 ? (float)val : DefaultR1;
  val = EEPROM.read(EE_Rate2);
  guideRates[2] = val > 0 ? (float)val : DefaultR2;
  val = EEPROM.read(EE_Rate3);
  guideRates[3] = val > 0 ? (float)val : DefaultR3;
  enableGuideRate(EEPROM.read(EE_DefaultRate));
  delay(10);

  // prep timers
  rtk.updateTimers();
  Focus_Serial.write(":F?#");
  digitalWrite(LEDPin, HIGH);
  delay(1000);
  hasGNSS = GNSS_Serial.available() > 0;
  char ret;
  while (Focus_Serial.available() > 0)
  {
    ret = Focus_Serial.read();
    if (ret == '?')
    {
      hasFocuser = true;
    }
  }
  digitalWrite(LEDPin, LOW);
}

void loop()
{
  static bool forceTracking = false;
  static unsigned long m;
  static ErrorsTraking StartLoopError = ERRT_NONE;
  StartLoopError = lastError;
  // GUIDING -------------------------------------------------------------------------------------------
  if (!movingTo)
  {
    checkST4();
    CheckSpiral();
    Guide();
  }

  // 0.01 SECOND TIMED ---------------------------------------------------------------------------------

  if (rtk.updatesiderealTimer())
  {
    // update Target position
    if (sideralTracking)
    {
      cli();
      if (!backlashA1.correcting)
      {
        staA1.target += staA1.fstep;
      }
      if (!backlashA2.correcting)
      {
        staA2.target += staA2.fstep;
      }
      sei();
    }
    // Goto Target
    if (movingTo)
    {
      moveTo();
    }

    if (rtk.m_lst % 16 == 0)
    {
      getHorApp(&currentAzm, &currentAlt);
      if (rtk.m_lst % 64 == 0)
      {
        computeTrackingRate(false);
      }
    }

    CheckEndOfMoveAxisAtRate();

    // check for fault signal, stop any slew or guide and turn tracking off
    if (staA1.fault || staA2.fault)
    {
      lastError = ERRT_MOTOR_FAULT;
      if (!forceTracking)
      {
        if (movingTo)
          abortSlew = true;
        else
        {
          sideralTracking = false;
          guideA1.brake();
          guideA2.brake();
        }
      }
    }
    else if (lastError == ERRT_MOTOR_FAULT)
    {
      lastError = ERRT_NONE;
    }
    // check altitude overhead limit and horizon limit
    if (currentAlt < minAlt || currentAlt > maxAlt)
    {
      if (!forceTracking)
      {
        lastError = ERRT_ALT;
        if (movingTo)
          abortSlew = true;
        else
          sideralTracking = false;
      }
    }
    else if (lastError == ERRT_ALT)
    {
      lastError = ERRT_NONE;
    }
  }

  // WORKLOAD MONITORING -------------------------------------------------------------------------------
  tlp.monitor();

  // HOUSEKEEPING --------------------------------------------------------------------------------------
  // timer... falls in once a second, keeps the universal time clock ticking,
  m = millis();
  forceTracking = (m - lastSetTrakingEnable < 10000);
  if (!forceTracking) lastSetTrakingEnable = m + 10000;
  if (rtk.updateclockTimer(m))
  {
    // adjust tracking rate for Alt/Azm mounts
    // adjust tracking rate for refraction
    ApplyTrackingRate();
    SafetyCheck(forceTracking);
  }
  else
  {
    // COMMAND PROCESSING --------------------------------------------------------------------------------
    // acts on commands recieved across Serial0 and Serial1 interfaces
    processCommands();
    smartDelay(0);
  }

  if (StartLoopError != lastError)
  {
    lastError == ERRT_NONE ? digitalWrite(LEDPin, LOW) : digitalWrite(LEDPin, HIGH);
  }
}



// safety checks,
// keeps mount from tracking past the meridian limit, past the underPoleLimit,
// below horizon limit, above the overhead limit, or past the Dec limits
void SafetyCheck(const bool forceTracking)
{
  // basic check to see if we're not at home
  PierSide currentSide = GetPierSide();
  long axis1, axis2;
  setAtMount(axis1, axis2);

  if (atHome)
    atHome = !sideralTracking;

  if (!geoA1.withinLimit(axis1))
  {
    lastError = ERRT_AXIS1;
    if (movingTo)
      abortSlew = true;
    else if (!forceTracking)
      sideralTracking = false;
    return;
  }
  else if (lastError == ERRT_AXIS1)
  {
    lastError = ERRT_NONE;
  }

  if (!geoA2.withinLimit(axis2))
  {
    lastError = ERRT_AXIS2;
    if (movingTo)
      abortSlew = true;
    else if (!forceTracking)
      sideralTracking = false;
    return;
  }
  else if (lastError == ERRT_AXIS2)
  {
    lastError = ERRT_NONE;
  }

  if (mountType == MOUNT_TYPE_GEM)
  {
    if (!checkMeridian(axis1, axis2, CHECKMODE_TRACKING))
    {
      if ((staA1.dir && currentSide == PIER_WEST) || (!staA2.dir && currentSide == PIER_EAST))
      {
        lastError = ERRT_MERIDIAN;
        if (movingTo)
        {
          abortSlew = true;
        }
        if (currentSide >= PIER_WEST && !forceTracking)
          sideralTracking = false;
        return;
      }
      else if (lastError == ERRT_MERIDIAN)
      {
        lastError = ERRT_NONE;
      }
    }
    else if (lastError == ERRT_MERIDIAN)
    {
      lastError = ERRT_NONE;
    }

    if (!checkPole(axis1, CHECKMODE_TRACKING))
    {
      if ((staA1.dir && currentSide == PIER_EAST) || (!staA2.dir && currentSide == PIER_WEST))
      {
        lastError = ERRT_UNDER_POLE;
        if (movingTo)
          abortSlew = true;
        if (currentSide == PIER_EAST && !forceTracking)
          sideralTracking = false;
        return;
      }
      else if (lastError == ERRT_UNDER_POLE)
      {
        lastError = ERRT_NONE;
      }
    }
    else if (lastError == ERRT_UNDER_POLE)
    {
      lastError = ERRT_NONE;
    }
  }

}


//enable Axis 
void enable_Axis(bool enable)
{
  if (enable)
  {
    staA1.enable = true;
    staA2.enable = true;
  }
  else
  {
    staA1.enable = false;
    staA2.enable = false;
  }
}


void initmount()
{
  long lval = 0;
  byte val = 0;
#ifdef D_mountType
  mountType = static_cast<Mount>(D_mountType);
  isMountTypeFix = true;
#else
  val = XEEPROM.read(EE_mountType);
  if (val < 1 || val >  4)
  {
    XEEPROM.write(EE_mountType, MOUNT_TYPE_GEM);
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
  DegreesForAcceleration = 0.1*EEPROM.read(EE_degAcc);
  if (DegreesForAcceleration == 0 || DegreesForAcceleration > 25)
  {
    DegreesForAcceleration = 3.0;
    XEEPROM.write(EE_degAcc, (uint8_t)(DegreesForAcceleration * 10));
  }
  // get the min. and max altitude
  minAlt = XEEPROM.read(EE_minAlt) - 128;
  maxAlt = XEEPROM.read(EE_maxAlt);
  minutesPastMeridianGOTOE = round(((EEPROM.read(EE_dpmE) - 128)*60.0) / 15.0);
  if (abs(minutesPastMeridianGOTOE) > 180)
    minutesPastMeridianGOTOE = 60;
  minutesPastMeridianGOTOW = round(((EEPROM.read(EE_dpmW) - 128)*60.0) / 15.0);
  if (abs(minutesPastMeridianGOTOW) > 180)
    minutesPastMeridianGOTOW = 60;
  underPoleLimitGOTO = (double)EEPROM.read(EE_dup) / 10;
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
  val = XEEPROM.read(EE_TC_Axis);
  tc = val < 0 || val >  2 ? TC_NONE : static_cast<TrackingCompensation>(val);
  lval = XEEPROM.read(EE_RA_Drift);
  storedTrakingRateRA  = lval < -50000 || lval > 50000? 0 :lval;
  lval = XEEPROM.read(EE_DEC_Drift);
  storedTrakingRateDEC = lval < -50000 || lval > 50000 ? 0 : lval;

}

void initTransformation(bool reset)
{
  float t11 = 0, t12 = 0, t13 = 0, t21 = 0, t22 = 0, t23 = 0, t31 = 0, t32 = 0, t33 = 0;
  hasStarAlignment = false;
  alignment.clean();
  byte TvalidFromEEPROM = XEEPROM.read(EE_Tvalid);

  if (TvalidFromEEPROM ==1 && reset)
  {
    XEEPROM.write(EE_Tvalid, 0);
  }
  if (TvalidFromEEPROM == 1 && !reset)
  {
    t11 = XEEPROM.readFloat(EE_T11);
    t12 = XEEPROM.readFloat(EE_T12);
    t13 = XEEPROM.readFloat(EE_T13);
    t21 = XEEPROM.readFloat(EE_T21);
    t22 = XEEPROM.readFloat(EE_T22);
    t23 = XEEPROM.readFloat(EE_T23);
    t31 = XEEPROM.readFloat(EE_T31);
    t32 = XEEPROM.readFloat(EE_T32);
    t33 = XEEPROM.readFloat(EE_T33);
    alignment.setT(t11, t12, t13, t21, t22, t23, t31, t32, t33);
    alignment.setTinvFromT();
    hasStarAlignment = true;
  }
  else
  {
    if (isAltAZ())
    {
      alignment.addReferenceDeg(0, 0, 180, 0);
      alignment.addReferenceDeg(0, 90, 180, 90);
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

void initCelestialPole()
{
  
  if (isAltAZ())
  {
    geoA1.poleDef = (*localSite.latitude() < 0) ? geoA1.halfRot : 0L;
    geoA2.poleDef = geoA2.quaterRot;
  }
  else
  {
    geoA1.poleDef = mountType == MOUNT_TYPE_GEM ? geoA1.quaterRot : 0L;
    geoA2.poleDef = (*localSite.latitude() < 0) ? -geoA2.quaterRot : geoA2.quaterRot;
  }
  HADir = *localSite.latitude() > 0 ? HADirNCPInit : HADirSCPInit;
}

void initmotor(bool deleteAlignment)
{
  readEEPROMmotor();
  updateRatios(deleteAlignment,false);
  motorA1.initMotor(static_cast<Driver::MOTORDRIVER>(AxisDriver), Axis1EnablePin, Axis1CSPin, Axis1DirPin, Axis1StepPin);
  motorA2.initMotor(static_cast<Driver::MOTORDRIVER>(AxisDriver), Axis2EnablePin, Axis2CSPin, Axis2DirPin, Axis2StepPin);
  readEEPROMmotorCurrent();
  motorA1.driver.setCurrent((unsigned int)motorA1.lowCurr);
  motorA2.driver.setCurrent((unsigned int)motorA2.lowCurr);
}
void readEEPROMmotorCurrent()
{
#ifdef D_motorA1lowCurr
  motorA1.lowCurr = D_motorA1lowCurr;
  motorA1.isLowCurrfix = true;
#else
  motorA1.lowCurr = (unsigned int)XEEPROM.read(EE_motorA1lowCurr) * 100;
  if (motorA1.lowCurr > motorA1.driver.getMaxCurrent() || motorA1.lowCurr < 200u)
  {
    motorA1.lowCurr = 1000u;
    XEEPROM.write(EE_motorA1lowCurr, 10u);
  }
  motorA1.isLowCurrfix = false;
#endif 

#ifdef D_motorA1highCurr
  motorA1.highCurr = D_motorA1highCurr;
  motorA1.isHighCurrfix = true;
#else
  motorA1.highCurr = (unsigned int)XEEPROM.read(EE_motorA1highCurr) * 100;
  if (motorA1.highCurr > motorA1.driver.getMaxCurrent() || motorA1.highCurr < 200u)
  {
    motorA1.highCurr = 1000u;
    XEEPROM.write(EE_motorA1highCurr, 10u);
  }
  motorA1.isHighCurrfix = false;
#endif


#ifdef D_motorA2lowCurr
  motorA2.lowCurr = D_motorA2lowCurr;
  motorA2.isLowCurrfix = true;
#else
  motorA2.lowCurr = (unsigned int)XEEPROM.read(EE_motorA2lowCurr) * 100;
  if (motorA2.lowCurr > motorA2.driver.getMaxCurrent() || motorA2.lowCurr < 200u)
  {
    motorA2.lowCurr = 1000u;
    XEEPROM.write(EE_motorA2lowCurr, 10u);
  }
  motorA2.isLowCurrfix = false;
#endif 

#ifdef D_motorA2highCurr
  motorA2.highCurr = D_motorA2highCurr;
  motorA2.isHighCurrfix = true;
#else
  motorA2.highCurr = (unsigned int)XEEPROM.read(EE_motorA2highCurr) * 100;
  if (motorA2.highCurr > motorA2.driver.getMaxCurrent() || motorA2.highCurr < 200u)
  {
    motorA2.highCurr = 1000u;
    XEEPROM.write(EE_motorA2highCurr, 10u);
  }
  motorA2.isHighCurrfix = false;
#endif

}

void readEEPROMmotor()
{
  backlashA1.inSeconds = XEEPROM.readInt(EE_backlashAxis1);
  backlashA1.movedSteps = 0;
  backlashA2.inSeconds = XEEPROM.readInt(EE_backlashAxis2);
  backlashA2.movedSteps = 0;

  //AXIS 1
#ifdef D_motorA1gear
  motorA1.gear = D_motorA1gear;
  motorA1.isGearFix = true;
#else
  motorA1.gear = XEEPROM.readInt(EE_motorA1gear);
  motorA1.isGearFix = false;
#endif

#ifdef D_motorA1stepRot
  motorA1.stepRot = D_motorA1stepRot;
  motorA1.isStepRotFix = true;
#else
  motorA1.stepRot = XEEPROM.readInt(EE_motorA1stepRot);
  motorA1.isStepRotFix = false;
#endif 

#ifdef D_motorA1micro
  motorA1.micro = D_motorA1micro;
  motorA1.isMicroFix = true;
#else
  motorA1.micro = XEEPROM.read(EE_motorA1micro);
  if (motorA1.micro > 8 || motorA1.micro < 1)
  {
    motorA1.micro = 4;
    XEEPROM.update(EE_motorA1micro, 4u);
  }
  motorA1.isMicroFix = false;
#endif

#ifdef D_motorA1reverse
  motorA1.reverse = D_motorA1reverse;
  motorA1.isReverseFix = true;
#else
  motorA1.reverse = XEEPROM.read(EE_motorA1reverse);
  motorA1.isReverseFix = false;
#endif 



#ifdef D_motorA1silent
  motorA1.silent = D_motorA1silent;
  motorA1.isSilentFix = true;
#else
  motorA1.silent = XEEPROM.read(EE_motorA1silent);
  motorA1.isSilentFix = false;
#endif

 //AXIS 2
#ifdef D_motorA2gear
  motorA2.gear = D_motorA2gear;
  motorA2.isGearFix = true;
#else
  motorA2.gear = XEEPROM.readInt(EE_motorA2gear);
  motorA2.isGearFix = false;
#endif

#ifdef D_motorA2stepRot
  motorA2.stepRot = D_motorA2stepRot;
  motorA2.isStepRotFix = true;
#else
  motorA2.stepRot = XEEPROM.readInt(EE_motorA2stepRot);
  motorA2.isStepRotFix = false;
#endif 

#ifdef D_motorA2micro
  motorA2.micro = D_motorA2micro;
  motorA2.isMicroFix = true;
#else
  motorA2.micro = XEEPROM.read(EE_motorA2micro);
  if (motorA2.micro > 8 || motorA2.micro < 1)
  {
    motorA2.micro = 4;
    XEEPROM.update(EE_motorA2micro, 4u);
  }
  motorA2.isMicroFix = false;
#endif

#ifdef D_motorA2reverse
  motorA2.reverse = D_motorA2reverse;
  motorA2.isReverseFix = true;
#else
  motorA2.reverse = XEEPROM.read(EE_motorA2reverse);
  motorA2.isReverseFix = false;
#endif 


#ifdef D_motorA2silent
  motorA2.silent = D_motorA2silent;
  motorA2.isSilentFix = true;
#else
  motorA2.silent = XEEPROM.read(EE_motorA2silent);
  motorA2.isSilentFix = false;
#endif

}

void writeDefaultEEPROMmotor()
{
  // init (clear) the backlash amounts
  XEEPROM.writeInt(EE_backlashAxis1, 0);
  XEEPROM.writeInt(EE_motorA1gear, 1800);
  XEEPROM.writeInt(EE_motorA1stepRot, 200);
  XEEPROM.write(EE_motorA1micro, 4);
  XEEPROM.write(EE_motorA1reverse, 0);
  XEEPROM.write(EE_motorA1highCurr, 10);
  XEEPROM.write(EE_motorA1lowCurr, 10);
  XEEPROM.write(EE_motorA1silent, 0);

  XEEPROM.writeInt(EE_backlashAxis2, 0);
  XEEPROM.writeInt(EE_motorA2gear, 1800);
  XEEPROM.writeInt(EE_motorA2stepRot, 200);
  XEEPROM.write(EE_motorA2micro, 4);
  XEEPROM.write(EE_motorA2reverse, 0);
  XEEPROM.write(EE_motorA2highCurr, 10);
  XEEPROM.write(EE_motorA2lowCurr, 10);
  XEEPROM.write(EE_motorA2silent, 0);
}

void updateRatios(bool deleteAlignment, bool deleteHP)
{
  cli();
  geoA1.setstepsPerRot((long)motorA1.gear * motorA1.stepRot * (int)pow(2, motorA1.micro));
  geoA2.setstepsPerRot((long)motorA2.gear * motorA2.stepRot * (int)pow(2, motorA2.micro));
  backlashA1.inSteps = (int)round(((double)backlashA1.inSeconds * 3600.0) / (double)geoA1.stepsPerDegree);
  backlashA2.inSteps = (int)round(((double)backlashA2.inSeconds * 3600.0) / (double)geoA2.stepsPerDegree);
  sei();

  guideA1.init(&geoA1.stepsPerCentiSecond, guideRates[activeGuideRate]);
  guideA2.init(&geoA2.stepsPerCentiSecond, guideRates[activeGuideRate]);

  initCelestialPole();
  initTransformation(deleteAlignment);
  if (deleteHP)
  {
    unsetPark();
    unsetHome();
  }
  initHome();
  initLimit();
  updateSideral();
  initMaxRate();
}

void updateSideral()
{
  cli();
  staA1.setSidereal(siderealClockSpeed, geoA1.stepsPerSecond, masterClockSpeed);
  staA2.setSidereal(siderealClockSpeed, geoA2.stepsPerSecond, masterClockSpeed);
  sei();

  SetTrackingRate(default_tracking_rate,0);

  // backlash takeup rates
  backlashA1.interval_Step = staA1.interval_Step_Cur / BacklashTakeupRate;
  backlashA2.interval_Step = staA2.interval_Step_Cur / BacklashTakeupRate;

  // initialize the sidereal clock, RA, and Dec
  SetsiderealClockSpeed(siderealClockSpeed);
}
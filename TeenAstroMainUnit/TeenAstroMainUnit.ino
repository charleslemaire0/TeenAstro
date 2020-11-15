/*
 * Title       On-Step
 * by          Howard Dutton, Charles Lemaire, Markus Noga, Francois Desvall�e
 *
 * Copyright (C) 2012 to 2016 Howard Dutton
 * Copyright (C) 2016 to 2020 Charles Lemaire, Markus Noga, Francois Desvall�e
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
    writeDefaultEEPROMmotor();

    // init the Park status
    XEEPROM.write(EE_parkSaved, false);
    XEEPROM.write(EE_parkStatus, PRK_UNPARKED);

    // init the  rate
    XEEPROM.write(EE_Rate0, 100);
    XEEPROM.write(EE_Rate1, 4);
    XEEPROM.write(EE_Rate2, 16);
    XEEPROM.write(EE_Rate3, 64);

    // init the default maxRate
    if (maxRate < 2L * 16L) maxRate = 2L * 16L;
    if (maxRate > 10000L * 16L) maxRate = 10000L * 16L;
    XEEPROM.writeInt(EE_maxRate, (int)(maxRate / 16L));


    // init degree for acceleration
    XEEPROM.write(EE_degAcc, (uint8_t)(DegreesForAcceleration * 10));

    // init the sidereal tracking rate, use this once - then issue the T+ and T- commands to fine tune
    // 1/16uS resolution timer, ticks per sidereal second
    XEEPROM.writeLong(EE_siderealInterval, siderealInterval);

    // the transformation is not valid
    XEEPROM.write(EE_Tvalid, 0);
    // reset flag for Tracking Correction
    XEEPROM.write(EE_corr_track, 0);
    // reset flag for Apparent Pole
    XEEPROM.write(EE_ApparentPole, 1);
    // finally, stop the init from happening again
    XEEPROM.writeLong(EE_autoInitKey, initKey);
  }
  // get the site information from EEPROM
  localSite.ReadCurrentSiteDefinition();

  initmount();
  initmotor(false);

  // init the date and time January 1, 2013. 0 hours LMT
  setSyncProvider(rtk.getTime);
  setSyncInterval(1);
  setTime(rtk.getTime());

  // initialize the stepper control pins Axis1 and Axis2
  pinMode(Axis1StepPin, OUTPUT);
  pinMode(Axis1DirPin, OUTPUT);
  pinMode(Axis2StepPin, OUTPUT);
  pinMode(Axis2DirPin, OUTPUT);

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

  // this sets the sidereal timer, controls the tracking speed so that the mount moves precisely with the stars
  siderealInterval = XEEPROM.readLong(EE_siderealInterval);
  updateSideral();
  beginTimers();

  // get ready for serial communications
  Serial.begin(BAUD);
  S_USB.attach_Stream((Stream *)&Serial, COMMAND_SERIAL);
  Serial1.begin(57600);
  S_SHC.attach_Stream((Stream *)&Serial1, COMMAND_SERIAL1);
  Serial2.setRX(FocuserRX);
  Serial2.setTX(FocuserTX);
  Serial2.begin(56000);
  Serial2.setTimeout(10);
  //GNSS connection
#if VERSION == 230 || VERSION == 240
  Serial3.begin(9600);
#endif

  rtk.resetLongitude(*localSite.longitude());
  // get the Park status
  if (!iniAtPark())
  {
    syncPolarHome();
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

  // makes onstep think that you parked the 'scope
  // combined with a hack in the goto syncEqu() function and you can quickly recover from
  // a reset without loosing much accuracy in the sky.  PEC is toast though.
  // set the default guide rate, 16x sidereal
  enableGuideRate(EEPROM.read(EE_DefaultRate), true);
  delay(10);

  // prep timers
  rtk.updateTimers();
  Serial2.write(":F?#");
  digitalWrite(LEDPin, HIGH);
  delay(1000);
  hasGNSS = Serial3.available() > 0;
  //hasFocuser = Serial2.available() > 0;
  char ret;
  while (Serial2.available() > 0)
  {
    ret = Serial2.read();
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
  static Errors StartLoopError = ERR_NONE;
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
      if ((isAltAZ() || correct_tracking) && (rtk.m_lst % 64 == 0))
      {
        do_compensation_calc();
      }
    }

    // check for fault signal, stop any slew or guide and turn tracking off
    if (staA1.fault || staA2.fault)
    {
      lastError = ERR_MOTOR_FAULT;
      if (!forceTracking)
      {
        if (movingTo)
          abortSlew = true;
        else
        {
          sideralTracking = false;
          if (guideA1.dir) guideA1.dir = 'b';
          if (guideA2.dir) guideA2.dir = 'b';
        }
      }
    }
    else if (lastError == ERR_MOTOR_FAULT)
    {
      lastError = ERR_NONE;
    }
    // check altitude overhead limit and horizon limit
    if (currentAlt < minAlt || currentAlt > maxAlt)
    {
      if (!forceTracking)
      {
        lastError = ERR_ALT;
        if (movingTo)
          abortSlew = true;
        else
          sideralTracking = false;
      }
    }
    else if (lastError == ERR_ALT)
    {
      lastError = ERR_NONE;
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
    // for testing, average steps per second
    if (debugv1 > 100000) debugv1 = 100000;
    if (debugv1 < 0) debugv1 = 0;
    debugv1 = (debugv1 * 19 + (staA1.target * 1000 - lasttargetAxis1)) / 20;
    lasttargetAxis1 = staA1.target * 1000;
    // adjust tracking rate for Alt/Azm mounts
    // adjust tracking rate for refraction
    SetDeltaTrackingRate();
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
    lastError == ERR_NONE ? digitalWrite(LEDPin, LOW) : digitalWrite(LEDPin, HIGH);
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
  // check for exceeding MinDec for Eq. Fork
  if (!isAltAZ())
  {
    if (!checkAxis2LimitEQ(axis2))
    {
      lastError = ERR_AXIS2;
      if (movingTo)
        abortSlew = true;
      else if (!forceTracking)
        sideralTracking = false;
    }
    else if (lastError == ERR_AXIS2)
    {
      lastError = ERR_NONE;
    }
    if (mountType == MOUNT_TYPE_GEM)
    {
      if (!checkMeridian(axis1, axis2, CHECKMODE_TRACKING))
      {
        if ((staA1.dir && currentSide == PIER_WEST) || (!staA2.dir && currentSide == PIER_EAST))
        {
          lastError = ERR_MERIDIAN;
          if (movingTo)
          {
            abortSlew = true;
          }
          if (currentSide >= PIER_WEST && !forceTracking)
            sideralTracking = false;
        }
        else if (lastError == ERR_MERIDIAN)
        {
          lastError = ERR_NONE;
        }
      }
      else if (lastError == ERR_MERIDIAN)
      {
        lastError = ERR_NONE;
      }
    }
    if (!checkPole(axis1, CHECKMODE_TRACKING))
    {
      if ((staA1.dir && currentSide == PIER_EAST) || (!staA2.dir && currentSide == PIER_WEST))
      {
        lastError = ERR_UNDER_POLE;
        if (movingTo)
          abortSlew = true;
        if (currentSide == PIER_EAST && !forceTracking)
          sideralTracking = false;
      }
      else if (lastError == ERR_UNDER_POLE)
      {
        lastError = ERR_NONE;
      }
    }
    else if (lastError == ERR_UNDER_POLE)
    {
      lastError = ERR_NONE;
    }
  }
  else
  {
    if (!checkAxis2LimitAZALT(axis2))
    {
      lastError = ERR_AXIS2;
      if (movingTo)
        abortSlew = true;
      else if (!forceTracking)
        sideralTracking = false;
    }
    else if (lastError == ERR_AXIS2)
    {
      lastError = ERR_NONE;
    }
    // when Alt/Azm mounted, just stop the mount if it passes MaxAzm
    if (!checkAxis1LimitAZALT(axis1))
    {
      lastError = ERR_AZM;
      if (movingTo)
        abortSlew = true;
      else if (!forceTracking)
        sideralTracking = false;
    }
    else if (lastError == ERR_AZM)
    {
      lastError = ERR_NONE;
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
  byte mountTypeFromEEPROM = XEEPROM.read(EE_mountType);

  mountType = mountTypeFromEEPROM < 1 || mountTypeFromEEPROM >  4 ? MOUNT_TYPE_GEM : static_cast<Mount>(mountTypeFromEEPROM);

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
  guideA1.amount = 0;
  guideA2.amount = 0;

  staA1.fstep = 0;
  staA2.fstep = 0;

  staA1.target = geoA1.quaterRot;
  staA2.target = geoA2.quaterRot;
  staA1.fstep = geoA1.stepsPerCentiSecond;
  // Tracking and rate control
  correct_tracking = XEEPROM.read(EE_corr_track);
}

void initTransformation(bool reset)
{
  float t11 = 0, t12 = 0, t13 = 0, t21 = 0, t22 = 0, t23 = 0, t31 = 0, t32 = 0, t33 = 0;
  hasStarAlignment = false;
  alignment.clean();
  if (reset)
  {
    XEEPROM.write(EE_Tvalid, 0);
    XEEPROM.writeFloat(EE_T11, t11);
    XEEPROM.writeFloat(EE_T12, t12);
    XEEPROM.writeFloat(EE_T13, t13);
    XEEPROM.writeFloat(EE_T21, t21);
    XEEPROM.writeFloat(EE_T22, t22);
    XEEPROM.writeFloat(EE_T23, t23);
    XEEPROM.writeFloat(EE_T31, t31);
    XEEPROM.writeFloat(EE_T32, t32);
    XEEPROM.writeFloat(EE_T33, t33);
  }
  byte TvalidFromEEPROM = XEEPROM.read(EE_Tvalid);
  if (TvalidFromEEPROM == 1)
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
      apparentPole = XEEPROM.read(EE_ApparentPole);
      double cosLat = *localSite.cosLat();
      double sinLat = *localSite.sinLat();
      if (apparentPole && abs(*localSite.latitude() > 10))
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
    geoA1.homeDef = geoA1.poleDef;
    geoA2.homeDef = 0;
  }
  else
  {
    geoA1.poleDef = mountType == MOUNT_TYPE_GEM ? geoA1.quaterRot : 0L;
    geoA2.poleDef = (*localSite.latitude() < 0) ? -geoA2.quaterRot : geoA2.quaterRot;
    geoA1.homeDef = geoA1.poleDef;
    geoA2.homeDef = geoA2.poleDef;
  }
  HADir = *localSite.latitude() > 0 ? HADirNCPInit : HADirSCPInit;
}

void initmotor(bool deleteAlignment)
{
  readEEPROMmotor();
  updateRatios(deleteAlignment);
  motorA1.initMotor(static_cast<Driver::MOTORDRIVER>(AxisDriver), Axis1EnablePin, Axis1CSPin, Axis1DirPin, Axis1StepPin);
  motorA2.initMotor(static_cast<Driver::MOTORDRIVER>(AxisDriver), Axis2EnablePin, Axis2CSPin, Axis2DirPin, Axis2StepPin);
}

void readEEPROMmotor()
{
  backlashA1.inSeconds = XEEPROM.readInt(EE_backlashAxis1);
  backlashA1.movedSteps = 0;
  motorA1.gear = XEEPROM.readInt(EE_motorA1gear);
  motorA1.stepRot = XEEPROM.readInt(EE_motorA1stepRot);
  motorA1.micro = XEEPROM.read(EE_motorA1micro);
  if (motorA1.micro > 8 || motorA1.micro < 1)
  {
    motorA1.micro = 4;
    XEEPROM.update(EE_motorA1micro, 4);
  }
  motorA1.reverse = XEEPROM.read(EE_motorA1reverse);
  motorA1.lowCurr = XEEPROM.read(EE_motorA1lowCurr);
  motorA1.highCurr = XEEPROM.read(EE_motorA1highCurr);
  motorA1.silent = XEEPROM.read(EE_motorA1silent);

  backlashA2.inSeconds = XEEPROM.readInt(EE_backlashAxis2);
  backlashA2.movedSteps = 0;
  motorA2.gear = XEEPROM.readInt(EE_motorA2gear);
  motorA2.stepRot = XEEPROM.readInt(EE_motorA2stepRot);
  motorA2.micro = XEEPROM.read(EE_motorA2micro);
  if (motorA2.micro > 8 || motorA2.micro < 1)
  {
    motorA2.micro = 4;
    XEEPROM.update(EE_motorA2micro, 4);
  }
  motorA2.reverse = XEEPROM.read(EE_motorA2reverse);
  motorA2.lowCurr = XEEPROM.read(EE_motorA2lowCurr);
  motorA2.highCurr = XEEPROM.read(EE_motorA2highCurr);
  motorA2.silent = XEEPROM.read(EE_motorA2silent);
}

void writeDefaultEEPROMmotor()
{
  // init (clear) the backlash amounts
  XEEPROM.writeInt(EE_backlashAxis1, 0);
  XEEPROM.writeInt(EE_motorA1gear, 1800);
  XEEPROM.writeInt(EE_motorA1stepRot, 200);
  XEEPROM.write(EE_motorA1micro, 4);
  XEEPROM.write(EE_motorA1reverse, 0);
  XEEPROM.write(EE_motorA1highCurr, 100);
  XEEPROM.write(EE_motorA1lowCurr, 100);
  XEEPROM.write(EE_motorA1silent, 0);

  XEEPROM.writeInt(EE_backlashAxis2, 0);
  XEEPROM.writeInt(EE_motorA2gear, 1800);
  XEEPROM.writeInt(EE_motorA2stepRot, 200);
  XEEPROM.write(EE_motorA2micro, 4);
  XEEPROM.write(EE_motorA2reverse, 0);
  XEEPROM.write(EE_motorA2highCurr, 100);
  XEEPROM.write(EE_motorA2lowCurr, 100);
  XEEPROM.write(EE_motorA2silent, 0);
}

void updateRatios(bool deleteAlignment)
{
  cli();
  geoA1.setstepsPerRot((long)motorA1.gear * motorA1.stepRot * (int)pow(2, motorA1.micro));
  geoA2.setstepsPerRot((long)motorA2.gear * motorA2.stepRot * (int)pow(2, motorA2.micro));
  backlashA1.inSteps = (int)round(((double)backlashA1.inSeconds * 3600.0) / (double)geoA1.stepsPerDegree);
  backlashA2.inSteps = (int)round(((double)backlashA2.inSeconds * 3600.0) / (double)geoA2.stepsPerDegree);
  timerRateRatio = geoA1.stepsPerSecond / geoA2.stepsPerSecond;
  sei();

  initCelestialPole();
  initTransformation(deleteAlignment);
  updateSideral();
  initMaxRate();
}

void updateSideral()
{
  // 16MHZ clocks for steps per second of sidereal tracking
  cli();
  SiderealRate = siderealInterval / geoA1.stepsPerSecond;
  TakeupRate = SiderealRate / 2L;
  sei();
  staA1.timerRate = SiderealRate;
  staA2.timerRate = SiderealRate;
  SetTrackingRate(default_tracking_rate);

  // backlash takeup rates
  backlashA1.timerRate = staA1.timerRate / BacklashTakeupRate;
  backlashA2.timerRate = staA2.timerRate / BacklashTakeupRate;

  // initialize the timers that handle the sidereal clock, RA, and Dec
  SetSiderealClockRate(siderealInterval);
}
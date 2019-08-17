/*
 * Title       On-Step
 * by          Howard Dutton, Charles Lemaire
 *
 * Copyright (C) 2012 to 2016 Howard Dutton
 * Copyright (C) 2016 to 2019 Charles Lemaire
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

#include <TinyGPS++.h>
#include <TeenAstroStepper.h>
#include <Time.h>
#include <math.h>
#include "Helper_math.h"
#include <errno.h>
#include <TimeLib.h>
#include "Global.h"
 // Use Config.h to configure OnStep to your requirements
 // help stepper driver configuration
#include "Align.h"
#include "Command.h"
#include "Config.TeenAstro.h"
#include "eeprom.h"
#include "EEPROM_adress.h"



// firmware info, these are returned by the ":GV?#" commands
#define FirmwareDate    __DATE__
#define FirmwareNumber  "1.1"
#define FirmwareName    "TeenAstro"
#define FirmwareTime    "12:00:00"


// forces initialialization of a host of settings in EEPROM. OnStep does this automatically, most likely, you will want to leave this alone
#define initKey     915307548                       // unique identifier for the current initialization format, do not change
TinyGPSPlus gps;


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
  long thisAutoInitKey = EEPROM_readLong(EE_autoInitKey);
  if (thisAutoInitKey != initKey)
  {
    for (int i = 0; i < EEPROM.length(); i++)
    {
      EEPROM.write(i, 0);
    }
    // init the site information, lat/long/tz/name
    localSite.initdefault();
    EEPROM.write(EE_mountType, MOUNT_TYPE_GEM);
    // init the min and max altitude
    minAlt = -10;
    maxAlt = 91;
    EEPROM.write(EE_minAlt, minAlt + 128);
    EEPROM.write(EE_maxAlt, maxAlt);
    EEPROM.write(EE_dpmE, 0);
    EEPROM.write(EE_dpmW, 0);
    EEPROM.write(EE_dup, (12 - 9) * 15);

    writeDefaultEEPROMmotor();

    // init the Park status
    EEPROM.write(EE_parkSaved, false);
    EEPROM.write(EE_parkStatus, PRK_UNPARKED);

    // init the pulse-guide rate
    EEPROM.write(EE_pulseGuideRate, GuideRate1x);

    // init the default maxRate
    if (maxRate < 2L * 16L) maxRate = 2L * 16L;
    if (maxRate > 10000L * 16L) maxRate = 10000L * 16L;
    EEPROM_writeInt(EE_maxRate, (int)(maxRate / 16L));


    // init degree for acceleration
    EEPROM.write(EE_degAcc, (uint8_t)(DegreesForAcceleration * 10));

    // init the sidereal tracking rate, use this once - then issue the T+ and T- commands to fine tune
    // 1/16uS resolution timer, ticks per sidereal second
    EEPROM_writeLong(EE_siderealInterval, siderealInterval);

    // finally, stop the init from happening again
    EEPROM_writeLong(EE_autoInitKey, initKey);

    // clear the pointing model
    saveAlignModel();
  }

  initmount();
  initmotor();

  // init the date and time January 1, 2013. 0 hours LMT
  setSyncProvider(getTeensy3Time);
  setSyncInterval(1);
  setTime(Teensy3Clock.get());

  // initialize the stepper control pins Axis1 and Axis2
  pinMode(Axis1StepPin, OUTPUT);
  pinMode(Axis1DirPin, OUTPUT);
  pinMode(Axis2StepPin, OUTPUT);
  pinMode(Axis2DirPin, OUTPUT);

  // light reticule LED
#ifdef RETICULE_LED_PINS
#if defined(__arm__) && defined(TEENSYDUINO) && !defined(ALTERNATE_PINMAP_ON)
#ifdef STATUS_LED_PINS_ON
#undef STATUS_LED_PINS_ON
#endif
#ifdef STATUS_LED_PINS
#undef STATUS_LED_PINS
#endif
#endif
  pinMode(ReticulePin, OUTPUT);
  analogWrite(ReticulePin, reticuleBrightness);
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
  siderealInterval = EEPROM_readLong(EE_siderealInterval);
  updateSideral();

  // set the system timer for millis() to the second highest priority
  SCB_SHPR3 = (32 << 24) | (SCB_SHPR3 & 0x00FFFFFF);

  itimer3.begin(TIMER3_COMPA_vect, (float)128 * 0.0625);
  itimer4.begin(TIMER4_COMPA_vect, (float)128 * 0.0625);

  // set the 1/100 second sidereal clock timer to run at the second highest priority
  NVIC_SET_PRIORITY(IRQ_PIT_CH0, 32);

  // set the motor timers to run at the highest priority
  NVIC_SET_PRIORITY(IRQ_PIT_CH1, 0);
  NVIC_SET_PRIORITY(IRQ_PIT_CH2, 0);


  // get ready for serial communications
  Serial1_Init(BAUD);
  Serial_Init(BAUD);                      // for Tiva TM4C the serial is redirected to serial5 in serial.ino file
  Serial2_Init(56000);
  //GNSS connection
#if VERSION == 230 || VERSION == 240
  Serial3.begin(9600);
#endif


  // get the site information, if a GPS were attached we would use that here instead
  localSite.ReadCurrentSiteDefinition();
  rtk.resetLongitude(*localSite.longitude());
  initCelestialPole();
  initLat();

  // get the Park status
  if (!iniAtPark())
  {
    syncPolarHome();
  }

  // get the pulse-guide rate
  guideRates[0] = (float)EEPROM.read(EE_pulseGuideRate) / 100.;

  // makes onstep think that you parked the 'scope
  // combined with a hack in the goto syncEqu() function and you can quickly recover from
  // a reset without loosing much accuracy in the sky.  PEC is toast though.
  // set the default guide rate, 16x sidereal
  enableGuideRate(GuideRateMax, true);
  delay(10);

  // prep timers
  rtk.updateTimers();
  digitalWrite(LEDPin, HIGH);
  delay(500);
  digitalWrite(LEDPin, LOW);
}

void loop()
{
  static bool forceTracking = false;
  StartLoopError = lastError;
  // GUIDING -------------------------------------------------------------------------------------------
  if (!movingTo)
  {
    checkST4();
    guideAxis1.fixed = 0;
    Guide();
  }

  // 0.01 SECOND TIMED ---------------------------------------------------------------------------------

  if (rtk.updatesiderealTimer())
  {
     // update Target position
     if (sideralTracking)
     {
       cli();
       if (!inbacklashAxis1)
       {
         targetAxis1.fixed += fstepAxis1.fixed;
       }
       if (!inbacklashAxis2)
       {
         targetAxis2.fixed += fstepAxis2.fixed;
       }
       sei();
     }
     // Goto Target
     if (movingTo)
     {
       moveTo();
     }


    // figure out the current Altitude
    do_fastalt_calc();
    if (isAltAZ())
    {
      // figure out the current Alt/Azm tracking rates
      if (rtk.m_lst % 3 != 0)
        do_altAzmRate_calc();
    }
    else
    {
      //// figure out the current refraction compensated tracking rate
      //if (refraction && (rtk.m_lst % 3 != 0))
      //  do_refractionRate_calc();
    }
    // check for fault signal, stop any slew or guide and turn tracking off
    if ((faultAxis1 || faultAxis2))
    {
      lastError = ERR_MOTOR_FAULT;
      if (!forceTracking)
      {
        if (movingTo)
          abortSlew = true;
        else
        {
          sideralTracking = false;
          if (guideDirAxis1) guideDirAxis1 = 'b';
          if (guideDirAxis2) guideDirAxis2 = 'b';
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
  unsigned long   m = millis();
  forceTracking = (m - lastSetTrakingEnable < 10000);
  if (!forceTracking) lastSetTrakingEnable = m + 10000;
  if (rtk.updateclockTimer(m))
  {
    // for testing, average steps per second
    if (debugv1 > 100000) debugv1 = 100000;
    if (debugv1 < 0) debugv1 = 0;
    debugv1 = (debugv1 * 19 + (targetAxis1.part.m * 1000 - lasttargetAxis1)) / 20;
    lasttargetAxis1 = targetAxis1.part.m * 1000;
    // adjust tracking rate for Alt/Azm mounts
    // adjust tracking rate for refraction
    SetDeltaTrackingRate();
    CheckPierSide();
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

void CheckPierSide()
{
  cli(); long pos = posAxis2; sei();
  if (pos == -quaterRotAxis2 || pos == quaterRotAxis2)
  {
    return;
  }
  bool isEast = -quaterRotAxis2 < pos && pos < quaterRotAxis2;
  pierSide = isEast ? PIER_EAST : PIER_WEST;
}

// safety checks,
// keeps mount from tracking past the meridian limit, past the underPoleLimit,
// below horizon limit, above the overhead limit, or past the Dec limits
void SafetyCheck(const bool forceTracking)
{
  // basic check to see if we're not at home

  if (atHome)
    atHome = !sideralTracking;

  if (meridianFlip != FLIP_NEVER)
  {
    double HA, Dec;
    PierSide side;
    GetInstr(&HA, &Dec, &side);
    if (!checkPole(HA, pierSide, CHECKMODE_TRACKING))
    {
      if ((dirAxis1 == 1 && pierSide == PIER_EAST) || (dirAxis1 == 0 && pierSide == PIER_WEST))
      {
        lastError = ERR_UNDER_POLE;
        if (movingTo)
          abortSlew = true;
        if (pierSide == PIER_EAST && !forceTracking)
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

    if (!checkMeridian(HA, pierSide, CHECKMODE_TRACKING))
    {
      if ((dirAxis1 == 1 && pierSide == PIER_WEST) || (dirAxis1 == 0 && pierSide == PIER_EAST))
      {
        lastError = ERR_MERIDIAN;
        if (movingTo)
        {
          abortSlew = true;
        }
        if (pierSide >= PIER_WEST && !forceTracking)
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
  else
  {
    if (!isAltAZ())
    {
      // when Fork mounted, ignore pierSide and just stop the mount if it passes the underPoleLimit
      double HA, Dec;
      PierSide Side;
      GetInstr(&HA, &Dec, &Side);
      double underPoleLimit = movingTo ? underPoleLimitGOTO : underPoleLimitGOTO + 5.0 / 60;  
      if (HA > underPoleLimit * 15.)
      {
        lastError = ERR_UNDER_POLE;
        if (movingTo)
          abortSlew = true;
        else if (!forceTracking)
          sideralTracking = false;
      }
      else if (lastError == ERR_UNDER_POLE)
      {
        lastError = ERR_NONE;
      }
    }
    else
    {
      // when Alt/Azm mounted, just stop the mount if it passes MaxAzm
      if (!checkAzimuth())
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

  // check for exceeding MinDec for Eq. Fork
  if (!checkDeclinatioLimit())
  {
    lastError = ERR_DEC;
    if (movingTo)
      abortSlew = true;
    else if (!forceTracking)
      sideralTracking = false;
  }
  else if (lastError == ERR_DEC)
  {
    lastError = ERR_NONE;
  }
}


//enable Axis 
void enable_Axis(bool enable)
{
  if (enable)
  {
    axis1Enabled = true;
    axis2Enabled = true;
  }
  else
  {
    axis1Enabled = false;
    axis2Enabled = false;
  }
}

time_t getTeensy3Time()
{
  return Teensy3Clock.get();
}

void initmount()
{
  byte mountTypeFromEEPROM = EEPROM.read(EE_mountType);

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
    EEPROM.write(EE_degAcc, (uint8_t)(DegreesForAcceleration * 10));
  }
  // get the min. and max altitude
  minAlt = EEPROM.read(EE_minAlt) - 128;
  maxAlt = EEPROM.read(EE_maxAlt);
  minutesPastMeridianGOTOE = round(((EEPROM.read(EE_dpmE) - 128)*60.0) / 15.0);
  if (abs(minutesPastMeridianGOTOE) > 180)
    minutesPastMeridianGOTOE = 60;
  minutesPastMeridianGOTOW = round(((EEPROM.read(EE_dpmW) - 128)*60.0) / 15.0);
  if (abs(minutesPastMeridianGOTOW) > 180)
    minutesPastMeridianGOTOW = 60;
  underPoleLimitGOTO = (double)EEPROM.read(EE_dup) / 10;
  if (underPoleLimitGOTO < 9 || underPoleLimitGOTO>12)
    underPoleLimitGOTO = 12;
  if (isAltAZ() && maxAlt > 87)
    maxAlt = 87;


  // initialize some fixed-point values
  amountGuideAxis1.fixed = 0;
  amountGuideAxis2.fixed = 0;
  guideAxis1.fixed = 0;
  guideAxis2.fixed = 0;

  fstepAxis1.fixed = 0;
  fstepAxis2.fixed = 0;

  targetAxis1.part.m = quaterRotAxis1;
  targetAxis1.part.f = 0;
  targetAxis2.part.m = quaterRotAxis2;
  targetAxis2.part.f = 0;
  fstepAxis1.fixed = doubleToFixed(StepsPerSecondAxis1 / 100.0);

  // initialize alignment



  // Tracking and rate control
  refraction_enable = isAltAZ() ? false : true;
  onTrack = false;

}

void initCelestialPole()
{
  if (isAltAZ())
  {
    //double lat = *localSite.latitude();
    poleStepAxis2 = quaterRotAxis2;
    poleStepAxis1 = (*localSite.latitude() < 0) ? halfRotAxis1 : 0L;
    homeStepAxis1 = poleStepAxis1;
    homeStepAxis2 = 0;
  }
  else
  {
    poleStepAxis1 = mountType == MOUNT_TYPE_GEM ? quaterRotAxis1 : 0L;
    poleStepAxis2 = (*localSite.latitude() < 0) ? -quaterRotAxis2 : quaterRotAxis2;
    homeStepAxis1 = poleStepAxis1;
    homeStepAxis2 = poleStepAxis2;
  }
}

void initLat()
{
  HADir = *localSite.latitude() > 0 ? HADirNCPInit : HADirSCPInit;
}

void initmotor()
{
  readEEPROMmotor();
  updateRatios();
  motorAxis1.initMotor(static_cast<Motor::Motor_Driver>(AxisDriver),StepRotAxis1, Axis1EnablePin, Axis1CSPin, Axis1DirPin, Axis1StepPin, (unsigned int)LowCurrAxis1 * 10, MicroAxis1);
  motorAxis2.initMotor(static_cast<Motor::Motor_Driver>(AxisDriver),StepRotAxis2, Axis2EnablePin, Axis2CSPin, Axis2DirPin, Axis2StepPin, (unsigned int)LowCurrAxis2 * 10, MicroAxis2);
}

void readEEPROMmotor()
{
  backlashAxis1 = EEPROM_readInt(EE_backlashAxis1);
  blAxis1 = 0;
  GearAxis1 = EEPROM_readInt(EE_GearAxis1);
  StepRotAxis1 = EEPROM_readInt(EE_StepRotAxis1);
  MicroAxis1 = EEPROM.read(EE_MicroAxis1);
  if (MicroAxis1 < 3) MicroAxis1 = 3; else if (MicroAxis1 > 8) MicroAxis1 = 8;
  ReverseAxis1 = EEPROM.read(EE_ReverseAxis1);
  LowCurrAxis1 = EEPROM.read(EE_LowCurrAxis1);
  HighCurrAxis1 = EEPROM.read(EE_HighCurrAxis1);

  backlashAxis2 = EEPROM_readInt(EE_backlashAxis2);
  blAxis2 = 0;
  GearAxis2 = EEPROM_readInt(EE_GearAxis2);
  StepRotAxis2 = EEPROM_readInt(EE_StepRotAxis2);
  MicroAxis2 = EEPROM.read(EE_MicroAxis2);
  if (MicroAxis2 < 3) MicroAxis2 = 3; else if (MicroAxis2 > 8) MicroAxis2 = 8;
  ReverseAxis2 = EEPROM.read(EE_ReverseAxis2);
  LowCurrAxis2 = EEPROM.read(EE_LowCurrAxis2);
  HighCurrAxis2 = EEPROM.read(EE_HighCurrAxis2);
}

void writeDefaultEEPROMmotor()
{
  // init (clear) the backlash amounts
  EEPROM_writeInt(EE_backlashAxis1, 0);
  EEPROM_writeInt(EE_GearAxis1, 1800);
  EEPROM_writeInt(EE_StepRotAxis1, 200);
  EEPROM.write(EE_MicroAxis1, 4);
  EEPROM.write(EE_ReverseAxis1, 0);
  EEPROM.write(EE_HighCurrAxis1, 100);
  EEPROM.write(EE_LowCurrAxis1, 100);

  EEPROM_writeInt(EE_backlashAxis2, 0);
  EEPROM_writeInt(EE_GearAxis2, 1800);
  EEPROM_writeInt(EE_StepRotAxis2, 200);
  EEPROM.write(EE_MicroAxis2, 4);
  EEPROM.write(EE_ReverseAxis2, 0);
  EEPROM.write(EE_HighCurrAxis2, 100);
  EEPROM.write(EE_LowCurrAxis2, 100);
}


void updateRatios()
{
  cli()
  StepsPerRotAxis1 = (long)GearAxis1 * StepRotAxis1 * (int)pow(2, MicroAxis1); // calculated as    :  stepper_steps * micro_steps * gear_reduction1 * (gear_reduction2/360)
  StepsPerRotAxis2 = (long)GearAxis2 * StepRotAxis2 * (int)pow(2, MicroAxis2); // calculated as    :  stepper_steps * micro_steps * gear_reduction1 * (gear_reduction2/360)
  StepsPerDegreeAxis1 = (double)StepsPerRotAxis1 / 360.0;
  StepsBacklashAxis1 = (int)round(((double)backlashAxis1 * 3600.0) / (double)StepsPerDegreeAxis1);
  StepsPerDegreeAxis2 = (double)StepsPerRotAxis2 / 360.0;
  StepsBacklashAxis2 = (int)round(((double)backlashAxis2 * 3600.0) / (double)StepsPerDegreeAxis2);

  StepsPerSecondAxis1 = StepsPerDegreeAxis1 / 240.0;
  StepsPerSecondAxis2 = StepsPerDegreeAxis2 / 240.0;


  timerRateRatio = (StepsPerSecondAxis1 / StepsPerSecondAxis2);
  useTimerRateRatio = (StepsPerRotAxis1 != StepsPerRotAxis2);
  sei();

  halfRotAxis1 = StepsPerRotAxis1 / 2L;
  quaterRotAxis1 = StepsPerRotAxis1 / 4L;
  halfRotAxis2 = StepsPerRotAxis2 / 2L;
  quaterRotAxis2 = StepsPerRotAxis2 / 4L;

  initCelestialPole();
  updateSideral();

  initMaxRate();
}

void updateSideral()
{
  // 16MHZ clocks for steps per second of sidereal tracking
  cli();
  SiderealRate = siderealInterval / StepsPerSecondAxis1;
  TakeupRate = SiderealRate / 4L;
  sei();
  timerRateAxis1 = SiderealRate;
  timerRateAxis2 = SiderealRate;
  SetTrackingRate(default_tracking_rate);

  // backlash takeup rates
  timerRateBacklashAxis1 = timerRateAxis1 / BacklashTakeupRate;
  timerRateBacklashAxis2 = timerRateAxis2 / BacklashTakeupRate;

  // initialize the timers that handle the sidereal clock, RA, and Dec
  SetSiderealClockRate(siderealInterval);
}
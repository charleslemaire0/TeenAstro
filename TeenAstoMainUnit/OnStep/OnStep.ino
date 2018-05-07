/*
 * Title       On-Step
 * by          Howard Dutton, Charles Lemaire
 *
 * Copyright (C) 2012 to 2016 Howard Dutton
 * Copyright (C) 2016 to 2017 Charles Lemaire
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
 * Arduino Stepper motor controller for Losmandy G11 mounts (and others)
 * with LX200 derived command set.
 *
 */
#include "math.h"
#include "Helper_math.h"
#include "errno.h"
#include "Time\TimeLib.h"
#include "Global.h"
 // Use Config.h to configure OnStep to your requirements
 // help stepper driver configuration
#include "Align.h"
#include "Command.h"
#include "Config.TeenAstro.h"
#include "eeprom.h"
#include "EEPROM_adress.h"

// There is a bug in Arduino/Energia which ignores #ifdef preprocessor directives when generating a list of files
// Until this is fixed YOU MUST MANUALLY UN-COMMENT the #include line below if using the Launchpad Connected device.
#if defined(W5100_ON)
#include "SPI.h"

// OnStep uses the EthernetPlus.h library for the W5100 on the Mega2560 and Launchpad TM4C:
// this is available at: https://github.com/hjd1964/EthernetPlus and should be installed in your "~\Documents\Arduino\libraries" folder
//#include "EthernetPlus.h"
// OnStep uses the Ethernet.h library for the W5100 on the Teensy3.2:
//#include "Ethernet.h"
#endif

// firmware info, these are returned by the ":GV?#" commands
#define FirmwareDate    "05 04 18"
#define FirmwareNumber  "0.0a"
#define FirmwareName    "On-Step-TeenAstro"
#define FirmwareTime    "12:00:00"

// forces initialialization of a host of settings in EEPROM. OnStep does this automatically, most likely, you will want to leave this alone
#define initKey     915307548                       // unique identifier for the current initialization format, do not change

void setup()
{
  pinMode(LEDPin, OUTPUT);
  for (int k = 0; k < 10; k++)
  {
    analogWrite(LEDPin, 255);
    delay(100);
    analogWrite(LEDPin, 0);
    delay(100);
  }

  // EEPROM automatic initialization
  long thisAutoInitKey = EEPROM_readLong(EE_autoInitKey);
  if (thisAutoInitKey != initKey)
  {
    // init the site information, lat/long/tz/name
    localSite.initdefault();
    // init the min and max altitude
    minAlt = -10;
    maxAlt = 91;
    EEPROM.write(EE_minAlt, minAlt + 128);
    EEPROM.write(EE_maxAlt, maxAlt);

    writeDefaultEEPROMmotor();

    // init the Park status
    EEPROM.write(EE_parkSaved, false);
    EEPROM.write(EE_parkStatus, NotParked);

    // init the pulse-guide rate
    EEPROM.write(EE_pulseGuideRate, GuideRate1x);

    // init the default maxRate
    if (maxRate < 2L * 16L) maxRate = 2L * 16L;
    if (maxRate > 10000L * 16L) maxRate = 10000L * 16L;
    EEPROM_writeInt(EE_maxRate, (int)(maxRate / 16L));

    // init autoContinue
    EEPROM.write(EE_autoContinue, autoContinue);

    // init the sidereal tracking rate, use this once - then issue the T+ and T- commands to fine tune
    // 1/16uS resolution timer, ticks per sidereal second
    EEPROM_writeLong(EE_siderealInterval, siderealInterval);

    // finally, stop the init from happening again
    EEPROM_writeLong(EE_autoInitKey, initKey);

    // clear the pointing model
    saveAlignModel();
  }

  initmotor();
  // init the date and time January 1, 2013. 0 hours LMT
  setSyncProvider(getTeensy3Time);
  setSyncInterval(1);
  setTime(Teensy3Clock.get());

  pinMode(PinFocusA, OUTPUT);
  pinMode(PinFocusB, OUTPUT);
  
  // initialize some fixed-point values
  amountGuideHA.fixed = 0;
  amountGuideDec.fixed = 0;
  guideHA.fixed = 0;
  guideDec.fixed = 0;

  fstepAxis1.fixed = 0;
  fstepAxis2.fixed = 0;

  origTargetAxis1.fixed = 0;
  targetAxis1.part.m = quaterRotAxis1;
  targetAxis1.part.f = 0;
  targetAxis2.part.m = quaterRotAxis2;
  targetAxis2.part.f = 0;
  fstepAxis1.fixed = doubleToFixed(StepsPerSecondAxis1 / 100.0);

  // initialize alignment
#ifdef MOUNT_TYPE_ALTAZM
  Align.init();
#endif
  GeoAlign.init();

  // initialize the stepper control pins Axis1 and Axis2
  pinMode(Axis1StepPin, OUTPUT);
  pinMode(Axis1DirPin, OUTPUT);
#ifdef Axis2GndPin
  pinMode(Axis2GndPin, OUTPUT);
  digitalWrite(Axis2GndPin, LOW);
#endif
  pinMode(Axis2StepPin, OUTPUT);
  pinMode(Axis2DirPin, OUTPUT);

  // override any status LED and set the reset pin HIGH
#if defined(W5100_ON) && defined(__arm__) && defined(TEENSYDUINO)
#ifdef STATUS_LED_PINS_ON
#undef STATUS_LED_PINS_ON
#endif
#ifdef STATUS_LED_PINS
#undef STATUS_LED_PINS
#endif
  pinMode(RstPin, OUTPUT);
  digitalWrite(RstPin, LOW);
  delay(500);
  digitalWrite(RstPin, HIGH);
#endif


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
#ifdef ST4_ON
  pinMode(ST4RAw, INPUT);
  pinMode(ST4RAe, INPUT);
  pinMode(ST4DEn, INPUT);
  pinMode(ST4DEs, INPUT);
#endif
#ifdef ST4_PULLUP
  pinMode(ST4RAw, INPUT_PULLUP);
  pinMode(ST4RAe, INPUT_PULLUP);
  pinMode(ST4DEn, INPUT_PULLUP);
  pinMode(ST4DEs, INPUT_PULLUP);
#endif

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


#if defined(__AVR_ATmega2560__)
  if (StepsPerSecondAxis1 < 31)
    TCCR3B = (1 << WGM12) | (1 << CS10) | (1 << CS11);  // ~0 to 0.25 seconds   (4 steps per second minimum, granularity of timer is 4uS)   /64 pre-scaler
  else
    TCCR3B = (1 << WGM12) | (1 << CS11);                // ~0 to 0.032 seconds (31 steps per second minimum, granularity of timer is 0.5uS) /8  pre-scaler
  TCCR3A = 0;
  TIMSK3 = (1 << OCIE3A);

  if (StepsPerSecondAxis1 < 31)
    TCCR4B = (1 << WGM12) | (1 << CS10) | (1 << CS11);  // ~0 to 0.25 seconds   (4 steps per second minimum, granularity of timer is 4uS)   /64 pre-scaler
  else
    TCCR4B = (1 << WGM12) | (1 << CS11);                // ~0 to 0.032 seconds (31 steps per second minimum, granularity of timer is 0.5uS) /8  pre-scaler
  TCCR4A = 0;
  TIMSK4 = (1 << OCIE4A);
#elif defined(__arm__) && defined(TEENSYDUINO)
  // set the system timer for millis() to the second highest priority
  SCB_SHPR3 = (32 << 24) | (SCB_SHPR3 & 0x00FFFFFF);

  itimer3.begin(TIMER3_COMPA_vect, (float)128 * 0.0625);
  itimer4.begin(TIMER4_COMPA_vect, (float)128 * 0.0625);

  // set the 1/100 second sidereal clock timer to run at the second highest priority
  NVIC_SET_PRIORITY(IRQ_PIT_CH0, 32);

  // set the motor timers to run at the highest priority
  NVIC_SET_PRIORITY(IRQ_PIT_CH1, 0);
  NVIC_SET_PRIORITY(IRQ_PIT_CH2, 0);
#endif

  // get ready for serial communications
  Serial1_Init(9600);
  Serial_Init(9600);                      // for Tiva TM4C the serial is redirected to serial5 in serial.ino file
#if defined(W5100_ON)
    // get ready for Ethernet communications
  Ethernet_Init();
#endif

  // get the site information, if a GPS were attached we would use that here instead
  localSite.ReadCurrentSiteDefinition();
  rtk.resetLongitude(*localSite.longitude());

#ifdef MOUNT_TYPE_ALTAZM
  celestialPoleStepAxis2 = AltAzmDecStartPos *StepsPerDegreeAxis2;
  if (*localSite.latitude() < 0)
    celestialPoleStepAxis1 = halfRotAxis1;
  else
    celestialPoleStepAxis1 = 0L;
#else
  if (*localSite.latitude() < 0)
    celestialPoleStepAxis2 = -quaterRotAxis2;
  else
    celestialPoleStepAxis2 = quaterRotAxis2;
#endif

  if (*localSite.latitude() > 0)
    HADir = HADirNCPInit;
  else
    HADir = HADirSCPInit;

  // get the Park status
  if (!iniAtPark())
  {
    syncPolarHome();
  }


  // get the min. and max altitude
  minAlt = EEPROM.read(EE_minAlt) - 128;
  maxAlt = EEPROM.read(EE_maxAlt);
#ifdef MOUNT_TYPE_ALTAZM
  if (maxAlt > 87) maxAlt = 87;
#endif


  // get the pulse-guide rate
  currentPulseGuideRate = EEPROM.read(EE_pulseGuideRate);
  if (currentPulseGuideRate > GuideRate1x)
  {
    currentPulseGuideRate = GuideRate1x;
    EEPROM.write(EE_pulseGuideRate, currentPulseGuideRate);
  }

  // get the Goto rate and constrain values to the limits (1/2 to 2X the MaxRate,) maxRate is in 16MHz clocks but stored in micro-seconds


  maxRate = EEPROM_readInt(EE_maxRate) * 16;
  if (maxRate < (MaxRate / 2L) * 16L) maxRate = (MaxRate / 2L) * 16L;
  if (maxRate > (MaxRate * 2L) * 16L) maxRate = (MaxRate * 2L) * 16L;
#ifndef RememberMaxRate_ON
  if (maxRate != MaxRate * 16L)
  {
    maxRate = MaxRate * 16L;
    EEPROM_writeInt(EE_maxRate, (int)(maxRate / 16L));
  }
#endif
  SetAccelerationRates(maxRate);          // set the new acceleration rate

  // get autoContinue
  autoContinue = EEPROM.read(EE_autoContinue);
  if (!autoContinue) autoContinue = true;

  // makes onstep think that you parked the 'scope
  // combined with a hack in the goto syncEqu() function and you can quickly recover from
  // a reset without loosing much accuracy in the sky.  PEC is toast though.
  // set the default guide rate, 16x sidereal
  setGuideRate(GuideRateMax);
  delay(110);

  // prep timers
  rtk.updateTimers();

  // autostart tracking
#if defined(AUTOSTART_TRACKING_ON) && \
        ( \
            defined(MOUNT_TYPE_GEM) || \
            defined(MOUNT_TYPE_FORK) || \
            defined(MOUNT_TYPE_FORKALT) \
        )

    // telescope should be set in the polar home (CWD) for a starting point
    // this command sets indexAxis1, indexAxis2, azmCor=0; altCor=0;
  setHome();

  // enable the stepper drivers
  digitalWrite(Axis1_EN, Axis1_Enabled);
  axis1Enabled = true;
  digitalWrite(Axis2_EN, Axis2_Enabled);
  axis2Enabled = true;
  delay(10);

  // start tracking
  trackingState = TrackingON;
#endif
  analogWrite(LEDPin, 128);
}

void loop()
{

  static bool forceTracking = false;
  // GUIDING -------------------------------------------------------------------------------------------
  if (trackingState == TrackingMoveTo)
  {
  }
  else
  {
    //checkST4();
    guideHA.fixed = 0;
    Guide();
  }

  // 0.01 SECOND TIMED ---------------------------------------------------------------------------------

  if (rtk.updatesiderealTimer())
  {
    // SIDEREAL TRACKING -------------------------------------------------------------------------------
    // only active while sidereal tracking with a guide rate that makes sense
    if (trackingState == TrackingON)
    {
      // apply the Tracking, Guiding
      cli();
      targetAxis1.fixed += fstepAxis1.fixed;
      targetAxis2.fixed += fstepAxis2.fixed;
      sei();
    }
    // SIDEREAL TRACKING DURING GOTOS ------------------------------------------------------------------
    // keeps the target where it's supposed to be while doing gotos
    else if (trackingState == TrackingMoveTo)
    {
      if (lastTrackingState == TrackingON)
      {
        // origTargetAxisn isn't used in Alt/Azm mode since meridian flips never happen
        origTargetAxis1.fixed += fstepAxis1.fixed;
        // don't advance the target during meridian flips
        if ((pierSide == PierSideEast) || (pierSide == PierSideWest))
        {
          cli();
          targetAxis1.fixed += fstepAxis1.fixed;
          targetAxis2.fixed += fstepAxis2.fixed;
          sei();
        }
      }
      moveTo();
    }

    // figure out the current Altitude
    if (rtk.m_lst % 3 == 0) do_fastalt_calc();

#ifdef MOUNT_TYPE_ALTAZM
    // figure out the current Alt/Azm tracking rates
    if (lst % 3 != 0) do_altAzmRate_calc();
#else
    // figure out the current refraction compensated tracking rate
    if (refraction && (rtk.m_lst % 3 != 0)) do_refractionRate_calc();
#endif
    // check for fault signal, stop any slew or guide and turn tracking off

    if ((faultAxis1 || faultAxis2) )
    {
      lastError = ERR_MOTOR_FAULT;
      if (!forceTracking)
      {
        if (trackingState == TrackingMoveTo)
          abortSlew = true;
        else
        {
          trackingState = TrackingOFF;
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
    if (currentAlt < minAlt  || currentAlt > maxAlt)
    {
      if (!forceTracking)
      {
        lastError = ERR_ALT;
        if (trackingState == TrackingMoveTo)
          abortSlew = true;
        else
          trackingState = TrackingOFF;
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
  // handles PPS GPS signal processing, watches safety limits, adjusts tracking rate for refraction
  unsigned long   m = millis();
  forceTracking = (m - lastSetTrakingEnable < 10000);
  if (!forceTracking) lastSetTrakingEnable = m + 10000;
  if (rtk.updateclockTimer(m))
  {
    // for testing, average steps per second
    if (debugv1 > 100000) debugv1 = 100000;
    if (debugv1 < 0) debugv1 = 0;

    debugv1 = ( debugv1 * 19 + (targetAxis1.part.m * 1000 - lasttargetAxis1) ) / 20;
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
  }
}

void CheckPierSide()
{
  cli();long pos = posAxis2;sei();
  if (pos == -quaterRotAxis2 || pos == quaterRotAxis2)
  {
    return;
  }
  bool isEast = -quaterRotAxis2 < pos && pos < quaterRotAxis2;
  if (isEast && pierSide >= PierSideWest)
  {
    cli(); blAxis2 = backlashAxis2 - blAxis2; sei();
    pierSide = PierSideEast;
  }
  else if (!isEast && pierSide < PierSideWest)
  {
    cli(); blAxis2 = backlashAxis2 - blAxis2; sei();
    pierSide = PierSideWest;
  }
}

// safety checks,
// keeps mount from tracking past the meridian limit, past the UnderPoleLimit,
// below horizon limit, above the overhead limit, or past the Dec limits
void SafetyCheck(const bool forceTracking)
{

  if (meridianFlip != MeridianFlipNever)
  {
    double HA, Dec;
    GeoAlign.GetInstr(&HA, &Dec);
    if (!checkPole(HA, pierSide, CheckModeTracking))
    {
      if ((dirAxis1 == 1 && pierSide == PierSideEast) || (dirAxis1 == 0 && pierSide == PierSideWest))
      {
        lastError = ERR_UNDER_POLE;
        if (trackingState == TrackingMoveTo)
          abortSlew = true;
        if (pierSide == PierSideEast && !forceTracking)
          trackingState = TrackingOFF;
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

    if (!checkMeridian(HA, pierSide, CheckModeTracking))
    {
      if ((dirAxis1 == 1 && pierSide == PierSideWest) || (dirAxis1 == 0 && pierSide == PierSideEast))
      {
        lastError = ERR_MERIDIAN;
        if (trackingState == TrackingMoveTo)
        {
          abortSlew = true;
        }
        if (pierSide >= PierSideWest && !forceTracking)
          trackingState = TrackingOFF;
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
#ifndef MOUNT_TYPE_ALTAZM
    // when Fork mounted, ignore pierSide and just stop the mount if it passes the UnderPoleLimit
    double HA, Dec;
    GeoAlign.GetInstr(&HA, &Dec);
    if (HA > UnderPoleLimitTracking )
    {
      lastError = ERR_UNDER_POLE;
      if (trackingState == TrackingMoveTo)
        abortSlew = true;
      else
        trackingState = TrackingOFF;
    }
    else if (lastError == ERR_UNDER_POLE)
    {
      lastError = ERR_NONE;
    }

#else
    // when Alt/Azm mounted, just stop the mount if it passes MaxAzm
    cli();
    if (posAxis1 + indexAxis1Steps >
      ((long)MaxAzm * (long)StepsPerDegreeAxis1))
    {
      lastError = ERR_AZM;
      if (trackingState == TrackingMoveTo)
        abortSlew = true;
      else
        trackingState = TrackingOFF;
    }

    sei();
#endif
  }

  // check for exceeding MinDec or MaxDec
#ifndef MOUNT_TYPE_ALTAZM
  if ((getApproxDec() < MinDec) || (getApproxDec() > MaxDec))
  {
    lastError = ERR_DEC;
    if (trackingState == TrackingMoveTo)
      abortSlew = true;
    else
      trackingState = TrackingOFF;
  }
  else if (lastError == ERR_DEC)
  {
    lastError = ERR_NONE;
  }
#endif
  // basic check to see if we're not at home
  if (trackingState != TrackingOFF) atHome = false;
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

void initmotor()
{
  readEEPROMmotor();
  updateRatios();
  tmc26XStepper1 = new TMC26XStepper(StepRotAxis1, Axis1CSPin, Axis1DirPin, Axis1StepPin, (unsigned int)LowCurrAxis1 * 10);
  tmc26XStepper2 = new TMC26XStepper(StepRotAxis2, Axis2CSPin, Axis2DirPin, Axis2StepPin, (unsigned int)LowCurrAxis2 * 10);


  tmc26XStepper1->setSpreadCycleChopper(2, 24, 8, 6, 0);
  tmc26XStepper1->setRandomOffTime(0);
  tmc26XStepper1->setMicrosteps((int)pow(2,MicroAxis1));
  tmc26XStepper1->setStallGuardThreshold(12, 0);
  tmc26XStepper1->setCoolStepConfiguration(480, 480, 1, 3, COOL_STEP_HALF_CS_LIMIT);
  tmc26XStepper1->setCoolStepEnabled(false);
  tmc26XStepper1->start();
  tmc26XStepper2->setSpreadCycleChopper(2, 24, 8, 6, 0);
  tmc26XStepper2->setRandomOffTime(0);
  tmc26XStepper2->setMicrosteps((int)pow(2,MicroAxis2));
  tmc26XStepper2->setStallGuardThreshold(12, 0);
  tmc26XStepper2->setCoolStepConfiguration(480, 57, 1, 3, COOL_STEP_HALF_CS_LIMIT);
  tmc26XStepper2->setCoolStepEnabled(false);
  tmc26XStepper2->start();
}

void readEEPROMmotor()
{
  backlashAxis1 = EEPROM_readInt(EE_backlashAxis1);
  GearAxis1 = EEPROM_readInt(EE_GearAxis1);
  StepRotAxis1 = EEPROM_readInt(EE_StepRotAxis1);
  MicroAxis1 = EEPROM.read(EE_MicroAxis1);
  ReverseAxis1 = EEPROM.read(EE_ReverseAxis1);
  LowCurrAxis1 = EEPROM.read(EE_LowCurrAxis1);
  HighCurrAxis1 = EEPROM.read(EE_HighCurrAxis1);

  backlashAxis2 = EEPROM_readInt(EE_backlashAxis2);
  GearAxis2 = EEPROM_readInt(EE_GearAxis2);
  StepRotAxis2 = EEPROM_readInt(EE_StepRotAxis2);
  MicroAxis2 = EEPROM.read(EE_MicroAxis2);
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
  StepsPerRotAxis1 = (long)GearAxis1 * StepRotAxis1 * (int)pow(2,MicroAxis1); // calculated as    :  stepper_steps * micro_steps * gear_reduction1 * (gear_reduction2/360)
  StepsPerRotAxis2 = (long)GearAxis2 * StepRotAxis2 * (int)pow(2,MicroAxis2); // calculated as    :  stepper_steps * micro_steps * gear_reduction1 * (gear_reduction2/360)
  StepsPerDegreeAxis1 = (double)StepsPerRotAxis1 / 360.0;
  StepsPerDegreeAxis2 = (double)StepsPerRotAxis2 / 360.0;
  StepsPerSecondAxis1 = StepsPerDegreeAxis1 / 240.0;
  StepsPerSecondAxis2 = StepsPerDegreeAxis2 / 240.0;


  timerRateRatio = (StepsPerSecondAxis1 / StepsPerSecondAxis2);
  useTimerRateRatio = (StepsPerRotAxis1 != StepsPerRotAxis2);
  sei();

  halfRotAxis1 = StepsPerRotAxis1 / 2L;
  quaterRotAxis1 = StepsPerRotAxis1 / 4L;
  halfRotAxis2 = StepsPerRotAxis2 / 2L;
  quaterRotAxis2 = StepsPerRotAxis2 / 4L;


#ifdef MOUNT_TYPE_GEM
    celestialPoleStepAxis1 = quaterRotAxis1;
#endif
#if defined(MOUNT_TYPE_FORK) || defined(MOUNT_TYPE_FORK_ALT) || defined \
        (MOUNT_TYPE_ALTAZM)
   celestialPoleStepAxis1 = 0L;
#endif
   celestialPoleStepAxis2 = quaterRotAxis2;
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
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

void reboot()
{
  Serial.end();
  Serial1.end();
  Focus_Serial.end();
  GNSS_Serial.end();
  delay(1000);
#if defined(ARDUINO_TEENSY40)  || defined(ARDUINO_TEENSY_MICROMOD) || defined(ARDUINO_TEENSY32)
#define CPU_RESTART_ADDR (uint32_t *)0xE000ED0C
#define CPU_RESTART_VAL 0x5FA0004
#define CPU_RESTART (*CPU_RESTART_ADDR = CPU_RESTART_VAL);
  CPU_RESTART;
#endif


}

void setup()
{
  pinMode(LEDPin, OUTPUT);
  //GNSS connection
#if VERSION != 220
  GNSS_Serial.begin(9600);
#endif
  for (int k = 0; k < 20; k++)
  {
    digitalWrite(LEDPin, HIGH);
    delay(10);
    digitalWrite(LEDPin, LOW);
    delay(50);
  }
  digitalWrite(LEDPin, HIGH);


  // Check that EEPROM is ok
  AutoinitEEPROM();
  
  // get the mount ready
  initMount();
  
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
  setupST4();

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
  siderealClockSpeed = (double)XEEPROM.readLong(getMountAddress(EE_siderealClockSpeed))/16.0;
  updateSideral();
  beginTimers();

  rtk.resetLongitude(*localSite.longitude());
  // get the Park status
  if (!iniAtPark())
  {
    syncAtHome();
  }

  // get the pulse-guide rate
  int val = EEPROM.read(getMountAddress(EE_Rate0));
  guideRates[0] = val > 0 ? (float)val / 100 : DefaultR0;
  val = EEPROM.read(getMountAddress(EE_Rate1));
  guideRates[1] = val > 0 ? (float)val : DefaultR1;
  val = EEPROM.read(getMountAddress(EE_Rate2));
  guideRates[2] = val > 0 ? (float)val : DefaultR2;
  val = EEPROM.read(getMountAddress(EE_Rate3));
  guideRates[3] = val > 0 ? (float)val : DefaultR3;
 
  recenterGuideRate = min(EEPROM.read(getMountAddress(EE_DefaultRate)), 4);
  activeGuideRate = recenterGuideRate;
  resetGuideRate();

  delay(10);

  // prep timers
  rtk.updateTimers();
  delay(2000);
  hasGNSS = GNSS_Serial.available() > 0;

  //Focuser connection
  Focus_Serial.setRX(FocuserRX);
  Focus_Serial.setTX(FocuserTX);
  Focus_Serial.begin(56000);
  Focus_Serial.setTimeout(10);
  Focus_Serial.write(":F?#");
  Focus_Serial.flush();
  delay(250);
  char ret;
  while (Focus_Serial.available() > 0)
  {
    ret = Focus_Serial.read();
    if (ret == '?')
    {
      hasFocuser = true;
    }
  }

  // get ready for serial communications
  Serial.begin(BAUD);
  S_USB.attach_Stream((Stream*)&Serial, COMMAND_SERIAL);
  Serial1.begin(57600);
  S_SHC.attach_Stream((Stream*)&Serial1, COMMAND_SERIAL1);

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
  // ENCODER -------------------------------------------------------------------------------------------
  if (encoderA1.calibrating() != encoderA2.calibrating())
  {
    encoderA1.delRef();
    encoderA2.delRef();
  }
  if (!movingTo && GuidingState == GuidingOFF && rtk.m_lst % 10)
  {
    EncoderSync mode = PushtoStatus == PT_OFF ? EncodeSyncMode : ES_ALWAYS;
    if (autoSyncWithEncoder(mode))
    {
      if (atHome) atHome = false;
      if (parkStatus != PRK_UNPARKED) parkStatus = PRK_UNPARKED;
    }
  }


  // 0.01 SECOND TIMED ---------------------------------------------------------------------------------

  if (rtk.updatesiderealTimer())
  {
    // update Target position
    if (sideralTracking)
    {
      cli();
      if (!staA1.backlash_correcting)
      {
        staA1.target += staA1.fstep;
      }
      if (!staA2.backlash_correcting)
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
      currentAlt = getHorTopo().Alt()*RAD_TO_DEG;
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
    UpdateGnss();
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
      if ((staA1.dir && currentSide == PIER_WEST) || (!staA1.dir && currentSide == PIER_EAST))
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

    if (!checkPole(axis1, axis2, CHECKMODE_TRACKING))
    {
      if ((staA1.dir && currentSide == PIER_EAST) || (!staA1.dir && currentSide == PIER_WEST))
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
  if (atHome && lastError != ERRT_NONE)
  {
    unsetHome();
    syncAtHome();
  }
  if (parkStatus == PRK_PARKED && lastError != ERRT_NONE)
  {
    unsetPark();
    syncAtHome();
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

void updateRatios(bool deleteAlignment, bool deleteHP)
{
  cli();
  geoA1.setstepsPerRot((double)motorA1.gear / 1000.0 * motorA1.stepRot * pow(2, motorA1.micro));
  geoA2.setstepsPerRot((double)motorA2.gear / 1000.0 * motorA2.stepRot * pow(2, motorA2.micro));
  staA1.setBacklash_inSteps(motorA1.backlashAmount , geoA1.stepsPerArcSecond);
  staA2.setBacklash_inSteps(motorA2.backlashAmount , geoA2.stepsPerArcSecond);
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
  staA1.setSidereal(siderealClockSpeed, geoA1.stepsPerSecond, masterClockSpeed, motorA1.backlashRate);
  staA2.setSidereal(siderealClockSpeed, geoA2.stepsPerSecond, masterClockSpeed, motorA2.backlashRate);
  sei();

  SetTrackingRate(default_tracking_rate,0);

  // initialize the sidereal clock, RA, and Dec
  SetsiderealClockSpeed(siderealClockSpeed);
}
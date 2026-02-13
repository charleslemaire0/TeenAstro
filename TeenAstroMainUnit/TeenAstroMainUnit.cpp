/*
 * Title       TeenAstro
 * by          Howard Dutton, Charles Lemaire, Markus Noga, Francois Desvallee
 *
 * Copyright (C) 2012 to 2016 On-Step by Howard Dutton
 * Copyright (C) 2016 to 2024 TeenAstro by Charles Lemaire, Markus Noga, Francois Desvallee
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
 * Revision History, see GitHub
 *
 * Author: Howard Dutton, Charles Lemaire
 *
 * Description: Arduino Stepper motor controller for Telescope mounts
 * with LX200 derived command set.
 * Main entry (setup/loop) - C++ build.
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

  AutoinitEEPROM();
  initMount();

  setSyncProvider(rtk.getTime);
  setSyncInterval(1);
  setTime(rtk.getTime());

#ifdef RETICULE_LED_PINS
  pinMode(RETICULE_LED_PINS, OUTPUT);
  analogWrite(RETICULE_LED_PINS, mount.reticuleBrightness);
#endif

  setupST4();

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

  enable_Axis(false);
  DecayModeTracking();

  mount.siderealClockSpeed = (double)XEEPROM.readLong(getMountAddress(EE_siderealClockSpeed))/16.0;
  updateSideral();
  beginTimers();

  rtk.resetLongitude(*localSite.longitude());
  if (!iniAtPark())
  {
    syncAtHome();
  }

  int val = EEPROM.read(getMountAddress(EE_Rate0));
  mount.guideRates[0] = val > 0 ? (float)val / 100 : DefaultR0;
  val = EEPROM.read(getMountAddress(EE_Rate1));
  mount.guideRates[1] = val > 0 ? (float)val : DefaultR1;
  val = EEPROM.read(getMountAddress(EE_Rate2));
  mount.guideRates[2] = val > 0 ? (float)val : DefaultR2;
  val = EEPROM.read(getMountAddress(EE_Rate3));
  mount.guideRates[3] = val > 0 ? (float)val : DefaultR3;

  mount.recenterGuideRate = min(EEPROM.read(getMountAddress(EE_DefaultRate)), 4);
  mount.activeGuideRate = mount.recenterGuideRate;
  resetGuideRate();

  delay(10);

  rtk.updateTimers();
  delay(2000);
  mount.hasGNSS = GNSS_Serial.available() > 0;

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
      mount.hasFocuser = true;
    }
  }

  Serial.begin(BAUD);
  commandState.S_USB_.attach_Stream((Stream*)&Serial, COMMAND_SERIAL);
  Serial1.begin(57600);
  commandState.S_SHC_.attach_Stream((Stream*)&Serial1, COMMAND_SERIAL1);

  digitalWrite(LEDPin, LOW);
}

void loop()
{
  static bool forceTracking = false;
  static unsigned long m;
  static long phase;
  static ErrorsTraking StartLoopError = ERRT_NONE;
  StartLoopError = mount.lastError;

  if (!mount.movingTo && mount.enableMotor)
  {
    checkST4();
    CheckSpiral();
    Guide();
  }

  if (mount.encoderA1.calibrating() != mount.encoderA2.calibrating())
  {
    mount.encoderA1.delRef();
    mount.encoderA2.delRef();
  }
  if (!TelescopeBusy() && rtk.m_lst % 10)
  {
    EncoderSync mode = mount.PushtoStatus == PT_OFF ? mount.EncodeSyncMode : ES_ALWAYS;
    if (autoSyncWithEncoder(mode))
    {
      if (mount.atHome) mount.atHome = false;
      if (mount.parkStatus != PRK_UNPARKED) mount.parkStatus = PRK_UNPARKED;
    }
  }

  if (rtk.updatesiderealTimer())
  {
    if (mount.sideralTracking)
    {
      cli();
      if (!mount.staA1.backlash_correcting)
      {
        mount.staA1.target += mount.staA1.fstep;
      }
      if (!mount.staA2.backlash_correcting)
      {
        mount.staA2.target += mount.staA2.fstep;
      }
      sei();
    }
    if (mount.movingTo)
    {
      moveTo();
    }

    phase = rtk.m_lst % 100;
    if (phase % 20 == 0)
    {
      mount.currentAlt = getHorTopo().Alt()*RAD_TO_DEG;
    }
    if (phase == 0)
    {
      computeTrackingRate(true);
    }

    CheckEndOfMoveAxisAtRate();

    if (mount.staA1.fault || mount.staA2.fault)
    {
      mount.lastError = ERRT_MOTOR_FAULT;
      if (!forceTracking)
      {
        if (mount.movingTo)
          mount.abortSlew = true;
        else
        {
          mount.sideralTracking = false;
          mount.guideA1.brake();
          mount.guideA2.brake();
        }
      }
    }
    else if (mount.lastError == ERRT_MOTOR_FAULT)
    {
      mount.lastError = ERRT_NONE;
    }
    if (mount.currentAlt < mount.minAlt || mount.currentAlt > mount.maxAlt)
    {
      if (!forceTracking)
      {
        mount.lastError = ERRT_ALT;
        if (mount.movingTo)
          mount.abortSlew = true;
        else
          mount.sideralTracking = false;
      }
    }
    else if (mount.lastError == ERRT_ALT)
    {
      mount.lastError = ERRT_NONE;
    }
  }

  tlp.monitor();

  m = millis();
  forceTracking = (m - mount.lastSetTrakingEnable < 10000);
  if (!forceTracking) mount.lastSetTrakingEnable = m + 10000;
  SafetyCheck(forceTracking);

  processCommands();
  UpdateGnss();

  if (StartLoopError != mount.lastError)
  {
    mount.lastError == ERRT_NONE ? digitalWrite(LEDPin, LOW) : digitalWrite(LEDPin, HIGH);
  }
}

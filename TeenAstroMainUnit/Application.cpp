/*
 * Title       TeenAstro
 * by          Howard Dutton, Charles Lemaire, Markus Noga, Francois Desvalee
 *
 * Copyright (C) 2012 to 2016 On-Step by Howard Dutton
 * Copyright (C) 2016 to 2024 TeenAstro by Charles Lemaire, Markus Noga, Francois Desvalee
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
 * Description: Application setup and main loop. See Application.h.
 */
#include "Command.h"
#include "Application.h"
#include "FirmwareDef.h"
#include "XEEPROM.hpp"
#include <Arduino.h>
#include <TimeLib.h>

namespace {
constexpr int STARTUP_BLINK_COUNT = 20;
constexpr unsigned long STARTUP_BLINK_ON_MS = 10;
constexpr unsigned long STARTUP_BLINK_OFF_MS = 50;
constexpr unsigned long PERIPHERALS_SETTLE_MS = 2000;
constexpr unsigned long FOCUSER_PROBE_DELAY_MS = 250;
constexpr unsigned long FORCE_TRACKING_GRACE_MS = 10000;
constexpr long SIDEREAL_PHASE_MOD = 100;
constexpr long ENCODER_SYNC_SKIP_MOD = 10;
constexpr int FOCUSER_BAUD = 56000;
constexpr int FOCUSER_TIMEOUT_MS = 10;
}  // namespace

// -----------------------------------------------------------------------------
// Setup: high-level sequence
// -----------------------------------------------------------------------------

void Application::setup()
{
  setupStartupBlink();
  setupEepromAndMount();
  setupTimeSync();
  setupReticule();
  setupMountPinsAndSidereal();
  setupParkHomeAndGuideRates();
  setupGnssProbe();
  setupFocuserProbe();
  setupCommandSerial();
}

// LED blink and optional GNSS serial begin.
void Application::setupStartupBlink()
{
  pinMode(LEDPin, OUTPUT);
#if VERSION != 220
  GNSS_Serial.begin(9600);
#endif
  for (int k = 0; k < STARTUP_BLINK_COUNT; k++)
  {
    digitalWrite(LEDPin, HIGH);
    delay(STARTUP_BLINK_ON_MS);
    digitalWrite(LEDPin, LOW);
    delay(STARTUP_BLINK_OFF_MS);
  }
  digitalWrite(LEDPin, HIGH);
}

// Initialize EEPROM and mount.
void Application::setupEepromAndMount()
{
  AutoinitEEPROM();
  mount.init();
}

// Set TimeLib sync from RTC/timer.
void Application::setupTimeSync()
{
  setSyncProvider(rtk.getTime);
  setSyncInterval(1);
  setTime(rtk.getTime());
}

// Reticule LED pins and brightness (if defined).
void Application::setupReticule()
{
#ifdef RETICULE_LED_PINS
  pinMode(RETICULE_LED_PINS, OUTPUT);
  // Reticule: direct member access (see Mount access convention).
  analogWrite(RETICULE_LED_PINS, mount.reticule.reticuleBrightness);
#endif
}

// ST4, fault pins, axes, sidereal and timers.
void Application::setupMountPinsAndSidereal()
{
  mount.setupST4();
  mount.configureFaultPins();
  mount.axes.enable(false);
  mount.decayModeTracking();
  mount.loadSiderealFromEEPROMAndStartTimers();
}

// Longitude, park/home init, guide rates from EEPROM.
void Application::setupParkHomeAndGuideRates()
{
  rtk.resetLongitude(*localSite.longitude());
  if (!mount.iniAtPark())
    mount.syncAtHome();
  mount.loadGuideRatesFromEEPROM();
  delay(STARTUP_BLINK_ON_MS);
}

// Update timers and detect GNSS availability.
void Application::setupGnssProbe()
{
  rtk.updateTimers();
  delay(PERIPHERALS_SETTLE_MS);
  mount.config.peripherals.hasGNSS = GNSS_Serial.available() > 0;
}

// Probe focuser via :F?# and set hasFocuser.
void Application::setupFocuserProbe()
{
  Focus_Serial.setRX(FocuserRX);
  Focus_Serial.setTX(FocuserTX);
  Focus_Serial.begin(FOCUSER_BAUD);
  Focus_Serial.setTimeout(FOCUSER_TIMEOUT_MS);
  Focus_Serial.write(":F?#");
  Focus_Serial.flush();
  delay(FOCUSER_PROBE_DELAY_MS);
  char ret;
  while (Focus_Serial.available() > 0)
  {
    ret = Focus_Serial.read();
    if (ret == '?')
      mount.config.peripherals.hasFocuser = true;
  }
}

// Serial/USB/SHC and turn LED off.
void Application::setupCommandSerial()
{
  Serial.begin(BAUD);
  commandState.S_USB_.attach_Stream((Stream*)&Serial, COMMAND_SERIAL);
  Serial1.begin(57600);
  commandState.S_SHC_.attach_Stream((Stream*)&Serial1, COMMAND_SERIAL1);
  digitalWrite(LEDPin, LOW);
}

// -----------------------------------------------------------------------------
// Loop: high-level sequence
// -----------------------------------------------------------------------------

void Application::loop()
{
  static bool forceTracking = false;
  static ErrorsTraking startLoopError = ERRT_NONE;

  startLoopError = mount.errors.lastError;
  loopSt4AndGuiding();
  loopEncoderSync();
  loopSiderealAndSafety(forceTracking);
  loopCommandsAndStatus(startLoopError);
}

// ST4, spiral, guide when not moving and motors enabled.
void Application::loopSt4AndGuiding()
{
  if (!mount.tracking.movingTo && mount.motorsEncoders.enableMotor)
  {
    mount.checkST4();
    mount.checkSpiral();
    mount.guide();
  }
}

// Encoder calibration fix and sync when not slewing.
void Application::loopEncoderSync()
{
  if (mount.motorsEncoders.encoderA1.calibrating() != mount.motorsEncoders.encoderA2.calibrating())
  {
    mount.motorsEncoders.encoderA1.delRef();
    mount.motorsEncoders.encoderA2.delRef();
  }
  if (!mount.isSlewing())
    mount.updateEncoderSync(rtk.m_lst % ENCODER_SYNC_SKIP_MOD != 0);
}

// Compute force-tracking grace state from last enable time.
void Application::updateForceTracking(bool& forceTracking)
{
  unsigned long m = millis();
  forceTracking = (m - mount.tracking.lastSetTrakingEnable < FORCE_TRACKING_GRACE_MS);
  if (!forceTracking)
    mount.tracking.lastSetTrakingEnable = m + FORCE_TRACKING_GRACE_MS;
}

// Sidereal tick, timer loop, then safety check.
void Application::loopSiderealAndSafety(bool& forceTracking)
{
  if (rtk.updatesiderealTimer())
  {
    long phase = rtk.m_lst % SIDEREAL_PHASE_MOD;
    updateForceTracking(forceTracking);
    mount.onSiderealTick(phase, forceTracking);
  }

  tlp.monitor();
  updateForceTracking(forceTracking);
  mount.safetyCheck(forceTracking);
}

// Process commands, update GNSS, update status LED on error change.
void Application::loopCommandsAndStatus(ErrorsTraking startLoopError)
{
  processCommands();
  UpdateGnss();
  if (startLoopError != mount.errors.lastError)
    mount.updateStatusLed(LEDPin);
}

Application application;
timerLoop tlp;  // single definition — declared extern in timerLoop.hpp

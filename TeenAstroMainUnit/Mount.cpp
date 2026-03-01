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
 * Description: Mount class constructor, init, refrOpt, ST4/park/home delegation. See Mount.h.
 */
#include "MainUnit.h"
#include "XEEPROM.hpp"
#include <EEPROM.h>
#include <Arduino.h>

// -----------------------------------------------------------------------------
// Constructor and global instance
// -----------------------------------------------------------------------------
Mount::Mount() : limits(*this), guiding(*this), limitManager(*this), st4(*this), parkHomeController(*this)
{
  parkHome.parkStatus = ParkState::PRK_UNPARKED;
  parkHome.parkSaved = false;
  parkHome.homeSaved = false;
  parkHome.atHome = true;
  parkHome.homeMount = false;
  parkHome.slewSettleDuration = 0U;
  parkHome.lastSettleTime = 0U;
  parkHome.settling = false;
  parkHome.backlashStatus = BacklashPhase::DONE;
  config.identity.DecayModeTrack = false;
  config.identity.meridianFlip = MeridianFlip::FLIP_NEVER;
  config.identity.mountType = MOUNT_TYPE_GEM;
  memset(config.identity.mountName, 0, sizeof(config.identity.mountName));
  config.identity.isMountTypeFix = false;
  alignment.maxAlignNumStar = 0;
  alignment.autoAlignmentBySync = false;
  alignment.hasValid = false;
  config.peripherals.PushtoStatus = Pushto::PT_OFF;
  config.peripherals.hasFocuser = false;
  config.peripherals.hasGNSS = true;
  refraction.forPole = false;
  refraction.forGoto = false;
  refraction.forTracking = false;
  targetCurrent.newTargetPoleSide = POLE_NOTVALID;
  targetCurrent.newTargetAlt = 0.0;
  targetCurrent.newTargetAzm = 0.0;
  targetCurrent.newTargetDec = 0.0;
  targetCurrent.newTargetRA = 0.0;
  targetCurrent.currentAzm = 0.0;
  targetCurrent.currentAlt = 45.0;
  errors.lastError = ErrorsTraking::ERRT_NONE;
  tracking.siderealClockSpeed = 997269.5625;
  tracking.trackComp = TrackingCompensation::TC_BOTH;
  tracking.lastSideralTracking = false;
  tracking.sideralTracking = false;
  tracking.sideralMode = SID_Mode::SIDM_STAR;
  tracking.RequestedTrackingRateHA = TrackingStar;
  tracking.RequestedTrackingRateDEC = 0.0;
  tracking.storedTrakingRateRA = 0;
  tracking.storedTrakingRateDEC = 0;
  tracking.lastSetTrakingEnable = 0;
  tracking.lastSecurityCheck = 0;
  tracking.abortSlew = false;
  tracking.doSpiral = false;
  tracking.SpiralFOV = 1.0;
  tracking.movingTo = false;
  guiding.GuidingState = Guiding::GuidingOFF;
  guiding.lastGuidingState = Guiding::GuidingOFF;
  guiding.guideRates[0] = DefaultR0;
  guiding.guideRates[1] = DefaultR1;
  guiding.guideRates[2] = DefaultR2;
  guiding.guideRates[3] = DefaultR3;
  guiding.guideRates[4] = DefaultR4;
  guiding.activeGuideRate = static_cast<byte>(GuideRate::RX);
  guiding.recenterGuideRate = static_cast<byte>(GuideRate::RX);
  guiding.pulseGuideRate = 0.25f;
  guiding.DegreesForAcceleration = 3.0;
  motorsEncoders.enableMotor = false;
  motorsEncoders.enableEncoder = false;
  motorsEncoders.EncodeSyncMode = EncoderSync::ES_OFF;
  motorsEncoders.minInterval1 = StepsMinInterval;
  motorsEncoders.maxInterval1 = StepsMaxInterval;
  motorsEncoders.minInterval2 = StepsMinInterval;
  motorsEncoders.maxInterval2 = StepsMaxInterval;
  motorsEncoders.reboot_unit = false;
#ifdef RETICULE_LED_PINS
  reticule.reticuleBrightness = 255;
#endif
}

Mount mount;

// -----------------------------------------------------------------------------
// ST4, park/home delegation, guiding delegation, init
// -----------------------------------------------------------------------------
void Mount::setupST4() { st4.setup(); }
void Mount::checkST4() { st4.check(); }

void Mount::guide() { guiding.guide(); }
void Mount::stopGuiding() { guiding.stopGuiding(); }
bool Mount::isGuidingStar() const { return guiding.isGuidingStar(); }
bool Mount::stopIfMountError() { return guiding.stopIfMountError(); }
void Mount::performPulseGuiding() { guiding.performPulseGuiding(); }
void Mount::performST4Guiding() { guiding.performST4Guiding(); }
void Mount::performGuidingRecenter() { guiding.performGuidingRecenter(); }
void Mount::performGuidingAtRate() { guiding.performGuidingAtRate(); }

bool Mount::setPark() { return parkHomeController.setPark(); }
void Mount::unsetPark() { parkHomeController.unsetPark(); }
void Mount::parkClearBacklash() { parkHomeController.parkClearBacklash(); }
void Mount::finalizePark() { parkHomeController.finalizePark(); }
byte Mount::park() { return parkHomeController.park(); }
bool Mount::syncAtPark() { return parkHomeController.syncAtPark(); }
bool Mount::iniAtPark() { return parkHomeController.iniAtPark(); }
void Mount::unpark() { parkHomeController.unpark(); }

bool Mount::setHome() { return parkHomeController.setHome(); }
void Mount::unsetHome() { parkHomeController.unsetHome(); }
bool Mount::goHome() { return parkHomeController.goHome(); }
void Mount::finalizeHome() { return parkHomeController.finalizeHome(); }
bool Mount::syncAtHome() { return parkHomeController.syncAtHome(); }
void Mount::initHome() { parkHomeController.initHome(); }

LA3::RefrOpt Mount::refrOptForPole() const
{
  return { refraction.forPole, temperature, pressure };
}
LA3::RefrOpt Mount::refrOptForGoto() const
{
  return { refraction.forGoto, temperature, pressure };
}
LA3::RefrOpt Mount::refrOptForTracking() const
{
  return { refraction.forTracking, temperature, pressure };
}

void Mount::init()
{
  ::initMount();
}

void Mount::loadGuideRatesFromEEPROM()
{
  int val = EEPROM.read(getMountAddress(EE_Rate0));
  guiding.guideRates[0] = val > 0 ? (float)val / 100 : DefaultR0;
  val = EEPROM.read(getMountAddress(EE_Rate1));
  guiding.guideRates[1] = val > 0 ? (float)val : DefaultR1;
  val = EEPROM.read(getMountAddress(EE_Rate2));
  guiding.guideRates[2] = val > 0 ? (float)val : DefaultR2;
  val = EEPROM.read(getMountAddress(EE_Rate3));
  guiding.guideRates[3] = val > 0 ? (float)val : DefaultR3;
  guiding.recenterGuideRate = min((int)EEPROM.read(getMountAddress(EE_DefaultRate)), 4);
  guiding.activeGuideRate = guiding.recenterGuideRate;
  resetGuideRate();
}

void Mount::loadSiderealFromEEPROMAndStartTimers()
{
  tracking.siderealClockSpeed = (double)XEEPROM.readLong(getMountAddress(EE_siderealClockSpeed)) / 16.0;
  updateSideral();
  beginTimers();
}

void Mount::configureFaultPins()
{
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
#ifdef AXIS2_FAULT_HIGH
  pinMode(Axis2_FAULT, INPUT_PULLDOWN);
#endif
#else
  pinMode(Axis2_FAULT, INPUT);
#endif
#endif
}

void Mount::updateStatusLed(int ledPin) const
{
  if (errors.lastError == ERRT_NONE)
    digitalWrite(ledPin, LOW);
  else
    digitalWrite(ledPin, HIGH);
}

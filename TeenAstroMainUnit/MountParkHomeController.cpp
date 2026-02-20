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
 * Description: Mount park/home controller - setPark, unpark, setHome, goHome, sync. See MountParkHomeController.h.
 */
#include "MountParkHomeController.h"
#include "Mount.h"
#include "MainUnit.h"

MountParkHomeController::MountParkHomeController(Mount& mount) : mount_(mount) {}

bool MountParkHomeController::setPark()
{
  if ((mount_.parkHome.parkStatus == PRK_UNPARKED) && !mount_.isSlewing())
  {
    mount_.tracking.lastSideralTracking = mount_.tracking.sideralTracking;
    mount_.tracking.sideralTracking = false;
    long h = (mount_.axes.staA1.target / 1024L) * 1024L;
    long d = (mount_.axes.staA2.target / 1024L) * 1024L;
    h /= pow(2, mount_.motorsEncoders.motorA1.micro);
    d /= pow(2, mount_.motorsEncoders.motorA2.micro);
    XEEPROM.writeLong(getMountAddress(EE_posAxis1), h);
    XEEPROM.writeLong(getMountAddress(EE_posAxis2), d);
    saveAlignModel();
    mount_.parkHome.parkSaved = true;
    XEEPROM.write(getMountAddress(EE_parkSaved), mount_.parkHome.parkSaved);
    mount_.tracking.sideralTracking = mount_.tracking.lastSideralTracking;
    return true;
  }
  return false;
}

void MountParkHomeController::unsetPark()
{
  if (mount_.parkHome.parkSaved)
  {
    mount_.parkHome.parkSaved = false;
    XEEPROM.write(getMountAddress(EE_parkSaved), mount_.parkHome.parkSaved);
  }
}

void MountParkHomeController::parkClearBacklash()
{
  static long LastIntervalAxis1, LastIntervalAxis2;
  if ((mount_.axes.staA1.backlash_inSteps == 0 && mount_.axes.staA2.backlash_inSteps == 0))
  {
    mount_.parkHome.backlashStatus = BacklashPhase::DONE;
    return;
  }
  switch (mount_.parkHome.backlashStatus)
  {
  case BacklashPhase::INIT:
  {
    cli();
    LastIntervalAxis1 = mount_.axes.staA1.interval_Step_Cur;
    LastIntervalAxis2 = mount_.axes.staA2.interval_Step_Cur;
    sei();
    long axis1Target = mount_.axes.staA1.target + mount_.axes.staA1.backlash_inSteps;
    long axis2Target = mount_.axes.staA2.target + mount_.axes.staA2.backlash_inSteps;
    mount_.gotoAxis(&axis1Target, &axis2Target);
    mount_.parkHome.backlashStatus = BacklashPhase::MOVE_IN;
    return;
  }
  case BacklashPhase::MOVE_IN:
  {
    mount_.axes.updateDeltaTarget();
    if (mount_.axes.staA1.backlash_movedSteps == mount_.axes.staA1.backlash_inSteps && mount_.axes.staA1.deltaTarget == 0 &&
      mount_.axes.staA2.backlash_movedSteps == mount_.axes.staA2.backlash_inSteps && mount_.axes.staA2.deltaTarget == 0)
    {
      long axis1Target = mount_.axes.staA1.target - mount_.axes.staA1.backlash_inSteps;
      long axis2Target = mount_.axes.staA2.target - mount_.axes.staA2.backlash_inSteps;
      mount_.gotoAxis(&axis1Target, &axis2Target);
      mount_.parkHome.backlashStatus = MOVE_OUT;
    }
    return;
  }
  case BacklashPhase::MOVE_OUT:
  {
    mount_.axes.updateDeltaTarget();
    if (mount_.axes.staA1.backlash_movedSteps == 0 && mount_.axes.staA1.deltaTarget == 0 &&
      mount_.axes.staA2.backlash_movedSteps == 0 && mount_.axes.staA2.deltaTarget == 0)
    {
      cli();
      mount_.axes.staA1.interval_Step_Cur = LastIntervalAxis1;
      mount_.axes.staA2.interval_Step_Cur = LastIntervalAxis2;
      sei();
      mount_.parkHome.backlashStatus = DONE;
    }
    return;
  }
  default:
    break;
  }
}

void MountParkHomeController::finalizePark()
{
  if (mount_.parkHome.backlashStatus == DONE)
  {
    mount_.parkHome.backlashStatus = INIT;
  }
  parkClearBacklash();
  if (mount_.parkHome.backlashStatus == DONE)
  {
    mount_.tracking.movingTo = false;
    mount_.parkHome.parkStatus = PRK_PARKED;
    mount_.axes.enable(false);
    XEEPROM.write(getMountAddress(EE_parkStatus), mount_.parkHome.parkStatus);
  }
}

uint8_t MountParkHomeController::park()
{
  if (mount_.parkHome.parkStatus == PRK_PARKED)
    return 0;
  if (!mount_.parkHome.parkSaved)
    return 1;
  if (mount_.parkHome.parkStatus != PRK_UNPARKED)
    return 2;
  if (mount_.isSlewing())
    return 3;
  if (mount_.errors.lastError != ERRT_NONE)
    return 4;
  if (!mount_.motorsEncoders.enableMotor)
    return 5;
  long h = XEEPROM.readLong(getMountAddress(EE_posAxis1));
  long d = XEEPROM.readLong(getMountAddress(EE_posAxis2));
  h *= pow(2, mount_.motorsEncoders.motorA1.micro);
  d *= pow(2, mount_.motorsEncoders.motorA2.micro);
  mount_.tracking.lastSideralTracking = false;
  mount_.tracking.sideralTracking = false;
  mount_.parkHome.parkStatus = PRK_PARKING;
  XEEPROM.write(getMountAddress(EE_parkStatus), mount_.parkHome.parkStatus);
  mount_.gotoAxis(&h, &d);
  return 0;
}

bool MountParkHomeController::syncAtPark()
{
  if (!mount_.parkHome.parkSaved)
    return false;
  mount_.parkHome.atHome = false;
  mount_.axes.staA1.enable = true;
  mount_.axes.staA2.enable = true;
  delay(10);
  long axis1, axis2;
  axis1 = XEEPROM.readLong(getMountAddress(EE_posAxis1));
  axis2 = XEEPROM.readLong(getMountAddress(EE_posAxis2));
  axis1 *= pow(2, mount_.motorsEncoders.motorA1.micro);
  axis2 *= pow(2, mount_.motorsEncoders.motorA2.micro);
  mount_.syncAxis(&axis1, &axis2);
  mount_.config.identity.meridianFlip = mount_.config.identity.mountType == MOUNT_TYPE_GEM ? FLIP_ALWAYS : FLIP_NEVER;
  mount_.syncEwithT();
  mount_.decayModeTracking();
  return true;
}

bool MountParkHomeController::iniAtPark()
{
  mount_.parkHome.parkSaved = XEEPROM.read(getMountAddress(EE_parkSaved));
  if (!mount_.parkHome.parkSaved)
  {
    mount_.parkHome.parkStatus = PRK_UNPARKED;
    return false;
  }
  byte parkStatusRead = XEEPROM.read(getMountAddress(EE_parkStatus));
  bool ok = false;
  switch (parkStatusRead)
  {
  case PRK_PARKED:
    if (syncAtPark())
    {
      mount_.parkHome.parkStatus = PRK_PARKED;
      ok = true;
    }
    else
    {
      mount_.parkHome.parkStatus = PRK_UNPARKED;
      XEEPROM.write(getMountAddress(EE_parkStatus), PRK_UNPARKED);
    }
    break;
  case PRK_UNPARKED:
    mount_.parkHome.parkStatus = PRK_UNPARKED;
    return false;
  default:
    mount_.parkHome.parkStatus = PRK_UNPARKED;
    XEEPROM.write(getMountAddress(EE_parkStatus), PRK_UNPARKED);
    break;
  }
  return ok;
}

void MountParkHomeController::unpark()
{
  if (mount_.parkHome.parkStatus == PRK_UNPARKED)
    return;
  mount_.parkHome.parkStatus = PRK_UNPARKED;
  XEEPROM.write(getMountAddress(EE_parkStatus), mount_.parkHome.parkStatus);
  if (mount_.motorsEncoders.enableMotor)
  {
    mount_.startSideralTracking();
  }
}

bool MountParkHomeController::setHome()
{
  if ((mount_.parkHome.parkStatus == PRK_UNPARKED) && !mount_.tracking.movingTo)
  {
    mount_.tracking.lastSideralTracking = mount_.tracking.sideralTracking;
    mount_.tracking.sideralTracking = false;
    long h = (mount_.axes.staA1.target / 1024L) * 1024L;
    long d = (mount_.axes.staA2.target / 1024L) * 1024L;
    h /= pow(2, mount_.motorsEncoders.motorA1.micro);
    d /= pow(2, mount_.motorsEncoders.motorA2.micro);
    XEEPROM.writeLong(getMountAddress(EE_homePosAxis1), h);
    XEEPROM.writeLong(getMountAddress(EE_homePosAxis2), d);
    XEEPROM.write(getMountAddress(EE_homeSaved), 1);
    initHome();
    mount_.tracking.sideralTracking = mount_.tracking.lastSideralTracking;
    return true;
  }
  return false;
}

void MountParkHomeController::unsetHome()
{
  XEEPROM.update(getMountAddress(EE_homeSaved), 0);
  initHome();
}

bool MountParkHomeController::goHome()
{
  if (!mount_.motorsEncoders.enableMotor) return false;
  if ((mount_.parkHome.parkStatus != PRK_UNPARKED) && (mount_.parkHome.parkStatus != PRK_PARKING)) return false;
  if (mount_.errors.lastError != ERRT_NONE) return false;
  if (mount_.isSlewing()) return false;
  mount_.tracking.lastSideralTracking = false;
  mount_.tracking.sideralTracking = false;
  mount_.gotoAxis(&mount_.axes.geoA1.homeDef, &mount_.axes.geoA2.homeDef);
  mount_.parkHome.homeMount = true;
  return true;
}

void MountParkHomeController::finalizeHome()
{
  if (mount_.parkHome.backlashStatus == DONE)
  {
    mount_.parkHome.backlashStatus = INIT;
  }
  parkClearBacklash();
  if (mount_.parkHome.backlashStatus == DONE)
  {
    mount_.parkHome.homeMount = false;
    mount_.tracking.movingTo = false;
    syncAtHome();
    mount_.axes.enable(false);
  }
}

bool MountParkHomeController::syncAtHome()
{
  if (mount_.isSlewing()) return false;
  mount_.axes.staA2.dir = true;
  mount_.axes.staA1.dir = true;
  mount_.targetCurrent.newTargetRA = 0.0;
  mount_.targetCurrent.newTargetDec = 0.0;
  mount_.targetCurrent.newTargetAlt = 0.0;
  mount_.targetCurrent.newTargetAzm = 0.0;
  mount_.setError(ErrorsTraking::ERRT_NONE);
  mount_.axes.staA1.resetToSidereal();
  mount_.axes.staA2.resetToSidereal();
  mount_.parkHome.parkStatus = ParkState::PRK_UNPARKED;
  XEEPROM.update(getMountAddress(EE_parkStatus), mount_.parkHome.parkStatus);
  mount_.guiding.guideA1.setIdle();
  mount_.guiding.guideA1.duration = 0UL;
  mount_.guiding.guideA1.durationLast = 0UL;
  mount_.guiding.guideA2.setIdle();
  mount_.guiding.guideA2.duration = 0UL;
  mount_.guiding.guideA2.durationLast = 0UL;
  mount_.syncAxis(&mount_.axes.geoA1.homeDef, &mount_.axes.geoA2.homeDef);
  mount_.decayModeTracking();
  mount_.tracking.sideralTracking = false;
  mount_.parkHome.atHome = true;
  mount_.syncEwithT();
  return true;
}

void MountParkHomeController::initHome()
{
  mount_.parkHome.homeSaved = XEEPROM.read(getMountAddress(EE_homeSaved));
  if (mount_.parkHome.homeSaved)
  {
    mount_.axes.geoA1.homeDef = XEEPROM.readLong(getMountAddress(EE_homePosAxis1)) * pow(2, mount_.motorsEncoders.motorA1.micro);
    mount_.axes.geoA2.homeDef = XEEPROM.readLong(getMountAddress(EE_homePosAxis2)) * pow(2, mount_.motorsEncoders.motorA2.micro);
    mount_.parkHome.homeSaved &= mount_.limits.withinLimit(mount_.axes.geoA1.homeDef, mount_.axes.geoA2.homeDef);
    if (!mount_.parkHome.homeSaved)
    {
      XEEPROM.write(getMountAddress(EE_homeSaved), 0);
    }
  }
  if (!mount_.parkHome.homeSaved)
  {
    if (mount_.isAltAZ())
    {
      mount_.axes.geoA1.homeDef = mount_.axes.geoA1.poleDef;
      mount_.axes.geoA2.homeDef = 0;
    }
    else
    {
      mount_.axes.geoA1.homeDef = mount_.axes.geoA1.poleDef;
      mount_.axes.geoA2.homeDef = mount_.axes.geoA2.poleDef;
    }
  }
}

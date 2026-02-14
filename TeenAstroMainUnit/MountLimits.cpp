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
 * Description: MountLimits - altitude, meridian, under-pole checks. See MountLimits.h.
 */
#include "MountLimits.h"
#include "MainUnit.h"

// Limit margins: tracking uses looser margins than GOTO.
// Meridian in minutes past E/W; under-pole in degrees; UNDER_POLE_OFFSET_DEG for the check.
namespace {
const double UNDER_POLE_TRACKING_MARGIN_DEG = 5.0 / 60.0;
const double UNDER_POLE_OFFSET_DEG = 6.0;
const long MERIDIAN_TRACKING_MARGIN_MIN = 5;
const double MERIDIAN_MINUTES_DISABLED = 360.0;
const double HOURS_PER_DEG_RA = 15.0;
}

MountLimits::MountLimits(Mount& mount) : mount_(mount) {}

// -----------------------------------------------------------------------------
// Simple accessors
// -----------------------------------------------------------------------------

long MountLimits::getMeridianEastLimit() const
{
  return minutesPastMeridianGOTOE;
}

long MountLimits::getMeridianWestLimit() const
{
  return minutesPastMeridianGOTOW;
}

bool MountLimits::checkAltitudeLimits(double alt) const
{
  return alt >= minAlt && alt <= maxAlt;
}

// -----------------------------------------------------------------------------
// checkPole (GEM): position must stay away from under-pole; limit in degrees
// -----------------------------------------------------------------------------

bool MountLimits::checkPole(long axis1, long axis2, CheckMode mode) const
{
  bool ok = false;
  double underPoleLimit = (mode == CHECKMODE_GOTO)
    ? underPoleLimitGOTO
    : underPoleLimitGOTO + UNDER_POLE_TRACKING_MARGIN_DEG;
  const MountAxes& axes = mount_.axes;
  PoleSide currPoleSide = axes.getPoleSideFromAxis2(axis2);

  switch (currPoleSide)
  {
  case POLE_UNDER:
    ok = (axis1 < axes.geoA1.poleDef + (underPoleLimit - UNDER_POLE_OFFSET_DEG) * HOURS_PER_DEG_RA * axes.geoA1.stepsPerDegree);
    break;
  case POLE_OVER:
    ok = (axis1 > axes.geoA1.poleDef - (underPoleLimit - UNDER_POLE_OFFSET_DEG) * HOURS_PER_DEG_RA * axes.geoA1.stepsPerDegree);
    break;
  default:
    ok = false;
    break;
  }
  return ok;
}

// -----------------------------------------------------------------------------
// checkMeridian (GEM): axis1 must stay within E/W minutes past meridian
// -----------------------------------------------------------------------------

bool MountLimits::checkMeridian(long axis1, long axis2, CheckMode mode) const
{
  bool ok = true;
  double minutesPastMeridianW;
  double minutesPastMeridianE;
  const MountAxes& axes = mount_.axes;
  PoleSide currPoleSide = axes.getPoleSideFromAxis2(axis2);

  if (mode == CHECKMODE_GOTO)
  {
    minutesPastMeridianW = minutesPastMeridianGOTOW;
    minutesPastMeridianE = minutesPastMeridianGOTOE;
  }
  else
  {
    minutesPastMeridianW = minutesPastMeridianGOTOW + MERIDIAN_TRACKING_MARGIN_MIN;
    minutesPastMeridianE = minutesPastMeridianGOTOE + MERIDIAN_TRACKING_MARGIN_MIN;
    if (distanceFromPoleToKeepTrackingOn < 180)
    {
      if (currPoleSide == POLE_UNDER)
      {
        if (axis2 < (90 - distanceFromPoleToKeepTrackingOn) * axes.geoA2.stepsPerDegree)
          minutesPastMeridianW = minutesPastMeridianE = MERIDIAN_MINUTES_DISABLED;
      }
      else if (currPoleSide == POLE_OVER)
      {
        if (axis2 > (90 + distanceFromPoleToKeepTrackingOn) * axes.geoA2.stepsPerDegree)
          minutesPastMeridianW = minutesPastMeridianE = MERIDIAN_MINUTES_DISABLED;
      }
    }
  }

  switch (currPoleSide)
  {
  case POLE_UNDER:
    ok = axis1 > axes.geoA1.poleDef - axes.geoA1.quaterRot
      - (minutesPastMeridianE / 60.0) * HOURS_PER_DEG_RA * axes.geoA1.stepsPerDegree;
    break;
  case POLE_OVER:
    ok = axis1 < axes.geoA1.poleDef + axes.geoA1.quaterRot
      + (minutesPastMeridianW / 60.0) * HOURS_PER_DEG_RA * axes.geoA1.stepsPerDegree;
    break;
  default:
    ok = false;
    break;
  }
  return ok;
}

// -----------------------------------------------------------------------------
// withinLimit: axis min/max, then (GEM) pole and meridian if applicable
// -----------------------------------------------------------------------------

bool MountLimits::withinLimit(long axis1, long axis2) const
{
  bool ok = mount_.axes.geoA1.withinLimit(axis1) && mount_.axes.geoA2.withinLimit(axis2);
  if (!ok)
    return ok;

  if (mount_.isAltAZ())
  {
    // AltAz: no pole/meridian checks
  }
  else
  {
    if (mount_.config.identity.mountType == MOUNT_TYPE_GEM)
    {
      ok = checkPole(axis1, axis2, CHECKMODE_GOTO);
      if (!ok)
        return ok;
    }
    if (mount_.config.identity.meridianFlip == FLIP_ALWAYS)
      ok = checkMeridian(axis1, axis2, CHECKMODE_GOTO);
  }
  return ok;
}

// -----------------------------------------------------------------------------
// Pole side and axis position (delegate to axes)
// -----------------------------------------------------------------------------

PoleSide MountLimits::getPoleSideFromAxis2(long axis2) const
{
  return mount_.axes.getPoleSideFromAxis2(axis2);
}

void MountLimits::getAxisPositions(long& axis1, long& axis2) const
{
  mount_.axes.getAxisPositions(axis1, axis2);
}

// -----------------------------------------------------------------------------
// Limit init and EEPROM (delegate to limitManager where applicable)
// -----------------------------------------------------------------------------

void MountLimits::initLimit()
{
  bool ok = initLimitMinAxis1();
  ok &= initLimitMaxAxis1();
  ok &= mount_.axes.geoA1.maxAxis > mount_.axes.geoA1.minAxis;
  ok &= initLimitMinAxis2();
  ok &= initLimitMaxAxis2();
  ok &= mount_.axes.geoA2.maxAxis > mount_.axes.geoA2.minAxis;
  if (!ok)
    mount_.limitManager.resetEELimit();
}

void MountLimits::resetEELimit()
{
  mount_.limitManager.resetEELimit();
}

void MountLimits::forceResetEELimit()
{
  mount_.limitManager.forceResetEELimit();
}

bool MountLimits::initLimitMinAxis1() { return mount_.limitManager.initLimitMinAxis1(); }
bool MountLimits::initLimitMaxAxis1() { return mount_.limitManager.initLimitMaxAxis1(); }
bool MountLimits::initLimitMinAxis2() { return mount_.limitManager.initLimitMinAxis2(); }
bool MountLimits::initLimitMaxAxis2() { return mount_.limitManager.initLimitMaxAxis2(); }

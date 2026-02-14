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
 * Description: Mount moveTo (slew loop) and decay mode. See Mount.h and MountClassDesign.md.
 */
#include "MainUnit.h"
#include "Site.hpp"

// -----------------------------------------------------------------------------
// moveTo() — main slew loop
// -----------------------------------------------------------------------------

void Mount::moveTo()
{
  // ----- Settling: wait after arrival before clearing movingTo -----
  if (parkHome.settling)
  {
    unsigned long elapsedTime = millis() - parkHome.lastSettleTime;
    if (elapsedTime > parkHome.slewSettleDuration * 1000)
    {
      parkHome.settling = false;
      tracking.movingTo = false;
    }
    return;
  }

  static long lastPosAxis2 = 0;
  volatile long distStartAxis1, distStartAxis2, distDestAxis1, distDestAxis2;

  axes.updateDeltaStart();
  distStartAxis1 = max(abs(axes.staA1.deltaStart), 1L);
  distStartAxis2 = max(abs(axes.staA2.deltaStart), 1L);

Again:
  axes.updateDeltaTarget();
  distDestAxis1 = max(abs(axes.staA1.deltaTarget), 1L);
  distDestAxis2 = max(abs(axes.staA2.deltaTarget), 1L);

  // ----- GEM only: slow down near horizon to avoid exceeding minAlt -----
  if (!isAltAZ())
  {
    cli();
    volatile long tempPosAxis2 = axes.staA2.pos;
    sei();
    if (tempPosAxis2 != lastPosAxis2)
    {
      bool decreasing = tempPosAxis2 < lastPosAxis2;
      if ((getPoleSide() == POLE_OVER) == localSite.northHemisphere())
        decreasing = !decreasing;

      if (decreasing)
      {
        // Dec decreasing: cap Dec (axis2) distance by altitude margin above horizon
        cli();
        long a = max((targetCurrent.currentAlt - limits.minAlt) * axes.geoA2.stepsPerDegree, 1);
        if (a < distDestAxis2)
          distDestAxis2 = a;
        sei();
      }
      else
      {
        // Dec increasing: cap HA (axis1) distance by altitude margin
        cli();
        long a = max((targetCurrent.currentAlt - limits.minAlt) * axes.geoA1.stepsPerDegree, 1);
        if (a < distDestAxis1)
          distDestAxis1 = a;
        sei();
      }
    }
    lastPosAxis2 = tempPosAxis2;
  }

  // ----- Abort slew: stop motors, restore park/home state if needed, re-enter loop -----
  if (tracking.abortSlew)
  {
    axes.staA1.breakMoveLowRate();
    guiding.guideA1.brake();
    axes.staA2.breakMoveLowRate();
    guiding.guideA2.brake();
    if (parkHome.parkStatus == PRK_PARKING)
    {
      tracking.sideralTracking = tracking.lastSideralTracking;
      parkHome.parkStatus = PRK_UNPARKED;
      parkHome.backlashStatus = DONE;
      XEEPROM.write(getMountAddress(EE_parkStatus), parkHome.parkStatus);
    }
    else if (parkHome.homeMount)
    {
      parkHome.backlashStatus = DONE;
      tracking.sideralTracking = tracking.lastSideralTracking;
      parkHome.homeMount = false;
    }
    tracking.abortSlew = false;
    goto Again;
  }

  // ----- Set axis step intervals from remaining distance (limits slew rate) -----
  volatile long d = distStartAxis1 < distDestAxis1 ? distStartAxis1 : distDestAxis1;
  if (axes.staA1.deltaTarget < 0)
    d = -d;
  cli();
  axes.staA1.setIntervalfromDist(d, tracking.sideralTracking, motorsEncoders.minInterval1, motorsEncoders.maxInterval1);
  sei();

  d = distStartAxis2 < distDestAxis2 ? distStartAxis2 : distDestAxis2;
  if (axes.staA2.deltaTarget < 0)
    d = -d;
  cli();
  axes.staA2.setIntervalfromDist(d, tracking.sideralTracking, motorsEncoders.minInterval2, motorsEncoders.maxInterval2);
  sei();

  // ----- Park/Home: on arrival, finalize and switch to tracking -----
  if (parkHome.parkStatus == PRK_PARKING || parkHome.homeMount)
  {
    if (axes.staA1.deltaTarget == 0 && axes.staA2.deltaTarget == 0)
    {
      if (parkHome.homeMount)
        finalizeHome();
      else if (parkHome.parkStatus == PRK_PARKING)
        finalizePark();
      if (parkHome.atHome || parkHome.parkStatus == PRK_PARKED)
      {
        SetsiderealClockSpeed(tracking.siderealClockSpeed);
        cli();
        axes.staA1.resetToSidereal();
        axes.staA2.resetToSidereal();
        sei();
        decayModeTracking();
      }
    }
  }
  // ----- Normal goto: on arrival, start settling and switch to tracking -----
  else if (axes.staA1.atTarget(false) && axes.staA2.atTarget(false))
  {
    parkHome.settling = true;
    parkHome.lastSettleTime = millis();
    SetsiderealClockSpeed(tracking.siderealClockSpeed);
    cli();
    axes.staA1.resetToSidereal();
    axes.staA2.resetToSidereal();
    sei();
    decayModeTracking();
  }
}

// -----------------------------------------------------------------------------
// Decay mode: low current when tracking, high current during slew
// -----------------------------------------------------------------------------

void Mount::decayModeTracking()
{
  if (config.identity.DecayModeTrack)
    return;
  config.identity.DecayModeTrack = true;
  motorsEncoders.motorA1.driver.setCurrent((unsigned int)motorsEncoders.motorA1.lowCurr);
  motorsEncoders.motorA2.driver.setCurrent((unsigned int)motorsEncoders.motorA2.lowCurr);
}

void Mount::decayModeGoto()
{
  if (!config.identity.DecayModeTrack)
    return;
  config.identity.DecayModeTrack = false;
  motorsEncoders.motorA1.driver.setCurrent((unsigned int)motorsEncoders.motorA1.highCurr);
  motorsEncoders.motorA2.driver.setCurrent((unsigned int)motorsEncoders.motorA2.highCurr);
}

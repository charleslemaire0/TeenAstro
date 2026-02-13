// -----------------------------------------------------------------------------------
// functions to move the mount to the a new position
#include "Global.h"

// moves the mount
void moveTo()
{
  // HA goes from +90...0..-90
  //                W   .   E

  if (mount.settling)
  {
    unsigned long elapsedTime = millis() - mount.lastSettleTime;
    if (elapsedTime > mount.slewSettleDuration * 1000)
    {
      mount.settling = false;
      mount.movingTo = false;
    }
    return;
  }
  static long lastPosAxis2 = 0;
  volatile long distStartAxis1, distStartAxis2, distDestAxis1, distDestAxis2;

  mount.staA1.updateDeltaStart();
  mount.staA2.updateDeltaStart();
  distStartAxis1 = max(abs(mount.staA1.deltaStart), 1L);  // distance from start HA
  distStartAxis2 = max(abs(mount.staA2.deltaStart), 1L);  // distance from start Dec

Again:
  mount.staA1.updateDeltaTarget();
  mount.staA2.updateDeltaTarget();

  distDestAxis1 = max(abs(mount.staA1.deltaTarget), 1L);  // distance from dest HA
  distDestAxis2 = max(abs(mount.staA2.deltaTarget), 1L);  // distance from dest Dec
  // adjust rates near the horizon to help keep from exceeding the mount.minAlt limit
  if (!isAltAZ())
  {
    cli();
    volatile long tempPosAxis2 = mount.staA2.pos;
    sei();
    if (tempPosAxis2 != lastPosAxis2)
    {
      bool decreasing = tempPosAxis2 < lastPosAxis2;
      //  Correct according to pier side and latitude
      if ((GetPoleSide() == POLE_OVER) == localSite.northHemisphere())
        decreasing = !decreasing;

      // if Dec is decreasing, slow down Dec
      if (decreasing)
      {
        cli();
        long a = max((mount.currentAlt - mount.minAlt) * mount.geoA2.stepsPerDegree, 1);
        if (a < distDestAxis2)
          distDestAxis2 = a;
        sei();
      }
      else
        // if Dec is increasing, slow down HA
      {
        cli();
        long a = max((mount.currentAlt - mount.minAlt) * mount.geoA1.stepsPerDegree, 1);
        if (a < distDestAxis1)
          distDestAxis1 = a;
        sei();
      }
    }
    lastPosAxis2 = tempPosAxis2;
  }

  // quickly slow the motors and stop in 1 degree
  if (mount.abortSlew)
  {
    mount.staA1.breakMoveLowRate();
    mount.guideA1.brake();
    mount.staA2.breakMoveLowRate();
    mount.guideA2.brake();
    if (mount.parkStatus == PRK_PARKING)
    {
      mount.sideralTracking = mount.lastSideralTracking;
      mount.parkStatus = PRK_UNPARKED;
      mount.backlashStatus = DONE;
      XEEPROM.write(getMountAddress(EE_parkStatus), mount.parkStatus);
    }
    else if (mount.homeMount)
    {
      mount.backlashStatus = DONE;
      mount.sideralTracking = mount.lastSideralTracking;
      mount.homeMount = false;
    }
    mount.abortSlew = false;
    goto Again;
  }


  // First, for Right Ascension
  volatile long d = distStartAxis1 < distDestAxis1 ? distStartAxis1 : distDestAxis1;
  if (mount.staA1.deltaTarget < 0)
    d = -d;
  cli();
  mount.staA1.setIntervalfromDist(d, mount.sideralTracking, mount.minInterval1, mount.maxInterval1);
  sei();

  // Now, for Declination
  d = distStartAxis2 < distDestAxis2 ? distStartAxis2 : distDestAxis2;
  if (mount.staA2.deltaTarget < 0)
    d = -d;
  cli();
  mount.staA2.setIntervalfromDist(d, mount.sideralTracking, mount.minInterval2, mount.maxInterval2);
  sei();


  if (mount.parkStatus == PRK_PARKING || mount.homeMount)
  {
    if (mount.staA1.deltaTarget == 0 && mount.staA2.deltaTarget == 0)
    {
      if (mount.homeMount)
      {
        finalizeHome();
      }
      else if (mount.parkStatus == PRK_PARKING)
      {
        finalizePark();
      }
      if (mount.atHome || mount.parkStatus == PRK_PARKED)
      {
        SetsiderealClockSpeed(mount.siderealClockSpeed);
        cli();
        mount.staA1.resetToSidereal();
        mount.staA2.resetToSidereal();
        sei();
        DecayModeTracking();
      }
    }
  }
  else if (mount.staA1.atTarget(false) && mount.staA2.atTarget(false))
  {
    mount.settling = true;
    mount.lastSettleTime = millis();
    SetsiderealClockSpeed(mount.siderealClockSpeed);
    cli();
    mount.staA1.resetToSidereal();
    mount.staA2.resetToSidereal();
    sei();
    DecayModeTracking();
  }
}

// if stepper drive can switch decay mode, set it here
void DecayModeTracking()
{
  if (mount.DecayModeTrack) return;
  mount.DecayModeTrack = true;
  mount.motorA1.driver.setCurrent((unsigned int)mount.motorA1.lowCurr);
  mount.motorA2.driver.setCurrent((unsigned int)mount.motorA2.lowCurr);
}

void DecayModeGoto()
{
  if (!mount.DecayModeTrack) return;
  mount.DecayModeTrack = false;
  mount.motorA1.driver.setCurrent((unsigned int)mount.motorA1.highCurr);
  mount.motorA2.driver.setCurrent((unsigned int)mount.motorA2.highCurr);
}

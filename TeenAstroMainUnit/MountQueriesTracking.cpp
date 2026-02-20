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
 * Description: Mount queries, tracking rates, safety, updateRatios, spiral. See Mount.h.
 */
#include "MainUnit.h"
#include "Site.hpp"
#include "TeenAstroCoord_LO.hpp"

// --- Queries and accessors ---
bool Mount::isParked() const { return parkHome.parkStatus == PRK_PARKED; }
bool Mount::isAtHome() const { return parkHome.atHome; }

bool Mount::isSlewing() const
{
  return tracking.movingTo || guiding.GuidingState != Guiding::GuidingOFF;
}

PoleSide Mount::getPoleSide() const
{
  long axis1, axis2;
  limits.getAxisPositions(axis1, axis2);
  return limits.getPoleSideFromAxis2(axis2);
}

PoleSide Mount::getTargetPoleSide() const
{
  long axis2;
  cli();
  axis2 = axes.staA2.target;
  sei();
  return limits.getPoleSideFromAxis2(axis2);
}

bool Mount::hasTracking() const { return tracking.sideralTracking; }
bool Mount::hasGuiding() const { return guiding.hasGuiding(); }
ErrorsTraking Mount::lastError() const { return errors.lastError; }
void Mount::setError(ErrorsTraking e) { errors.lastError = e; }
void Mount::clearError() { setError(ERRT_NONE); }
bool Mount::isAltAZ() const { return config.identity.mountType == MOUNT_TYPE_ALTAZM || config.identity.mountType == MOUNT_TYPE_FORK_ALT; }

ParkState Mount::getParkStatus() const { return parkHome.parkStatus; }
bool Mount::getParkSaved() const { return parkHome.parkSaved; }
MountParkHome& Mount::getParkHome() { return parkHome; }
const MountParkHome& Mount::getParkHome() const { return parkHome; }
MountIdentity& Mount::getIdentity() { return config.identity; }
const MountIdentity& Mount::getIdentity() const { return config.identity; }
MountPeripherals& Mount::getPeripherals() { return config.peripherals; }
const MountPeripherals& Mount::getPeripherals() const { return config.peripherals; }
MountConfig& Mount::getConfig() { return config; }
const MountConfig& Mount::getConfig() const { return config; }
MountLimits& Mount::getLimits() { return limits; }
const MountLimits& Mount::getLimits() const { return limits; }
MountAlignment& Mount::getAlignment() { return alignment; }
const MountAlignment& Mount::getAlignment() const { return alignment; }
MountTargetCurrent& Mount::getTargetCurrent() { return targetCurrent; }
const MountTargetCurrent& Mount::getTargetCurrent() const { return targetCurrent; }
MountErrors& Mount::getErrors() { return errors; }
const MountErrors& Mount::getErrors() const { return errors; }
MountTracking& Mount::getTracking() { return tracking; }
const MountTracking& Mount::getTracking() const { return tracking; }
MountGuiding& Mount::getGuiding() { return guiding; }
const MountGuiding& Mount::getGuiding() const { return guiding; }
MountMotorsEncoders& Mount::getMotorsEncoders() { return motorsEncoders; }
const MountMotorsEncoders& Mount::getMotorsEncoders() const { return motorsEncoders; }
MountAxes& Mount::getAxes() { return axes; }
const MountAxes& Mount::getAxes() const { return axes; }
#ifdef RETICULE_LED_PINS
MountReticule& Mount::getReticule() { return reticule; }
const MountReticule& Mount::getReticule() const { return reticule; }
#endif

void Mount::setParkStatus(ParkState s) { parkHome.parkStatus = s; }
void Mount::setTracking(bool on) { tracking.sideralTracking = on; }
void Mount::setSiderealMode(SID_Mode m) { tracking.sideralMode = m; }
void Mount::setGuidingState(Guiding g) { guiding.setGuidingState(g); }
void Mount::setTargetRaDec(double ra, double dec) { targetCurrent.newTargetRA = ra; targetCurrent.newTargetDec = dec; }
void Mount::setTargetAltAz(double alt, double azm) { targetCurrent.newTargetAlt = alt; targetCurrent.newTargetAzm = azm; }
void Mount::setTargetPoleSide(PoleSide s) { targetCurrent.newTargetPoleSide = s; }
void Mount::abortSlew() { tracking.abortSlew = true; }
void Mount::setMeridianFlip(MeridianFlip m) { config.identity.meridianFlip = m; }

// --- Tracking rates, compensation, safety, updateRatios ---
void Mount::applyTrackingRate()
{
  axes.applyTrackingRates();
}

void Mount::setTrackingRate(double rHA, double rDEC)
{
  tracking.RequestedTrackingRateHA = rHA;
  tracking.RequestedTrackingRateDEC = rDEC;
  computeTrackingRate(true);
}

void Mount::rateFromMovingTarget(Coord_EQ& EQprev, Coord_EQ& EQnext, double TimeRange, PoleSide side, bool refr,
  double& A1_trackingRate, double& A2_trackingRate)
{
  long axis1_before, axis1_after = 0;
  long axis2_before, axis2_after = 0;
  double axis1_delta = 0.0, axis2_delta = 0.0;
  LA3::RefrOpt rop = { refraction.forTracking, 10, 101 };
  Coord_IN INprev = EQprev.To_Coord_IN(*localSite.latitude() * DEG_TO_RAD, rop, alignment.conv.Tinv);
  angle2Step(INprev.Axis1() * RAD_TO_DEG, INprev.Axis2() * RAD_TO_DEG, side, &axis1_before, &axis2_before);
  Coord_IN INnext = EQnext.To_Coord_IN(*localSite.latitude() * DEG_TO_RAD, rop, alignment.conv.Tinv);
  angle2Step(INnext.Axis1() * RAD_TO_DEG, INnext.Axis2() * RAD_TO_DEG, side, &axis1_after, &axis2_after);
  axis1_delta = distStepAxis1(&axis1_before, &axis1_after) / axes.geoA1.stepsPerDegree;
  while (axis1_delta < -180) axis1_delta += 360.;
  while (axis1_delta >= 180) axis1_delta -= 360.;
  axis2_delta = distStepAxis2(&axis2_before, &axis2_after) / axes.geoA2.stepsPerDegree;
  while (axis2_delta < -180) axis2_delta += 360.;
  while (axis2_delta >= 180) axis2_delta -= 360.;
  A1_trackingRate = 0.5 * axis1_delta * (3600. / (TimeRange * 15));
  A2_trackingRate = 0.5 * axis2_delta * (3600. / (TimeRange * 15));
}

void Mount::doCompensationCalc()
{
  const double TimeRange = 60.0;
  double HA_now = 0.0, Dec_now = 0.0, DriftHA = 0.0, DriftDEC = 0.0, RateA1 = 0.0, RateA2 = 0.0;
  if (!tracking.sideralTracking)
  {
    axes.staA1.RequestedTrackingRate = 0.;
    axes.staA2.RequestedTrackingRate = 0.;
    return;
  }
  DriftHA = tracking.RequestedTrackingRateHA * TimeRange * 15 / 3600;
  DriftDEC = tracking.RequestedTrackingRateDEC * TimeRange / 3600;
  PoleSide side_tmp;
  if (tracking.movingTo)
  {
    Coord_EQ EQ_T = getEquTarget(*localSite.latitude() * DEG_TO_RAD);
    HA_now = EQ_T.Ha() * RAD_TO_DEG;
    Dec_now = EQ_T.Dec() * RAD_TO_DEG;
    side_tmp = getTargetPoleSide();
  }
  else
  {
    Coord_EQ EQ_T = getEqu(*localSite.latitude() * DEG_TO_RAD);
    HA_now = EQ_T.Ha() * RAD_TO_DEG;
    Dec_now = EQ_T.Dec() * RAD_TO_DEG;
    side_tmp = getPoleSide();
  }
  Coord_EQ EQ_prev(0, (Dec_now - DriftDEC) * DEG_TO_RAD, (HA_now - DriftHA) * DEG_TO_RAD);
  Coord_EQ EQ_next(0, (Dec_now + DriftDEC) * DEG_TO_RAD, (HA_now + DriftHA) * DEG_TO_RAD);
  rateFromMovingTarget(EQ_prev, EQ_next, TimeRange, side_tmp, refraction.forTracking, RateA1, RateA2);
  axes.staA1.RequestedTrackingRate = min(max(RateA1, -16.0), 16.0);
  axes.staA2.RequestedTrackingRate = min(max(RateA2, -16.0), 16.0);
}

void Mount::computeTrackingRate(bool apply)
{
  if (tracking.RequestedTrackingRateHA == 1 && tracking.RequestedTrackingRateDEC == 0)
    tracking.sideralMode = SIDM_STAR;
  if (isAltAZ())
    doCompensationCalc();
  else if (refraction.forTracking || alignment.hasValid)
  {
    doCompensationCalc();
    if (tracking.trackComp == TC_RA)
      axes.staA2.RequestedTrackingRate = 0.0;
  }
  else
  {
    double sign = localSite.northHemisphere() ? 1 : -1;
    axes.staA1.RequestedTrackingRate = sign * tracking.RequestedTrackingRateHA;
    sign = getTargetPoleSide() == POLE_UNDER ? 1 : -1;
    axes.staA2.RequestedTrackingRate = sign * tracking.RequestedTrackingRateDEC / 15;
    if (tracking.trackComp == TC_RA)
      axes.staA2.RequestedTrackingRate = 0.0;
  }
  if (apply)
    axes.applyTrackingRates();
}

void Mount::enableGuideRate(int g, bool force)
{
  if (g < 0) g = 0;
  if (g > 4) g = 4;
  if (guiding.activeGuideRate != g || force)
  {
    guiding.activeGuideRate = g;
    guiding.guideA1.enableAtRate(guiding.guideRates[g]);
    guiding.guideA2.enableAtRate(guiding.guideRates[g]);
  }
}

void Mount::enableST4GuideRate() { enableGuideRate(0); }
void Mount::enableRecenterGuideRate() { enableGuideRate(guiding.recenterGuideRate); }
void Mount::resetGuideRate() { enableGuideRate(guiding.activeGuideRate, true); }

void Mount::initMaxRate()
{
  double maxslewEEPROM = XEEPROM.readUShort(getMountAddress(EE_maxRate));
  setRates(maxslewEEPROM);
}

void Mount::setRates(double maxslewrate)
{
  double fact1 = masterClockSpeed / axes.geoA1.stepsPerSecond;
  double fact2 = masterClockSpeed / axes.geoA2.stepsPerSecond;
  motorsEncoders.minInterval1 = max(fact1 / maxslewrate, StepsMinInterval);
  motorsEncoders.minInterval2 = max(fact2 / maxslewrate, StepsMinInterval);
  double maxslewCorrected = min(fact1 / motorsEncoders.minInterval1, fact2 / motorsEncoders.minInterval2);
  if (abs(maxslewrate - maxslewCorrected) > 2)
    XEEPROM.writeUShort(getMountAddress(EE_maxRate), (int)maxslewCorrected);
  motorsEncoders.minInterval1 = fact1 / maxslewCorrected;
  motorsEncoders.minInterval2 = fact2 / maxslewCorrected;
  guiding.guideRates[4] = maxslewCorrected;
  if (guiding.guideRates[3] >= maxslewCorrected)
  {
    guiding.guideRates[3] = maxslewCorrected / 2;
    XEEPROM.write(getMountAddress(EE_Rate3), guiding.guideRates[3]);
  }
  resetGuideRate();
  axes.setAcceleration(motorsEncoders.minInterval1, motorsEncoders.minInterval2, guiding.DegreesForAcceleration);
}

void Mount::setAcceleration()
{
  axes.setAcceleration(motorsEncoders.minInterval1, motorsEncoders.minInterval2, guiding.DegreesForAcceleration);
}

void Mount::updateSideral()
{
  axes.setSidereal(tracking.siderealClockSpeed, axes.geoA1.stepsPerSecond, axes.geoA2.stepsPerSecond,
    masterClockSpeed, motorsEncoders.motorA1.backlashRate, motorsEncoders.motorA2.backlashRate);
  setTrackingRate(default_tracking_rate, 0);
  SetsiderealClockSpeed(tracking.siderealClockSpeed);
}

void Mount::safetyCheck(bool forceTracking)
{
  PoleSide currentSide = getPoleSide();
  long axis1, axis2;
  limits.getAxisPositions(axis1, axis2);
  if (parkHome.atHome)
    parkHome.atHome = !tracking.sideralTracking;
  if (!axes.geoA1.withinLimit(axis1))
  {
    setError(ERRT_AXIS1);
    if (tracking.movingTo) abortSlew();
    else if (!forceTracking) tracking.sideralTracking = false;
    return;
  }
  else if (errors.lastError == ERRT_AXIS1)
    setError(ERRT_NONE);
  if (!axes.geoA2.withinLimit(axis2))
  {
    setError(ERRT_AXIS2);
    if (tracking.movingTo) abortSlew();
    else if (!forceTracking) tracking.sideralTracking = false;
    return;
  }
  else if (errors.lastError == ERRT_AXIS2)
    setError(ERRT_NONE);
  if (config.identity.mountType == MOUNT_TYPE_GEM)
  {
    if (!limits.checkMeridian(axis1, axis2, CHECKMODE_TRACKING))
    {
      if ((axes.staA1.dir && currentSide == POLE_OVER) || (!axes.staA1.dir && currentSide == POLE_UNDER))
      {
        setError(ERRT_MERIDIAN);
        if (tracking.movingTo) abortSlew();
        if (currentSide >= POLE_OVER && !forceTracking) tracking.sideralTracking = false;
        return;
      }
      else if (errors.lastError == ERRT_MERIDIAN)
        setError(ERRT_NONE);
    }
    else if (errors.lastError == ERRT_MERIDIAN)
      setError(ERRT_NONE);
    if (!limits.checkPole(axis1, axis2, CHECKMODE_TRACKING))
    {
      if ((axes.staA1.dir && currentSide == POLE_UNDER) || (!axes.staA1.dir && currentSide == POLE_OVER))
      {
        setError(ERRT_UNDER_POLE);
        if (tracking.movingTo) abortSlew();
        if (currentSide == POLE_UNDER && !forceTracking) tracking.sideralTracking = false;
        return;
      }
      else if (errors.lastError == ERRT_UNDER_POLE)
        setError(ERRT_NONE);
    }
    else if (errors.lastError == ERRT_UNDER_POLE)
      setError(ERRT_NONE);
  }
  if (parkHome.atHome && errors.lastError != ERRT_NONE)
  {
    unsetHome();
    syncAtHome();
  }
  if (parkHome.parkStatus == PRK_PARKED && errors.lastError != ERRT_NONE)
  {
    unsetPark();
    syncAtHome();
  }
}

void Mount::onSiderealTick(long phase, bool forceTracking)
{
  if (tracking.sideralTracking)
  {
    cli();
    if (!axes.staA1.backlash_correcting)
      axes.staA1.target += axes.staA1.fstep;
    if (!axes.staA2.backlash_correcting)
      axes.staA2.target += axes.staA2.fstep;
    sei();
  }
  if (tracking.movingTo)
    moveTo();

  if (phase % 20 == 0)
    targetCurrent.currentAlt = getHorTopo().Alt() * RAD_TO_DEG;
  if (phase == 0)
    computeTrackingRate(true);

  checkEndOfMoveAxisAtRate();

  if (axes.staA1.fault || axes.staA2.fault)
  {
    setError(ERRT_MOTOR_FAULT);
    if (!forceTracking)
    {
      if (tracking.movingTo)
        abortSlew();
      else
      {
        tracking.sideralTracking = false;
        guiding.guideA1.brake();
        guiding.guideA2.brake();
      }
    }
  }
  else if (errors.lastError == ERRT_MOTOR_FAULT)
    setError(ERRT_NONE);

  if (targetCurrent.currentAlt < limits.minAlt || targetCurrent.currentAlt > limits.maxAlt)
  {
    if (!forceTracking)
    {
      setError(ERRT_ALT);
      if (tracking.movingTo)
        abortSlew();
      else
        tracking.sideralTracking = false;
    }
  }
  else if (errors.lastError == ERRT_ALT)
    setError(ERRT_NONE);
}

void Mount::updateEncoderSync(bool runThisLoop)
{
  if (!runThisLoop) return;
  EncoderSync mode = config.peripherals.PushtoStatus == PT_OFF ? motorsEncoders.EncodeSyncMode : ES_ALWAYS;
  if (autoSyncWithEncoder(mode))
  {
    if (parkHome.atHome) parkHome.atHome = false;
    if (parkHome.parkStatus != PRK_UNPARKED) parkHome.parkStatus = PRK_UNPARKED;
  }
}

void Mount::updateRatios(bool deleteAlignment, bool deleteHP)
{
  if (motorsEncoders.enableMotor)
  {
    cli();
    axes.geoA1.setstepsPerRot((double)motorsEncoders.motorA1.gear / 1000.0 * motorsEncoders.motorA1.stepRot * pow(2, motorsEncoders.motorA1.micro));
    axes.geoA2.setstepsPerRot((double)motorsEncoders.motorA2.gear / 1000.0 * motorsEncoders.motorA2.stepRot * pow(2, motorsEncoders.motorA2.micro));
    axes.staA1.setBacklash_inSteps(motorsEncoders.motorA1.backlashAmount, axes.geoA1.stepsPerArcSecond);
    axes.staA2.setBacklash_inSteps(motorsEncoders.motorA2.backlashAmount, axes.geoA2.stepsPerArcSecond);
    axes.staA1.target = axes.staA1.pos;
    axes.staA2.target = axes.staA2.pos;
    sei();
  }
  else
  {
    cli();
    axes.geoA1.setstepsPerRot(motorsEncoders.encoderA1.pulsePerDegree * 360);
    axes.geoA2.setstepsPerRot(motorsEncoders.encoderA2.pulsePerDegree * 360);
    axes.staA1.setBacklash_inSteps(motorsEncoders.motorA1.backlashAmount, axes.geoA1.stepsPerArcSecond);
    axes.staA2.setBacklash_inSteps(motorsEncoders.motorA2.backlashAmount, axes.geoA2.stepsPerArcSecond);
    axes.staA1.target = axes.staA1.pos;
    axes.staA2.target = axes.staA2.pos;
    sei();
  }
  guiding.guideA1.init(&axes.geoA1.stepsPerCentiSecond, guiding.guideRates[guiding.activeGuideRate]);
  guiding.guideA2.init(&axes.geoA2.stepsPerCentiSecond, guiding.guideRates[guiding.activeGuideRate]);
  initCelestialPole();
  initTransformation(deleteAlignment);
  if (deleteHP)
  {
    unsetPark();
    unsetHome();
  }
  limits.initLimit();
  initHome();
  updateSideral();
  initMaxRate();
}

// --- Start tracking, end-of-move, spiral ---
void Mount::startSideralTracking()
{
  if (!tracking.sideralTracking)
  {
    tracking.sideralTracking = true;
    computeTrackingRate(true);
    tracking.lastSetTrakingEnable = millis();
    parkHome.atHome = false;
  }
}

void Mount::checkEndOfMoveAxisAtRate()
{
  if (!motorsEncoders.enableMotor) return;
  if (guiding.lastGuidingState == GuidingAtRate && guiding.GuidingState == GuidingOFF)
  {
    if (tracking.lastSideralTracking)
    {
      startSideralTracking();
    }
    resetGuideRate();
  }
  guiding.lastGuidingState = guiding.GuidingState;
}

void Mount::checkSpiral()
{
  static bool startPointDefined = false;
  static CoordConv helper;
  static unsigned long clk_ini, clk_last, clk_now;

  if (!tracking.doSpiral)
  {
    if (startPointDefined)
      startPointDefined = false;
    return;
  }

  if (errors.lastError != ERRT_NONE)
  {
    stopAxis1();
    stopAxis2();
    tracking.doSpiral = false;
    return;
  }

  if (!startPointDefined)
  {
    Coord_EQ EQ_R = getEqu(*localSite.latitude() * DEG_TO_RAD);
    helper.addReference(EQ_R.Ha(), EQ_R.Dec(), 0, 0);
    double shift = EQ_R.Dec() > 0 ? -M_PI_4 : M_PI_4;
    helper.addReference(EQ_R.Ha(), EQ_R.Dec() + shift, 0, shift);
    clk_ini = millis();
    clk_last = clk_ini;
    startPointDefined = true;
    return;
  }

  clk_now = millis();
  unsigned long t = clk_now - clk_ini;
  unsigned long dt = clk_now - clk_last;

  if (t > 300000)
  {
    stopAxis1();
    stopAxis2();
    tracking.doSpiral = false;
    return;
  }

  if (dt < 200)
    return;

  double SpiraleRateA1, SpiraleRateA2;
  double t_prev = 0.001 * (t - dt);
  double t_next = 0.001 * (t + dt);
  double hl_prev = 0.4 * tracking.SpiralFOV * sqrt(t_prev) * cos(sqrt(t_prev));
  double dl_prev = 0.4 * tracking.SpiralFOV * sqrt(t_prev) * sin(sqrt(t_prev));
  double hl_next = 0.4 * tracking.SpiralFOV * sqrt(t_next) * cos(sqrt(t_next));
  double dl_next = 0.4 * tracking.SpiralFOV * sqrt(t_next) * sin(sqrt(t_next));

  Coord_EQ EQ_prev = Coord_LO(0, dl_prev * DEG_TO_RAD, hl_prev * DEG_TO_RAD).To_Coord_EQ(helper.T);
  Coord_EQ EQ_next = Coord_LO(0, dl_next * DEG_TO_RAD, hl_next * DEG_TO_RAD).To_Coord_EQ(helper.T);

  PoleSide side_tmp = getPoleSide();
  rateFromMovingTarget(EQ_prev, EQ_next, 0.002 * dt, side_tmp, refraction.forGoto, SpiraleRateA1, SpiraleRateA2);

  if (abs(SpiraleRateA1) > guiding.guideRates[4] || abs(SpiraleRateA2) > guiding.guideRates[4])
  {
    stopAxis1();
    stopAxis2();
    tracking.doSpiral = false;
    return;
  }

  moveAxisAtRate1(SpiraleRateA1);
  moveAxisAtRate2(SpiraleRateA2);
  clk_last = clk_now;
}

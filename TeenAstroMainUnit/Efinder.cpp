/**
 * EfinderManager — plate-solving finder attached to Focuser or GNSS port.
 */
#include "Efinder.h"
#include "Command.h"
#include "MainUnitDecl.h"
#include "XEEPROM.hpp"
#include "EEPROM_address.h"
#include <TimeLib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

EfinderManager efinder;

namespace {
constexpr unsigned long PROBE_PERIOD_MS = 5000;
constexpr unsigned long PROBE_TIMEOUT_MS = 200;
constexpr unsigned long REPLY_TIMEOUT_MS = 800;
constexpr unsigned long SOLVE_TIMEOUT_MS = 20000;
constexpr unsigned long SITE_PUSH_PERIOD_MS = 60000;
constexpr uint8_t MAX_FAILURES = 3;
// Reject a stored rotation bigger than this (NaN and erased EEPROM fail too).
constexpr float AIM_MAX_RAD = 0.5f;
// cos(80 deg): closer to the pole the east/west rotation is not separable.
constexpr double AIM_MIN_COS = 0.17364817766693033;

bool aimSane(float r)
{
  return r == r && r > -AIM_MAX_RAD && r < AIM_MAX_RAD;
}

Stream* focuserStream()
{
  return (Stream*)&Focus_Serial;
}

Stream* gnssStream()
{
#if VERSION == 220
  return nullptr;
#else
  return (Stream*)&GNSS_Serial;
#endif
}

Stream* probeStream(EfinderPort p)
{
  if (p == EF_PORT_FOCUSER) return focuserStream();
  if (p == EF_PORT_GNSS) return gnssStream();
  return nullptr;
}

// A GPS that already announced itself at boot keeps the GNSS port.
bool gnssProbeAllowed()
{
#if VERSION == 220
  return false;
#else
  return !mount.config.peripherals.hasGNSS && gnssStream() != nullptr;
#endif
}
}  // namespace

void EfinderManager::setup()
{
  loadSettings();
  m_lastProbe = millis() - PROBE_PERIOD_MS;
  m_rxLen = 0;
  m_inNmea = false;
  m_replyReady = false;
  m_link = LINK_IDLE;
  m_job = EF_JOB_IDLE;
  m_step = STEP_NONE;
  m_prevGotoState = (uint8_t)mount.tracking.gotoState;
}

void EfinderManager::loadSettings()
{
  // 584+ is unused until an eFinder setting is saved. Erased EEPROM reads
  // 0xFF, which must not turn GoTo++ on.
  uint8_t flags = XEEPROM.read(EE_efinderFlags);
  if (flags & ~0x03u)
    flags = 0;
  m_gotoPP = (flags & 0x01) != 0;
  uint8_t thr = XEEPROM.read(EE_efinderThresh);
  if (thr == 0 || thr > 60) thr = 2;
  m_thresholdArcmin = thr;
  float hz = XEEPROM.readFloat(EE_efinderAimHz);
  float ve = XEEPROM.readFloat(EE_efinderAimVe);
  m_aimHz = aimSane(hz) ? hz : 0.f;
  m_aimVe = aimSane(ve) ? ve : 0.f;
  m_aimHor = (flags & 0x02) != 0;
}

void EfinderManager::saveSettings()
{
  uint8_t flags = m_gotoPP ? 0x01 : 0x00;
  if (m_aimHor) flags |= 0x02;
  XEEPROM.write(EE_efinderFlags, flags);
  XEEPROM.write(EE_efinderThresh, m_thresholdArcmin);
  XEEPROM.writeFloat(EE_efinderAimHz, m_aimHz);
  XEEPROM.writeFloat(EE_efinderAimVe, m_aimVe);
}

void EfinderManager::setGotoPlusPlus(bool on)
{
  m_gotoPP = on;
  saveSettings();
}

void EfinderManager::setThresholdArcmin(uint8_t v)
{
  if (v < 1) v = 1;
  if (v > 60) v = 60;
  m_thresholdArcmin = v;
  saveSettings();
}

Stream* EfinderManager::stream() const
{
  if (m_port == EF_PORT_FOCUSER) return focuserStream();
  if (m_port == EF_PORT_GNSS) return gnssStream();
  return nullptr;
}

bool EfinderManager::acceptProbe(EfinderPort p, const char* buf)
{
  if (!buf || strlen(buf) < 10) return false;
  if (strncmp(buf, "TeenAstroEfinder", 16) != 0) return false;

  m_port = p;
  m_version[0] = 0;
  // TeenAstroEfinder,<version>#
  const char* comma = strchr(buf, ',');
  if (comma)
  {
    strncpy(m_version, comma + 1, sizeof(m_version) - 1);
    m_version[sizeof(m_version) - 1] = 0;
    char* hash = strchr(m_version, '#');
    if (hash) *hash = 0;
  }
  else
  {
    strcpy(m_version, "ok");
  }
  mount.config.peripherals.hasEfinder = true;
  if (p == EF_PORT_GNSS)
  {
    // GPS may arrive via NMEA relay; keep GNSS flag so UpdateGnss time sync can run once NMEA flows.
    mount.config.peripherals.hasGNSS = true;
    m_nmea = true;
  }
  m_lastSeen = millis();
  m_rxLen = 0;
  m_inNmea = false;
  pushSiteAndTime();
  return true;
}

void EfinderManager::beginProbe(EfinderPort p)
{
  Stream* s = probeStream(p);
  if (!s)
  {
    advanceProbe(p);
    return;
  }

  // Drop only what is already buffered. Do not wait, and do not flush():
  // flush waits until the UART is idle and would stall the control loop.
  while (s->available() > 0) s->read();
  m_probe = p;
  m_probeLen = 0;
  m_probeStart = millis();
  s->print(":e?#");
  // A loopback port (the emulator) answers inside print().
  collectProbe();
}

bool EfinderManager::collectProbe()
{
  Stream* s = probeStream(m_probe);
  if (!s) return false;

  while (s->available() > 0)
  {
    char c = (char)s->read();
    if (m_probeLen < (uint8_t)(sizeof(m_probeBuf) - 1))
      m_probeBuf[m_probeLen++] = c;
    if (c != '#') continue;

    m_probeBuf[m_probeLen] = 0;
    EfinderPort p = m_probe;
    m_probe = EF_PORT_NONE;
    m_probeLen = 0;
    if (acceptProbe(p, m_probeBuf))
      return true;
    advanceProbe(p);
    return true;
  }
  return false;
}

void EfinderManager::advanceProbe(EfinderPort failed)
{
  m_probe = EF_PORT_NONE;
  m_probeLen = 0;
  if (failed == EF_PORT_FOCUSER && gnssProbeAllowed())
  {
    beginProbe(EF_PORT_GNSS);
    return;
  }
  m_lastProbe = millis();
}

void EfinderManager::pollProbe()
{
  if (m_probe != EF_PORT_NONE)
  {
    if (collectProbe()) return;
    if ((unsigned long)(millis() - m_probeStart) < PROBE_TIMEOUT_MS) return;
    advanceProbe(m_probe);
    return;
  }

  if ((unsigned long)(millis() - m_lastProbe) < PROBE_PERIOD_MS) return;

  if (!mount.config.peripherals.hasFocuser && probeStream(EF_PORT_FOCUSER))
  {
    beginProbe(EF_PORT_FOCUSER);
    return;
  }
  if (gnssProbeAllowed())
  {
    beginProbe(EF_PORT_GNSS);
    return;
  }
  // Both ports belong to something else. Leave them alone.
  m_lastProbe = millis();
}

void EfinderManager::update()
{
  if (m_port == EF_PORT_NONE)
  {
    pollProbe();
    return;
  }

  pump();
  if (m_job != EF_JOB_IDLE) runJob();
  watchGoto();

  if (millis() - m_lastSeen > SITE_PUSH_PERIOD_MS)
  {
    pushSiteAndTime();
    m_lastSeen = millis();
  }
}

void EfinderManager::pump()
{
  Stream* s = stream();
  if (!s) return;

  while (s->available() > 0)
  {
    char c = (char)s->read();
    if (m_inNmea || c == '$')
    {
      m_inNmea = true;
      if (m_port == EF_PORT_GNSS)
        mount.gnss.encode((uint8_t)c);
      if (c == '\n') m_inNmea = false;
      m_rxLen = 0;
      continue;
    }

    // Bytes that arrive when no command is outstanding are noise (a trailing
    // '#' , NMEA leftovers). A waiting reply is collected until '#' so values
    // such as "2.5" and "80" are not dropped for starting with the wrong glyph.
    if (m_link != LINK_WAIT)
      continue;

    if (c == '#')
    {
      m_rx[m_rxLen] = 0;
      handleLine(m_rx);
      m_rxLen = 0;
      continue;
    }
    if (m_rxLen < (int)sizeof(m_rx) - 1)
      m_rx[m_rxLen++] = c;
    else
      m_rxLen = 0;
  }
}

void EfinderManager::handleLine(const char* line)
{
  m_lastSeen = millis();
  strncpy(m_reply, line, sizeof(m_reply) - 1);
  m_reply[sizeof(m_reply) - 1] = 0;
  m_replyReady = true;
  m_link = LINK_IDLE;
}

bool EfinderManager::sendRequest(const char* cmd)
{
  Stream* s = stream();
  if (!s) return false;
  m_replyReady = false;
  m_reply[0] = 0;
  m_link = LINK_WAIT;
  m_reqTime = millis();
  s->print(cmd);
  s->flush();
  return true;
}

bool EfinderManager::transact(const char* cmd, char* reply, int len, unsigned long timeoutMs)
{
  if (!sendRequest(cmd)) return false;
  unsigned long t0 = millis();
  while (millis() - t0 < timeoutMs)
  {
    pump();
    if (m_replyReady)
    {
      strncpy(reply, m_reply, len - 1);
      reply[len - 1] = 0;
      m_replyReady = false;
      return true;
    }
    delay(1);
  }
  m_link = LINK_IDLE;
  return false;
}

void EfinderManager::pushPointing()
{
  telescopePointing(m_pushedRaH, m_pushedDecD);
  char cmd[48];
  char reply[16];
  snprintf(cmd, sizeof(cmd), ":eC%.6f,%.5f#", m_pushedRaH, m_pushedDecD);
  transact(cmd, reply, sizeof(reply), REPLY_TIMEOUT_MS);
}

void EfinderManager::pushSiteAndTime()
{
  char cmd[48];
  snprintf(cmd, sizeof(cmd), ":eT%04d%02d%02d%02d%02d%02d#",
           year(), month(), day(), hour(), minute(), second());
  char reply[16];
  transact(cmd, reply, sizeof(reply), REPLY_TIMEOUT_MS);

  double lat = *localSite.latitude();
  double lon = *localSite.longitude();
  int elev = *localSite.elevation();
  snprintf(cmd, sizeof(cmd), ":eL%.5f,%.5f,%d#", lat, lon, elev);
  transact(cmd, reply, sizeof(reply), REPLY_TIMEOUT_MS);

  pushPointing();

  if (m_nmea)
    transact(":eN1#", reply, sizeof(reply), REPLY_TIMEOUT_MS);
}

bool EfinderManager::startSolve(bool sync)
{
  if (!present() || m_job != EF_JOB_IDLE) return false;
  m_job = sync ? EF_JOB_SOLVE_SYNC : EF_JOB_SOLVE;
  m_step = STEP_START;
  m_stepTime = millis();
  m_failures = 0;
  m_lastResult = EF_RES_NONE;
  m_failCode = 0;
  return true;
}

bool EfinderManager::startOffset()
{
  if (!present() || m_job != EF_JOB_IDLE) return false;
  m_job = EF_JOB_OFFSET;
  m_step = STEP_START;
  m_stepTime = millis();
  m_failures = 0;
  m_lastResult = EF_RES_NONE;
  return true;
}

bool EfinderManager::startAimCal()
{
  if (!present() || m_job != EF_JOB_IDLE) return false;
  m_job = EF_JOB_AIM;
  m_step = STEP_START;
  m_stepTime = millis();
  m_failures = 0;
  m_lastResult = EF_RES_NONE;
  m_failCode = 0;
  return true;
}

bool EfinderManager::skyReaches(double raDeg, double decDeg, PoleSide side) const
{
  double ha = haRange(rtk.LST() * 15.0 - raDeg);
  double lat = *localSite.latitude() * DEG_TO_RAD;
  Coord_EQ eq(0, decDeg * DEG_TO_RAD, ha * DEG_TO_RAD);
  Coord_HO ho = eq.To_Coord_HO(lat, mount.refrOptForGoto());
  const double altDeg = ho.Alt() * RAD_TO_DEG;
  if (altDeg < (double)mount.limits.minAlt + 12.0) return false;
  if (altDeg > (double)mount.limits.maxAlt - 8.0) return false;
  if (altDeg > 78.0) return false;
  long a1 = 0, a2 = 0;
  PoleSide out = POLE_NOTVALID;
  if (!mount.predictTargetHO(ho, side, a1, a2, out, false, false))
    return false;
  return out == side;
}

bool EfinderManager::fillAlignSide(PoleSide side, uint8_t slot0, uint8_t count, double preferSign)
{
  if (count < 2 || count > 4 || slot0 + count > 6) return false;
  const double lstDeg = rtk.LST() * 15.0;
  double dec0 = *localSite.latitude();
  if (dec0 > 50.0) dec0 = 50.0;
  if (dec0 < -50.0) dec0 = -50.0;
  double hours[4];
  double decOff[4];
  if (count == 2)
  {
    hours[0] = 2.2; hours[1] = 5.0;
    decOff[0] = -12.0; decOff[1] = 8.0;
  }
  else if (count == 4)
  {
    hours[0] = 1.6; hours[1] = 3.0; hours[2] = 4.4; hours[3] = 5.8;
    decOff[0] = -18.0; decOff[1] = -4.0; decOff[2] = 8.0; decOff[3] = 14.0;
  }
  else
  {
    hours[0] = 2.0; hours[1] = 3.6; hours[2] = 5.2;
    decOff[0] = -18.0; decOff[1] = 0.0; decOff[2] = 12.0;
  }
  const double signs[2] = { preferSign, -preferSign };
  for (int s = 0; s < 2; s++)
  {
    bool ok = true;
    AlignStop trial[4];
    for (uint8_t i = 0; i < count; i++)
    {
      const double haDeg = signs[s] * hours[i] * 15.0;
      double ra = lstDeg - haDeg;
      while (ra < 0.0) ra += 360.0;
      while (ra >= 360.0) ra -= 360.0;
      trial[i].raDeg = ra;
      trial[i].decDeg = dec0 + decOff[i];
      trial[i].side = side;
      if (!skyReaches(trial[i].raDeg, trial[i].decDeg, side))
      {
        ok = false;
        break;
      }
    }
    if (!ok) continue;
    for (uint8_t i = 0; i < count; i++)
      m_plan[slot0 + i] = trial[i];
    return true;
  }
  return false;
}

bool EfinderManager::fillAlignSpread(uint8_t count)
{
  if (count != 2 && count != 4 && count != 6) return false;
  const double lstDeg = rtk.LST() * 15.0;
  double dec0 = *localSite.latitude();
  if (dec0 > 40.0) dec0 = 40.0;
  if (dec0 < -40.0) dec0 = -40.0;
  double hours[6];
  double decOff[6];
  if (count == 2)
  {
    hours[0] = -3.5; hours[1] = 3.5;
    decOff[0] = -10.0; decOff[1] = 8.0;
  }
  else if (count == 4)
  {
    hours[0] = -4.8; hours[1] = -2.0; hours[2] = 2.0; hours[3] = 4.8;
    decOff[0] = -12.0; decOff[1] = 6.0; decOff[2] = -4.0; decOff[3] = 14.0;
  }
  else
  {
    hours[0] = -5.0; hours[1] = -3.2; hours[2] = -1.4;
    hours[3] = 1.4; hours[4] = 3.2; hours[5] = 5.0;
    decOff[0] = -10.0; decOff[1] = 8.0; decOff[2] = -16.0;
    decOff[3] = 12.0; decOff[4] = 0.0; decOff[5] = -8.0;
  }
  for (uint8_t i = 0; i < count; i++)
  {
    double ra = lstDeg - hours[i] * 15.0;
    while (ra < 0.0) ra += 360.0;
    while (ra >= 360.0) ra -= 360.0;
    m_plan[i].raDeg = ra;
    m_plan[i].decDeg = dec0 + decOff[i];
    m_plan[i].side = POLE_UNDER;
    if (!skyReaches(m_plan[i].raDeg, m_plan[i].decDeg, POLE_UNDER))
      return false;
  }
  return true;
}

bool EfinderManager::buildAlignPlan()
{
  m_alignFlipped = false;
  m_pendingFlip = false;
  // Six stars is 3+3 from the home pier. An alt-azimuth mount has one pier,
  // so those six places are spread on both sides of the meridian.
  if (m_alignStars == 6)
  {
    m_alignFlip = !mount.isAltAZ();
    if (!m_alignFlip)
      return fillAlignSpread(6);
    const PoleSide side = mount.getPoleSide();
    const PoleSide other = (side == POLE_UNDER) ? POLE_OVER : POLE_UNDER;
    if (!fillAlignSide(side, 0, 3, 1.0)) return false;
    const double ha0 = haRange(rtk.LST() * 15.0 - m_plan[0].raDeg);
    if (!fillAlignSide(other, 3, 3, (ha0 < 0.0) ? 1.0 : -1.0)) return false;
    // The two sides must not have picked the same hour-angle sign.
    const double ha3 = haRange(rtk.LST() * 15.0 - m_plan[3].raDeg);
    if ((ha0 < 0.0) == (ha3 < 0.0))
      return false;
    return true;
  }
  // 2 Stars and 4 Stars stay on the home pier. An alt-azimuth mount spreads them.
  m_alignFlip = false;
  if (mount.isAltAZ())
    return fillAlignSpread(m_alignStars);
  return fillAlignSide(mount.getPoleSide(), 0, m_alignStars, 1.0);
}

bool EfinderManager::slewToPlan(uint8_t index)
{
  if (index >= m_alignStars) return false;
  const AlignStop& stop = m_plan[index];
  double ha = haRange(rtk.LST() * 15.0 - stop.raDeg);
  Coord_EQ eq(0, stop.decDeg * DEG_TO_RAD, ha * DEG_TO_RAD);
  m_suppressGotoWatch = true;
  byte err = mount.goToEqu(eq, stop.side, *localSite.latitude() * DEG_TO_RAD);
  if (err != 0)
  {
    m_suppressGotoWatch = false;
    return false;
  }
  mount.alignment.alignPhase = ALIGN_SLEW;
  mount.alignment.alignStarNum = (uint8_t)(index + 1);
  m_step = STEP_WAIT_GOTO;
  m_stepTime = millis();
  return true;
}

bool EfinderManager::beginAlignFlip()
{
  m_suppressGotoWatch = true;
  ErrorsGoTo err = mount.flip();
  if (err != ErrorsGoTo::ERRGOTO_NONE)
  {
    m_suppressGotoWatch = false;
    return false;
  }
  m_alignFlipped = true;
  m_pendingFlip = true;
  mount.alignment.alignPhase = ALIGN_SLEW;
  m_step = STEP_WAIT_GOTO;
  m_stepTime = millis();
  return true;
}

void EfinderManager::snapshotAlignModel()
{
  m_hadModel = XEEPROM.read(getMountAddress(EE_Tvalid)) == 1;
  m_hadHead = false;
  if (!m_hadModel) return;
  const int addr[9] = {
    EE_T11, EE_T12, EE_T13, EE_T21, EE_T22, EE_T23, EE_T31, EE_T32, EE_T33
  };
  for (int i = 0; i < 9; i++)
    m_savedT[i] = XEEPROM.readFloat(getMountAddress(addr[i]));
  m_hadHead = XEEPROM.read(getHeadAddress(EE_head_valid)) == 1;
  m_savedHead[0] = XEEPROM.readFloat(getHeadAddress(EE_head_cone));
  m_savedHead[1] = XEEPROM.readFloat(getHeadAddress(EE_head_perp));
  m_savedHead[2] = XEEPROM.readFloat(getHeadAddress(EE_head_idx2));
}

void EfinderManager::restoreAlignModel()
{
  if (!m_alignActive) return;
  m_alignActive = false;
  mount.alignment.plateSolveSession = false;
  if (!m_hadModel)
  {
    initTransformation(true);
    return;
  }
  const int addr[9] = {
    EE_T11, EE_T12, EE_T13, EE_T21, EE_T22, EE_T23, EE_T31, EE_T32, EE_T33
  };
  for (int i = 0; i < 9; i++)
    XEEPROM.writeFloat(getMountAddress(addr[i]), m_savedT[i]);
  XEEPROM.write(getMountAddress(EE_Tvalid), 1);
  XEEPROM.writeFloat(getHeadAddress(EE_head_cone), m_savedHead[0]);
  XEEPROM.writeFloat(getHeadAddress(EE_head_perp), m_savedHead[1]);
  XEEPROM.writeFloat(getHeadAddress(EE_head_idx2), m_savedHead[2]);
  XEEPROM.write(getHeadAddress(EE_head_valid), m_hadHead ? 1 : 0);
  initTransformation(false);
}

void EfinderManager::failAlign(char code)
{
  m_failCode = code;
  restoreAlignModel();
  mount.alignment.alignPhase = ALIGN_IDLE;
  mount.alignment.alignStarNum = 0;
  mount.alignment.autoAlignmentBySync = false;
  m_pendingFlip = false;
  m_suppressGotoWatch = false;
  finishJob(EF_RES_FAILED);
}

void EfinderManager::closeAlignSession()
{
  saveAlignModel();
  m_alignActive = false;
  mount.alignment.plateSolveSession = false;
  mount.alignment.alignPhase = ALIGN_IDLE;
  mount.alignment.autoAlignmentBySync = false;
  m_pendingFlip = false;
  m_suppressGotoWatch = false;
  finishJob(EF_RES_ALIGNED);
}

bool EfinderManager::startAlign(uint8_t stars)
{
  if (!present() || m_job != EF_JOB_IDLE) return false;
  if (mount.isParked() || mount.isSlewing()) return false;
  if (stars != 2 && stars != 4 && stars != 6) return false;
  if (!mount.parkHome.atHome) return false;
  m_alignStars = stars;
  snapshotAlignModel();
  initTransformation(true);
  m_alignActive = true;
  if (!mount.syncAtHome())
  {
    restoreAlignModel();
    return false;
  }
  mount.axes.enable(true);
  if (mount.motorsEncoders.enableMotor)
    mount.startSideralTracking();
  if (!buildAlignPlan())
  {
    restoreAlignModel();
    return false;
  }
  // Two stars close with the classic pole estimate. Four and six use the
  // rigid fit, which needs at least four stars.
  mount.alignment.alignRigidStars = (m_alignStars >= 4) ? m_alignStars : 0;
  mount.alignment.alignNumStarsSession = m_alignStars;
  mount.alignment.alignPhase = ALIGN_SLEW;
  mount.alignment.alignStarNum = 1;
  mount.alignment.alignPolarThirdPending = false;
  mount.alignment.autoAlignmentBySync = false;
  mount.alignment.plateSolveSession = true;
  m_job = EF_JOB_ALIGN;
  m_step = STEP_WAIT_GOTO;
  m_stepTime = millis();
  m_pass = 0;
  m_failures = 0;
  m_alignGot = 0;
  m_lastResult = EF_RES_NONE;
  m_failCode = 0;
  if (!slewToPlan(0))
  {
    failAlign('G');
    return false;
  }
  return true;
}

void EfinderManager::abort()
{
  if (m_alignActive)
    restoreAlignModel();
  mount.alignment.plateSolveSession = false;
  mount.alignment.autoAlignmentBySync = false;
  m_pendingFlip = false;
  m_suppressGotoWatch = false;
  m_job = EF_JOB_IDLE;
  m_step = STEP_NONE;
  m_link = LINK_IDLE;
}

bool EfinderManager::resetOffset()
{
  if (!present()) return false;
  m_aimHz = 0;
  m_aimVe = 0;
  m_aimHor = false;
  saveSettings();
  char reply[16];
  return transact(":eo#", reply, sizeof(reply), REPLY_TIMEOUT_MS);
}

bool EfinderManager::passthrough(const char* cmd, char* reply, int len)
{
  if (!present() || m_job != EF_JOB_IDLE) return false;
  return transact(cmd, reply, len, REPLY_TIMEOUT_MS);
}

void EfinderManager::finishJob(EfinderResult r)
{
  m_lastResult = r;
  m_job = EF_JOB_IDLE;
  m_step = STEP_NONE;
  m_link = LINK_IDLE;
}

void EfinderManager::onFailed(char code)
{
  m_failCode = code;
  m_failures++;
  if (m_failures >= MAX_FAILURES || m_job == EF_JOB_SOLVE || m_job == EF_JOB_OFFSET ||
      m_job == EF_JOB_AIM)
  {
    if (m_job == EF_JOB_ALIGN)
      failAlign(code);
    else
      finishJob(EF_RES_FAILED);
    return;
  }
  // retry for GoTo++ / align
  m_step = STEP_START;
  m_stepTime = millis();
}

bool EfinderManager::applySync(double raH, double decD)
{
  mount.targetCurrent.newTargetRA = raH * 15.0;
  mount.targetCurrent.newTargetDec = decD;
  syncOnTarget(mount.getPoleSide());
  return true;
}

void EfinderManager::telescopePointing(double& raH, double& decD) const
{
  Coord_EQ EQ = mount.getEqu(*localSite.latitude() * DEG_TO_RAD);
  raH = EQ.Ra(rtk.LST() * HOUR_TO_RAD) * RAD_TO_HOUR;
  decD = EQ.Dec() * RAD_TO_DEG;
}

bool EfinderManager::mountIsAltAz() const
{
  MountType t = mount.config.identity.mountType;
  return t == MOUNT_TYPE_ALTAZM || t == MOUNT_TYPE_FORK_ALT;
}

void EfinderManager::equToHor(double raH, double decD, double& altRad, double& azRad) const
{
  double haDeg = haRange(rtk.LST() * 15.0 - raH * 15.0);
  Coord_EQ eq(0.0, decD * DEG_TO_RAD, haDeg * DEG_TO_RAD);
  LA3::RefrOpt opt = mount.refrOptForTracking();
  opt.use = false;
  Coord_HO ho = eq.To_Coord_HO(*localSite.latitude() * DEG_TO_RAD, opt);
  altRad = ho.Alt();
  azRad = ho.Az();
}

void EfinderManager::horToEqu(double altRad, double azRad, double& raH, double& decD) const
{
  Coord_HO ho(0.0, altRad, azRad, false);
  Coord_EQ eq = ho.To_Coord_EQ(*localSite.latitude() * DEG_TO_RAD);
  raH = eq.Ra(rtk.LST() * HOUR_TO_RAD) * RAD_TO_HOUR;
  decD = eq.Dec() * RAD_TO_DEG;
}

void EfinderManager::finderToTelescope(double& raH, double& decD) const
{
  if (m_aimHz == 0.f && m_aimVe == 0.f)
    return;
  if (m_aimHor)
  {
    double alt = 0, az = 0;
    equToHor(raH, decD, alt, az);
    double c = cos(alt);
    if (c > -AIM_MIN_COS && c < AIM_MIN_COS)
      c = (c < 0.0) ? -AIM_MIN_COS : AIM_MIN_COS;
    az -= (double)m_aimHz / c;
    alt -= (double)m_aimVe;
    horToEqu(alt, az, raH, decD);
    return;
  }
  double c = cos(decD * DEG_TO_RAD);
  if (c > -AIM_MIN_COS && c < AIM_MIN_COS)
    c = (c < 0.0) ? -AIM_MIN_COS : AIM_MIN_COS;
  raH -= ((double)m_aimHz / c) * RAD_TO_DEG / 15.0;
  if (raH < 0.0) raH += 24.0;
  else if (raH >= 24.0) raH -= 24.0;
  decD -= (double)m_aimVe * RAD_TO_DEG;
}

void EfinderManager::measureAim(double scopeRaH, double scopeDecD,
                                 double finderRaH, double finderDecD)
{
  // The sync and the plate solve are sky positions. Rotate that separation
  // into the telescope frame. The stored cones are the telescope's own
  // horizontal and vertical, not sky coordinates.
  m_aimHor = mountIsAltAz();
  if (m_aimHor)
  {
    double sAlt = 0, sAz = 0, fAlt = 0, fAz = 0;
    equToHor(scopeRaH, scopeDecD, sAlt, sAz);
    equToHor(finderRaH, finderDecD, fAlt, fAz);
    double dAz = fAz - sAz;
    if (dAz > M_PI) dAz -= 2.0 * M_PI;
    else if (dAz < -M_PI) dAz += 2.0 * M_PI;
    m_aimHz = (float)(dAz * cos(sAlt));
    m_aimVe = (float)(fAlt - sAlt);
  }
  else
  {
    double dRaH = finderRaH - scopeRaH;
    if (dRaH > 12.0) dRaH -= 24.0;
    else if (dRaH < -12.0) dRaH += 24.0;
    double c = cos(scopeDecD * DEG_TO_RAD);
    m_aimHz = (float)(dRaH * 15.0 * DEG_TO_RAD * c);
    m_aimVe = (float)((finderDecD - scopeDecD) * DEG_TO_RAD);
  }
  saveSettings();
}

void EfinderManager::onSolved(double raH, double decD)
{
  if (m_job == EF_JOB_AIM)
  {
    if (mountIsAltAz())
    {
      double alt = 0, az = 0;
      equToHor(m_pushedRaH, m_pushedDecD, alt, az);
      if (fabs(cos(alt)) < AIM_MIN_COS)
      {
        onFailed('Z');
        return;
      }
    }
    else if (fabs(cos(m_pushedDecD * DEG_TO_RAD)) < AIM_MIN_COS)
    {
      onFailed('P');
      return;
    }
    measureAim(m_pushedRaH, m_pushedDecD, raH, decD);
    double hzArcmin = (double)m_aimHz * RAD_TO_DEG * 60.0;
    double veArcmin = (double)m_aimVe * RAD_TO_DEG * 60.0;
    snprintf(m_offsetInfo, sizeof(m_offsetInfo), "%.2f,%.2f,cone", hzArcmin, veArcmin);
    finishJob(EF_RES_OFFSET);
    return;
  }

  // Plate solve is the efinder axis. Sync, align and GoTo++ follow the telescope.
  finderToTelescope(raH, decD);
  m_solvedRaH = raH;
  m_solvedDecD = decD;

  if (m_job == EF_JOB_SOLVE)
  {
    finishJob(EF_RES_SOLVED);
    return;
  }

  if (m_job == EF_JOB_ALIGN)
  {
    const uint8_t star = (uint8_t)(m_alignGot + 1);
    // The solved telescope axis is the measurement, not the coordinates of the slew.
    if (!alignmentAcceptSolved(star, raH * 15.0, decD))
    {
      failAlign('A');
      return;
    }
    m_alignGot++;
    m_failures = 0;
    if (m_alignGot >= m_alignStars)
    {
      closeAlignSession();
      return;
    }
    mount.alignment.plateSolveSession = true;
    mount.alignment.alignPhase = ALIGN_SLEW;
    if (m_alignFlip && m_alignGot == 3 && !m_alignFlipped)
    {
      if (!beginAlignFlip())
        failAlign('G');
      return;
    }
    if (!slewToPlan(m_alignGot))
      failAlign('G');
    return;
  }

  if (m_job == EF_JOB_SOLVE_SYNC)
  {
    applySync(raH, decD);
    finishJob(EF_RES_SYNCED);
    return;
  }

  if (m_job == EF_JOB_GOTOPP)
  {
    applySync(raH, decD);
    double solvedRaDeg = raH * 15.0;
    double dra = solvedRaDeg - m_targetRaH;
    while (dra > 180.0) dra -= 360.0;
    while (dra < -180.0) dra += 360.0;
    dra *= cos(m_targetDecD * DEG_TO_RAD);
    double ddec = decD - m_targetDecD;
    m_errorArcmin = sqrt(dra * dra + ddec * ddec) * 60.0;

    if (m_errorArcmin <= (double)m_thresholdArcmin || m_pass > 0)
    {
      finishJob(EF_RES_SYNCED);
      return;
    }

    // one refinement slew back to the original target
    m_pass = 1;
    double ha = haRange(rtk.LST() * 15.0 - m_targetRaH);
    Coord_EQ EQ_T(0, m_targetDecD * DEG_TO_RAD, ha * DEG_TO_RAD);
    m_suppressGotoWatch = true;
    byte err = mount.goToEqu(EQ_T, mount.getPoleSide(), *localSite.latitude() * DEG_TO_RAD);
    if (err != 0)
    {
      m_suppressGotoWatch = false;
      finishJob(EF_RES_SYNCED);
      return;
    }
    m_step = STEP_WAIT_GOTO;
    m_stepTime = millis();
    return;
  }
}

void EfinderManager::runJob()
{
  // pump()/handleLine() clears LINK_WAIT as soon as the bytes arrive, which
  // is before this runs. Gate on the reply itself, not on the link still
  // being in WAIT, or a fast slave (the in-process stub, or a Pi that answers
  // within one loop) is never observed and the job stays busy.
  if (m_replyReady)
  {
    m_replyReady = false;
    m_link = LINK_IDLE;
    const char* r = m_reply;

    if (m_step == STEP_START)
    {
      if (r[0] == '0')
      {
        onFailed('0');
        return;
      }
      m_step = STEP_POLL;
      m_stepTime = millis();
      return;
    }

    if (m_step == STEP_POLL)
    {
      if (r[0] == 'B')
        return;
      if (r[0] == 'N')
      {
        onFailed('N');
        return;
      }
      if (r[0] == 'F')
      {
        onFailed(r[1] ? r[1] : 'F');
        return;
      }
      if (r[0] == 'O')
      {
        strncpy(m_offsetInfo, r + 1, sizeof(m_offsetInfo) - 1);
        char* hash = strchr(m_offsetInfo, '#');
        if (hash) *hash = 0;
        finishJob(EF_RES_OFFSET);
        return;
      }
      if (r[0] == 'S')
      {
        double raH = 0, decD = 0;
        int stars = 0, ms = 0;
        if (sscanf(r + 1, "%lf,%lf,%d,%d", &raH, &decD, &stars, &ms) >= 2)
          onSolved(raH, decD);
        else
          onFailed('P');
        return;
      }
      onFailed('?');
      return;
    }
    return;
  }

  if (m_link == LINK_WAIT)
  {
    if (millis() - m_reqTime > ((m_step == STEP_POLL) ? SOLVE_TIMEOUT_MS : REPLY_TIMEOUT_MS))
    {
      m_link = LINK_IDLE;
      onFailed('T');
    }
    return;
  }

  if (m_step == STEP_START)
  {
    // Refresh pointing so a SimCamera / plate-solve hint matches the mount
    pushPointing();
    const char* cmd = (m_job == EF_JOB_OFFSET) ? ":eO#" : ":eS#";
    sendRequest(cmd);
    return;
  }

  if (m_step == STEP_POLL)
  {
    if (millis() - m_stepTime > SOLVE_TIMEOUT_MS)
    {
      onFailed('T');
      return;
    }
    // poll every 400 ms
    if (millis() - m_reqTime < 400 && m_reqTime != 0) return;
    sendRequest(":eR#");
    return;
  }

  if (m_step == STEP_SETTLE)
  {
    unsigned long settle = (unsigned long)mount.parkHome.slewSettleDuration * 1000UL;
    if (settle < 500UL) settle = 500UL;
#ifdef EMU_MAINUNIT
    settle = 200UL;
#endif
    if (millis() - m_stepTime >= settle)
    {
      m_step = STEP_START;
      m_stepTime = millis();
    }
    return;
  }

  if (m_step == STEP_WAIT_GOTO)
  {
    const bool idle = mount.tracking.gotoState == GOTO_NONE && !mount.parkHome.settling && !mount.isSlewing();
    if (idle)
    {
      if (m_job == EF_JOB_ALIGN && m_pendingFlip)
      {
        m_pendingFlip = false;
        if (!slewToPlan(m_alignGot))
          failAlign('G');
        return;
      }
      if (m_job == EF_JOB_ALIGN)
      {
        m_step = STEP_SETTLE;
        m_stepTime = millis();
        return;
      }
      m_step = STEP_START;
      m_stepTime = millis();
    }
    else if (millis() - m_stepTime > (m_job == EF_JOB_ALIGN ? 180000UL : 120000UL))
    {
      if (m_job == EF_JOB_ALIGN)
        failAlign('T');
      else
        finishJob(EF_RES_SYNCED);
    }
  }
}

void EfinderManager::watchGoto()
{
  uint8_t gs = (uint8_t)mount.tracking.gotoState;
  if (m_suppressGotoWatch)
  {
    if (m_prevGotoState != (uint8_t)GOTO_NONE && gs == (uint8_t)GOTO_NONE)
      m_suppressGotoWatch = false;
    m_prevGotoState = gs;
    return;
  }
  if (m_gotoPP && present() && m_job == EF_JOB_IDLE &&
      m_prevGotoState != (uint8_t)GOTO_NONE && gs == (uint8_t)GOTO_NONE &&
      !mount.isParked() && !mount.parkHome.homeMount && !mount.parkHome.settling)
  {
    m_targetRaH = mount.targetCurrent.newTargetRA; // degrees
    m_targetDecD = mount.targetCurrent.newTargetDec;
    m_job = EF_JOB_GOTOPP;
    m_step = STEP_START;
    m_stepTime = millis();
    m_pass = 0;
    m_failures = 0;
    m_lastResult = EF_RES_NONE;
  }
  m_prevGotoState = gs;
}

void EfinderManager::formatResult(char* out, int len) const
{
  switch (m_lastResult)
  {
  case EF_RES_NONE:
    snprintf(out, len, "N#");
    break;
  case EF_RES_FAILED:
    snprintf(out, len, "F%c#", m_failCode ? m_failCode : 'F');
    break;
  case EF_RES_OFFSET:
    snprintf(out, len, "O%s#", m_offsetInfo);
    break;
  case EF_RES_SOLVED:
  case EF_RES_SYNCED:
  case EF_RES_ALIGNED:
    snprintf(out, len, "S%.6f,%.5f,0,0#", m_solvedRaH, m_solvedDecD);
    break;
  default:
    snprintf(out, len, "N#");
    break;
  }
}

void EfinderManager::formatStatus(char* out, int len) const
{
  if (!present())
  {
    snprintf(out, len, "0#");
    return;
  }
  const char* port = (m_port == EF_PORT_FOCUSER) ? "F" : "G";
  snprintf(out, len, "1,%s,%s,%u,%u,%u,%u#",
           port, m_version, (unsigned)m_job, (unsigned)m_lastResult,
           m_gotoPP ? 1u : 0u, (unsigned)m_thresholdArcmin);
}

/**
 * EfinderManager — plate-solving finder attached to Focuser or GNSS port.
 */
#include "Efinder.h"
#include "Command.h"
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
  uint8_t flags = XEEPROM.read(EE_efinderFlags);
  m_gotoPP = (flags & 0x01) != 0;
  uint8_t thr = XEEPROM.read(EE_efinderThresh);
  if (thr == 0 || thr > 60) thr = 2;
  m_thresholdArcmin = thr;
}

void EfinderManager::saveSettings()
{
  uint8_t flags = m_gotoPP ? 0x01 : 0x00;
  XEEPROM.write(EE_efinderFlags, flags);
  XEEPROM.write(EE_efinderThresh, m_thresholdArcmin);
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

bool EfinderManager::probePort(EfinderPort p)
{
  Stream* s = (p == EF_PORT_FOCUSER) ? focuserStream() : gnssStream();
  if (!s) return false;

  while (s->available() > 0) s->read();
  s->print(":e?#");
  s->flush();

  char buf[48];
  int n = 0;
  unsigned long t0 = millis();
  while (millis() - t0 < PROBE_TIMEOUT_MS)
  {
    if (s->available() <= 0)
    {
      delay(1);
      continue;
    }
    char c = (char)s->read();
    if (n < (int)sizeof(buf) - 1) buf[n++] = c;
    if (c == '#') break;
  }
  buf[n] = 0;
  if (n < 10) return false;
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

void EfinderManager::update()
{
  if (m_port == EF_PORT_NONE)
  {
    if (millis() - m_lastProbe < PROBE_PERIOD_MS) return;
    m_lastProbe = millis();
    if (!mount.config.peripherals.hasFocuser)
    {
      if (probePort(EF_PORT_FOCUSER)) return;
    }
#if VERSION != 220
    // Only claim the GNSS port when nothing already produced NMEA at boot,
    // or when hasGNSS was cleared because the eFinder took over.
    if (!mount.config.peripherals.hasGNSS || !GNSS_Serial.available())
    {
      // Avoid stealing a live GPS that already flagged hasGNSS at boot.
      if (!mount.config.peripherals.hasGNSS)
      {
        if (probePort(EF_PORT_GNSS)) return;
      }
    }
#endif
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
      continue;
    }

    if (m_rxLen == 0 && c != 'T' && c != 'B' && c != 'N' && c != 'F' && c != 'S' &&
        c != 'O' && c != '1' && c != '0' && c != 'y' && c != 'n')
    {
      // ignore noise until a plausible reply start
      continue;
    }

    if (m_rxLen < sizeof(m_rx) - 1)
      m_rx[m_rxLen++] = c;

    if (c == '#' || (m_rxLen == 1 && (c == '1' || c == '0')))
    {
      // short bool replies may omit '#'
      if (c != '#' && (c == '1' || c == '0'))
      {
        m_rx[m_rxLen++] = '#';
      }
      m_rx[m_rxLen] = 0;
      handleLine(m_rx);
      m_rxLen = 0;
    }
    else if (m_rxLen >= sizeof(m_rx) - 1)
    {
      m_rxLen = 0;
    }
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

bool EfinderManager::startAlign()
{
  if (!present() || m_job != EF_JOB_IDLE) return false;
  // Enable alignment-by-sync then solve+sync for each star the user confirms via :eY# / startSolve(true).
  mount.alignment.autoAlignmentBySync = true;
  m_job = EF_JOB_ALIGN;
  m_step = STEP_START;
  m_stepTime = millis();
  m_pass = 0;
  m_failures = 0;
  m_lastResult = EF_RES_NONE;
  return true;
}

void EfinderManager::abort()
{
  m_job = EF_JOB_IDLE;
  m_step = STEP_NONE;
  m_link = LINK_IDLE;
}

bool EfinderManager::resetOffset()
{
  if (!present()) return false;
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
  if (m_failures >= MAX_FAILURES || m_job == EF_JOB_SOLVE || m_job == EF_JOB_OFFSET)
  {
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

void EfinderManager::onSolved(double raH, double decD)
{
  m_solvedRaH = raH;
  m_solvedDecD = decD;

  if (m_job == EF_JOB_SOLVE)
  {
    finishJob(EF_RES_SOLVED);
    return;
  }

  if (m_job == EF_JOB_SOLVE_SYNC || m_job == EF_JOB_ALIGN)
  {
    applySync(raH, decD);
    if (m_job == EF_JOB_ALIGN)
    {
      m_pass++;
      if (!mount.alignment.autoAlignmentBySync)
      {
        finishJob(EF_RES_ALIGNED);
        return;
      }
      // wait for next :eY# / startSolve from host while keeping ALIGN job idle-ish
      finishJob(EF_RES_SYNCED);
      return;
    }
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
  if (m_link == LINK_WAIT)
  {
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
        {
          // still busy
          return;
        }
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
          {
            onSolved(raH, decD);
          }
          else
          {
            onFailed('P');
          }
          return;
        }
        onFailed('?');
      }
    }
    else if (millis() - m_reqTime > ((m_step == STEP_POLL) ? SOLVE_TIMEOUT_MS : REPLY_TIMEOUT_MS))
    {
      m_link = LINK_IDLE;
      onFailed('T');
    }
    return;
  }

  if (m_step == STEP_START)
  {
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

  if (m_step == STEP_WAIT_GOTO)
  {
    if (mount.tracking.gotoState == GOTO_NONE && !mount.parkHome.settling && !mount.isSlewing())
    {
      m_step = STEP_START;
      m_stepTime = millis();
    }
    else if (millis() - m_stepTime > 120000UL)
    {
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

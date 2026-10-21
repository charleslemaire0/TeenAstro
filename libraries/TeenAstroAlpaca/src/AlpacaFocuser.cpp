/*
 * AlpacaFocuser.cpp - ASCOM Alpaca IFocuserV3 for TeenAstro
 *
 * Copyright (C) 2026 TeenAstro by Charles Lemaire
 * GNU General Public License v3
 */
#include "AlpacaFocuser.h"
#include "TeenAstroAlpaca.h"
#include <LX200Client.h>
#include <stdlib.h>

namespace
{

// `:F?#` is "?position speed temperature" or "0" when no focuser is attached.
// A leading 's' on the temperature is a minus sign, matching the COM driver.
bool parseFocuserStatus(const char* raw, int& pos, int& speed, double& temp)
{
  const char* p = raw ? raw : "";
  while (*p == ' ') ++p;
  if (*p == '\0' || (*p == '0' && p[1] == '\0')) return false;
  if (*p == '?') ++p;

  char* end = nullptr;
  long v = strtol(p, &end, 10);
  if (end == p) return false;
  pos = (int)v;
  speed = 0;
  temp = 0;

  p = end;
  while (*p == ' ') ++p;
  if (*p)
  {
    v = strtol(p, &end, 10);
    if (end != p)
    {
      speed = (int)v;
      p = end;
    }
  }
  while (*p == ' ') ++p;
  if (*p)
  {
    bool neg = false;
    if (*p == 's' || *p == 'S') { neg = true; ++p; }
    double t = strtod(p, &end);
    if (end != p) temp = neg ? -t : t;
  }
  return true;
}

// `:F~#` is "~park max minSpeed maxSpeed ...". The second integer is MaxStep.
bool parseFocuserMax(const char* raw, int& maxStep)
{
  const char* p = raw ? raw : "";
  while (*p == ' ') ++p;
  if (*p == '~') ++p;
  char* end = nullptr;
  strtol(p, &end, 10);
  if (end == p) return false;
  p = end;
  while (*p == ' ') ++p;
  long v = strtol(p, &end, 10);
  if (end == p || v <= 0) return false;
  maxStep = (int)v;
  return true;
}

}  // namespace

void AlpacaFocuser::begin(LX200Client& client, TeenAstroAlpaca& parent)
{
  m_client = &client;
  m_parent = &parent;
}

bool AlpacaFocuser::requireConnected(AlpacaWebServer& s, const AlpacaRequest& r)
{
  if (m_connected) return true;
  sendAlpacaError(s, r, m_parent->nextServerTransactionId(),
                  AE_NOT_CONNECTED, "Focuser not connected");
  return false;
}

bool AlpacaFocuser::moving() const
{
  if (m_speed != 0) return true;
  if (!m_targetSet) return false;
  int d = m_position - m_target;
  if (d < 0) d = -d;
  return d > 2;
}

int AlpacaFocuser::maxStep() const
{
  return m_maxValid ? m_maxStep : 65535;
}

void AlpacaFocuser::prepareForRequest()
{
  if (!m_client) return;
  unsigned long now = millis();
  unsigned long period = moving() ? 200UL : 500UL;
  if (m_statusValid && (now - m_statusMs) < period) return;

  char out[64];
  int pos = 0, speed = 0;
  double temp = 0;
  bool ok = isOk(m_client->getFocuserStatus(out, sizeof(out)))
            && parseFocuserStatus(out, pos, speed, temp);
  m_present = ok;
  m_statusValid = true;
  m_statusMs = now;
  if (!ok)
  {
    m_targetSet = false;
    m_speed = 0;
    return;
  }

  m_position = pos;
  m_speed = speed;
  m_temperature = temp;
  if (m_targetSet)
  {
    int d = pos - m_target;
    if (d < 0) d = -d;
    if (d <= 2) m_targetSet = false;
  }

  if (!m_maxValid || (now - m_configMs) >= 30000UL)
  {
    if (isOk(m_client->getFocuserConfigRaw(out, sizeof(out))))
    {
      int mx = 0;
      if (parseFocuserMax(out, mx))
      {
        m_maxStep = mx;
        m_maxValid = true;
        m_configMs = now;
      }
    }
  }
}

#define MATCH(name) if (m == F(name))

void AlpacaFocuser::dispatch(AlpacaWebServer& server, const AlpacaRequest& req)
{
  const String& m = req.member;

  MATCH("connected")         { req.isPut ? putConnected(server, req) : getConnected(server, req); return; }
  MATCH("name")              { getName(server, req); return; }
  MATCH("description")       { getDescription(server, req); return; }
  MATCH("driverinfo")        { getDriverInfo(server, req); return; }
  MATCH("driverversion")     { getDriverVersion(server, req); return; }
  MATCH("interfaceversion")  { getInterfaceVersion(server, req); return; }
  MATCH("supportedactions")  { getSupportedActions(server, req); return; }
  MATCH("action")            { putAction(server, req); return; }
  MATCH("commandblind")      { putCommand(server, req); return; }
  MATCH("commandbool")       { putCommand(server, req); return; }
  MATCH("commandstring")     { putCommand(server, req); return; }

  MATCH("absolute")          { getAbsolute(server, req); return; }
  MATCH("ismoving")          { getIsMoving(server, req); return; }
  MATCH("maxincrement")      { getMaxIncrement(server, req); return; }
  MATCH("maxstep")           { getMaxStep(server, req); return; }
  MATCH("position")          { getPosition(server, req); return; }
  MATCH("stepsize")          { getStepSize(server, req); return; }
  MATCH("tempcomp")          { req.isPut ? putTempComp(server, req) : getTempComp(server, req); return; }
  MATCH("tempcompavailable") { getTempCompAvailable(server, req); return; }
  MATCH("temperature")       { getTemperature(server, req); return; }
  MATCH("halt")              { putHalt(server, req); return; }
  MATCH("move")              { putMove(server, req); return; }

  sendAlpacaResponse(server, req, m_parent->nextServerTransactionId(),
                     AE_NOT_IMPLEMENTED, "Focuser member not implemented",
                     String(), 400);
}

#undef MATCH

void AlpacaFocuser::getConnected(AlpacaWebServer& s, const AlpacaRequest& r)
{
  sendAlpacaOk(s, r, m_parent->nextServerTransactionId(),
               AlpacaJson::boolStr(m_connected));
}

void AlpacaFocuser::putConnected(AlpacaWebServer& s, const AlpacaRequest& r)
{
  bool on = false;
  if (!alpacaArgBool(s, r, m_parent->nextServerTransactionId(), "Connected", on))
    return;
  m_connected = on;
  sendAlpacaVoid(s, r, m_parent->nextServerTransactionId());
}

void AlpacaFocuser::getName(AlpacaWebServer& s, const AlpacaRequest& r)
{
  sendAlpacaOk(s, r, m_parent->nextServerTransactionId(),
               AlpacaJson::escape(deviceName()));
}

void AlpacaFocuser::getDescription(AlpacaWebServer& s, const AlpacaRequest& r)
{
  sendAlpacaOk(s, r, m_parent->nextServerTransactionId(),
               AlpacaJson::escape("TeenAstro focuser via ASCOM Alpaca"));
}

void AlpacaFocuser::getDriverInfo(AlpacaWebServer& s, const AlpacaRequest& r)
{
  sendAlpacaOk(s, r, m_parent->nextServerTransactionId(),
               AlpacaJson::escape("TeenAstro Alpaca focuser"));
}

void AlpacaFocuser::getDriverVersion(AlpacaWebServer& s, const AlpacaRequest& r)
{
  sendAlpacaOk(s, r, m_parent->nextServerTransactionId(),
               AlpacaJson::escape("1.0.0"));
}

void AlpacaFocuser::getInterfaceVersion(AlpacaWebServer& s, const AlpacaRequest& r)
{
  sendAlpacaOk(s, r, m_parent->nextServerTransactionId(), F("3"));
}

void AlpacaFocuser::getSupportedActions(AlpacaWebServer& s, const AlpacaRequest& r)
{
  sendAlpacaOk(s, r, m_parent->nextServerTransactionId(), F("[]"));
}

void AlpacaFocuser::putAction(AlpacaWebServer& s, const AlpacaRequest& r)
{
  sendAlpacaError(s, r, m_parent->nextServerTransactionId(),
                  AE_ACTION_NOT_IMPL, "No custom actions implemented");
}

void AlpacaFocuser::putCommand(AlpacaWebServer& s, const AlpacaRequest& r)
{
  if (!s.hasArg("Command"))
  {
    sendAlpacaResponse(s, r, m_parent->nextServerTransactionId(),
                       AE_INVALID_VALUE, "Invalid parameter", String(), 400);
    return;
  }
  if (!requireConnected(s, r)) return;
  sendAlpacaError(s, r, m_parent->nextServerTransactionId(),
                  AE_NOT_IMPLEMENTED, "Focuser commands are not implemented");
}

void AlpacaFocuser::getAbsolute(AlpacaWebServer& s, const AlpacaRequest& r)
{
  sendAlpacaOk(s, r, m_parent->nextServerTransactionId(), F("true"));
}

void AlpacaFocuser::getIsMoving(AlpacaWebServer& s, const AlpacaRequest& r)
{
  if (!requireConnected(s, r)) return;
  if (!m_present)
  {
    sendAlpacaError(s, r, m_parent->nextServerTransactionId(),
                    AE_DRIVER, "Focuser not present");
    return;
  }
  sendAlpacaOk(s, r, m_parent->nextServerTransactionId(),
               AlpacaJson::boolStr(moving()));
}

void AlpacaFocuser::getMaxIncrement(AlpacaWebServer& s, const AlpacaRequest& r)
{
  getMaxStep(s, r);
}

void AlpacaFocuser::getMaxStep(AlpacaWebServer& s, const AlpacaRequest& r)
{
  if (!requireConnected(s, r)) return;
  if (!m_present)
  {
    sendAlpacaError(s, r, m_parent->nextServerTransactionId(),
                    AE_DRIVER, "Focuser not present");
    return;
  }
  sendAlpacaOk(s, r, m_parent->nextServerTransactionId(),
               AlpacaJson::intStr(maxStep()));
}

void AlpacaFocuser::getPosition(AlpacaWebServer& s, const AlpacaRequest& r)
{
  if (!requireConnected(s, r)) return;
  if (!m_present)
  {
    sendAlpacaError(s, r, m_parent->nextServerTransactionId(),
                    AE_DRIVER, "Focuser not present");
    return;
  }
  sendAlpacaOk(s, r, m_parent->nextServerTransactionId(),
               AlpacaJson::intStr(m_position));
}

void AlpacaFocuser::getStepSize(AlpacaWebServer& s, const AlpacaRequest& r)
{
  sendAlpacaOk(s, r, m_parent->nextServerTransactionId(),
               AlpacaJson::doubleStr(1.0));
}

void AlpacaFocuser::getTempComp(AlpacaWebServer& s, const AlpacaRequest& r)
{
  if (!requireConnected(s, r)) return;
  sendAlpacaOk(s, r, m_parent->nextServerTransactionId(), F("false"));
}

void AlpacaFocuser::putTempComp(AlpacaWebServer& s, const AlpacaRequest& r)
{
  bool on = false;
  if (!alpacaArgBool(s, r, m_parent->nextServerTransactionId(), "TempComp", on))
    return;
  if (!requireConnected(s, r)) return;
  (void)on;
  sendAlpacaError(s, r, m_parent->nextServerTransactionId(),
                  AE_NOT_IMPLEMENTED, "TempComp is not available");
}

void AlpacaFocuser::getTempCompAvailable(AlpacaWebServer& s, const AlpacaRequest& r)
{
  sendAlpacaOk(s, r, m_parent->nextServerTransactionId(), F("false"));
}

void AlpacaFocuser::getTemperature(AlpacaWebServer& s, const AlpacaRequest& r)
{
  if (!requireConnected(s, r)) return;
  if (!m_present)
  {
    sendAlpacaError(s, r, m_parent->nextServerTransactionId(),
                    AE_DRIVER, "Focuser not present");
    return;
  }
  sendAlpacaOk(s, r, m_parent->nextServerTransactionId(),
               AlpacaJson::doubleStr(m_temperature));
}

void AlpacaFocuser::putHalt(AlpacaWebServer& s, const AlpacaRequest& r)
{
  if (!requireConnected(s, r)) return;
  if (!m_present)
  {
    sendAlpacaError(s, r, m_parent->nextServerTransactionId(),
                    AE_DRIVER, "Focuser not present");
    return;
  }
  m_client->focuserStop();
  m_targetSet = false;
  m_speed = 0;
  sendAlpacaVoid(s, r, m_parent->nextServerTransactionId());
}

void AlpacaFocuser::putMove(AlpacaWebServer& s, const AlpacaRequest& r)
{
  long pos = 0;
  if (!alpacaArgLong(s, r, m_parent->nextServerTransactionId(), "Position", pos))
    return;
  if (!requireConnected(s, r)) return;
  if (!m_present)
  {
    sendAlpacaError(s, r, m_parent->nextServerTransactionId(),
                    AE_DRIVER, "Focuser not present");
    return;
  }
  int limit = maxStep();
  if (pos < 0 || pos > limit)
  {
    sendAlpacaError(s, r, m_parent->nextServerTransactionId(),
                    AE_INVALID_VALUE, "Position out of range");
    return;
  }
  if (moving())
  {
    sendAlpacaError(s, r, m_parent->nextServerTransactionId(),
                    AE_INVALID_OPERATION, "Focuser is moving");
    return;
  }
  if (!isOk(m_client->focuserGotoPosition((int)pos)))
  {
    sendAlpacaError(s, r, m_parent->nextServerTransactionId(),
                    AE_DRIVER, "Move refused by focuser");
    return;
  }
  m_target = (int)pos;
  m_targetSet = (m_target != m_position);
  sendAlpacaVoid(s, r, m_parent->nextServerTransactionId());
}

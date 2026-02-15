/*
 * LX200Client.cpp - Object-oriented LX200 serial client for TeenAstro
 *
 * Copyright (C) 2024 TeenAstro by Charles Lemaire
 * GNU General Public License v3
 */
#include "LX200Client.h"
#include <TeenAstroFunction.h>

// ===========================================================================
//  Construction
// ===========================================================================

LX200Client::LX200Client(Stream& serial, unsigned long timeoutMs)
  : m_serial(serial), m_timeout(timeoutMs)
{
}

// ===========================================================================
//  Private helpers
// ===========================================================================

void LX200Client::flushInput()
{
  while (m_serial.available() > 0) m_serial.read();
}

// ===========================================================================
//  Core I/O
// ===========================================================================

bool LX200Client::sendReceive(const char* command, CMDREPLY replyType,
                              char* recvBuffer, int bufferSize,
                              unsigned long timeOutMs, bool keepHashtag)
{
  m_serial.setTimeout(timeOutMs);
  memset(recvBuffer, 0, bufferSize);
  m_serial.flush();
  flushInput();

  if (replyType == CMDR_INVALID) return false;

  m_serial.print(command);

  switch (replyType)
  {
  case CMDR_NO:
    recvBuffer[0] = '\0';
    return true;

  case CMDR_SHORT:
  case CMDR_SHORT_BOOL:
  {
    unsigned long start = millis();
    recvBuffer[0] = '\0';
    while (millis() - start < timeOutMs)
    {
      if (m_serial.available())
      {
        recvBuffer[0] = (char)m_serial.read();
        break;
      }
    }
    return recvBuffer[0] != '\0';
  }

  case CMDR_LONG:
  {
    unsigned long start = millis();
    int pos = 0;
    bool hashFound = false;
    bool ok = true;
    while (millis() - start < timeOutMs)
    {
      recvBuffer[pos] = 0;
      if (m_serial.available())
      {
        char b = (char)m_serial.read();
        if (b == '#')
        {
          if (keepHashtag)
          {
            recvBuffer[pos] = b;
            pos++;
          }
          hashFound = true;
          break;
        }
        start = millis();
        recvBuffer[pos] = b;
        pos++;
        if (pos > bufferSize - 1)
        {
          ok = false;
          pos = bufferSize - 1;
        }
      }
    }
    ok &= (recvBuffer[0] != 0);
    ok &= hashFound;
    return ok;
  }

  default:
    return false;
  }
  return false;
}

bool LX200Client::sendReceiveAuto(char* command, CMDREPLY& cmdreply,
                                  char* recvBuffer, int bufferSize,
                                  unsigned long timeOutMs, bool keepHashtag)
{
  cmdreply = getReplyType(command);

  // Double timeout for G (get) commands
  if (command[0] == ':' && command[1] == 'G')
    timeOutMs *= 2;

  return sendReceive(command, cmdreply, recvBuffer, bufferSize, timeOutMs, keepHashtag);
}

LX200RETURN LX200Client::get(const char* command, char* output, int bufferSize)
{
  CMDREPLY cmdreply = getReplyType(command);
  bool ok = sendReceive(command, cmdreply, output, bufferSize, m_timeout);
  if (ok)
    return LX200_VALUEGET;
  return cmdreply == CMDR_INVALID ? LX200_INVALIDCOMMAND : LX200_GETVALUEFAILED;
}

LX200RETURN LX200Client::getShort(const char* command, short* value)
{
  char out[LX200_SBUF];
  LX200RETURN ret = get(command, out, sizeof(out));
  if (ret != LX200_VALUEGET)
    return ret;
  *value = (short)strtol(&out[0], NULL, 10);
  return LX200_VALUEGET;
}

LX200RETURN LX200Client::getFloat(const char* command, float* value)
{
  char out[LX200_SBUF];
  char* conv_end;
  LX200RETURN ret = get(command, out, sizeof(out));
  if (ret != LX200_VALUEGET)
    return ret;
  float f = (float)strtod(out, &conv_end);
  if ((&out[0] != conv_end) && (f >= -12.0f && f <= 12.0f))
  {
    *value = f;
    return LX200_VALUEGET;
  }
  return LX200_GETVALUEFAILED;
}

LX200RETURN LX200Client::set(const char* command)
{
  char out[LX200_SBUF];
  CMDREPLY cmdreply = getReplyType(command);
  bool ok = sendReceive(command, cmdreply, out, sizeof(out), m_timeout);
  if (ok)
  {
    if (cmdreply == CMDR_SHORT_BOOL)
      return out[0] == '1' ? LX200_VALUESET : LX200_SETVALUEFAILED;
    else
      return LX200_VALUESET;
  }
  return cmdreply == CMDR_INVALID ? LX200_INVALIDCOMMAND : LX200_INVALIDREPLY;
}

// ===========================================================================
//  Time / Date
// ===========================================================================

LX200RETURN LX200Client::getLocalTime(unsigned int& hour, unsigned int& minute, unsigned int& second)
{
  char out[LX200_SBUF];
  LX200RETURN ret = get(":GL#", out, sizeof(out));
  if (ret != LX200_VALUEGET) return ret;
  char2RA(out, hour, minute, second);
  return LX200_VALUEGET;
}

LX200RETURN LX200Client::getLocalTime(long& totalSeconds)
{
  unsigned int h, m, s;
  LX200RETURN ret = getLocalTime(h, m, s);
  if (ret != LX200_VALUEGET) return ret;
  totalSeconds = (long)h * 3600L + (long)m * 60L + (long)s;
  return LX200_VALUEGET;
}

LX200RETURN LX200Client::setLocalTime(long& value)
{
  char cmd[LX200_SBUF];
  unsigned int h, m, s;
  long v = value;
  s = v % 60; v /= 60;
  m = v % 60; v /= 60;
  h = (unsigned int)v;
  sprintf(cmd, ":SL%02d:%02d:%02d#", h, m, s);
  return set(cmd);
}

LX200RETURN LX200Client::getUTCTime(unsigned int& hour, unsigned int& minute, unsigned int& second)
{
  char out[LX200_SBUF];
  LX200RETURN ret = get(":GXT0#", out, sizeof(out));
  if (ret != LX200_VALUEGET) return ret;
  char2RA(out, hour, minute, second);
  return LX200_VALUEGET;
}

LX200RETURN LX200Client::getUTCTime(long& totalSeconds)
{
  unsigned int h, m, s;
  LX200RETURN ret = getUTCTime(h, m, s);
  if (ret != LX200_VALUEGET) return ret;
  totalSeconds = (long)h * 3600L + (long)m * 60L + (long)s;
  return LX200_VALUEGET;
}

LX200RETURN LX200Client::setUTCTime(long& value)
{
  char cmd[LX200_SBUF];
  unsigned int h, m, s;
  long v = value;
  s = v % 60; v /= 60;
  m = v % 60; v /= 60;
  h = (unsigned int)v;
  sprintf(cmd, ":SXT0%02d:%02d:%02d#", h, m, s);
  return set(cmd);
}

LX200RETURN LX200Client::getLocalDate(unsigned int& day, unsigned int& month, unsigned int& year)
{
  char out[LX200_SBUF];
  if (get(":GC#", out, sizeof(out)) == LX200_VALUEGET)
  {
    char* pEnd;
    month = (unsigned int)strtol(&out[0], &pEnd, 10);
    day   = (unsigned int)strtol(&out[3], &pEnd, 10);
    year  = (unsigned int)strtol(&out[6], &pEnd, 10) + 2000L;
    return LX200_VALUEGET;
  }
  return LX200_GETVALUEFAILED;
}

LX200RETURN LX200Client::getUTCDate(unsigned int& day, unsigned int& month, unsigned int& year)
{
  char out[LX200_SBUF];
  if (get(":GXT1#", out, sizeof(out)) == LX200_VALUEGET)
  {
    char* pEnd;
    month = (unsigned int)strtol(&out[0], &pEnd, 10);
    day   = (unsigned int)strtol(&out[3], &pEnd, 10);
    year  = (unsigned int)strtol(&out[6], &pEnd, 10) + 2000L;
    return LX200_VALUEGET;
  }
  return LX200_GETVALUEFAILED;
}

// ===========================================================================
//  Location / Observatory
// ===========================================================================

LX200RETURN LX200Client::getLatitude(double& degree)
{
  char out[LX200_SBUF];
  LX200RETURN ret = get(":Gtf#", out, sizeof(out));
  if (ret != LX200_VALUEGET) return ret;
  if (dmsToDouble(&degree, out, true, true))
    return LX200_VALUEGET;
  return LX200_GETVALUEFAILED;
}

LX200RETURN LX200Client::getLongitude(double& degree)
{
  char out[LX200_SBUF];
  LX200RETURN ret = get(":Ggf#", out, sizeof(out));
  if (ret != LX200_VALUEGET) return ret;

  double longi = 0;
  if ((out[0] == '-') || (out[0] == '+'))
  {
    if (dmsToDouble(&longi, (char*)&out[1], false, true))
    {
      degree = out[0] == '-' ? -longi : longi;
      return LX200_VALUEGET;
    }
  }
  return LX200_GETVALUEFAILED;
}

LX200RETURN LX200Client::getLstT0(double& T0)
{
  char out[LX200_SBUF];
  LX200RETURN ret = get(":GS#", out, sizeof(out));
  if (ret != LX200_VALUEGET) return ret;
  if (hmsToDouble(&T0, out))
    return LX200_VALUEGET;
  return LX200_GETVALUEFAILED;
}

LX200RETURN LX200Client::getTrackingRate(double& rate)
{
  char out[LX200_SBUF];
  if (get(":GT#", out, sizeof(out)) == LX200_VALUEGET)
  {
    rate = atof(out);
    return LX200_VALUEGET;
  }
  return LX200_GETVALUEFAILED;
}

// ===========================================================================
//  Site / Mount
// ===========================================================================

LX200RETURN LX200Client::getSite(int& value)
{
  char out[LX200_SBUF];
  LX200RETURN ret = get(":W?#", out, sizeof(out));
  if (ret != LX200_VALUEGET) return ret;
  value = (int)strtol(&out[0], NULL, 10);
  return LX200_VALUEGET;
}

LX200RETURN LX200Client::setSite(int& value)
{
  char cmd[LX200_SBUF];
  sprintf(cmd, ":W%d#", value);
  return set(cmd);
}

LX200RETURN LX200Client::getSiteName(int idx, char* name, int len)
{
  char cmd[10] = ":GM#";
  cmd[2] += idx;
  return get(cmd, name, len);
}

LX200RETURN LX200Client::getMountIdx(int& value)
{
  char out[LX200_SBUF];
  LX200RETURN ret = get(":GXOI#", out, sizeof(out));
  if (ret != LX200_VALUEGET) return ret;
  value = (int)strtol(&out[0], NULL, 10);
  return LX200_VALUEGET;
}

LX200RETURN LX200Client::getMountName(int idx, char* name, int len)
{
  char cmd[10] = ":GXOB#";
  cmd[4] += idx;
  return get(cmd, name, len);
}

LX200RETURN LX200Client::setMount(int& value)
{
  char cmd[LX200_SBUF];
  sprintf(cmd, ":SXOI,%d#", value);
  return set(cmd);
}

LX200RETURN LX200Client::setMountName(int idx, char* name)
{
  char cmd[LX200_SBUF];
  sprintf(cmd, ":SXOB,%s#", name);
  cmd[4] += idx;
  return set(cmd);
}

// ===========================================================================
//  Target setting
// ===========================================================================

LX200RETURN LX200Client::setTargetRA(uint8_t& vr1, uint8_t& vr2, uint8_t& vr3)
{
  char cmd[LX200_SBUF];
  sprintf(cmd, ":Sr%02u:%02u:%02u#", vr1, vr2, vr3);
  return set(cmd);
}

LX200RETURN LX200Client::setTargetDec(bool& ispos, uint16_t& vd1, uint8_t& vd2, uint8_t& vd3)
{
  char cmd[LX200_SBUF];
  sprintf(cmd, ":Sd+%02u:%02u:%02u#", vd1, vd2, vd3);
  if (!ispos) cmd[3] = '-';
  return set(cmd);
}

LX200RETURN LX200Client::setTargetAz(uint16_t& v1, uint8_t& v2, uint8_t& v3)
{
  char cmd[LX200_SBUF];
  sprintf(cmd, ":Sz%03u*%02u:%02u#", v1, v2, v3);
  return set(cmd);
}

LX200RETURN LX200Client::setTargetAlt(bool& ispos, uint16_t& vd1, uint8_t& vd2, uint8_t& vd3)
{
  char cmd[LX200_SBUF];
  sprintf(cmd, ":Sa+%02u*%02u'%02u#", vd1, vd2, vd3);
  if (!ispos) cmd[3] = '-';
  return set(cmd);
}

// ===========================================================================
//  Navigation
// ===========================================================================

LX200RETURN LX200Client::moveToTarget(TARGETTYPE target)
{
  char out[LX200_SBUF];
  const char* cmd;
  switch (target)
  {
  case T_RADEC:     cmd = ":MS#"; break;
  case T_AZALT:     cmd = ":MA#"; break;
  case T_USERRADEC: cmd = ":MU#"; break;
  default:          return LX200_ERRGOTO_UNKOWN;
  }
  sendReceive(cmd, CMDR_SHORT, out, sizeof(out), m_timeout);
  ErrorsGoTo err = static_cast<ErrorsGoTo>(out[0] - '0');
  return gotoErrorToReturn(err, true);
}

LX200RETURN LX200Client::pushToTarget(TARGETTYPE target)
{
  char out[LX200_SBUF];
  const char* cmd;
  switch (target)
  {
  case T_RADEC:     cmd = ":EMS#"; break;
  case T_AZALT:     cmd = ":EMA#"; break;
  case T_USERRADEC: cmd = ":EMU#"; break;
  default:          return LX200_ERRGOTO_UNKOWN;
  }
  sendReceive(cmd, CMDR_SHORT, out, sizeof(out), m_timeout);
  ErrorsGoTo err = static_cast<ErrorsGoTo>(out[0] - '0');
  return gotoErrorToReturn(err, false);
}

LX200RETURN LX200Client::syncGoHome(NAV mode)
{
  if (mode == NAV_PUSHTO) return LX200_SETVALUEFAILED;

  char cmd[5];
  sprintf(cmd, ":hX#");
  cmd[2] = navSelectChar(mode, 'F', 'C', 0);

  char out[LX200_SBUF];
  bool ok = sendReceive(cmd, CMDR_SHORT_BOOL, out, sizeof(out), m_timeout);
  if (!ok) return LX200_SETVALUEFAILED;

  if (mode == NAV_SYNC) return LX200_SYNCED;
  return out[0] == '0' ? LX200_GOHOME_FAILED : LX200_GOHOME;
}

LX200RETURN LX200Client::syncGoPark(NAV mode)
{
  if (mode == NAV_PUSHTO) return LX200_SETVALUEFAILED;

  char cmd[LX200_SBUF];
  sprintf(cmd, ":hX#");
  cmd[2] = navSelectChar(mode, 'O', 'P', 0);

  char out[LX200_SBUF];
  bool ok = sendReceive(cmd, CMDR_SHORT_BOOL, out, sizeof(out), m_timeout);
  if (!ok) return LX200_SETVALUEFAILED;

  if (mode == NAV_SYNC) return LX200_SYNCED;
  return out[0] == '0' ? LX200_GOPARK_FAILED : LX200_GOPARK;
}

LX200RETURN LX200Client::syncGoto(NAV mode, uint8_t& vr1, uint8_t& vr2, uint8_t& vr3,
                                   bool& ispos, uint16_t& vd1, uint8_t& vd2, uint8_t& vd3)
{
  if (setTargetRA(vr1, vr2, vr3) != LX200_VALUESET ||
      setTargetDec(ispos, vd1, vd2, vd3) != LX200_VALUESET)
    return LX200_SETTARGETFAILED;

  if (mode == NAV_SYNC)
  {
    char out[LX200_SBUF];
    if (get(":CM#", out, LX200_SBUF) == LX200_VALUEGET)
      return (strcmp(out, "N/A") == 0) ? LX200_SYNCED : LX200_SYNCFAILED;
    return LX200_SYNCFAILED;
  }
  if (mode == NAV_GOTO)  return moveToTarget(T_RADEC);
  if (mode == NAV_PUSHTO) return pushToTarget(T_RADEC);
  return LX200_SETVALUEFAILED;
}

LX200RETURN LX200Client::syncGoto(NAV mode, float& Ra, float& Dec)
{
  uint8_t vr1, vr2, vr3, vd2, vd3;
  uint16_t vd1;
  bool ispos = false;
  long Ras  = (long)(Ra  * 3600);
  long Decs = (long)(Dec * 3600);
  gethms(Ras, vr1, vr2, vr3);
  getdms(Decs, ispos, vd1, vd2, vd3);
  return syncGoto(mode, vr1, vr2, vr3, ispos, vd1, vd2, vd3);
}

LX200RETURN LX200Client::syncGotoAltAz(NAV mode, uint16_t& vz1, uint8_t& vz2, uint8_t& vz3,
                                        bool& ispos, uint16_t& va1, uint8_t& va2, uint8_t& va3)
{
  if (setTargetAz(vz1, vz2, vz3) != LX200_VALUESET ||
      setTargetAlt(ispos, va1, va2, va3) != LX200_VALUESET)
    return LX200_SETTARGETFAILED;

  if (mode == NAV_SYNC)
  {
    char out[LX200_SBUF];
    if (get(":CA#", out, LX200_SBUF) == LX200_VALUEGET)
      return (strcmp(out, "N/A") == 0) ? LX200_SYNCED : LX200_SYNCFAILED;
    return LX200_SYNCFAILED;
  }
  if (mode == NAV_GOTO)   return moveToTarget(T_AZALT);
  if (mode == NAV_PUSHTO) return pushToTarget(T_AZALT);
  return LX200_SETTARGETFAILED;
}

LX200RETURN LX200Client::syncGotoAltAz(NAV mode, float& Az, float& Alt)
{
  uint8_t  vz2, vz3, va2, va3;
  uint16_t vz1, va1;
  bool ispos = false;
  long Azs   = (long)(Az  * 3600);
  long Altcs = (long)(Alt * 3600);
  getdms(Azs,   ispos, vz1, vz2, vz3);
  getdms(Altcs, ispos, va1, va2, va3);
  return syncGotoAltAz(mode, vz1, vz2, vz3, ispos, va1, va2, va3);
}

LX200RETURN LX200Client::syncGotoUser(NAV mode)
{
  if (mode == NAV_SYNC)
  {
    char out[LX200_SBUF];
    if (get(":CU#", out, LX200_SBUF) == LX200_VALUEGET)
      return (strcmp(out, "N/A") == 0) ? LX200_SYNCED : LX200_SYNCFAILED;
    return LX200_SYNCFAILED;
  }
  if (mode == NAV_GOTO)   return moveToTarget(T_USERRADEC);
  if (mode == NAV_PUSHTO) return pushToTarget(T_USERRADEC);
  return LX200_SYNCFAILED;
}

// ===========================================================================
//  Tracking
// ===========================================================================

LX200RETURN LX200Client::enableTracking(bool on)       { return set(on ? ":Te#" : ":Td#"); }
LX200RETURN LX200Client::setTrackRateSidereal()         { return set(":TQ#"); }
LX200RETURN LX200Client::setTrackRateLunar()            { return set(":TL#"); }
LX200RETURN LX200Client::setTrackRateSolar()            { return set(":TS#"); }
LX200RETURN LX200Client::setTrackRateUser()             { return set(":TT#"); }
LX200RETURN LX200Client::incrementTrackRate()           { return set(":T+#"); }
LX200RETURN LX200Client::decrementTrackRate()           { return set(":T-#"); }
LX200RETURN LX200Client::resetTrackRate()               { return set(":TR#"); }

LX200RETURN LX200Client::setStepperMode(uint8_t mode)
{
  char cmd[5] = ":T0#";
  cmd[2] = '0' + mode;
  return set(cmd);
}

// ===========================================================================
//  Movement / Slew
// ===========================================================================

LX200RETURN LX200Client::startMoveNorth()  { return set(":Mn#"); }
LX200RETURN LX200Client::startMoveSouth()  { return set(":Ms#"); }
LX200RETURN LX200Client::startMoveEast()   { return set(":Me#"); }
LX200RETURN LX200Client::startMoveWest()   { return set(":Mw#"); }
LX200RETURN LX200Client::stopMoveNorth()   { return set(":Qn#"); }
LX200RETURN LX200Client::stopMoveSouth()   { return set(":Qs#"); }
LX200RETURN LX200Client::stopMoveEast()    { return set(":Qe#"); }
LX200RETURN LX200Client::stopMoveWest()    { return set(":Qw#"); }
LX200RETURN LX200Client::stopSlew()        { return set(":Q#"); }
LX200RETURN LX200Client::meridianFlip()    { return set(":MF#"); }

LX200RETURN LX200Client::setSpeed(uint8_t level)
{
  char cmd[5] = ":R0#";
  cmd[2] = '0' + level;
  return set(cmd);
}

// ===========================================================================
//  Home / Park
// ===========================================================================

LX200RETURN LX200Client::homeReset()        { return set(":hF#"); }
LX200RETURN LX200Client::homeGoto()         { return set(":hC#"); }
LX200RETURN LX200Client::park()             { return set(":hP#"); }
LX200RETURN LX200Client::unpark()           { return set(":hR#"); }
LX200RETURN LX200Client::setPark()          { return set(":hQ#"); }
LX200RETURN LX200Client::parkReset()        { return set(":hO#"); }
LX200RETURN LX200Client::setHomeCurrent()   { return set(":hB#"); }
LX200RETURN LX200Client::resetHomeCurrent() { return set(":hb#"); }

// ===========================================================================
//  Alignment
// ===========================================================================

LX200RETURN LX200Client::alignStart()       { return set(":A0#"); }
LX200RETURN LX200Client::alignAcceptStar()  { return set(":A*#"); }
LX200RETURN LX200Client::alignAtHome()      { return set(":AA#"); }
LX200RETURN LX200Client::alignSave()        { return set(":AW#"); }
LX200RETURN LX200Client::alignClear()       { return set(":AC#"); }
LX200RETURN LX200Client::getAlignError(char* out, int len) { return get(":AE#", out, len); }
LX200RETURN LX200Client::alignNextStar()    { return set(":A+#"); }
LX200RETURN LX200Client::setPierSideEast()  { return set(":SmE#"); }
LX200RETURN LX200Client::setPierSideWest()  { return set(":SmW#"); }
LX200RETURN LX200Client::setPierSideNone()  { return set(":SmN#"); }

LX200RETURN LX200Client::alignSelectStar(uint8_t n)
{
  char cmd[5] = ":A0#";
  cmd[2] = '0' + n;
  return set(cmd);
}

LX200RETURN LX200Client::getAlignErrorPolar(char* out, int len) { return get(":GXAw#", out, len); }
LX200RETURN LX200Client::getAlignErrorAz(char* out, int len)    { return get(":GXAz#", out, len); }
LX200RETURN LX200Client::getAlignErrorAlt(char* out, int len)   { return get(":GXAa#", out, len); }

// ===========================================================================
//  Focuser actions
// ===========================================================================

LX200RETURN LX200Client::focuserSetZero()    { return set(":FP#"); }
LX200RETURN LX200Client::focuserStop()       { return set(":FQ#"); }
LX200RETURN LX200Client::focuserMoveOut()    { return set(":F+#"); }
LX200RETURN LX200Client::focuserMoveIn()     { return set(":F-#"); }
LX200RETURN LX200Client::getFocuserVersion(char* out, int len) { return get(":FV#", out, len); }
LX200RETURN LX200Client::focuserResetConfig() { return set(":F!#"); }
LX200RETURN LX200Client::focuserSaveConfig() { return set(":F$#"); }
LX200RETURN LX200Client::focuserGotoHome()   { return set(":FS,0#"); }

LX200RETURN LX200Client::focuserIsConnected(bool& connected)
{
  char out[LX200_SBUF];
  LX200RETURN ret = get(":F~#", out, sizeof(out));
  if (ret == LX200_VALUEGET)
    connected = (out[0] == '~');
  return ret;
}

// ===========================================================================
//  Rotator
// ===========================================================================

LX200RETURN LX200Client::rotatorCenter()          { return set(":rC#"); }
LX200RETURN LX200Client::rotatorReset()            { return set(":rF#"); }
LX200RETURN LX200Client::rotatorIncrDeRotate()     { return set(":r+#"); }
LX200RETURN LX200Client::rotatorDecrDeRotate()     { return set(":r-#"); }
LX200RETURN LX200Client::rotatorReverse()          { return set(":rR#"); }
LX200RETURN LX200Client::rotatorDeRotateToggle()   { return set(":rP#"); }

LX200RETURN LX200Client::rotatorMove(uint8_t speed, bool forward)
{
  char cmd1[5] = ":r0#";
  cmd1[2] = '0' + speed;
  LX200RETURN ret = set(cmd1);
  if (ret != LX200_VALUESET) return ret;
  return set(forward ? ":r>#" : ":r<#");
}

// ===========================================================================
//  Extended GX/SX — Rates & Acceleration
// ===========================================================================

LX200RETURN LX200Client::getAcceleration(float& val)
{
  char out[LX200_SBUF];
  LX200RETURN ret = get(":GXRA#", out, sizeof(out));
  if (ret == LX200_VALUEGET) val = (float)strtof(out, NULL);
  return ret;
}

LX200RETURN LX200Client::setAcceleration(float val)
{
  char cmd[LX200_SBUF];
  sprintf(cmd, ":SXRA,%d#", (int)val);
  return set(cmd);
}

LX200RETURN LX200Client::getMaxRate(int& val)
{
  char out[LX200_SBUF];
  LX200RETURN ret = get(":GXRX#", out, sizeof(out));
  if (ret == LX200_VALUEGET) val = (int)strtol(out, NULL, 10);
  return ret;
}

LX200RETURN LX200Client::setMaxRate(int val)
{
  char cmd[LX200_SBUF];
  sprintf(cmd, ":SXRX,%d#", val);
  return set(cmd);
}

LX200RETURN LX200Client::getDeadband(int& val)
{
  char out[LX200_SBUF];
  LX200RETURN ret = get(":GXRD#", out, sizeof(out));
  if (ret == LX200_VALUEGET) val = (int)strtol(out, NULL, 10);
  return ret;
}

LX200RETURN LX200Client::setDeadband(int val)
{
  char cmd[LX200_SBUF];
  sprintf(cmd, ":SXRD,%d#", val);
  return set(cmd);
}

LX200RETURN LX200Client::getSpeedRate(uint8_t idx, float& val)
{
  char cmd[LX200_SBUF];
  sprintf(cmd, ":GXR%c#", '0' + idx);
  char out[LX200_SBUF];
  LX200RETURN ret = get(cmd, out, sizeof(out));
  if (ret == LX200_VALUEGET) val = (float)strtof(out, NULL);
  return ret;
}

LX200RETURN LX200Client::setSpeedRate(uint8_t idx, float val)
{
  char cmd[LX200_SBUF];
  sprintf(cmd, ":SXR%c,%d#", '0' + idx, (int)val);
  return set(cmd);
}

// ===========================================================================
//  Extended GX/SX — Limits
// ===========================================================================

LX200RETURN LX200Client::getMinAltitude(int& val)
{
  char out[LX200_SBUF];
  LX200RETURN ret = get(":GXLH#", out, sizeof(out));
  if (ret == LX200_VALUEGET) val = (int)strtol(out, NULL, 10);
  return ret;
}

LX200RETURN LX200Client::setMinAltitude(int val)
{
  char cmd[LX200_SBUF];
  sprintf(cmd, ":SXLH,%d#", val);
  return set(cmd);
}

LX200RETURN LX200Client::getMaxAltitude(int& val)
{
  char out[LX200_SBUF];
  LX200RETURN ret = get(":GXLO#", out, sizeof(out));
  if (ret == LX200_VALUEGET) val = (int)strtol(out, NULL, 10);
  return ret;
}

LX200RETURN LX200Client::setMaxAltitude(int val)
{
  char cmd[LX200_SBUF];
  sprintf(cmd, ":SXLO,%d#", val);
  return set(cmd);
}

LX200RETURN LX200Client::getUnderPoleLimit(char* out, int len) { return get(":GXLU#", out, len); }

LX200RETURN LX200Client::setUnderPoleLimit(float val)
{
  char cmd[LX200_SBUF];
  sprintf(cmd, ":SXLU,%d#", (int)(val * 10));
  return set(cmd);
}

LX200RETURN LX200Client::getMinDistFromPole(int& val)
{
  char out[LX200_SBUF];
  LX200RETURN ret = get(":GXLS#", out, sizeof(out));
  if (ret == LX200_VALUEGET) val = (int)strtol(out, NULL, 10);
  return ret;
}

LX200RETURN LX200Client::setMinDistFromPole(int val)
{
  char cmd[LX200_SBUF];
  sprintf(cmd, ":SXLS,%d#", val);
  return set(cmd);
}

LX200RETURN LX200Client::getLimitEast(char* out, int len)  { return get(":GXLE#", out, len); }
LX200RETURN LX200Client::getLimitWest(char* out, int len)  { return get(":GXLW#", out, len); }

LX200RETURN LX200Client::getAxisLimit(char mode, char* out, int len)
{
  char cmd[10] = ":GXlX#";
  cmd[4] = mode;
  return get(cmd, out, len);
}

LX200RETURN LX200Client::setAxisLimit(char mode, float val)
{
  char cmd[LX200_SBUF];
  sprintf(cmd, ":SXLX,%d#", (int)(val * 10));
  cmd[4] = mode;
  return set(cmd);
}

// ===========================================================================
//  Extended GX/SX — Mount flags
// ===========================================================================

LX200RETURN LX200Client::getRefractionEnabled(char* out, int len)   { return get(":GXrt#", out, len); }
LX200RETURN LX200Client::enableRefraction(bool on)
{
  return set(on ? ":SXrt,y#" : ":SXrt,n#");
}

LX200RETURN LX200Client::getPolarAlignEnabled(char* out, int len)   { return get(":GXrp#", out, len); }
LX200RETURN LX200Client::enablePolarAlign(bool on)
{
  return set(on ? ":SXrp,y#" : ":SXrp,n#");
}

LX200RETURN LX200Client::getGoToEnabled(char* out, int len)         { return get(":GXrg#", out, len); }
LX200RETURN LX200Client::enableGoTo(bool on)
{
  return set(on ? ":SXrg,y#" : ":SXrg,n#");
}

LX200RETURN LX200Client::enableMotors(bool on)
{
  return set(on ? ":SXME,y#" : ":SXME,n#");
}

LX200RETURN LX200Client::enableEncoders(bool on)
{
  return set(on ? ":SXEE,y#" : ":SXEE,n#");
}

// ===========================================================================
//  Extended GX/SX — Other
// ===========================================================================

LX200RETURN LX200Client::getMountDescription(char* out, int len)    { return get(":GXOA#", out, len); }

LX200RETURN LX200Client::getStepsPerSecond(char* out, int len)      { return get(":GXOS#", out, len); }

LX200RETURN LX200Client::setStepsPerSecond(int val)
{
  char cmd[LX200_SBUF];
  sprintf(cmd, ":SXOS,%d#", val);
  return set(cmd);
}

// ===========================================================================
//  Miscellaneous
// ===========================================================================

LX200RETURN LX200Client::saveUserPosition()   { return set(":SU#"); }
LX200RETURN LX200Client::getDate(char* out, int len) { return get(":GC#", out, len); }
LX200RETURN LX200Client::getElevation(char* out, int len) { return get(":Ge#", out, len); }
LX200RETURN LX200Client::syncTime()           { return set(":gs#"); }
LX200RETURN LX200Client::syncLocation()       { return set(":gt#"); }
LX200RETURN LX200Client::reboot()             { return set(":$!#"); }
LX200RETURN LX200Client::factoryReset()       { return set(":$$#"); }
LX200RETURN LX200Client::encoderSync()        { return set(":ECS#"); }
LX200RETURN LX200Client::encoderStopMotion()  { return set(":EMQ#"); }

// ===========================================================================
//  Motor configuration
// ===========================================================================

LX200RETURN LX200Client::readReverse(const uint8_t& axis, bool& reverse)
{
  char out[LX200_SBUF];
  LX200RETURN ok = axis == 1 ? get(":GXMRR#", out, sizeof(out))
                              : get(":GXMRD#", out, sizeof(out));
  if (ok == LX200_VALUEGET)
    reverse = (out[0] == '1');
  return ok;
}

LX200RETURN LX200Client::writeReverse(const uint8_t& axis, const bool& reverse)
{
  char cmd[LX200_SBUF];
  sprintf(cmd, ":SXMRX,%u#", (unsigned int)reverse);
  cmd[5] = axisChar(axis);
  return set(cmd);
}

LX200RETURN LX200Client::readBacklash(const uint8_t& axis, float& backlash)
{
  char out[LX200_SBUF];
  LX200RETURN ok = axis == 1 ? get(":GXMBR#", out, sizeof(out))
                              : get(":GXMBD#", out, sizeof(out));
  if (ok == LX200_VALUEGET)
    backlash = (float)strtol(&out[0], NULL, 10);
  return ok;
}

LX200RETURN LX200Client::writeBacklash(const uint8_t& axis, const float& backlash)
{
  char cmd[LX200_SBUF];
  sprintf(cmd, ":SXMBX,%u#", (unsigned int)backlash);
  cmd[5] = axisChar(axis);
  return set(cmd);
}

LX200RETURN LX200Client::readBacklashRate(const uint8_t& axis, float& rate)
{
  char out[LX200_SBUF];
  LX200RETURN ok = axis == 1 ? get(":GXMbR#", out, sizeof(out))
                              : get(":GXMbD#", out, sizeof(out));
  if (ok == LX200_VALUEGET)
    rate = (float)strtol(&out[0], NULL, 10);
  return ok;
}

LX200RETURN LX200Client::writeBacklashRate(const uint8_t& axis, const float& rate)
{
  char cmd[LX200_SBUF];
  sprintf(cmd, ":SXMbX,%u#", (unsigned int)rate);
  cmd[5] = axisChar(axis);
  return set(cmd);
}

LX200RETURN LX200Client::readTotGear(const uint8_t& axis, float& gear)
{
  char out[LX200_SBUF];
  LX200RETURN ok = axis == 1 ? get(":GXMGR#", out, sizeof(out))
                              : get(":GXMGD#", out, sizeof(out));
  if (ok == LX200_VALUEGET)
    gear = (float)((double)strtol(&out[0], NULL, 10) / 1000.0);
  return ok;
}

LX200RETURN LX200Client::writeTotGear(const uint8_t& axis, const float& gear)
{
  char cmd[LX200_SBUF];
  sprintf(cmd, ":SXMGX,%lu#", (unsigned long)((double)gear * 1000));
  cmd[5] = axisChar(axis);
  return set(cmd);
}

LX200RETURN LX200Client::readStepPerRot(const uint8_t& axis, float& steps)
{
  char out[LX200_SBUF];
  LX200RETURN ok = axis == 1 ? get(":GXMSR#", out, sizeof(out))
                              : get(":GXMSD#", out, sizeof(out));
  if (ok == LX200_VALUEGET)
    steps = (float)strtol(&out[0], NULL, 10);
  return ok;
}

LX200RETURN LX200Client::writeStepPerRot(const uint8_t& axis, const float& steps)
{
  char cmd[LX200_SBUF];
  sprintf(cmd, ":SXMSX,%u#", (unsigned int)steps);
  cmd[5] = axisChar(axis);
  return set(cmd);
}

LX200RETURN LX200Client::readMicro(const uint8_t& axis, uint8_t& micro)
{
  char out[LX200_SBUF];
  LX200RETURN ok = axis == 1 ? get(":GXMMR#", out, sizeof(out))
                              : get(":GXMMD#", out, sizeof(out));
  if (ok == LX200_VALUEGET)
  {
    long value = strtol(&out[0], NULL, 10);
    if (value >= 0 && value < 9) { micro = (uint8_t)value; return ok; }
    return LX200_GETVALUEFAILED;
  }
  return ok;
}

LX200RETURN LX200Client::writeMicro(const uint8_t& axis, const uint8_t& micro)
{
  char cmd[LX200_SBUF];
  sprintf(cmd, ":SXMMX,%u#", micro);
  cmd[5] = axisChar(axis);
  return set(cmd);
}

LX200RETURN LX200Client::readSilentStep(const uint8_t& axis, uint8_t& silent)
{
  char out[LX200_SBUF];
  LX200RETURN ok = axis == 1 ? get(":GXMmR#", out, sizeof(out))
                              : get(":GXMmD#", out, sizeof(out));
  if (ok == LX200_VALUEGET)
  {
    long value = strtol(&out[0], NULL, 10);
    if (value >= 0 && value < 2) { silent = (uint8_t)value; return ok; }
    return LX200_GETVALUEFAILED;
  }
  return ok;
}

LX200RETURN LX200Client::writeSilentStep(const uint8_t& axis, const uint8_t& silent)
{
  char cmd[LX200_SBUF];
  sprintf(cmd, ":SXMmX,%u#", silent);
  cmd[5] = axisChar(axis);
  return set(cmd);
}

LX200RETURN LX200Client::readLowCurr(const uint8_t& axis, unsigned int& curr)
{
  char out[LX200_SBUF];
  LX200RETURN ok = axis == 1 ? get(":GXMcR#", out, sizeof(out))
                              : get(":GXMcD#", out, sizeof(out));
  if (ok == LX200_VALUEGET)
  {
    long value = strtol(&out[0], NULL, 10);
    if (value >= 0 && value <= 2800) { curr = (unsigned int)value; return ok; }
    return LX200_GETVALUEFAILED;
  }
  return ok;
}

LX200RETURN LX200Client::writeLowCurr(const uint8_t& axis, const unsigned int& curr)
{
  char cmd[LX200_SBUF];
  sprintf(cmd, ":SXMcX,%u#", curr);
  cmd[5] = axisChar(axis);
  return set(cmd);
}

LX200RETURN LX200Client::readHighCurr(const uint8_t& axis, unsigned int& curr)
{
  char out[LX200_SBUF];
  LX200RETURN ok = axis == 1 ? get(":GXMCR#", out, sizeof(out))
                              : get(":GXMCD#", out, sizeof(out));
  if (ok == LX200_VALUEGET)
  {
    long value = strtol(&out[0], NULL, 10);
    if (value >= 100 && value <= 2800) { curr = (unsigned int)value; return ok; }
    return LX200_GETVALUEFAILED;
  }
  return ok;
}

LX200RETURN LX200Client::writeHighCurr(const uint8_t& axis, const unsigned int& curr)
{
  char cmd[LX200_SBUF];
  sprintf(cmd, ":SXMCX,%u#", curr);
  cmd[5] = axisChar(axis);
  return set(cmd);
}

// ===========================================================================
//  Encoder configuration
// ===========================================================================

LX200RETURN LX200Client::readEncoderReverse(const uint8_t& axis, bool& reverse)
{
  char out[LX200_SBUF];
  LX200RETURN ok = axis == 1 ? get(":GXErR#", out, sizeof(out))
                              : get(":GXErD#", out, sizeof(out));
  if (ok == LX200_VALUEGET)
    reverse = (out[0] == '1');
  return ok;
}

LX200RETURN LX200Client::writeEncoderReverse(const uint8_t& axis, const bool& reverse)
{
  char cmd[LX200_SBUF];
  sprintf(cmd, ":SXErX,%u#", (unsigned int)reverse);
  cmd[5] = axisChar(axis);
  return set(cmd);
}

LX200RETURN LX200Client::readPulsePerDegree(const uint8_t& axis, float& ppd)
{
  char out[LX200_SBUF];
  LX200RETURN ok = axis == 1 ? get(":GXEPR#", out, sizeof(out))
                              : get(":GXEPD#", out, sizeof(out));
  if (ok == LX200_VALUEGET)
    ppd = (float)strtol(&out[0], NULL, 10);
  return ok;
}

LX200RETURN LX200Client::writePulsePerDegree(const uint8_t& axis, const float& ppd)
{
  char cmd[LX200_SBUF];
  sprintf(cmd, ":SXEPX,%u#", (unsigned long)(ppd));
  cmd[5] = axisChar(axis);
  return set(cmd);
}

LX200RETURN LX200Client::startEncoderCalibration()
{
  return set(":EAS#");
}

LX200RETURN LX200Client::cancelEncoderCalibration()
{
  return set(":EAQ#");
}

LX200RETURN LX200Client::completeEncoderCalibration()
{
  return set(":EAE#");
}

LX200RETURN LX200Client::readEncoderAutoSync(uint8_t& syncmode)
{
  char out[LX200_SBUF];
  LX200RETURN ok = get(":GXEO#", out, sizeof(out));
  if (ok == LX200_VALUEGET)
  {
    syncmode = out[0] - '0';
    if (syncmode < 8) return ok;
  }
  return LX200_GETVALUEFAILED;
}

LX200RETURN LX200Client::writeEncoderAutoSync(const uint8_t syncmode)
{
  char cmd[LX200_SBUF];
  sprintf(cmd, ":SXEO,%d#", (int)syncmode);
  return set(cmd);
}

// ===========================================================================
//  Focuser
// ===========================================================================

LX200RETURN LX200Client::readFocuserConfig(unsigned int& startPosition, unsigned int& maxPosition,
                                            unsigned int& minSpeed, unsigned int& maxSpeed,
                                            unsigned int& cmdAcc, unsigned int& manAcc,
                                            unsigned int& manDec)
{
  char out[LX200_LBUF];
  // retry up to 3 times
  if (get(":F~#", out, sizeof(out)) == LX200_GETVALUEFAILED)
    if (get(":F~#", out, sizeof(out)) == LX200_GETVALUEFAILED)
      if (get(":F~#", out, sizeof(out)) == LX200_GETVALUEFAILED)
        return LX200_GETVALUEFAILED;
  if (strlen(out) != 32)
    return LX200_GETVALUEFAILED;
  char* pEnd;
  startPosition = (unsigned int)strtol(&out[1], &pEnd, 10);
  maxPosition   = (unsigned int)strtol(&out[7], &pEnd, 10);
  minSpeed      = (unsigned int)strtol(&out[13], &pEnd, 10);
  maxSpeed      = (unsigned int)strtol(&out[17], &pEnd, 10);
  cmdAcc        = (unsigned int)strtol(&out[21], &pEnd, 10);
  manAcc        = (unsigned int)strtol(&out[25], &pEnd, 10);
  manDec        = (unsigned int)strtol(&out[29], &pEnd, 10);
  return LX200_VALUEGET;
}

LX200RETURN LX200Client::readFocuserMotor(bool& reverse, unsigned int& micro,
                                           unsigned int& incr, unsigned int& curr,
                                           unsigned int& steprot)
{
  char out[LX200_LBUF];
  if (get(":FM#", out, sizeof(out)) == LX200_GETVALUEFAILED)
    if (get(":FM#", out, sizeof(out)) == LX200_GETVALUEFAILED)
      if (get(":FM#", out, sizeof(out)) == LX200_GETVALUEFAILED)
        return LX200_GETVALUEFAILED;
  if (strlen(out) != 16)
    return LX200_GETVALUEFAILED;
  char* pEnd;
  reverse = (out[1] == '1');
  micro   = out[3] - '0';
  incr    = (unsigned int)strtol(&out[5], &pEnd, 10);
  curr    = (unsigned int)strtol(&out[9], &pEnd, 10);
  steprot = (unsigned int)strtol(&out[13], &pEnd, 10);
  return LX200_VALUEGET;
}

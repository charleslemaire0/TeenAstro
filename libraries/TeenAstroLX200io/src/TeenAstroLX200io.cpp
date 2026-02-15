/*
 * TeenAstroLX200io.cpp - Backward-compatible free-function wrappers
 *
 * All functions delegate to the global LX200Client instance (bound to the
 * platform serial port "Ser").  Higher-level convenience functions that
 * depend on Ephemeris / TeenAstroCatalog are implemented directly here.
 *
 * Copyright (C) 2024 TeenAstro by Charles Lemaire
 * GNU General Public License v3
 */
#include <Arduino.h>
#include <Ephemeris.h>
#include <TeenAstroLX200io.h>
#include <TeenAstroFunction.h>

// ===========================================================================
//  Global LX200Client
// ===========================================================================

LX200Client& lx200client()
{
  static LX200Client instance(Ser);
  return instance;
}

// ===========================================================================
//  Core I/O wrappers
// ===========================================================================

bool readLX200Bytes(char* command, CMDREPLY& cmdreply, char* recvBuffer,
                    int bufferSize, unsigned long timeOutMs, bool keepHashtag)
{
  return lx200client().sendReceiveAuto(command, cmdreply, recvBuffer,
                                       bufferSize, timeOutMs, keepHashtag);
}

LX200RETURN GetLX200(char* command, char* output, int buffersize)
{
  return lx200client().get(command, output, buffersize);
}

LX200RETURN GetLX200Short(char* command, short* value)
{
  return lx200client().getShort(command, value);
}

LX200RETURN GetLX200Float(char* command, float* value)
{
  return lx200client().getFloat(command, value);
}

LX200RETURN SetLX200(char* command)
{
  return lx200client().set(command);
}

LX200RETURN SetBoolLX200(char* command)
{
  // SetBool is functionally identical to Set for the wire protocol
  return lx200client().set(command);
}

// ===========================================================================
//  Time / Date
// ===========================================================================

LX200RETURN GetLocalTimeLX200(unsigned int& hour, unsigned int& minute, unsigned int& second)
{
  return lx200client().getLocalTime(hour, minute, second);
}

LX200RETURN GetLocalTimeLX200(long& value)
{
  return lx200client().getLocalTime(value);
}

LX200RETURN SetLocalTimeLX200(long& value)
{
  return lx200client().setLocalTime(value);
}

LX200RETURN GetUTCTimeLX200(unsigned int& hour, unsigned int& minute, unsigned int& second)
{
  return lx200client().getUTCTime(hour, minute, second);
}

LX200RETURN GetUTCTimeLX200(long& value)
{
  return lx200client().getUTCTime(value);
}

LX200RETURN SetUTCTimeLX200(long& value)
{
  return lx200client().setUTCTime(value);
}

LX200RETURN GetLocalDateLX200(unsigned int& day, unsigned int& month, unsigned int& year)
{
  return lx200client().getLocalDate(day, month, year);
}

LX200RETURN GetUTCDateLX200(unsigned int& day, unsigned int& month, unsigned int& year)
{
  return lx200client().getUTCDate(day, month, year);
}

// ===========================================================================
//  Location
// ===========================================================================

LX200RETURN GetLstT0LX200(double& T0)
{
  return lx200client().getLstT0(T0);
}

LX200RETURN GetLatitudeLX200(double& degree)
{
  return lx200client().getLatitude(degree);
}

LX200RETURN GetLongitudeLX200(double& degree)
{
  return lx200client().getLongitude(degree);
}

LX200RETURN GetTrackingRateLX200(double& rate)
{
  return lx200client().getTrackingRate(rate);
}

// ===========================================================================
//  Site / Mount
// ===========================================================================

LX200RETURN GetSiteLX200(int& value)           { return lx200client().getSite(value); }
LX200RETURN SetSiteLX200(int& value)           { return lx200client().setSite(value); }
LX200RETURN GetSiteNameLX200(int idx, char* name, int len) { return lx200client().getSiteName(idx, name, len); }
LX200RETURN GetMountIdxLX200(int& value)       { return lx200client().getMountIdx(value); }
LX200RETURN GetMountNameLX200(int idx, char* name, int len) { return lx200client().getMountName(idx, name, len); }
LX200RETURN SetMountLX200(int& value)          { return lx200client().setMount(value); }
LX200RETURN SetMountNameLX200(int idx, char* name) { return lx200client().setMountName(idx, name); }

// ===========================================================================
//  Target
// ===========================================================================

LX200RETURN SetTargetRaLX200(uint8_t& vr1, uint8_t& vr2, uint8_t& vr3)
{
  return lx200client().setTargetRA(vr1, vr2, vr3);
}

LX200RETURN SetTargetDecLX200(bool& ispos, uint16_t& vd1, uint8_t& vd2, uint8_t& vd3)
{
  return lx200client().setTargetDec(ispos, vd1, vd2, vd3);
}

LX200RETURN SetTargetAzLX200(uint16_t& v1, uint8_t& v2, uint8_t& v3)
{
  return lx200client().setTargetAz(v1, v2, v3);
}

LX200RETURN SetTargetAltLX200(bool& ispos, uint16_t& vd1, uint8_t& vd2, uint8_t& vd3)
{
  return lx200client().setTargetAlt(ispos, vd1, vd2, vd3);
}

// ===========================================================================
//  Navigation wrappers
// ===========================================================================

LX200RETURN Move2TargetLX200(TARGETTYPE target)  { return lx200client().moveToTarget(target); }
LX200RETURN Push2TargetLX200(TARGETTYPE target)  { return lx200client().pushToTarget(target); }
LX200RETURN SyncGoHomeLX200(NAV mode)             { return lx200client().syncGoHome(mode); }
LX200RETURN SyncGoParkLX200(NAV mode)             { return lx200client().syncGoPark(mode); }

LX200RETURN SyncGotoLX200(NAV mode, uint8_t& vr1, uint8_t& vr2, uint8_t& vr3,
                           bool& ispos, uint16_t& vd1, uint8_t& vd2, uint8_t& vd3)
{
  return lx200client().syncGoto(mode, vr1, vr2, vr3, ispos, vd1, vd2, vd3);
}

LX200RETURN SyncGotoLX200(NAV mode, float& Ra, float& Dec)
{
  return lx200client().syncGoto(mode, Ra, Dec);
}

LX200RETURN SyncGotoLX200AltAz(NAV mode, float& Az, float& Alt)
{
  return lx200client().syncGotoAltAz(mode, Az, Alt);
}

LX200RETURN SyncGotoUserLX200(NAV mode)
{
  return lx200client().syncGotoUser(mode);
}

// ===========================================================================
//  Higher-level convenience functions (depend on Ephemeris / Catalog)
// ===========================================================================

LX200RETURN SyncGotoLX200(LX200Client& client, NAV mode, float& Ra, float& Dec, double epoch)
{
  unsigned int day, month, year;
  if (client.getUTCDate(day, month, year) != LX200_VALUEGET)
    return LX200_GETVALUEFAILED;
  EquatorialCoordinates coo;
  coo.ra = Ra;
  coo.dec = Dec;
  EquatorialCoordinates cooNow;
  cooNow = Ephemeris::equatorialEquinoxToEquatorialJNowAtDateAndTime(
    coo, epoch, day, month, year, 0, 0, 0);
  return client.syncGoto(mode, cooNow.ra, cooNow.dec);
}

LX200RETURN SyncGotoCatLX200(LX200Client& client, NAV mode)
{
  int epoch;
  unsigned int day, month, year;
  if (client.getUTCDate(day, month, year) == LX200_GETVALUEFAILED)
    return LX200_GETVALUEFAILED;
  if (!cat_mgr.isStarCatalog() && !cat_mgr.isDsoCatalog())
    return LX200_ERRGOTO_UNKOWN;
  EquatorialCoordinates coo;
  coo.ra = cat_mgr.rah();
  coo.dec = cat_mgr.dec();
  epoch = cat_mgr.epoch();
  if (epoch == 0) return LX200_GETVALUEFAILED;
  EquatorialCoordinates cooNow;
  cooNow = Ephemeris::equatorialEquinoxToEquatorialJNowAtDateAndTime(
    coo, epoch, day, month, year, 0, 0, 0);
  return client.syncGoto(mode, cooNow.ra, cooNow.dec);
}

LX200RETURN SyncGotoPlanetLX200(LX200Client& client, NAV mode, unsigned short objSys)
{
  unsigned int day, month, year, hour, minute, second;
  double degreeLat, degreeLong;

  if (client.getUTCDate(day, month, year) == LX200_GETVALUEFAILED)
    return LX200_GETVALUEFAILED;
  if (client.getUTCTime(hour, minute, second) == LX200_GETVALUEFAILED)
    return LX200_GETVALUEFAILED;
  if (client.getLongitude(degreeLong) == LX200_GETVALUEFAILED)
    return LX200_GETVALUEFAILED;
  if (client.getLatitude(degreeLat) == LX200_GETVALUEFAILED)
    return LX200_GETVALUEFAILED;

  Ephemeris Eph;
  Eph.flipLongitude(true);
  Eph.setLocationOnEarth((float)degreeLat, 0, 0, (float)degreeLong, 0, 0);
  SolarSystemObjectIndex objI = static_cast<SolarSystemObjectIndex>(objSys);
  SolarSystemObject obj = Eph.solarSystemObjectAtDateAndTime(
    objI, day, month, year, hour, minute, second);
  return client.syncGoto(mode, obj.equaCoordinates.ra, obj.equaCoordinates.dec);
}

LX200RETURN SyncSelectedStarLX200(LX200Client& client, unsigned short alignSelectedStar)
{
  if (alignSelectedStar >= 0)
    return SyncGotoCatLX200(client, NAV_GOTO);
  return LX200_ERRGOTO_UNKOWN;
}

// ===========================================================================
//  Motor wrappers
// ===========================================================================

LX200RETURN readReverseLX200(const uint8_t& axis, bool& reverse)            { return lx200client().readReverse(axis, reverse); }
LX200RETURN writeReverseLX200(const uint8_t& axis, const bool& reverse)     { return lx200client().writeReverse(axis, reverse); }
LX200RETURN readBacklashLX200(const uint8_t& axis, float& backlash)         { return lx200client().readBacklash(axis, backlash); }
LX200RETURN writeBacklashLX200(const uint8_t& axis, const float& backlash)  { return lx200client().writeBacklash(axis, backlash); }
LX200RETURN readBacklashRateLX200(const uint8_t& axis, float& rate)         { return lx200client().readBacklashRate(axis, rate); }
LX200RETURN writeBacklashRateLX200(const uint8_t& axis, const float& rate)  { return lx200client().writeBacklashRate(axis, rate); }
LX200RETURN readTotGearLX200(const uint8_t& axis, float& totGear)           { return lx200client().readTotGear(axis, totGear); }
LX200RETURN writeTotGearLX200(const uint8_t& axis, const float& totGear)    { return lx200client().writeTotGear(axis, totGear); }
LX200RETURN readStepPerRotLX200(const uint8_t& axis, float& stepPerRot)     { return lx200client().readStepPerRot(axis, stepPerRot); }
LX200RETURN writeStepPerRotLX200(const uint8_t& axis, const float& stepPerRot) { return lx200client().writeStepPerRot(axis, stepPerRot); }
LX200RETURN readMicroLX200(const uint8_t& axis, uint8_t& microStep)         { return lx200client().readMicro(axis, microStep); }
LX200RETURN writeMicroLX200(const uint8_t& axis, const uint8_t& microStep)  { return lx200client().writeMicro(axis, microStep); }
LX200RETURN readSilentStepLX200(const uint8_t& axis, uint8_t& silent)       { return lx200client().readSilentStep(axis, silent); }
LX200RETURN writeSilentStepLX200(const uint8_t& axis, const uint8_t& silent) { return lx200client().writeSilentStep(axis, silent); }
LX200RETURN readLowCurrLX200(const uint8_t& axis, unsigned int& lowCurr)    { return lx200client().readLowCurr(axis, lowCurr); }
LX200RETURN writeLowCurrLX200(const uint8_t& axis, const unsigned int& lowCurr) { return lx200client().writeLowCurr(axis, lowCurr); }
LX200RETURN readHighCurrLX200(const uint8_t& axis, unsigned int& highCurr)  { return lx200client().readHighCurr(axis, highCurr); }
LX200RETURN writeHighCurrLX200(const uint8_t& axis, const unsigned int& highCurr) { return lx200client().writeHighCurr(axis, highCurr); }

// ===========================================================================
//  Encoder wrappers
// ===========================================================================

LX200RETURN readEncoderReverseLX200(const uint8_t& axis, bool& reverse)     { return lx200client().readEncoderReverse(axis, reverse); }
LX200RETURN writeEncoderReverseLX200(const uint8_t& axis, const bool& reverse) { return lx200client().writeEncoderReverse(axis, reverse); }
LX200RETURN readPulsePerDegreeLX200(const uint8_t& axis, float& ppd)        { return lx200client().readPulsePerDegree(axis, ppd); }
LX200RETURN writePulsePerDegreeLX200(const uint8_t& axis, const float& ppd) { return lx200client().writePulsePerDegree(axis, ppd); }
LX200RETURN StartEncoderCalibration()  { return lx200client().startEncoderCalibration(); }
LX200RETURN CancelEncoderCalibration() { return lx200client().cancelEncoderCalibration(); }
LX200RETURN CompleteEncoderCalibration() { return lx200client().completeEncoderCalibration(); }
LX200RETURN readEncoderAutoSync(uint8_t& syncmode) { return lx200client().readEncoderAutoSync(syncmode); }
LX200RETURN writeEncoderAutoSync(const uint8_t syncmode) { return lx200client().writeEncoderAutoSync(syncmode); }

// ===========================================================================
//  Focuser wrappers
// ===========================================================================

LX200RETURN readFocuserConfig(unsigned int& startPosition, unsigned int& maxPosition,
                              unsigned int& minSpeed, unsigned int& maxSpeed,
                              unsigned int& cmdAcc, unsigned int& manAcc, unsigned int& manDec)
{
  return lx200client().readFocuserConfig(startPosition, maxPosition,
                                          minSpeed, maxSpeed, cmdAcc, manAcc, manDec);
}

LX200RETURN readFocuserMotor(bool& reverse, unsigned int& micro, unsigned int& incr,
                             unsigned int& curr, unsigned int& steprot)
{
  return lx200client().readFocuserMotor(reverse, micro, incr, curr, steprot);
}

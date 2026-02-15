/*
 * LX200Client.h - Object-oriented LX200 serial client for TeenAstro
 *
 * Encapsulates all serial communication with the TeenAstro main unit using
 * the LX200 protocol.  Replaces the former procedural free-function API
 * with a class that owns its Stream reference and provides typed,
 * domain-organised methods.
 *
 * Usage:
 *   LX200Client client(Serial1);
 *   double ra;
 *   if (isOk(client.getLatitude(ra))) { ... }
 *
 * Copyright (C) 2024 TeenAstro by Charles Lemaire
 * GNU General Public License v3
 */
#pragma once

#include <Arduino.h>
#include <TeenAstroCommandDef.h>

#define LX200_SBUF 20
#define LX200_LBUF 50
#define LX200_DEFAULT_TIMEOUT 30   // ms

class LX200Client
{
public:
  // -----------------------------------------------------------------------
  //  Construction
  // -----------------------------------------------------------------------

  /// Construct a client attached to the given serial stream.
  explicit LX200Client(Stream& serial, unsigned long timeoutMs = LX200_DEFAULT_TIMEOUT);

  /// Change the default command timeout (milliseconds).
  void setTimeout(unsigned long ms) { m_timeout = ms; }

  /// Access the underlying stream (e.g. for direct use or diagnostics).
  Stream& stream() { return m_serial; }

  // -----------------------------------------------------------------------
  //  Core I/O  (public for advanced / generic use)
  // -----------------------------------------------------------------------

  /// Low-level send + receive.  The caller specifies the expected reply type.
  /// Returns true on success; recvBuffer holds the response.
  bool sendReceive(const char* command, CMDREPLY replyType,
                   char* recvBuffer, int bufferSize,
                   unsigned long timeOutMs, bool keepHashtag = false);

  /// Low-level send + receive with automatic reply type lookup.
  /// Populates cmdreply with the determined type.  Returns true on success.
  bool sendReceiveAuto(char* command, CMDREPLY& cmdreply,
                       char* recvBuffer, int bufferSize,
                       unsigned long timeOutMs, bool keepHashtag = false);

  /// Get a string response.
  LX200RETURN get(const char* command, char* output, int bufferSize);

  /// Get a short integer value.
  LX200RETURN getShort(const char* command, short* value);

  /// Get a float value (range-checked to [-12, 12]).
  LX200RETURN getFloat(const char* command, float* value);

  /// Send a set command (expects SHORT_BOOL '1'/'0' or NO reply).
  LX200RETURN set(const char* command);

  // -----------------------------------------------------------------------
  //  Time / Date
  // -----------------------------------------------------------------------
  LX200RETURN getLocalTime(unsigned int& hour, unsigned int& minute, unsigned int& second);
  LX200RETURN getLocalTime(long& totalSeconds);
  LX200RETURN setLocalTime(long& value);

  LX200RETURN getUTCTime(unsigned int& hour, unsigned int& minute, unsigned int& second);
  LX200RETURN getUTCTime(long& totalSeconds);
  LX200RETURN setUTCTime(long& value);

  LX200RETURN getLocalDate(unsigned int& day, unsigned int& month, unsigned int& year);
  LX200RETURN getUTCDate(unsigned int& day, unsigned int& month, unsigned int& year);

  // -----------------------------------------------------------------------
  //  Location / Observatory
  // -----------------------------------------------------------------------
  LX200RETURN getLatitude(double& degree);
  LX200RETURN getLongitude(double& degree);
  LX200RETURN getLstT0(double& T0);
  LX200RETURN getTrackingRate(double& rate);

  // -----------------------------------------------------------------------
  //  Site / Mount
  // -----------------------------------------------------------------------
  LX200RETURN getSite(int& value);
  LX200RETURN setSite(int& value);
  LX200RETURN getSiteName(int idx, char* name, int len);

  LX200RETURN getMountIdx(int& value);
  LX200RETURN getMountName(int idx, char* name, int len);
  LX200RETURN setMount(int& value);
  LX200RETURN setMountName(int idx, char* name);

  // -----------------------------------------------------------------------
  //  Target setting
  // -----------------------------------------------------------------------
  LX200RETURN setTargetRA(uint8_t& vr1, uint8_t& vr2, uint8_t& vr3);
  LX200RETURN setTargetDec(bool& ispos, uint16_t& vd1, uint8_t& vd2, uint8_t& vd3);
  LX200RETURN setTargetAz(uint16_t& v1, uint8_t& v2, uint8_t& v3);
  LX200RETURN setTargetAlt(bool& ispos, uint16_t& vd1, uint8_t& vd2, uint8_t& vd3);

  // -----------------------------------------------------------------------
  //  Navigation (Sync / Goto / Pushto)
  // -----------------------------------------------------------------------
  LX200RETURN moveToTarget(TARGETTYPE target);
  LX200RETURN pushToTarget(TARGETTYPE target);
  LX200RETURN syncGoHome(NAV mode);
  LX200RETURN syncGoPark(NAV mode);

  /// Sync/Goto/Pushto using explicit RA/Dec components.
  LX200RETURN syncGoto(NAV mode, uint8_t& vr1, uint8_t& vr2, uint8_t& vr3,
                       bool& ispos, uint16_t& vd1, uint8_t& vd2, uint8_t& vd3);

  /// Sync/Goto/Pushto using RA (hours) and Dec (degrees) as floats.
  LX200RETURN syncGoto(NAV mode, float& Ra, float& Dec);

  /// Sync/Goto/Pushto using explicit Az/Alt components.
  LX200RETURN syncGotoAltAz(NAV mode, uint16_t& vz1, uint8_t& vz2, uint8_t& vz3,
                             bool& ispos, uint16_t& va1, uint8_t& va2, uint8_t& va3);

  /// Sync/Goto/Pushto using Az/Alt as floats (degrees).
  LX200RETURN syncGotoAltAz(NAV mode, float& Az, float& Alt);

  /// Sync/Goto/Pushto to user-defined stored target.
  LX200RETURN syncGotoUser(NAV mode);

  // -----------------------------------------------------------------------
  //  Motor configuration (per-axis: 1 = RA/Az, 2 = Dec/Alt)
  // -----------------------------------------------------------------------
  LX200RETURN readReverse(const uint8_t& axis, bool& reverse);
  LX200RETURN writeReverse(const uint8_t& axis, const bool& reverse);

  LX200RETURN readBacklash(const uint8_t& axis, float& backlash);
  LX200RETURN writeBacklash(const uint8_t& axis, const float& backlash);

  LX200RETURN readBacklashRate(const uint8_t& axis, float& rate);
  LX200RETURN writeBacklashRate(const uint8_t& axis, const float& rate);

  LX200RETURN readTotGear(const uint8_t& axis, float& gear);
  LX200RETURN writeTotGear(const uint8_t& axis, const float& gear);

  LX200RETURN readStepPerRot(const uint8_t& axis, float& steps);
  LX200RETURN writeStepPerRot(const uint8_t& axis, const float& steps);

  LX200RETURN readMicro(const uint8_t& axis, uint8_t& micro);
  LX200RETURN writeMicro(const uint8_t& axis, const uint8_t& micro);

  LX200RETURN readSilentStep(const uint8_t& axis, uint8_t& silent);
  LX200RETURN writeSilentStep(const uint8_t& axis, const uint8_t& silent);

  LX200RETURN readLowCurr(const uint8_t& axis, unsigned int& curr);
  LX200RETURN writeLowCurr(const uint8_t& axis, const unsigned int& curr);

  LX200RETURN readHighCurr(const uint8_t& axis, unsigned int& curr);
  LX200RETURN writeHighCurr(const uint8_t& axis, const unsigned int& curr);

  // -----------------------------------------------------------------------
  //  Encoder configuration
  // -----------------------------------------------------------------------
  LX200RETURN readEncoderReverse(const uint8_t& axis, bool& reverse);
  LX200RETURN writeEncoderReverse(const uint8_t& axis, const bool& reverse);

  LX200RETURN readPulsePerDegree(const uint8_t& axis, float& ppd);
  LX200RETURN writePulsePerDegree(const uint8_t& axis, const float& ppd);

  LX200RETURN startEncoderCalibration();
  LX200RETURN cancelEncoderCalibration();
  LX200RETURN completeEncoderCalibration();

  LX200RETURN readEncoderAutoSync(uint8_t& syncmode);
  LX200RETURN writeEncoderAutoSync(const uint8_t syncmode);

  // -----------------------------------------------------------------------
  //  Focuser
  // -----------------------------------------------------------------------
  LX200RETURN readFocuserConfig(unsigned int& startPosition, unsigned int& maxPosition,
                                unsigned int& minSpeed, unsigned int& maxSpeed,
                                unsigned int& cmdAcc, unsigned int& manAcc, unsigned int& manDec);

  LX200RETURN readFocuserMotor(bool& reverse, unsigned int& micro, unsigned int& incr,
                               unsigned int& curr, unsigned int& steprot);

private:
  Stream&        m_serial;
  unsigned long  m_timeout;

  void flushInput();
};

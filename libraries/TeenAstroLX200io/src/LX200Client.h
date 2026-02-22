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
#define LX200_FOCUSER_TIMEOUT 200  // ms — focuser binary commands pass through main unit

// Semantic timeout presets for consumers
#ifndef TIMEOUT_CMD
#define TIMEOUT_CMD 30
#endif
#ifndef TIMEOUT_WEB
#define TIMEOUT_WEB 15
#endif

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
  //  Firmware version
  // -----------------------------------------------------------------------
  LX200RETURN getProductName(char* out, int len);      // :GVP#
  LX200RETURN getVersionNumber(char* out, int len);    // :GVN#
  LX200RETURN getVersionDate(char* out, int len);      // :GVD#
  LX200RETURN getBoardVersion(char* out, int len);     // :GVB#
  LX200RETURN getDriverType(char* out, int len);       // :GVb#

  // -----------------------------------------------------------------------
  //  Position (string form for display)
  // -----------------------------------------------------------------------
  LX200RETURN getRaStr(char* out, int len);            // :GR#
  LX200RETURN getDecStr(char* out, int len);           // :GD#
  LX200RETURN getHaStr(char* out, int len);            // :GXT3#
  LX200RETURN getTargetRaStr(char* out, int len);      // :Gr#
  LX200RETURN getTargetDecStr(char* out, int len);     // :Gd#
  LX200RETURN getAzStr(char* out, int len);            // :GZ#
  LX200RETURN getAltStr(char* out, int len);           // :GA#

  // -----------------------------------------------------------------------
  //  Axis position (string form)
  // -----------------------------------------------------------------------
  LX200RETURN getAxisSteps(uint8_t axis, char* out, int len);       // :GXDP1# / :GXDP2#
  LX200RETURN getAxisDegrees(uint8_t axis, char* out, int len);     // :GXP1# / :GXP2#
  LX200RETURN getAxisDegreesCorr(uint8_t axis, char* out, int len); // :GXP3# / :GXP4#
  LX200RETURN getEncoderDegrees(uint8_t axis, char* out, int len);  // :GXE1# / :GXE2#
  LX200RETURN getEncoderDelta(char* out, int len);                  // :ED#

  // -----------------------------------------------------------------------
  //  Time / Date (string form for display)
  // -----------------------------------------------------------------------
  LX200RETURN getUTCTimeStr(char* out, int len);       // :GXT0#
  LX200RETURN getUTCDateStr(char* out, int len);       // :GXT1#
  LX200RETURN getSiderealStr(char* out, int len);      // :GS#

  // -----------------------------------------------------------------------
  //  Time / Date (typed)
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
  //  Mount state & diagnostics
  // -----------------------------------------------------------------------
  LX200RETURN getMountStateRaw(char* out, int len);    // :GXI#
  LX200RETURN getFocuserStatus(char* out, int len);    // :F?#

  // -----------------------------------------------------------------------
  //  Tracking rates (typed)
  // -----------------------------------------------------------------------
  LX200RETURN getTrackRateRA(long& value);             // :GXRr#
  LX200RETURN getTrackRateDec(long& value);            // :GXRd#
  LX200RETURN getStoredTrackRateRA(long& value);       // :GXRe#
  LX200RETURN getStoredTrackRateDec(long& value);      // :GXRf#

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
  //  Tracking
  // -----------------------------------------------------------------------
  LX200RETURN enableTracking(bool on);          // :Te# / :Td#
  LX200RETURN setTrackRateSidereal();            // :TQ#
  LX200RETURN setTrackRateLunar();               // :TL#
  LX200RETURN setTrackRateSolar();               // :TS#
  LX200RETURN setTrackRateUser();                // :TT#
  LX200RETURN incrementTrackRate();              // :T+#
  LX200RETURN decrementTrackRate();              // :T-#
  LX200RETURN resetTrackRate();                  // :TR#
  LX200RETURN setStepperMode(uint8_t mode);      // :T1# / :T2#

  // -----------------------------------------------------------------------
  //  Movement / Slew
  // -----------------------------------------------------------------------
  LX200RETURN startMoveNorth();                  // :Mn#
  LX200RETURN startMoveSouth();                  // :Ms#
  LX200RETURN startMoveEast();                   // :Me#
  LX200RETURN startMoveWest();                   // :Mw#
  LX200RETURN stopMoveNorth();                   // :Qn#
  LX200RETURN stopMoveSouth();                   // :Qs#
  LX200RETURN stopMoveEast();                    // :Qe#
  LX200RETURN stopMoveWest();                    // :Qw#
  LX200RETURN stopSlew();                        // :Q#
  LX200RETURN meridianFlip();                    // :MF#
  LX200RETURN setSpeed(uint8_t level);           // :R0# .. :R4#

  // -----------------------------------------------------------------------
  //  Home / Park
  // -----------------------------------------------------------------------
  LX200RETURN homeReset();                       // :hF#
  LX200RETURN homeGoto();                        // :hC#
  LX200RETURN park();                            // :hP#
  LX200RETURN unpark();                          // :hR#
  LX200RETURN setPark();                         // :hQ#
  LX200RETURN parkReset();                       // :hO#
  LX200RETURN setHomeCurrent();                  // :hB#
  LX200RETURN resetHomeCurrent();                // :hb#

  // -----------------------------------------------------------------------
  //  Alignment
  // -----------------------------------------------------------------------
  LX200RETURN alignStart();                      // :A0#
  LX200RETURN alignAcceptStar();                 // :A*#
  LX200RETURN alignAtHome();                     // :AA#
  LX200RETURN alignSave();                       // :AW#
  LX200RETURN alignClear();                      // :AC#
  LX200RETURN getAlignError(char* out, int len); // :AE#
  LX200RETURN alignSelectStar(uint8_t n);        // :A1# .. :A9#
  LX200RETURN alignNextStar();                   // :A+#
  LX200RETURN setPierSideEast();                 // :SmE#
  LX200RETURN setPierSideWest();                 // :SmW#
  LX200RETURN setPierSideNone();                 // :SmN#
  LX200RETURN getAlignErrorPolar(char* out, int len);  // :GXAw#
  LX200RETURN getAlignErrorAz(char* out, int len);     // :GXAz#
  LX200RETURN getAlignErrorAlt(char* out, int len);    // :GXAa#

  // -----------------------------------------------------------------------
  //  Focuser actions
  // -----------------------------------------------------------------------
  LX200RETURN focuserSetZero();                  // :FP#
  LX200RETURN focuserStop();                     // :FQ#
  LX200RETURN focuserMoveOut();                  // :F+#
  LX200RETURN focuserMoveIn();                   // :F-#
  LX200RETURN getFocuserVersion(char* out, int len); // :FV#
  LX200RETURN focuserResetConfig();              // :F!#
  LX200RETURN focuserSaveConfig();               // :F$#
  LX200RETURN focuserGotoHome();                 // :FS,0#
  LX200RETURN focuserIsConnected(bool& connected); // :F~#

  // -----------------------------------------------------------------------
  //  Rotator
  // -----------------------------------------------------------------------
  LX200RETURN rotatorCenter();                   // :rC#
  LX200RETURN rotatorReset();                    // :rF#
  LX200RETURN rotatorIncrDeRotate();             // :r+#
  LX200RETURN rotatorDecrDeRotate();             // :r-#
  LX200RETURN rotatorReverse();                  // :rR#
  LX200RETURN rotatorDeRotateToggle();           // :rP#
  LX200RETURN rotatorMove(uint8_t speed, bool forward); // :rN#:r>|<#

  // -----------------------------------------------------------------------
  //  Extended GX/SX config — Rates & Acceleration
  // -----------------------------------------------------------------------
  LX200RETURN getAcceleration(float& val);       // :GXRA#
  LX200RETURN setAcceleration(float val);        // :SXRA,val#
  LX200RETURN getMaxRate(int& val);              // :GXRX#
  LX200RETURN setMaxRate(int val);               // :SXRX,val#
  LX200RETURN getDeadband(int& val);             // :GXRD#
  LX200RETURN setDeadband(int val);              // :SXRD,val#
  LX200RETURN getSpeedRate(uint8_t idx, float& val);   // :GXRn#
  LX200RETURN setSpeedRate(uint8_t idx, float val);    // :SXRn,val#

  // -----------------------------------------------------------------------
  //  Extended GX/SX config — Limits
  // -----------------------------------------------------------------------
  LX200RETURN getMinAltitude(int& val);          // :GXLH#
  LX200RETURN setMinAltitude(int val);           // :SXLH,val#
  LX200RETURN getMaxAltitude(int& val);          // :GXLO#
  LX200RETURN setMaxAltitude(int val);           // :SXLO,val#
  LX200RETURN getUnderPoleLimit(char* out, int len);   // :GXLU#
  LX200RETURN setUnderPoleLimit(float val);      // :SXLU,val#
  LX200RETURN getMinDistFromPole(int& val);      // :GXLS#
  LX200RETURN setMinDistFromPole(int val);       // :SXLS,val#
  LX200RETURN getLimitEast(char* out, int len);  // :GXLE#
  LX200RETURN setLimitEast(int val);             // :SXLE,val#
  LX200RETURN getLimitWest(char* out, int len);  // :GXLW#
  LX200RETURN setLimitWest(int val);             // :SXLW,val#
  LX200RETURN getAxisLimit(char mode, char* out, int len);   // :GXlA#..D# or :GXLA#..D#
  LX200RETURN setAxisLimit(char mode, float val);             // :SXLx,val#

  // -----------------------------------------------------------------------
  //  Extended GX/SX config — Mount flags
  // -----------------------------------------------------------------------
  LX200RETURN getRefractionEnabled(char* out, int len);   // :GXrt#
  LX200RETURN enableRefraction(bool on);                    // :SXrt,y/n#
  LX200RETURN getPolarAlignEnabled(char* out, int len);   // :GXrp#
  LX200RETURN enablePolarAlign(bool on);                    // :SXrp,y/n#
  LX200RETURN getGoToEnabled(char* out, int len);          // :GXrg#
  LX200RETURN enableGoTo(bool on);                          // :SXrg,y/n#
  LX200RETURN enableMotors(bool on);                        // :SXME,y/n#
  LX200RETURN enableEncoders(bool on);                      // :SXEE,y/n#

  // -----------------------------------------------------------------------
  //  Extended GX/SX config — Other
  // -----------------------------------------------------------------------
  LX200RETURN getMountDescription(char* out, int len);      // :GXOA#
  LX200RETURN setMountDescription(const char* name);         // :SXOA,name#
  LX200RETURN getStepsPerSecond(char* out, int len);        // :GXOS#
  LX200RETURN setStepsPerSecond(int val);                    // :SXOS,val#

  // -----------------------------------------------------------------------
  //  Site — additional
  // -----------------------------------------------------------------------
  LX200RETURN getSelectedSite(int& val);                    // :W?#
  LX200RETURN setSelectedSite(int val);                     // :W0#..:W3#
  LX200RETURN setSiteName(const char* name);                // :Sn name#
  LX200RETURN getTimeZoneStr(char* out, int len);           // :GG#
  LX200RETURN setTimeZone(float tz);                        // :SG+NN:MM#
  LX200RETURN getLatitudeStr(char* out, int len);           // :Gtf#
  LX200RETURN getLongitudeStr(char* out, int len);          // :Ggf#
  LX200RETURN setLatitudeDMS(int sign, int deg, int min, int sec);   // :St+DD:MM:SS#
  LX200RETURN setLongitudeDMS(int sign, int deg, int min, int sec);  // :Sg+DDD:MM:SS#
  LX200RETURN setElevation(int val);                        // :Se+NNNN#
  LX200RETURN setUTCDateRaw(int month, int day, int year);  // :SXT1MM/DD/YY#
  LX200RETURN setUTCTimeRaw(int h, int m, int s);           // :SXT0HH:MM:SS#
  LX200RETURN setMountType(int type);                       // :S!1#..:S!4#

  // -----------------------------------------------------------------------
  //  Tracking rates — stored (write)
  // -----------------------------------------------------------------------
  LX200RETURN setStoredTrackRateRA(long val);               // :SXRe,val#
  LX200RETURN setStoredTrackRateDec(long val);              // :SXRf,val#

  // -----------------------------------------------------------------------
  //  Focuser config (write)
  // -----------------------------------------------------------------------
  LX200RETURN getFocuserConfigRaw(char* out, int len);      // :F~#
  LX200RETURN getFocuserAllConfig(char* out, int len);     // :FA# binary base64
  LX200RETURN getFocuserAllState(char* out, int len);      // :Fa# binary base64
  LX200RETURN setFocuserPark(int val);                      // :F0,val#
  LX200RETURN setFocuserMaxPos(int val);                    // :F1,val#
  LX200RETURN setFocuserLowSpeed(int val);                  // :F2,val#
  LX200RETURN setFocuserHighSpeed(int val);                 // :F3,val#
  LX200RETURN setFocuserGotoAcc(int val);                   // :F4,val#
  LX200RETURN setFocuserManAcc(int val);                    // :F5,val#
  LX200RETURN setFocuserDecel(int val);                     // :F6,val#
  LX200RETURN setFocuserRotation(int val);                  // :F7,val#
  LX200RETURN setFocuserResolution(int val);                // :F8,val#
  LX200RETURN setFocuserStepPerRot(int val);                // :Fr,val#
  LX200RETURN setFocuserMicro(int val);                     // :Fm,val#
  LX200RETURN setFocuserCurrent(int val);                   // :Fc,val#
  LX200RETURN getFocuserUserPos(int idx, char* out, int len);                 // :Fx0#..:Fx9#
  LX200RETURN setFocuserUserPos(int idx, int pos, const char* name);          // :Fs0,NNNNN_name#

  // -----------------------------------------------------------------------
  //  Miscellaneous
  // -----------------------------------------------------------------------
  LX200RETURN saveUserPosition();                // :SU#
  LX200RETURN getDate(char* out, int len);       // :GC#
  LX200RETURN getElevation(char* out, int len);  // :Ge#
  LX200RETURN syncTime();                        // :gs#
  LX200RETURN syncLocation();                    // :gt#
  LX200RETURN reboot();                          // :$!#
  LX200RETURN factoryReset();                    // :$$#
  LX200RETURN encoderSync();                     // :ECS#
  LX200RETURN encoderStopMotion();               // :EMQ#

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

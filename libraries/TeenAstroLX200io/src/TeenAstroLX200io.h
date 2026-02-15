/*
 * TeenAstroLX200io.h - LX200 serial I/O for TeenAstro
 *
 * This header provides backward-compatible free-function declarations that
 * delegate to the OO LX200Client class.  New code should use LX200Client
 * directly for cleaner, testable, non-global serial usage.
 *
 * Enums (LX200RETURN, CMDREPLY, ErrorsGoTo, TARGETTYPE, NAV) are now defined
 * in the shared TeenAstroCommandDef library and re-exported here.
 *
 * Copyright (C) 2024 TeenAstro by Charles Lemaire
 * GNU General Public License v3
 */
#pragma once

#include <Arduino.h>
#include <TeenAstroCatalog.h>
#include <TeenAstroCommandDef.h>
#include "LX200Client.h"

// ---------------------------------------------------------------------------
//  Platform-specific serial port alias (unchanged)
// ---------------------------------------------------------------------------
#ifdef ARDUINO_ESP8266_WEMOS_D1MINI
#define Ser Serial
#endif
#ifdef ARDUINO_LOLIN_C3_MINI
#define Ser Serial1
#endif

// ---------------------------------------------------------------------------
//  Timeouts (unchanged)
// ---------------------------------------------------------------------------
#define TIMEOUT_CMD 30
#define TIMEOUT_WEB 15

// ---------------------------------------------------------------------------
//  DEPRECATED -- Global LX200Client singleton
// ---------------------------------------------------------------------------
/// @deprecated Create an LX200Client explicitly and inject it instead.
LX200Client& lx200client();

// ---------------------------------------------------------------------------
//  DEPRECATED free-function wrappers
//  All consumers have been migrated to LX200Client.  These remain only for
//  third-party or user sketch backward compatibility and will be removed in
//  a future release.  New code must use LX200Client directly.
// ---------------------------------------------------------------------------

// --- Core I/O ---
bool readLX200Bytes(char* command, CMDREPLY& cmdreply, char* recvBuffer,
                    int bufferSize, unsigned long timeOutMs, bool keepHashtag = false);
LX200RETURN GetLX200(char* command, char* output, int buffersize);
LX200RETURN GetLX200Short(char* command, short* value);
LX200RETURN GetLX200Float(char* command, float* value);
LX200RETURN SetLX200(char* command);
LX200RETURN SetBoolLX200(char* command);

// --- Time / Date ---
LX200RETURN GetLocalTimeLX200(unsigned int &hour, unsigned int &minute, unsigned int &second);
LX200RETURN GetLocalTimeLX200(long &value);
LX200RETURN SetLocalTimeLX200(long &value);
LX200RETURN GetUTCTimeLX200(unsigned int &hour, unsigned int &minute, unsigned int &second);
LX200RETURN GetUTCTimeLX200(long &value);
LX200RETURN SetUTCTimeLX200(long &value);
LX200RETURN GetLocalDateLX200(unsigned int &day, unsigned int &month, unsigned int &year);
LX200RETURN GetUTCDateLX200(unsigned int &day, unsigned int &month, unsigned int &year);

// --- Location ---
LX200RETURN GetLstT0LX200(double &T0);
LX200RETURN GetLatitudeLX200(double &degree);
LX200RETURN GetLongitudeLX200(double &degree);
LX200RETURN GetTrackingRateLX200(double &rate);

// --- Site / Mount ---
LX200RETURN GetSiteLX200(int &value);
LX200RETURN SetSiteLX200(int &value);
LX200RETURN GetSiteNameLX200(int idx, char* name, int len);
LX200RETURN GetMountIdxLX200(int &value);
LX200RETURN GetMountNameLX200(int idx, char* name, int len);
LX200RETURN SetMountLX200(int &value);
LX200RETURN SetMountNameLX200(int idx, char* name);

// --- Target ---
LX200RETURN SetTargetRaLX200(uint8_t& vr1, uint8_t& vr2, uint8_t& vr3);
LX200RETURN SetTargetDecLX200(bool& ispos, uint16_t& vd1, uint8_t& vd2, uint8_t& vd3);
LX200RETURN SetTargetAzLX200(uint16_t& v1, uint8_t& v2, uint8_t& v3);
LX200RETURN SetTargetAltLX200(bool& ispos, uint16_t& vd1, uint8_t& vd2, uint8_t& vd3);

// --- Navigation ---
LX200RETURN Move2TargetLX200(TARGETTYPE target);
LX200RETURN Push2TargetLX200(TARGETTYPE target);
LX200RETURN SyncGoHomeLX200(NAV mode);
LX200RETURN SyncGoParkLX200(NAV mode);
LX200RETURN SyncGotoLX200(NAV mode, uint8_t& vr1, uint8_t& vr2, uint8_t& vr3,
                           bool& ispos, uint16_t& vd1, uint8_t& vd2, uint8_t& vd3);
LX200RETURN SyncGotoLX200(NAV mode, float &Ra, float &Dec);
LX200RETURN SyncGotoLX200AltAz(NAV mode, float &Az, float &Alt);
LX200RETURN SyncGotoUserLX200(NAV mode);
LX200RETURN SyncGotoLX200(LX200Client& client, NAV mode, float &Ra, float &Dec, double epoch);
LX200RETURN SyncSelectedStarLX200(LX200Client& client, unsigned short alignSelectedStar);
LX200RETURN SyncGotoCatLX200(LX200Client& client, NAV mode);
LX200RETURN SyncGotoPlanetLX200(LX200Client& client, NAV mode, unsigned short obj);

// --- Motor ---
LX200RETURN readReverseLX200(const uint8_t &axis, bool &reverse);
LX200RETURN writeReverseLX200(const uint8_t &axis, const bool &reverse);
LX200RETURN readBacklashLX200(const uint8_t &axis, float &backlash);
LX200RETURN writeBacklashLX200(const uint8_t &axis, const float &backlash);
LX200RETURN readBacklashRateLX200(const uint8_t& axis, float& rate);
LX200RETURN writeBacklashRateLX200(const uint8_t& axis, const float& rate);
LX200RETURN readTotGearLX200(const uint8_t &axis, float &totGear);
LX200RETURN writeTotGearLX200(const uint8_t &axis, const float &totGear);
LX200RETURN readStepPerRotLX200(const uint8_t &axis, float &stepPerRot);
LX200RETURN writeStepPerRotLX200(const uint8_t &axis, const float &stepPerRot);
LX200RETURN readMicroLX200(const uint8_t &axis, uint8_t &microStep);
LX200RETURN writeMicroLX200(const uint8_t &axis, const uint8_t &microStep);
LX200RETURN readSilentStepLX200(const uint8_t &axis, uint8_t &silent);
LX200RETURN writeSilentStepLX200(const uint8_t &axis, const uint8_t &silent);
LX200RETURN readLowCurrLX200(const uint8_t &axis, unsigned int &lowCurr);
LX200RETURN writeLowCurrLX200(const uint8_t &axis, const unsigned int &lowCurr);
LX200RETURN readHighCurrLX200(const uint8_t &axis, unsigned int &highCurr);
LX200RETURN writeHighCurrLX200(const uint8_t &axis, const unsigned int &highCurr);

// --- Encoder ---
LX200RETURN readEncoderReverseLX200(const uint8_t& axis, bool& reverse);
LX200RETURN writeEncoderReverseLX200(const uint8_t& axis, const bool& reverse);
LX200RETURN readPulsePerDegreeLX200(const uint8_t& axis, float& ppd);
LX200RETURN writePulsePerDegreeLX200(const uint8_t& axis, const float& ppd);
LX200RETURN StartEncoderCalibration();
LX200RETURN CancelEncoderCalibration();
LX200RETURN CompleteEncoderCalibration();
LX200RETURN readEncoderAutoSync(uint8_t& syncmode);
LX200RETURN writeEncoderAutoSync(const uint8_t syncmode);

// --- Focuser ---
LX200RETURN readFocuserConfig(unsigned int& startPosition, unsigned int& maxPosition,
                              unsigned int& minSpeed, unsigned int& maxSpeed,
                              unsigned int& cmdAcc, unsigned int& manAcc, unsigned int& manDec);
LX200RETURN readFocuserMotor(bool& reverse, unsigned int& micro, unsigned int& incr,
                             unsigned int& curr, unsigned int& steprot);

// --- Codec utilities (re-exported from CommandCodec) ---
// hmsToDouble and dmsToDouble are now in <CommandCodec.h>
// They are transitively included via <TeenAstroCommandDef.h>.

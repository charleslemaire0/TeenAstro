/*
 * AlpacaTelescope.h - ASCOM Alpaca Telescope device for TeenAstro
 *
 * Copyright (C) 2026 TeenAstro by Charles Lemaire
 * GNU General Public License v3
 *
 * Implements the `/api/v1/telescope/0/...` endpoints defined in
 *   https://ascom-standards.org/api/AlpacaDeviceAPI_v1.yaml
 *
 * The handler is stateless: every dispatch call reads / writes the mount
 * via LX200Client and reads cached state from TeenAstroMountStatus.
 * The "Connected" property is local soft state (the serial link to the
 * MainUnit is always live regardless of what an Alpaca client thinks).
 */
#pragma once

#include <Arduino.h>
#include "AlpacaResponse.h"

class LX200Client;
class TeenAstroMountStatus;

class TeenAstroAlpaca;  // forward (for serverTxId allocator)

class AlpacaTelescope
{
public:
  AlpacaTelescope() = default;

  void begin(LX200Client& client,
             TeenAstroMountStatus& mountStatus,
             TeenAstroAlpaca& parent);

  /// Dispatch a single telescope-scoped Alpaca request that has already been
  /// decoded into `req` (devicePath / member / params).  Sends the JSON
  /// response on `server`.
  void dispatch(AlpacaWebServer& server, const AlpacaRequest& req);

  /// Used by the management API to advertise the telescope.
  static const char* deviceName()        { return "TeenAstro Telescope"; }
  static const char* driverInfo()        { return "TeenAstro Alpaca driver — bridges the ASCOM Alpaca REST API to the TeenAstro LX200 protocol."; }
  static const char* driverVersion()     { return "1.0.0"; }
  static int         interfaceVersion()  { return 4; }

private:
  // ---------- common ----------
  void getConnected(AlpacaWebServer& s, const AlpacaRequest& r);
  void putConnected(AlpacaWebServer& s, const AlpacaRequest& r);
  void getName(AlpacaWebServer& s, const AlpacaRequest& r);
  void getDescription(AlpacaWebServer& s, const AlpacaRequest& r);
  void getDriverInfo(AlpacaWebServer& s, const AlpacaRequest& r);
  void getDriverVersion(AlpacaWebServer& s, const AlpacaRequest& r);
  void getInterfaceVersion(AlpacaWebServer& s, const AlpacaRequest& r);
  void getSupportedActions(AlpacaWebServer& s, const AlpacaRequest& r);
  /// ASCOM Telescope Interface Version 4+ (aggregated operational state).
  void getDeviceState(AlpacaWebServer& s, const AlpacaRequest& r);
  /// ASCOM Platform 7 — async Connect()/Disconnect() completion flag (instant attach here).
  void getConnecting(AlpacaWebServer& s, const AlpacaRequest& r);
  /// ASCOM Platform 7 — async disconnect entry point (immediate on this bridge).
  void putDisconnect(AlpacaWebServer& s, const AlpacaRequest& r);
  /// ASCOM Platform 7 — async connect entry point (immediate on this bridge).
  void putConnect(AlpacaWebServer& s, const AlpacaRequest& r);

  // ---------- read-only properties ----------
  void getRightAscension(AlpacaWebServer& s, const AlpacaRequest& r);
  void getDeclination(AlpacaWebServer& s, const AlpacaRequest& r);
  void getAltitude(AlpacaWebServer& s, const AlpacaRequest& r);
  void getAzimuth(AlpacaWebServer& s, const AlpacaRequest& r);
  void getSiderealTime(AlpacaWebServer& s, const AlpacaRequest& r);
  void getSlewing(AlpacaWebServer& s, const AlpacaRequest& r);
  void getAtPark(AlpacaWebServer& s, const AlpacaRequest& r);
  void getAtHome(AlpacaWebServer& s, const AlpacaRequest& r);
  void getIsPulseGuiding(AlpacaWebServer& s, const AlpacaRequest& r);
  void getSideOfPier(AlpacaWebServer& s, const AlpacaRequest& r);
  void putSideOfPier(AlpacaWebServer& s, const AlpacaRequest& r);
  void getDestinationSideOfPier(AlpacaWebServer& s, const AlpacaRequest& r);
  void getAlignmentMode(AlpacaWebServer& s, const AlpacaRequest& r);
  void getEquatorialSystem(AlpacaWebServer& s, const AlpacaRequest& r);
  void getApertureArea(AlpacaWebServer& s, const AlpacaRequest& r);
  void getApertureDiameter(AlpacaWebServer& s, const AlpacaRequest& r);
  void getFocalLength(AlpacaWebServer& s, const AlpacaRequest& r);
  void getDoesRefraction(AlpacaWebServer& s, const AlpacaRequest& r);
  void putDoesRefraction(AlpacaWebServer& s, const AlpacaRequest& r);

  // ---------- target ----------
  void getTargetRightAscension(AlpacaWebServer& s, const AlpacaRequest& r);
  void putTargetRightAscension(AlpacaWebServer& s, const AlpacaRequest& r);
  void getTargetDeclination(AlpacaWebServer& s, const AlpacaRequest& r);
  void putTargetDeclination(AlpacaWebServer& s, const AlpacaRequest& r);

  // ---------- tracking ----------
  void getTracking(AlpacaWebServer& s, const AlpacaRequest& r);
  void putTracking(AlpacaWebServer& s, const AlpacaRequest& r);
  void getTrackingRate(AlpacaWebServer& s, const AlpacaRequest& r);
  void putTrackingRate(AlpacaWebServer& s, const AlpacaRequest& r);
  void getTrackingRates(AlpacaWebServer& s, const AlpacaRequest& r);
  void getRightAscensionRate(AlpacaWebServer& s, const AlpacaRequest& r);
  void putRightAscensionRate(AlpacaWebServer& s, const AlpacaRequest& r);
  void getDeclinationRate(AlpacaWebServer& s, const AlpacaRequest& r);
  void putDeclinationRate(AlpacaWebServer& s, const AlpacaRequest& r);
  void getGuideRateRA(AlpacaWebServer& s, const AlpacaRequest& r);
  void putGuideRateRA(AlpacaWebServer& s, const AlpacaRequest& r);
  void getGuideRateDec(AlpacaWebServer& s, const AlpacaRequest& r);
  void putGuideRateDec(AlpacaWebServer& s, const AlpacaRequest& r);
  void getSlewSettleTime(AlpacaWebServer& s, const AlpacaRequest& r);
  void putSlewSettleTime(AlpacaWebServer& s, const AlpacaRequest& r);

  // ---------- site ----------
  void getSiteLatitude(AlpacaWebServer& s, const AlpacaRequest& r);
  void putSiteLatitude(AlpacaWebServer& s, const AlpacaRequest& r);
  void getSiteLongitude(AlpacaWebServer& s, const AlpacaRequest& r);
  void putSiteLongitude(AlpacaWebServer& s, const AlpacaRequest& r);
  void getSiteElevation(AlpacaWebServer& s, const AlpacaRequest& r);
  void putSiteElevation(AlpacaWebServer& s, const AlpacaRequest& r);
  void getUtcDate(AlpacaWebServer& s, const AlpacaRequest& r);
  void putUtcDate(AlpacaWebServer& s, const AlpacaRequest& r);

  // ---------- capabilities ----------
  void getCanFindHome(AlpacaWebServer& s, const AlpacaRequest& r);
  void getCanPark(AlpacaWebServer& s, const AlpacaRequest& r);
  void getCanUnpark(AlpacaWebServer& s, const AlpacaRequest& r);
  void getCanSetPark(AlpacaWebServer& s, const AlpacaRequest& r);
  void getCanSetTracking(AlpacaWebServer& s, const AlpacaRequest& r);
  void getCanSlew(AlpacaWebServer& s, const AlpacaRequest& r);
  void getCanSlewAsync(AlpacaWebServer& s, const AlpacaRequest& r);
  void getCanSlewAltAz(AlpacaWebServer& s, const AlpacaRequest& r);
  void getCanSlewAltAzAsync(AlpacaWebServer& s, const AlpacaRequest& r);
  void getCanSync(AlpacaWebServer& s, const AlpacaRequest& r);
  void getCanSyncAltAz(AlpacaWebServer& s, const AlpacaRequest& r);
  void getCanPulseGuide(AlpacaWebServer& s, const AlpacaRequest& r);
  void getCanMoveAxis(AlpacaWebServer& s, const AlpacaRequest& r);
  void getCanSetGuideRates(AlpacaWebServer& s, const AlpacaRequest& r);
  void getCanSetRightAscensionRate(AlpacaWebServer& s, const AlpacaRequest& r);
  void getCanSetDeclinationRate(AlpacaWebServer& s, const AlpacaRequest& r);
  void getCanSetPierSide(AlpacaWebServer& s, const AlpacaRequest& r);
  void getCanSlewAltAzCommon(AlpacaWebServer& s, const AlpacaRequest& r);
  void getFalse(AlpacaWebServer& s, const AlpacaRequest& r);
  void getTrue(AlpacaWebServer& s, const AlpacaRequest& r);

  // ---------- methods ----------
  void putAbortSlew(AlpacaWebServer& s, const AlpacaRequest& r);
  void putFindHome(AlpacaWebServer& s, const AlpacaRequest& r);
  void putPark(AlpacaWebServer& s, const AlpacaRequest& r);
  void putUnpark(AlpacaWebServer& s, const AlpacaRequest& r);
  void putSetPark(AlpacaWebServer& s, const AlpacaRequest& r);
  void putSlewToCoordinates(AlpacaWebServer& s, const AlpacaRequest& r);
  void putSlewToCoordinatesAsync(AlpacaWebServer& s, const AlpacaRequest& r);
  void putSyncToCoordinates(AlpacaWebServer& s, const AlpacaRequest& r);
  void putSlewToTarget(AlpacaWebServer& s, const AlpacaRequest& r);
  void putSlewToTargetAsync(AlpacaWebServer& s, const AlpacaRequest& r);
  void putSyncToTarget(AlpacaWebServer& s, const AlpacaRequest& r);
  void putSlewToAltAz(AlpacaWebServer& s, const AlpacaRequest& r);
  void putSlewToAltAzAsync(AlpacaWebServer& s, const AlpacaRequest& r);
  void putSyncToAltAz(AlpacaWebServer& s, const AlpacaRequest& r);
  void putPulseGuide(AlpacaWebServer& s, const AlpacaRequest& r);
  void putMoveAxis(AlpacaWebServer& s, const AlpacaRequest& r);
  void getAxisRates(AlpacaWebServer& s, const AlpacaRequest& r);
  void getCanMoveAxisQ(AlpacaWebServer& s, const AlpacaRequest& r);
  void putAction(AlpacaWebServer& s, const AlpacaRequest& r);
  void putCommandBlind(AlpacaWebServer& s, const AlpacaRequest& r);
  void putCommandBool(AlpacaWebServer& s, const AlpacaRequest& r);
  void putCommandString(AlpacaWebServer& s, const AlpacaRequest& r);

  // ---------- helpers ----------
  bool requireConnected(AlpacaWebServer& s, const AlpacaRequest& r);
  /// One-shot UTC sync: pushes the host UTC date+time to the firmware so
  /// the mount's sidereal-time computation lines up with the client's
  /// expectations.  Best-effort (silently ignores firmware refusal).
  void syncUtcToHost();
  /// Wait for pulse guide / goto / MoveAxis to finish before :MS# / :CM# (ConformU).
  void settleMotionBeforeRadecCmd();
  /// Turn sidereal tracking on when motors are enabled (Conform resumes EQ ops after AltAz).
  void ensureTrackingOnForRadecOps();
  /// Shared attach behaviour for PUT connected=true and PUT connect (Alpaca Platform 7).
  void noteAlpacaAttached(bool wasAlreadySoftConnected);
  /// Common helper to start a slew to (raHours, decDeg) using LX200 syncGoto
  /// in NAV_GOTO mode.  Sends the response itself and returns true if the
  /// operation was accepted.
  bool slewToRaDec(AlpacaWebServer& s, const AlpacaRequest& r,
                   double raHours, double decDeg);
  /// Common helper to sync to (raHours, decDeg).
  bool syncToRaDec(AlpacaWebServer& s, const AlpacaRequest& r,
                   double raHours, double decDeg);
  /// Common helper to slew/sync to (azDeg, altDeg).
  bool slewSyncToAltAz(AlpacaWebServer& s, const AlpacaRequest& r,
                       double azDeg, double altDeg, bool sync);

  LX200Client*           m_client       = nullptr;
  TeenAstroMountStatus*  m_status       = nullptr;
  TeenAstroAlpaca*       m_parent       = nullptr;

  // Soft connection state — Alpaca clients call PUT connected=true to
  // attach, and we honour that as a contract even though our serial link
  // to the MainUnit is always live.
  bool                   m_connected    = false;

  // Cached writes that have no equivalent in the firmware: Alpaca
  // applications expect to round-trip these values even if we cannot
  // physically apply them.
  double                 m_targetRaHours = 0;
  double                 m_targetDecDeg  = 0;
  bool                   m_targetRaSet   = false;
  bool                   m_targetDecSet  = false;
  // Site lat/lon writes via :St / :Sg only succeed when the mount is
  // parked or at home — a constraint the spec / ConformU does not know
  // about.  Cache the user's value locally so reads round-trip even
  // when the firmware silently rejects the write.
  double                 m_siteLat       = 0;
  double                 m_siteLon       = 0;
  bool                   m_siteLatSet    = false;
  bool                   m_siteLonSet    = false;
  double                 m_raRate        = 0;
  double                 m_decRate       = 0;
  /// Same as TeenAstro ASCOM driver RateRoundTripCacheMs — GXAS quantizes tiny tracking offsets.
  unsigned long          m_raRateCacheStartMs  = 0;
  unsigned long          m_decRateCacheStartMs = 0;
  bool                   m_raRateCacheActive   = false;
  bool                   m_decRateCacheActive  = false;
  int                    m_trackingRate  = 0;   // Alpaca DriveRates: 0=Sidereal, 1=Solar, 2=Lunar, 3=King

  // ---- Soft state to honour ASCOM async semantics ----
  // MoveAxis must report Slewing=true while a non-zero rate is active.
  // The mount-state cache in TeenAstroMountStatus only flips into
  // TRK_SLEWING during a goto, so we shadow MoveAxis state here.
  bool                   m_moveAxisActive[2] = { false, false };
  /// ConformU: firmware suspends sidereal during :M1/:M2 AtRate — restore when stopping if it was on.
  bool                   m_moveAxisRestoreTracking = false;
  // PulseGuide is asynchronous: the call returns immediately, and
  // IsPulseGuiding must report true for `duration` ms afterwards.  The
  // firmware does not expose a flag for this, so we shadow it.
  unsigned long          m_pulseGuideEndMs   = 0;
  // Forced "we just kicked off a slew, treat Slewing=true for ~settle ms"
  // so async-slew callers can observe Slewing=true on the very next GET.
  unsigned long          m_slewKickEndMs     = 0;
};

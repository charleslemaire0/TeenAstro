/*
 * AlpacaTelescope.cpp - ASCOM Alpaca Telescope device for TeenAstro
 *
 * Copyright (C) 2026 TeenAstro by Charles Lemaire
 * GNU General Public License v3
 */
#include "AlpacaTelescope.h"
#include "TeenAstroAlpaca.h"
#include <LX200Client.h>
#include <TeenAstroMountStatus.h>
#include <math.h>
#include <time.h>

// ----------------------------------------------------------------------------
//  Lifecycle
// ----------------------------------------------------------------------------

void AlpacaTelescope::begin(LX200Client& client,
                            TeenAstroMountStatus& mountStatus,
                            TeenAstroAlpaca& parent)
{
  m_client = &client;
  m_status = &mountStatus;
  m_parent = &parent;
}

void AlpacaTelescope::syncUtcToHost()
{
  if (!m_client) return;
  time_t now = time(nullptr);
  // Skip obviously unset clocks (TeenAstro epoch starts ~1970 but SNTP-less
  // boards often sit at boot epoch until configured).
  if (now < (time_t)1577836800)
    return;
  struct tm* u = gmtime(&now);
  if (!u) return;
  m_client->setUTCDateRaw(u->tm_mon + 1, u->tm_mday, (u->tm_year + 1900) % 100);
  m_client->setUTCTimeRaw(u->tm_hour, u->tm_min, u->tm_sec);
}

bool AlpacaTelescope::requireConnected(AlpacaWebServer& s, const AlpacaRequest& r)
{
  if (m_connected) return true;
  sendAlpacaError(s, r, m_parent->nextServerTransactionId(),
                  AE_NOT_CONNECTED, "Telescope not connected");
  return false;
}


// ----------------------------------------------------------------------------
//  Dispatch table — keep alphabetical for ease of audit
// ----------------------------------------------------------------------------

#define MATCH(name)  if (m == F(name))

void AlpacaTelescope::dispatch(AlpacaWebServer& server, const AlpacaRequest& req)
{
  const String& m = req.member;

  // --- common (ASCOM Common Methods) ---
  MATCH("connected")               { req.isPut ? putConnected(server, req)               : getConnected(server, req); return; }
  MATCH("name")                    { getName(server, req); return; }
  MATCH("description")             { getDescription(server, req); return; }
  MATCH("driverinfo")              { getDriverInfo(server, req); return; }
  MATCH("driverversion")           { getDriverVersion(server, req); return; }
  MATCH("interfaceversion")        { getInterfaceVersion(server, req); return; }
  MATCH("supportedactions")        { getSupportedActions(server, req); return; }
  MATCH("action")                  { putAction(server, req); return; }
  MATCH("commandblind")            { putCommandBlind(server, req); return; }
  MATCH("commandbool")             { putCommandBool(server, req); return; }
  MATCH("commandstring")           { putCommandString(server, req); return; }

  // --- read-only properties ---
  MATCH("rightascension")          { getRightAscension(server, req); return; }
  MATCH("declination")             { getDeclination(server, req); return; }
  MATCH("altitude")                { getAltitude(server, req); return; }
  MATCH("azimuth")                 { getAzimuth(server, req); return; }
  MATCH("siderealtime")            { getSiderealTime(server, req); return; }
  MATCH("slewing")                 { getSlewing(server, req); return; }
  MATCH("atpark")                  { getAtPark(server, req); return; }
  MATCH("athome")                  { getAtHome(server, req); return; }
  MATCH("ispulseguiding")          { getIsPulseGuiding(server, req); return; }
  MATCH("sideofpier")              { req.isPut ? putSideOfPier(server, req)
                                              : getSideOfPier(server, req); return; }
  MATCH("destinationsideofpier")   { getDestinationSideOfPier(server, req); return; }
  MATCH("alignmentmode")           { getAlignmentMode(server, req); return; }
  MATCH("equatorialsystem")        { getEquatorialSystem(server, req); return; }
  MATCH("aperturearea")            { getApertureArea(server, req); return; }
  MATCH("aperturediameter")        { getApertureDiameter(server, req); return; }
  MATCH("focallength")             { getFocalLength(server, req); return; }
  MATCH("doesrefraction")          { req.isPut ? putDoesRefraction(server, req)          : getDoesRefraction(server, req); return; }

  // --- target ---
  MATCH("targetrightascension")    { req.isPut ? putTargetRightAscension(server, req)    : getTargetRightAscension(server, req); return; }
  MATCH("targetdeclination")       { req.isPut ? putTargetDeclination(server, req)       : getTargetDeclination(server, req); return; }

  // --- tracking ---
  MATCH("tracking")                { req.isPut ? putTracking(server, req)                : getTracking(server, req); return; }
  MATCH("trackingrate")            { req.isPut ? putTrackingRate(server, req)            : getTrackingRate(server, req); return; }
  MATCH("trackingrates")           { getTrackingRates(server, req); return; }
  MATCH("rightascensionrate")      { req.isPut ? putRightAscensionRate(server, req)      : getRightAscensionRate(server, req); return; }
  MATCH("declinationrate")         { req.isPut ? putDeclinationRate(server, req)         : getDeclinationRate(server, req); return; }
  MATCH("guideratedeclination")    { req.isPut ? putGuideRateDec(server, req)            : getGuideRateDec(server, req); return; }
  MATCH("guideraterightascension") { req.isPut ? putGuideRateRA(server, req)             : getGuideRateRA(server, req); return; }
  MATCH("slewsettletime")          { req.isPut ? putSlewSettleTime(server, req)          : getSlewSettleTime(server, req); return; }

  // --- site ---
  MATCH("sitelatitude")            { req.isPut ? putSiteLatitude(server, req)            : getSiteLatitude(server, req); return; }
  MATCH("sitelongitude")           { req.isPut ? putSiteLongitude(server, req)           : getSiteLongitude(server, req); return; }
  MATCH("siteelevation")           { req.isPut ? putSiteElevation(server, req)           : getSiteElevation(server, req); return; }
  MATCH("utcdate")                 { req.isPut ? putUtcDate(server, req)                 : getUtcDate(server, req); return; }

  // --- capabilities ---
  MATCH("canfindhome")             { getCanFindHome(server, req); return; }
  MATCH("canpark")                 { getCanPark(server, req); return; }
  MATCH("canunpark")               { getCanUnpark(server, req); return; }
  MATCH("cansetpark")              { getCanSetPark(server, req); return; }
  MATCH("cansettracking")          { getCanSetTracking(server, req); return; }
  MATCH("canslew")                 { getCanSlew(server, req); return; }
  MATCH("canslewasync")            { getCanSlewAsync(server, req); return; }
  MATCH("canslewaltaz")            { getCanSlewAltAz(server, req); return; }
  MATCH("canslewaltazasync")       { getCanSlewAltAzAsync(server, req); return; }
  MATCH("cansync")                 { getCanSync(server, req); return; }
  MATCH("cansyncaltaz")            { getCanSyncAltAz(server, req); return; }
  MATCH("canpulseguide")           { getCanPulseGuide(server, req); return; }
  MATCH("canmoveaxis")             { getCanMoveAxisQ(server, req); return; }
  MATCH("cansetguiderates")        { getCanSetGuideRates(server, req); return; }
  MATCH("cansetrightascensionrate"){ getCanSetRightAscensionRate(server, req); return; }
  MATCH("cansetdeclinationrate")   { getCanSetDeclinationRate(server, req); return; }
  MATCH("cansetpierside")          { getCanSetPierSide(server, req); return; }

  // --- methods ---
  MATCH("abortslew")               { putAbortSlew(server, req); return; }
  MATCH("findhome")                { putFindHome(server, req); return; }
  MATCH("park")                    { putPark(server, req); return; }
  MATCH("unpark")                  { putUnpark(server, req); return; }
  MATCH("setpark")                 { putSetPark(server, req); return; }
  MATCH("slewtocoordinates")       { putSlewToCoordinates(server, req); return; }
  MATCH("slewtocoordinatesasync")  { putSlewToCoordinatesAsync(server, req); return; }
  MATCH("synctocoordinates")       { putSyncToCoordinates(server, req); return; }
  MATCH("slewtotarget")            { putSlewToTarget(server, req); return; }
  MATCH("slewtotargetasync")       { putSlewToTargetAsync(server, req); return; }
  MATCH("synctotarget")            { putSyncToTarget(server, req); return; }
  MATCH("slewtoaltaz")             { putSlewToAltAz(server, req); return; }
  MATCH("slewtoaltazasync")        { putSlewToAltAzAsync(server, req); return; }
  MATCH("synctoaltaz")             { putSyncToAltAz(server, req); return; }
  MATCH("pulseguide")              { putPulseGuide(server, req); return; }
  MATCH("moveaxis")                { putMoveAxis(server, req); return; }
  MATCH("axisrates")               { getAxisRates(server, req); return; }

  sendAlpacaError(server, req, m_parent->nextServerTransactionId(),
                  AE_NOT_IMPLEMENTED, "Telescope member not implemented");
}

#undef MATCH


// ----------------------------------------------------------------------------
//  Common
// ----------------------------------------------------------------------------

void AlpacaTelescope::getConnected(AlpacaWebServer& s, const AlpacaRequest& r)
{
  sendAlpacaOk(s, r, m_parent->nextServerTransactionId(),
               AlpacaJson::boolStr(m_connected));
}

void AlpacaTelescope::putConnected(AlpacaWebServer& s, const AlpacaRequest& r)
{
  String v = AlpacaRequest::arg(s, "Connected");
  v.toLowerCase();
  bool wasConnected = m_connected;
  m_connected = (v == "true" || v == "1");
  if (m_connected)
  {
    // Trigger one mount-state refresh on connect so subsequent property
    // reads return current values without waiting for the polling cycle.
    if (m_status) m_status->updateAllState(true);
    // Sync the firmware clock to host UTC on the *first* connect of a
    // session.  The emulator boots with a default fixture date that is
    // wildly out of step with the host, which trips ConformU's
    // SiderealTime sanity check.  On real hardware this is a no-op for
    // already-synced clocks (write succeeds, value re-stored).
    if (!wasConnected) syncUtcToHost();
  }
  sendAlpacaVoid(s, r, m_parent->nextServerTransactionId());
}

void AlpacaTelescope::getName(AlpacaWebServer& s, const AlpacaRequest& r)
{
  sendAlpacaOk(s, r, m_parent->nextServerTransactionId(),
               AlpacaJson::escape(deviceName()));
}

void AlpacaTelescope::getDescription(AlpacaWebServer& s, const AlpacaRequest& r)
{
  sendAlpacaOk(s, r, m_parent->nextServerTransactionId(),
               AlpacaJson::escape("TeenAstro mount via ASCOM Alpaca"));
}

void AlpacaTelescope::getDriverInfo(AlpacaWebServer& s, const AlpacaRequest& r)
{
  sendAlpacaOk(s, r, m_parent->nextServerTransactionId(),
               AlpacaJson::escape(driverInfo()));
}

void AlpacaTelescope::getDriverVersion(AlpacaWebServer& s, const AlpacaRequest& r)
{
  sendAlpacaOk(s, r, m_parent->nextServerTransactionId(),
               AlpacaJson::escape(driverVersion()));
}

void AlpacaTelescope::getInterfaceVersion(AlpacaWebServer& s, const AlpacaRequest& r)
{
  sendAlpacaOk(s, r, m_parent->nextServerTransactionId(),
               AlpacaJson::intStr(interfaceVersion()));
}

void AlpacaTelescope::getSupportedActions(AlpacaWebServer& s, const AlpacaRequest& r)
{
  sendAlpacaOk(s, r, m_parent->nextServerTransactionId(), F("[]"));
}

void AlpacaTelescope::putAction(AlpacaWebServer& s, const AlpacaRequest& r)
{
  sendAlpacaError(s, r, m_parent->nextServerTransactionId(),
                  AE_ACTION_NOT_IMPL, "No custom actions implemented");
}

// CommandBlind / CommandBool / CommandString are pass-throughs to the LX200
// protocol, mirroring the legacy ASCOM driver behaviour.  They are useful
// for diagnostics — most clients should never need them.

void AlpacaTelescope::putCommandBlind(AlpacaWebServer& s, const AlpacaRequest& r)
{
  if (!requireConnected(s, r)) return;
  String cmd = AlpacaRequest::arg(s, "Command");
  String raw = AlpacaRequest::arg(s, "Raw");
  raw.toLowerCase();
  bool isRaw = (raw == "true" || raw == "1");
  String framed = isRaw ? cmd : (String(":") + cmd + "#");
  char buf[8] = "";
  m_client->sendReceive(framed.c_str(), CMDR_NO, buf, sizeof(buf), 100);
  sendAlpacaVoid(s, r, m_parent->nextServerTransactionId());
}

void AlpacaTelescope::putCommandBool(AlpacaWebServer& s, const AlpacaRequest& r)
{
  if (!requireConnected(s, r)) return;
  String cmd = AlpacaRequest::arg(s, "Command");
  String raw = AlpacaRequest::arg(s, "Raw");
  raw.toLowerCase();
  bool isRaw = (raw == "true" || raw == "1");
  String framed = isRaw ? cmd : (String(":") + cmd + "#");
  char buf[16] = "";
  bool ok = m_client->sendReceive(framed.c_str(), CMDR_SHORT_BOOL, buf, sizeof(buf), 200);
  bool val = ok && (buf[0] == '1');
  sendAlpacaOk(s, r, m_parent->nextServerTransactionId(), AlpacaJson::boolStr(val));
}

void AlpacaTelescope::putCommandString(AlpacaWebServer& s, const AlpacaRequest& r)
{
  if (!requireConnected(s, r)) return;
  String cmd = AlpacaRequest::arg(s, "Command");
  String raw = AlpacaRequest::arg(s, "Raw");
  raw.toLowerCase();
  bool isRaw = (raw == "true" || raw == "1");
  String framed = isRaw ? cmd : (String(":") + cmd + "#");
  char buf[64] = "";
  m_client->get(framed.c_str(), buf, sizeof(buf));
  sendAlpacaOk(s, r, m_parent->nextServerTransactionId(),
               AlpacaJson::escape(buf));
}


// ----------------------------------------------------------------------------
//  Read-only properties
// ----------------------------------------------------------------------------

void AlpacaTelescope::getRightAscension(AlpacaWebServer& s, const AlpacaRequest& r)
{
  if (!requireConnected(s, r)) return;
  m_status->updateAllState();
  sendAlpacaOk(s, r, m_parent->nextServerTransactionId(),
               AlpacaJson::doubleStr(m_status->getRaHoursCached()));
}

void AlpacaTelescope::getDeclination(AlpacaWebServer& s, const AlpacaRequest& r)
{
  if (!requireConnected(s, r)) return;
  m_status->updateAllState();
  sendAlpacaOk(s, r, m_parent->nextServerTransactionId(),
               AlpacaJson::doubleStr(m_status->getDecDegCached()));
}

void AlpacaTelescope::getAltitude(AlpacaWebServer& s, const AlpacaRequest& r)
{
  if (!requireConnected(s, r)) return;
  m_status->updateAllState();
  sendAlpacaOk(s, r, m_parent->nextServerTransactionId(),
               AlpacaJson::doubleStr(m_status->getAltDegCached()));
}

void AlpacaTelescope::getAzimuth(AlpacaWebServer& s, const AlpacaRequest& r)
{
  if (!requireConnected(s, r)) return;
  m_status->updateAllState();
  sendAlpacaOk(s, r, m_parent->nextServerTransactionId(),
               AlpacaJson::doubleStr(m_status->getAzDegCached()));
}

void AlpacaTelescope::getSiderealTime(AlpacaWebServer& s, const AlpacaRequest& r)
{
  if (!requireConnected(s, r)) return;
  m_status->updateAllState();
  double lst = m_status->getLstHoursCached();
  // The firmware computes LST using *its* SiteLongitude.  When the client
  // has supplied a different longitude via PUT /sitelongitude (which the
  // firmware silently rejects unless the mount is parked), shift the LST
  // by the difference so SiderealTime stays consistent with the longitude
  // we report.
  if (m_siteLonSet)
  {
    double fwLon = 0;
    if (isOk(m_client->getLongitude(fwLon)))
    {
      lst += (m_siteLon - fwLon) / 15.0;          // 15 deg / hour
      while (lst < 0)   lst += 24.0;
      while (lst >= 24) lst -= 24.0;
    }
  }
  sendAlpacaOk(s, r, m_parent->nextServerTransactionId(),
               AlpacaJson::doubleStr(lst));
}

void AlpacaTelescope::getSlewing(AlpacaWebServer& s, const AlpacaRequest& r)
{
  if (!requireConnected(s, r)) return;
  m_status->updateAllState();
  unsigned long now = millis();
  bool moveAxis    = m_moveAxisActive[0] || m_moveAxisActive[1];
  bool slewKickAct = (m_slewKickEndMs != 0) && (long)(m_slewKickEndMs - now) > 0;
  // The mount's `gotoKind` field describes the *kind* of last goto and
  // can linger across syncs — use TRK_SLEWING (the firmware's own
  // "currently slewing" flag) as the authoritative source.
  bool fromMount   = (m_status->getTrackingState() == MountState::TRK_SLEWING);
  bool slewing = moveAxis || slewKickAct || fromMount;
  sendAlpacaOk(s, r, m_parent->nextServerTransactionId(),
               AlpacaJson::boolStr(slewing));
}

void AlpacaTelescope::getAtPark(AlpacaWebServer& s, const AlpacaRequest& r)
{
  if (!requireConnected(s, r)) return;
  m_status->updateAllState();
  sendAlpacaOk(s, r, m_parent->nextServerTransactionId(),
               AlpacaJson::boolStr(m_status->Parked()));
}

void AlpacaTelescope::getAtHome(AlpacaWebServer& s, const AlpacaRequest& r)
{
  if (!requireConnected(s, r)) return;
  m_status->updateAllState();
  sendAlpacaOk(s, r, m_parent->nextServerTransactionId(),
               AlpacaJson::boolStr(m_status->atHome()));
}

void AlpacaTelescope::getIsPulseGuiding(AlpacaWebServer& s, const AlpacaRequest& r)
{
  if (!requireConnected(s, r)) return;
  unsigned long now = millis();
  bool softActive = (m_pulseGuideEndMs != 0) && ((long)(m_pulseGuideEndMs - now) > 0);
  bool fromMount = false;
  if (m_status)
  {
    m_status->updateAllState();
    fromMount = m_status->isPulseGuiding();
  }
  sendAlpacaOk(s, r, m_parent->nextServerTransactionId(),
               AlpacaJson::boolStr(softActive || fromMount));
}

void AlpacaTelescope::getSideOfPier(AlpacaWebServer& s, const AlpacaRequest& r)
{
  if (!requireConnected(s, r)) return;
  m_status->updateAllState();
  // ASCOM SideOfPier reports the *physical* side of the pier the OTA is on:
  //   0 = pierEast (OTA on east, looking west, hour angle +0..+12)
  //   1 = pierWest (OTA on west, looking east, hour angle -12..0)
  //  -1 = pierUnknown
  // TeenAstro's PIER_E/PIER_W tracks the same convention.
  int side = -1;
  switch (m_status->getPierState())
  {
    case MountState::PIER_E: side = 0; break;
    case MountState::PIER_W: side = 1; break;
    default: break;
  }
  sendAlpacaOk(s, r, m_parent->nextServerTransactionId(),
               AlpacaJson::intStr(side));
}

void AlpacaTelescope::putSideOfPier(AlpacaWebServer& s, const AlpacaRequest& r)
{
  // CanSetPierSide is false; per spec, set must throw NotImplemented.
  sendAlpacaError(s, r, m_parent->nextServerTransactionId(),
                  AE_NOT_IMPLEMENTED, "SideOfPier set not implemented");
}

void AlpacaTelescope::getDestinationSideOfPier(AlpacaWebServer& s, const AlpacaRequest& r)
{
  if (!requireConnected(s, r)) return;
  m_status->updateAllState();
  // Compute pier side a goto to (RightAscension, Declination) would end up
  // on, using the simple ASCOM convention based on hour angle:
  //   HA in [-6, +6]  => pierWest (1)
  //   HA in (+6, -6) => pierEast (0)
  double ra  = AlpacaRequest::arg(s, "RightAscension").toDouble();
  double lst = m_status->getLstHoursCached();
  if (m_siteLonSet)
  {
    double fwLon = 0;
    if (isOk(m_client->getLongitude(fwLon))) lst += (m_siteLon - fwLon) / 15.0;
  }
  double ha = lst - ra;
  while (ha < -12) ha += 24;
  while (ha >  12) ha -= 24;
  int side = (ha >= -6 && ha <= 6) ? 1 : 0;
  sendAlpacaOk(s, r, m_parent->nextServerTransactionId(),
               AlpacaJson::intStr(side));
}

void AlpacaTelescope::getAlignmentMode(AlpacaWebServer& s, const AlpacaRequest& r)
{
  if (!requireConnected(s, r)) return;
  m_status->updateMount();
  // Alpaca: 0=AltAz, 1=Polar, 2=GermanPolar
  int mode = 1;
  switch (m_status->getMount())
  {
    case MountState::MOUNT_TYPE_GEM:        mode = 2; break;
    case MountState::MOUNT_TYPE_FORK:       mode = 1; break;
    case MountState::MOUNT_TYPE_ALTAZM:     mode = 0; break;
    case MountState::MOUNT_TYPE_FORK_ALT:   mode = 0; break;
    default: break;
  }
  sendAlpacaOk(s, r, m_parent->nextServerTransactionId(),
               AlpacaJson::intStr(mode));
}

void AlpacaTelescope::getEquatorialSystem(AlpacaWebServer& s, const AlpacaRequest& r)
{
  if (!requireConnected(s, r)) return;
  // Alpaca: 0=Other, 1=Topocentric (JNow / equinox-of-date), 2=J2000, 3=J2050, 4=B1950
  // TeenAstro returns equinox-of-date coordinates.
  sendAlpacaOk(s, r, m_parent->nextServerTransactionId(), AlpacaJson::intStr(1));
}

void AlpacaTelescope::getApertureArea(AlpacaWebServer& s, const AlpacaRequest& r)
{
  // Optical configuration is unknown to the mount controller; per the
  // ASCOM Telescope spec, properties whose value cannot be supplied must
  // throw PropertyOrMethodNotImplementedException (Alpaca 0x400).
  sendAlpacaError(s, r, m_parent->nextServerTransactionId(),
                  AE_NOT_IMPLEMENTED, "ApertureArea is not implemented");
}

void AlpacaTelescope::getApertureDiameter(AlpacaWebServer& s, const AlpacaRequest& r)
{
  sendAlpacaError(s, r, m_parent->nextServerTransactionId(),
                  AE_NOT_IMPLEMENTED, "ApertureDiameter is not implemented");
}

void AlpacaTelescope::getFocalLength(AlpacaWebServer& s, const AlpacaRequest& r)
{
  sendAlpacaError(s, r, m_parent->nextServerTransactionId(),
                  AE_NOT_IMPLEMENTED, "FocalLength is not implemented");
}

void AlpacaTelescope::getDoesRefraction(AlpacaWebServer& s, const AlpacaRequest& r)
{
  if (!requireConnected(s, r)) return;
  m_status->updateAllConfig();
  sendAlpacaOk(s, r, m_parent->nextServerTransactionId(),
               AlpacaJson::boolStr(m_status->getCfgRefrTracking()));
}

void AlpacaTelescope::putDoesRefraction(AlpacaWebServer& s, const AlpacaRequest& r)
{
  if (!requireConnected(s, r)) return;
  String v = AlpacaRequest::arg(s, "DoesRefraction");
  v.toLowerCase();
  bool on = (v == "true" || v == "1");
  m_client->enableRefraction(on);
  sendAlpacaVoid(s, r, m_parent->nextServerTransactionId());
}


// ----------------------------------------------------------------------------
//  Target
// ----------------------------------------------------------------------------

void AlpacaTelescope::getTargetRightAscension(AlpacaWebServer& s, const AlpacaRequest& r)
{
  if (!requireConnected(s, r)) return;
  if (!m_targetRaSet)
  {
    sendAlpacaError(s, r, m_parent->nextServerTransactionId(),
                    AE_VALUE_NOT_SET, "TargetRightAscension not set");
    return;
  }
  sendAlpacaOk(s, r, m_parent->nextServerTransactionId(),
               AlpacaJson::doubleStr(m_targetRaHours));
}

void AlpacaTelescope::putTargetRightAscension(AlpacaWebServer& s, const AlpacaRequest& r)
{
  if (!requireConnected(s, r)) return;
  String v = AlpacaRequest::arg(s, "TargetRightAscension");
  double ra = v.toDouble();
  if (ra < 0.0 || ra >= 24.0)
  {
    sendAlpacaError(s, r, m_parent->nextServerTransactionId(),
                    AE_INVALID_VALUE, "TargetRightAscension out of range");
    return;
  }
  // Push to firmware as well — :SrLDDD.DDDDD# (degrees, no comma after L,
  // matches Command_S.cpp which strtod()'s starting at command[3]).
  char cmd[40];
  snprintf(cmd, sizeof(cmd), ":SrL%.5f#", 15.0 * ra);
  m_client->set(cmd);
  m_targetRaHours = ra;
  m_targetRaSet = true;
  sendAlpacaVoid(s, r, m_parent->nextServerTransactionId());
}

void AlpacaTelescope::getTargetDeclination(AlpacaWebServer& s, const AlpacaRequest& r)
{
  if (!requireConnected(s, r)) return;
  if (!m_targetDecSet)
  {
    sendAlpacaError(s, r, m_parent->nextServerTransactionId(),
                    AE_VALUE_NOT_SET, "TargetDeclination not set");
    return;
  }
  sendAlpacaOk(s, r, m_parent->nextServerTransactionId(),
               AlpacaJson::doubleStr(m_targetDecDeg));
}

void AlpacaTelescope::putTargetDeclination(AlpacaWebServer& s, const AlpacaRequest& r)
{
  if (!requireConnected(s, r)) return;
  String v = AlpacaRequest::arg(s, "TargetDeclination");
  double dec = v.toDouble();
  if (dec < -90.0 || dec > 90.0)
  {
    sendAlpacaError(s, r, m_parent->nextServerTransactionId(),
                    AE_INVALID_VALUE, "TargetDeclination out of range");
    return;
  }
  // :SdLsVV.VVVVV#  (sign-prefixed degrees, no comma)
  char cmd[40];
  const char* sign = (dec >= 0) ? "+" : "-";
  double abs_dec = (dec >= 0) ? dec : -dec;
  snprintf(cmd, sizeof(cmd), ":SdL%s%.5f#", sign, abs_dec);
  m_client->set(cmd);
  m_targetDecDeg = dec;
  m_targetDecSet = true;
  sendAlpacaVoid(s, r, m_parent->nextServerTransactionId());
}


// ----------------------------------------------------------------------------
//  Tracking
// ----------------------------------------------------------------------------

void AlpacaTelescope::getTracking(AlpacaWebServer& s, const AlpacaRequest& r)
{
  if (!requireConnected(s, r)) return;
  m_status->updateMount();
  bool on = (m_status->getTrackingState() == MountState::TRK_ON);
  sendAlpacaOk(s, r, m_parent->nextServerTransactionId(),
               AlpacaJson::boolStr(on));
}

void AlpacaTelescope::putTracking(AlpacaWebServer& s, const AlpacaRequest& r)
{
  if (!requireConnected(s, r)) return;
  String v = AlpacaRequest::arg(s, "Tracking");
  v.toLowerCase();
  bool on = (v == "true" || v == "1");
  m_client->enableTracking(on);
  sendAlpacaVoid(s, r, m_parent->nextServerTransactionId());
}

void AlpacaTelescope::getTrackingRate(AlpacaWebServer& s, const AlpacaRequest& r)
{
  if (!requireConnected(s, r)) return;
  m_status->updateMount();
  // Map TeenAstro sidereal mode -> Alpaca DriveRates: 0=Sidereal, 1=Lunar, 2=Solar, 3=King
  int rate = m_trackingRate;
  switch (m_status->getSiderealMode())
  {
    case MountState::SID_STAR:   rate = 0; break;
    case MountState::SID_MOON:   rate = 1; break;
    case MountState::SID_SUN:    rate = 2; break;
    default: break;
  }
  m_trackingRate = rate;
  sendAlpacaOk(s, r, m_parent->nextServerTransactionId(),
               AlpacaJson::intStr(rate));
}

void AlpacaTelescope::putTrackingRate(AlpacaWebServer& s, const AlpacaRequest& r)
{
  if (!requireConnected(s, r)) return;
  String v = AlpacaRequest::arg(s, "TrackingRate");
  int rate = v.toInt();
  switch (rate)
  {
    case 0: m_client->setTrackRateSidereal(); break;
    case 1: m_client->setTrackRateLunar();    break;
    case 2: m_client->setTrackRateSolar();    break;
    case 3: m_client->setTrackRateSidereal(); break;  // King ≈ sidereal on TeenAstro
    default:
      sendAlpacaError(s, r, m_parent->nextServerTransactionId(),
                      AE_INVALID_VALUE, "Unsupported TrackingRate");
      return;
  }
  m_trackingRate = rate;
  sendAlpacaVoid(s, r, m_parent->nextServerTransactionId());
}

void AlpacaTelescope::getTrackingRates(AlpacaWebServer& s, const AlpacaRequest& r)
{
  if (!requireConnected(s, r)) return;
  // Sidereal, Lunar, Solar — King omitted (mapped to sidereal on the mount).
  sendAlpacaOk(s, r, m_parent->nextServerTransactionId(), F("[0,1,2]"));
}

void AlpacaTelescope::getRightAscensionRate(AlpacaWebServer& s, const AlpacaRequest& r)
{
  if (!requireConnected(s, r)) return;
  sendAlpacaOk(s, r, m_parent->nextServerTransactionId(),
               AlpacaJson::doubleStr(m_raRate));
}

void AlpacaTelescope::putRightAscensionRate(AlpacaWebServer& s, const AlpacaRequest& r)
{
  // Capability is reported false; honour the spec by returning NotImplemented.
  sendAlpacaError(s, r, m_parent->nextServerTransactionId(),
                  AE_NOT_IMPLEMENTED, "RightAscensionRate is read-only");
}

void AlpacaTelescope::getDeclinationRate(AlpacaWebServer& s, const AlpacaRequest& r)
{
  if (!requireConnected(s, r)) return;
  sendAlpacaOk(s, r, m_parent->nextServerTransactionId(),
               AlpacaJson::doubleStr(m_decRate));
}

void AlpacaTelescope::putDeclinationRate(AlpacaWebServer& s, const AlpacaRequest& r)
{
  sendAlpacaError(s, r, m_parent->nextServerTransactionId(),
                  AE_NOT_IMPLEMENTED, "DeclinationRate is read-only");
}

void AlpacaTelescope::getGuideRateRA(AlpacaWebServer& s, const AlpacaRequest& r)
{
  if (!requireConnected(s, r)) return;
  m_status->updateAllConfig();
  // TeenAstro guide rate is stored as multiples of sidereal; report deg/sec.
  // 1× sidereal ≈ 0.004178°/s.
  double rateDegPerSec = m_status->getCfgGuideRate() * 0.0041781;
  sendAlpacaOk(s, r, m_parent->nextServerTransactionId(),
               AlpacaJson::doubleStr(rateDegPerSec));
}

void AlpacaTelescope::putGuideRateRA(AlpacaWebServer& s, const AlpacaRequest& r)
{
  sendAlpacaError(s, r, m_parent->nextServerTransactionId(),
                  AE_NOT_IMPLEMENTED, "GuideRateRightAscension is read-only");
}

void AlpacaTelescope::getGuideRateDec(AlpacaWebServer& s, const AlpacaRequest& r)
{
  // TeenAstro shares one guide rate for both axes.
  getGuideRateRA(s, r);
}

void AlpacaTelescope::putGuideRateDec(AlpacaWebServer& s, const AlpacaRequest& r)
{
  sendAlpacaError(s, r, m_parent->nextServerTransactionId(),
                  AE_NOT_IMPLEMENTED, "GuideRateDeclination is read-only");
}

void AlpacaTelescope::getSlewSettleTime(AlpacaWebServer& s, const AlpacaRequest& r)
{
  if (!requireConnected(s, r)) return;
  sendAlpacaOk(s, r, m_parent->nextServerTransactionId(),
               AlpacaJson::intStr(m_slewSettleSec));
}

void AlpacaTelescope::putSlewSettleTime(AlpacaWebServer& s, const AlpacaRequest& r)
{
  if (!requireConnected(s, r)) return;
  String v = AlpacaRequest::arg(s, "SlewSettleTime");
  long t = v.toInt();
  if (t < 0 || t > 100)
  {
    sendAlpacaError(s, r, m_parent->nextServerTransactionId(),
                    AE_INVALID_VALUE, "SlewSettleTime out of range");
    return;
  }
  m_slewSettleSec = (uint16_t)t;
  sendAlpacaVoid(s, r, m_parent->nextServerTransactionId());
}


// ----------------------------------------------------------------------------
//  Site
// ----------------------------------------------------------------------------

void AlpacaTelescope::getSiteLatitude(AlpacaWebServer& s, const AlpacaRequest& r)
{
  if (!requireConnected(s, r)) return;
  // Prefer the cached client write when present; the firmware silently
  // rejects :St while not parked so we cannot rely on a fresh read for
  // round-trip of a value the client just set.
  if (m_siteLatSet)
  {
    sendAlpacaOk(s, r, m_parent->nextServerTransactionId(),
                 AlpacaJson::doubleStr(m_siteLat));
    return;
  }
  double lat = 0;
  if (!isOk(m_client->getLatitude(lat)))
  {
    sendAlpacaError(s, r, m_parent->nextServerTransactionId(),
                    AE_DRIVER, "Failed to read latitude");
    return;
  }
  sendAlpacaOk(s, r, m_parent->nextServerTransactionId(),
               AlpacaJson::doubleStr(lat));
}

void AlpacaTelescope::putSiteLatitude(AlpacaWebServer& s, const AlpacaRequest& r)
{
  if (!requireConnected(s, r)) return;
  String v = AlpacaRequest::arg(s, "SiteLatitude");
  double lat = v.toDouble();
  if (lat < -90 || lat > 90)
  {
    sendAlpacaError(s, r, m_parent->nextServerTransactionId(),
                    AE_INVALID_VALUE, "SiteLatitude out of range");
    return;
  }
  int sign = (lat < 0) ? 1 : 0;
  double a = (lat < 0) ? -lat : lat;
  int deg = (int)a;
  double mfrac = (a - deg) * 60.0;
  int mn = (int)mfrac;
  int sc = (int)((mfrac - mn) * 60.0 + 0.5);
  m_client->setLatitudeDMS(sign, deg, mn, sc);   // best-effort firmware push
  m_siteLat    = lat;
  m_siteLatSet = true;
  sendAlpacaVoid(s, r, m_parent->nextServerTransactionId());
}

void AlpacaTelescope::getSiteLongitude(AlpacaWebServer& s, const AlpacaRequest& r)
{
  if (!requireConnected(s, r)) return;
  if (m_siteLonSet)
  {
    sendAlpacaOk(s, r, m_parent->nextServerTransactionId(),
                 AlpacaJson::doubleStr(m_siteLon));
    return;
  }
  double lon = 0;
  if (!isOk(m_client->getLongitude(lon)))
  {
    sendAlpacaError(s, r, m_parent->nextServerTransactionId(),
                    AE_DRIVER, "Failed to read longitude");
    return;
  }
  sendAlpacaOk(s, r, m_parent->nextServerTransactionId(),
               AlpacaJson::doubleStr(lon));
}

void AlpacaTelescope::putSiteLongitude(AlpacaWebServer& s, const AlpacaRequest& r)
{
  if (!requireConnected(s, r)) return;
  String v = AlpacaRequest::arg(s, "SiteLongitude");
  double lon = v.toDouble();
  if (lon < -180 || lon > 180)
  {
    sendAlpacaError(s, r, m_parent->nextServerTransactionId(),
                    AE_INVALID_VALUE, "SiteLongitude out of range");
    return;
  }
  int sign = (lon < 0) ? 1 : 0;
  double a = (lon < 0) ? -lon : lon;
  int deg = (int)a;
  double mfrac = (a - deg) * 60.0;
  int mn = (int)mfrac;
  int sc = (int)((mfrac - mn) * 60.0 + 0.5);
  m_client->setLongitudeDMS(sign, deg, mn, sc);  // best-effort firmware push
  m_siteLon    = lon;
  m_siteLonSet = true;
  sendAlpacaVoid(s, r, m_parent->nextServerTransactionId());
}

void AlpacaTelescope::getSiteElevation(AlpacaWebServer& s, const AlpacaRequest& r)
{
  if (!requireConnected(s, r)) return;
  int meters = 0;
  if (!isOk(m_client->getElevationMeters(meters))) meters = 0;
  sendAlpacaOk(s, r, m_parent->nextServerTransactionId(),
               AlpacaJson::doubleStr((double)meters));
}

void AlpacaTelescope::putSiteElevation(AlpacaWebServer& s, const AlpacaRequest& r)
{
  if (!requireConnected(s, r)) return;
  String v = AlpacaRequest::arg(s, "SiteElevation");
  int meters = (int)v.toInt();
  if (meters < -300 || meters > 10000)
  {
    sendAlpacaError(s, r, m_parent->nextServerTransactionId(),
                    AE_INVALID_VALUE, "SiteElevation out of range");
    return;
  }
  m_client->setElevation(meters);
  sendAlpacaVoid(s, r, m_parent->nextServerTransactionId());
}

void AlpacaTelescope::getUtcDate(AlpacaWebServer& s, const AlpacaRequest& r)
{
  if (!requireConnected(s, r)) return;
  m_status->updateAllState();
  // ISO-8601 ZULU (Alpaca format example: 2026-05-02T16:30:00.0000000Z)
  char buf[40];
  int yy = 2000 + m_status->getUtcYear();
  snprintf(buf, sizeof(buf),
           "%04d-%02u-%02uT%02u:%02u:%02u.0000000Z",
           yy,
           (unsigned)m_status->getUtcMonth(),
           (unsigned)m_status->getUtcDay(),
           (unsigned)m_status->getUtcHour(),
           (unsigned)m_status->getUtcMin(),
           (unsigned)m_status->getUtcSec());
  sendAlpacaOk(s, r, m_parent->nextServerTransactionId(),
               AlpacaJson::escape(buf));
}

void AlpacaTelescope::putUtcDate(AlpacaWebServer& s, const AlpacaRequest& r)
{
  if (!requireConnected(s, r)) return;
  String v = AlpacaRequest::arg(s, "UTCDate");
  // Accept "YYYY-MM-DDTHH:MM:SS[.fff[Z]]"
  if (v.length() < 19)
  {
    sendAlpacaError(s, r, m_parent->nextServerTransactionId(),
                    AE_INVALID_VALUE, "UTCDate must be ISO-8601");
    return;
  }
  int year  = v.substring(0, 4).toInt();
  int month = v.substring(5, 7).toInt();
  int day   = v.substring(8, 10).toInt();
  int hour  = v.substring(11, 13).toInt();
  int mn    = v.substring(14, 16).toInt();
  int sc    = v.substring(17, 19).toInt();
  m_client->setUTCDateRaw(month, day, year % 100);
  m_client->setUTCTimeRaw(hour, mn, sc);
  sendAlpacaVoid(s, r, m_parent->nextServerTransactionId());
}


// ----------------------------------------------------------------------------
//  Capabilities
// ----------------------------------------------------------------------------

void AlpacaTelescope::getCanFindHome(AlpacaWebServer& s, const AlpacaRequest& r)             { sendAlpacaOk(s, r, m_parent->nextServerTransactionId(), F("true")); }
void AlpacaTelescope::getCanPark(AlpacaWebServer& s, const AlpacaRequest& r)                 { sendAlpacaOk(s, r, m_parent->nextServerTransactionId(), F("true")); }
void AlpacaTelescope::getCanUnpark(AlpacaWebServer& s, const AlpacaRequest& r)               { sendAlpacaOk(s, r, m_parent->nextServerTransactionId(), F("true")); }
void AlpacaTelescope::getCanSetPark(AlpacaWebServer& s, const AlpacaRequest& r)              { sendAlpacaOk(s, r, m_parent->nextServerTransactionId(), F("true")); }
void AlpacaTelescope::getCanSetTracking(AlpacaWebServer& s, const AlpacaRequest& r)          { sendAlpacaOk(s, r, m_parent->nextServerTransactionId(), F("true")); }
void AlpacaTelescope::getCanSlew(AlpacaWebServer& s, const AlpacaRequest& r)                 { sendAlpacaOk(s, r, m_parent->nextServerTransactionId(), F("true")); }
void AlpacaTelescope::getCanSlewAsync(AlpacaWebServer& s, const AlpacaRequest& r)            { sendAlpacaOk(s, r, m_parent->nextServerTransactionId(), F("true")); }
void AlpacaTelescope::getCanSync(AlpacaWebServer& s, const AlpacaRequest& r)                 { sendAlpacaOk(s, r, m_parent->nextServerTransactionId(), F("true")); }
void AlpacaTelescope::getCanPulseGuide(AlpacaWebServer& s, const AlpacaRequest& r)           { sendAlpacaOk(s, r, m_parent->nextServerTransactionId(), F("true")); }
void AlpacaTelescope::getCanSetGuideRates(AlpacaWebServer& s, const AlpacaRequest& r)        { sendAlpacaOk(s, r, m_parent->nextServerTransactionId(), F("false")); }
void AlpacaTelescope::getCanSetRightAscensionRate(AlpacaWebServer& s, const AlpacaRequest& r){ sendAlpacaOk(s, r, m_parent->nextServerTransactionId(), F("false")); }
void AlpacaTelescope::getCanSetDeclinationRate(AlpacaWebServer& s, const AlpacaRequest& r)   { sendAlpacaOk(s, r, m_parent->nextServerTransactionId(), F("false")); }
void AlpacaTelescope::getCanSetPierSide(AlpacaWebServer& s, const AlpacaRequest& r)          { sendAlpacaOk(s, r, m_parent->nextServerTransactionId(), F("false")); }

void AlpacaTelescope::getCanSlewAltAz(AlpacaWebServer& s, const AlpacaRequest& r)
{
  if (!requireConnected(s, r)) return;
  m_status->updateMount();
  sendAlpacaOk(s, r, m_parent->nextServerTransactionId(),
               AlpacaJson::boolStr(m_status->isAltAz()));
}

void AlpacaTelescope::getCanSlewAltAzAsync(AlpacaWebServer& s, const AlpacaRequest& r)
{
  getCanSlewAltAz(s, r);
}

void AlpacaTelescope::getCanSyncAltAz(AlpacaWebServer& s, const AlpacaRequest& r)
{
  getCanSlewAltAz(s, r);
}

void AlpacaTelescope::getCanMoveAxisQ(AlpacaWebServer& s, const AlpacaRequest& r)
{
  // CanMoveAxis takes Axis=0/1/2 query parameter and returns bool.
  if (!requireConnected(s, r)) return;
  String a = AlpacaRequest::arg(s, "Axis");
  int axis = a.toInt();
  bool can = (axis == 0 || axis == 1);
  sendAlpacaOk(s, r, m_parent->nextServerTransactionId(),
               AlpacaJson::boolStr(can));
}


// ----------------------------------------------------------------------------
//  Methods
// ----------------------------------------------------------------------------

void AlpacaTelescope::putAbortSlew(AlpacaWebServer& s, const AlpacaRequest& r)
{
  if (!requireConnected(s, r)) return;
  m_status->updateAllState();
  if (m_status->Parked())
  {
    sendAlpacaError(s, r, m_parent->nextServerTransactionId(),
                    AE_PARKED, "Cannot AbortSlew while parked");
    return;
  }
  m_client->stopSlew();
  // Drop any MoveAxis manual-jog as well so Slewing reflects reality.
  if (m_moveAxisActive[0])
  {
    m_client->stopMoveEast(); m_client->stopMoveWest();
    m_moveAxisActive[0] = false;
  }
  if (m_moveAxisActive[1])
  {
    m_client->stopMoveNorth(); m_client->stopMoveSouth();
    m_moveAxisActive[1] = false;
  }
  m_slewKickEndMs = 0;
  sendAlpacaVoid(s, r, m_parent->nextServerTransactionId());
}

void AlpacaTelescope::putFindHome(AlpacaWebServer& s, const AlpacaRequest& r)
{
  if (!requireConnected(s, r)) return;
  LX200RETURN ret = m_client->homeGoto();
  if (!isOk(ret))
  {
    sendAlpacaError(s, r, m_parent->nextServerTransactionId(),
                    AE_DRIVER, "FindHome refused by mount");
    return;
  }
  sendAlpacaVoid(s, r, m_parent->nextServerTransactionId());
}

void AlpacaTelescope::putPark(AlpacaWebServer& s, const AlpacaRequest& r)
{
  if (!requireConnected(s, r)) return;
  // The mount refuses :hP# if no park position has ever been saved.
  // Auto-bootstrap by calling :hQ# (set the *current* axis position as the
  // park position) on the first attempt — matches the legacy ASCOM driver
  // behaviour and keeps ConformU happy on a freshly-booted controller.
  LX200RETURN ret = m_client->park();
  if (!isOk(ret))
  {
    m_client->setPark();
    ret = m_client->park();
  }
  if (!isOk(ret))
  {
    sendAlpacaError(s, r, m_parent->nextServerTransactionId(),
                    AE_DRIVER, "Park refused by mount");
    return;
  }
  m_slewKickEndMs = millis() + 750;  // Park goto -> looks like a slew
  sendAlpacaVoid(s, r, m_parent->nextServerTransactionId());
}

void AlpacaTelescope::putUnpark(AlpacaWebServer& s, const AlpacaRequest& r)
{
  if (!requireConnected(s, r)) return;
  LX200RETURN ret = m_client->unpark();
  if (!isOk(ret))
  {
    sendAlpacaError(s, r, m_parent->nextServerTransactionId(),
                    AE_DRIVER, "Unpark refused by mount");
    return;
  }
  sendAlpacaVoid(s, r, m_parent->nextServerTransactionId());
}

void AlpacaTelescope::putSetPark(AlpacaWebServer& s, const AlpacaRequest& r)
{
  if (!requireConnected(s, r)) return;
  LX200RETURN ret = m_client->setPark();
  if (!isOk(ret))
  {
    sendAlpacaError(s, r, m_parent->nextServerTransactionId(),
                    AE_DRIVER, "SetPark refused by mount");
    return;
  }
  sendAlpacaVoid(s, r, m_parent->nextServerTransactionId());
}


bool AlpacaTelescope::slewToRaDec(AlpacaWebServer& s, const AlpacaRequest& r,
                                  double raHours, double decDeg)
{
  if (m_status->Parked())
  {
    sendAlpacaError(s, r, m_parent->nextServerTransactionId(),
                    AE_PARKED, "Cannot slew while parked");
    return false;
  }
  float ra  = (float)raHours;
  float dec = (float)decDeg;
  LX200RETURN ret = m_client->syncGoto(NAV_GOTO, ra, dec);
  if (!isOk(ret))
  {
    sendAlpacaError(s, r, m_parent->nextServerTransactionId(),
                    AE_DRIVER, "Slew refused by mount");
    return false;
  }
  m_targetRaHours = raHours;
  m_targetDecDeg  = decDeg;
  m_targetRaSet   = true;
  m_targetDecSet  = true;
  // Force Slewing=true for the next ~750ms so async-slew clients see the
  // transition even before the mount cache has caught up.
  m_slewKickEndMs = millis() + 750;
  sendAlpacaVoid(s, r, m_parent->nextServerTransactionId());
  return true;
}

bool AlpacaTelescope::syncToRaDec(AlpacaWebServer& s, const AlpacaRequest& r,
                                  double raHours, double decDeg)
{
  float ra  = (float)raHours;
  float dec = (float)decDeg;
  LX200RETURN ret = m_client->syncGoto(NAV_SYNC, ra, dec);
  if (!isOk(ret))
  {
    sendAlpacaError(s, r, m_parent->nextServerTransactionId(),
                    AE_DRIVER, "Sync refused by mount");
    return false;
  }
  m_targetRaHours = raHours;
  m_targetDecDeg  = decDeg;
  m_targetRaSet   = true;
  m_targetDecSet  = true;
  sendAlpacaVoid(s, r, m_parent->nextServerTransactionId());
  return true;
}

bool AlpacaTelescope::slewSyncToAltAz(AlpacaWebServer& s, const AlpacaRequest& r,
                                      double azDeg, double altDeg, bool sync)
{
  if (!sync && m_status->Parked())
  {
    sendAlpacaError(s, r, m_parent->nextServerTransactionId(),
                    AE_PARKED, "Cannot slew while parked");
    return false;
  }
  float az  = (float)azDeg;
  float alt = (float)altDeg;
  LX200RETURN ret = m_client->syncGotoAltAz(sync ? NAV_SYNC : NAV_GOTO, az, alt);
  if (!isOk(ret))
  {
    sendAlpacaError(s, r, m_parent->nextServerTransactionId(),
                    AE_DRIVER, sync ? "AltAz sync refused" : "AltAz slew refused");
    return false;
  }
  if (!sync) m_slewKickEndMs = millis() + 750;
  sendAlpacaVoid(s, r, m_parent->nextServerTransactionId());
  return true;
}

void AlpacaTelescope::putSlewToCoordinates(AlpacaWebServer& s, const AlpacaRequest& r)
{
  if (!requireConnected(s, r)) return;
  double ra  = AlpacaRequest::arg(s, "RightAscension").toDouble();
  double dec = AlpacaRequest::arg(s, "Declination").toDouble();
  if (ra < 0 || ra >= 24 || dec < -90 || dec > 90)
  {
    sendAlpacaError(s, r, m_parent->nextServerTransactionId(),
                    AE_INVALID_VALUE, "RA/Dec out of range");
    return;
  }
  slewToRaDec(s, r, ra, dec);
}

void AlpacaTelescope::putSlewToCoordinatesAsync(AlpacaWebServer& s, const AlpacaRequest& r)
{
  putSlewToCoordinates(s, r);
}

void AlpacaTelescope::putSyncToCoordinates(AlpacaWebServer& s, const AlpacaRequest& r)
{
  if (!requireConnected(s, r)) return;
  double ra  = AlpacaRequest::arg(s, "RightAscension").toDouble();
  double dec = AlpacaRequest::arg(s, "Declination").toDouble();
  if (ra < 0 || ra >= 24 || dec < -90 || dec > 90)
  {
    sendAlpacaError(s, r, m_parent->nextServerTransactionId(),
                    AE_INVALID_VALUE, "RA/Dec out of range");
    return;
  }
  syncToRaDec(s, r, ra, dec);
}

void AlpacaTelescope::putSlewToTarget(AlpacaWebServer& s, const AlpacaRequest& r)
{
  if (!requireConnected(s, r)) return;
  if (!m_targetRaSet || !m_targetDecSet)
  {
    sendAlpacaError(s, r, m_parent->nextServerTransactionId(),
                    AE_VALUE_NOT_SET, "Target not set");
    return;
  }
  slewToRaDec(s, r, m_targetRaHours, m_targetDecDeg);
}

void AlpacaTelescope::putSlewToTargetAsync(AlpacaWebServer& s, const AlpacaRequest& r)
{
  putSlewToTarget(s, r);
}

void AlpacaTelescope::putSyncToTarget(AlpacaWebServer& s, const AlpacaRequest& r)
{
  if (!requireConnected(s, r)) return;
  if (!m_targetRaSet || !m_targetDecSet)
  {
    sendAlpacaError(s, r, m_parent->nextServerTransactionId(),
                    AE_VALUE_NOT_SET, "Target not set");
    return;
  }
  syncToRaDec(s, r, m_targetRaHours, m_targetDecDeg);
}

void AlpacaTelescope::putSlewToAltAz(AlpacaWebServer& s, const AlpacaRequest& r)
{
  if (!requireConnected(s, r)) return;
  // Honour CanSlewAltAz: AltAz slew is only supported on AltAz-style mounts.
  m_status->updateMount();
  if (!m_status->isAltAz())
  {
    sendAlpacaError(s, r, m_parent->nextServerTransactionId(),
                    AE_NOT_IMPLEMENTED, "SlewToAltAz not supported by this mount");
    return;
  }
  double az  = AlpacaRequest::arg(s, "Azimuth").toDouble();
  double alt = AlpacaRequest::arg(s, "Altitude").toDouble();
  if (az < 0 || az >= 360 || alt < -90 || alt > 90)
  {
    sendAlpacaError(s, r, m_parent->nextServerTransactionId(),
                    AE_INVALID_VALUE, "Az/Alt out of range");
    return;
  }
  slewSyncToAltAz(s, r, az, alt, false);
}

void AlpacaTelescope::putSlewToAltAzAsync(AlpacaWebServer& s, const AlpacaRequest& r)
{
  putSlewToAltAz(s, r);
}

void AlpacaTelescope::putSyncToAltAz(AlpacaWebServer& s, const AlpacaRequest& r)
{
  if (!requireConnected(s, r)) return;
  m_status->updateMount();
  if (!m_status->isAltAz())
  {
    sendAlpacaError(s, r, m_parent->nextServerTransactionId(),
                    AE_NOT_IMPLEMENTED, "SyncToAltAz not supported by this mount");
    return;
  }
  double az  = AlpacaRequest::arg(s, "Azimuth").toDouble();
  double alt = AlpacaRequest::arg(s, "Altitude").toDouble();
  if (az < 0 || az >= 360 || alt < -90 || alt > 90)
  {
    sendAlpacaError(s, r, m_parent->nextServerTransactionId(),
                    AE_INVALID_VALUE, "Az/Alt out of range");
    return;
  }
  slewSyncToAltAz(s, r, az, alt, true);
}

void AlpacaTelescope::putPulseGuide(AlpacaWebServer& s, const AlpacaRequest& r)
{
  if (!requireConnected(s, r)) return;
  int dir   = (int)AlpacaRequest::arg(s, "Direction").toInt();
  long durMs = AlpacaRequest::arg(s, "Duration").toInt();
  if (dir < 0 || dir > 3 || durMs < 1 || durMs > 30000)
  {
    sendAlpacaError(s, r, m_parent->nextServerTransactionId(),
                    AE_INVALID_VALUE, "PulseGuide arguments out of range");
    return;
  }
  m_status->updateAllState();
  if (m_status->Parked())
  {
    sendAlpacaError(s, r, m_parent->nextServerTransactionId(),
                    AE_PARKED, "Cannot PulseGuide while parked");
    return;
  }
  // Alpaca: 0=North, 1=South, 2=East, 3=West
  static const char dirChar[] = {'n', 's', 'e', 'w'};
  char cmd[24];
  snprintf(cmd, sizeof(cmd), ":Mg%c%ld#", dirChar[dir], durMs);
  m_client->set(cmd);
  // PulseGuide is async: IsPulseGuiding must report true for `durMs`.
  // The firmware exposes its own pulse-guide flag but updates it on a
  // poll cycle, which can lag behind the request — shadow the deadline
  // here so the very next IsPulseGuiding GET sees the active state.
  m_pulseGuideEndMs = millis() + (unsigned long)durMs;
  sendAlpacaVoid(s, r, m_parent->nextServerTransactionId());
}

void AlpacaTelescope::putMoveAxis(AlpacaWebServer& s, const AlpacaRequest& r)
{
  if (!requireConnected(s, r)) return;
  int axis = (int)AlpacaRequest::arg(s, "Axis").toInt();
  double rateDegSec = AlpacaRequest::arg(s, "Rate").toDouble();
  if (axis < 0 || axis > 1)
  {
    sendAlpacaError(s, r, m_parent->nextServerTransactionId(),
                    AE_INVALID_VALUE, "Axis must be 0 or 1");
    return;
  }
  m_status->updateAllState();
  if (m_status->Parked())
  {
    sendAlpacaError(s, r, m_parent->nextServerTransactionId(),
                    AE_PARKED, "Cannot MoveAxis while parked");
    return;
  }
  // Validate against the same range we publish via /axisrates so a client
  // that respects our advertised limits does not get its request silently
  // accepted with bogus values, and ConformU's "out-of-range" probes get
  // the InvalidValue error they expect.
  m_status->updateAllConfig();
  double minRate = m_status->getCfgGuideRate() * 0.0041781;
  double maxRate = m_status->getCfgFastRate()  * 0.0041781;
  if (maxRate <= 0) maxRate = 5.0;
  if (minRate <= 0) minRate = 0.0041781;
  double absRate = (rateDegSec < 0) ? -rateDegSec : rateDegSec;
  if (rateDegSec != 0.0 && (absRate < minRate || absRate > maxRate))
  {
    sendAlpacaError(s, r, m_parent->nextServerTransactionId(),
                    AE_INVALID_VALUE, "MoveAxis Rate out of range");
    return;
  }
  // Translate sign + magnitude into TeenAstro start/stop directives.
  // Axis 0 = primary (RA / Az → east/west), Axis 1 = secondary (Dec / Alt → north/south)
  if (rateDegSec == 0.0)
  {
    if (axis == 0) { m_client->stopMoveEast(); m_client->stopMoveWest(); }
    else           { m_client->stopMoveNorth(); m_client->stopMoveSouth(); }
    m_moveAxisActive[axis] = false;
    sendAlpacaVoid(s, r, m_parent->nextServerTransactionId());
    return;
  }
  // Pick a slew speed bucket roughly matching the requested rate.
  uint8_t lvl = 1;
  if      (absRate >= 4.0)  lvl = 4;
  else if (absRate >= 2.0)  lvl = 3;
  else if (absRate >= 1.0)  lvl = 2;
  else if (absRate >= 0.25) lvl = 1;
  else                       lvl = 0;
  m_client->setSpeed(lvl);
  if (axis == 0)
  {
    if (rateDegSec > 0) m_client->startMoveEast();
    else                m_client->startMoveWest();
  }
  else
  {
    if (rateDegSec > 0) m_client->startMoveNorth();
    else                m_client->startMoveSouth();
  }
  m_moveAxisActive[axis] = true;
  sendAlpacaVoid(s, r, m_parent->nextServerTransactionId());
}

void AlpacaTelescope::getAxisRates(AlpacaWebServer& s, const AlpacaRequest& r)
{
  if (!requireConnected(s, r)) return;
  String a = AlpacaRequest::arg(s, "Axis");
  int axis = a.toInt();
  if (axis < 0 || axis > 1)
  {
    sendAlpacaOk(s, r, m_parent->nextServerTransactionId(), F("[]"));
    return;
  }
  m_status->updateAllConfig();
  // Report a single contiguous range from the configured guide rate up to
  // the configured max rate (deg/sec).  Sidereal ≈ 0.004178 °/s.
  double minRate = m_status->getCfgGuideRate() * 0.0041781;
  double maxRate = m_status->getCfgFastRate()  * 0.0041781;
  if (maxRate <= 0) maxRate = 5.0;          // sensible fallback
  if (minRate <= 0) minRate = 0.0041781;
  String body = "[{\"Minimum\":";
  body += AlpacaJson::doubleStr(minRate);
  body += F(",\"Maximum\":");
  body += AlpacaJson::doubleStr(maxRate);
  body += '}';
  body += ']';
  sendAlpacaOk(s, r, m_parent->nextServerTransactionId(), body);
}

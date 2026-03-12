/*
 * TeenAstroAscomNative.cpp - C API implementation for ASCOM P/Invoke
 *
 * Copyright (C) 2024 TeenAstro by Charles Lemaire
 * GNU General Public License v3
 */
#define TEENASTROASCOMNATIVE_EXPORTS
#include "TeenAstroAscomNative.h"
#include "Win32Transport.h"
#include "LX200Client.h"
#include "TeenAstroMountStatus.h"
#include <stdlib.h>
#include <string.h>

// Context for each connection
struct AscomContext
{
  Win32SerialTransport* serial;
  WinSockTransport* tcp;
  IX200Transport* transport;
  LX200Client* client;
  TeenAstroMountStatus* mountStatus;

  AscomContext() : serial(nullptr), tcp(nullptr), transport(nullptr), client(nullptr), mountStatus(nullptr) {}
};

static void buildCommand(const char* cmd, int raw, char* out, int outSize)
{
  if (raw)
  {
    strncpy(out, cmd, outSize - 1);
    out[outSize - 1] = '\0';
  }
  else
  {
    snprintf(out, outSize, ":%s#", cmd);
  }
}

extern "C" {

TA_ASCOM_API TeenAstroAscom_Handle TeenAstroAscom_ConnectSerial(const char* port)
{
  if (!port || !port[0]) return nullptr;
  AscomContext* ctx = new AscomContext();
  ctx->serial = new Win32SerialTransport();
  if (!ctx->serial->open(port, 57600))
  {
    delete ctx->serial;
    delete ctx;
    return nullptr;
  }
  ctx->transport = ctx->serial;
  ctx->client = new LX200Client(*ctx->transport, 30);
  ctx->mountStatus = new TeenAstroMountStatus();
  ctx->mountStatus->setClient(*ctx->client);
  return ctx;
}

TA_ASCOM_API TeenAstroAscom_Handle TeenAstroAscom_ConnectTcp(const char* ip, int port)
{
  if (!ip || !ip[0] || port <= 0) return nullptr;
  AscomContext* ctx = new AscomContext();
  ctx->tcp = new WinSockTransport();
  if (!ctx->tcp->open(ip, port))
  {
    delete ctx->tcp;
    delete ctx;
    return nullptr;
  }
  ctx->transport = ctx->tcp;
  ctx->client = new LX200Client(*ctx->transport, 30);
  ctx->mountStatus = new TeenAstroMountStatus();
  ctx->mountStatus->setClient(*ctx->client);
  return ctx;
}

TA_ASCOM_API void TeenAstroAscom_Disconnect(TeenAstroAscom_Handle h)
{
  if (!h) return;
  AscomContext* ctx = (AscomContext*)h;
  delete ctx->mountStatus;
  delete ctx->client;
  if (ctx->serial) { ctx->serial->close(); delete ctx->serial; }
  if (ctx->tcp) { ctx->tcp->close(); delete ctx->tcp; }
  delete ctx;
}

TA_ASCOM_API int TeenAstroAscom_CommandBlind(TeenAstroAscom_Handle h, const char* cmd, int raw)
{
  if (!h || !cmd) return 0;
  AscomContext* ctx = (AscomContext*)h;
  char framed[256];
  buildCommand(cmd, raw, framed, sizeof(framed));
  char out[20];
  bool ok = ctx->client->sendReceive(framed, CMDR_NO, out, sizeof(out), 100);
  return ok ? 1 : 0;
}

TA_ASCOM_API int TeenAstroAscom_CommandBool(TeenAstroAscom_Handle h, const char* cmd, int raw)
{
  if (!h || !cmd) return 0;
  AscomContext* ctx = (AscomContext*)h;
  char framed[256];
  buildCommand(cmd, raw, framed, sizeof(framed));
  char out[20];
  CMDREPLY reply = getReplyType(framed);
  bool ok = ctx->client->sendReceive(framed, reply == CMDR_INVALID ? CMDR_SHORT_BOOL : reply,
                                      out, sizeof(out), 100);
  if (!ok) return 0;
  return (out[0] == '1') ? 1 : 0;
}

TA_ASCOM_API int TeenAstroAscom_CommandString(TeenAstroAscom_Handle h, const char* cmd, int raw,
                                               char* outBuf, int outBufSize)
{
  if (!h || !cmd || !outBuf || outBufSize < 1) return 0;
  AscomContext* ctx = (AscomContext*)h;
  char framed[256];
  buildCommand(cmd, raw, framed, sizeof(framed));
  outBuf[0] = '\0';
  LX200RETURN ret = ctx->client->get(framed, outBuf, outBufSize);
  return (ret == LX200_VALUEGET) ? 1 : 0;
}

TA_ASCOM_API int TeenAstroAscom_GetMountState(TeenAstroAscom_Handle h, TeenAstroAscom_GXASState* out)
{
  if (!h || !out) return 0;
  memset(out, 0, sizeof(TeenAstroAscom_GXASState));
  AscomContext* ctx = (AscomContext*)h;
  ctx->mountStatus->updateAllState(true);
  const MountState& m = ctx->mountStatus->mountState();
  if (!m.valid) return 0;
  out->valid = 1;
  out->rightAscensionHours = ctx->mountStatus->getRaHoursCached();
  out->declinationDegrees = ctx->mountStatus->getDecDegCached();
  out->altitudeDegrees = ctx->mountStatus->getAltDegCached();
  out->azimuthDegrees = ctx->mountStatus->getAzDegCached();
  out->targetRAHours = ctx->mountStatus->getTargetRaHoursCached();
  out->targetDecDegrees = ctx->mountStatus->getTargetDecDegCached();
  out->siderealTimeHours = ctx->mountStatus->getLstHoursCached();
  out->tracking = (m.tracking == MountState::TRK_ON) ? 1 : 0;
  out->slewing = (m.tracking == MountState::TRK_SLEWING) ? 1 : 0;
  out->atHome = m.atHome ? 1 : 0;
  out->parkState = (int)m.parkState;
  out->pierSideWest = (m.pierSide == MountState::PIER_W) ? 1 : 0;
  out->isPulseGuiding = m.pulseGuiding ? 1 : 0;
  out->utcYear = ctx->mountStatus->getUtcYear() + 2000;
  out->utcMonth = ctx->mountStatus->getUtcMonth();
  out->utcDay = ctx->mountStatus->getUtcDay();
  out->utcHour = ctx->mountStatus->getUtcHour();
  out->utcMin = ctx->mountStatus->getUtcMin();
  out->utcSec = ctx->mountStatus->getUtcSec();
  return 1;
}

// Semantic API - protocol strings stay in C++ layer only
TA_ASCOM_API int TeenAstroAscom_AbortSlew(TeenAstroAscom_Handle h)
{
  if (!h) return 0;
  AscomContext* ctx = (AscomContext*)h;
  LX200RETURN ret = ctx->client->stopSlew();
  return (ret == LX200_VALUEGET || ret == LX200_OK) ? 1 : 0;
}

TA_ASCOM_API int TeenAstroAscom_HasSite(TeenAstroAscom_Handle h)
{
  if (!h) return 0;
  AscomContext* ctx = (AscomContext*)h;
  char out[8];
  if (ctx->client->get(":hS#", out, sizeof(out)) != LX200_VALUEGET) return 0;
  return (out[0] == '1') ? 1 : 0;
}

TA_ASCOM_API int TeenAstroAscom_HasMotors(TeenAstroAscom_Handle h)
{
  if (!h) return 0;
  AscomContext* ctx = (AscomContext*)h;
  char out[8];
  if (ctx->client->get(":GXJm#", out, sizeof(out)) != LX200_VALUEGET) return 0;
  return (out[0] == '1') ? 1 : 0;
}

TA_ASCOM_API int TeenAstroAscom_Park(TeenAstroAscom_Handle h)
{
  if (!h) return 0;
  AscomContext* ctx = (AscomContext*)h;
  return (ctx->client->park() == LX200_VALUEGET || ctx->client->park() == LX200_OK) ? 1 : 0;
}

TA_ASCOM_API int TeenAstroAscom_Unpark(TeenAstroAscom_Handle h)
{
  if (!h) return 0;
  AscomContext* ctx = (AscomContext*)h;
  return (ctx->client->unpark() == LX200_OK) ? 1 : 0;
}

TA_ASCOM_API int TeenAstroAscom_SetPark(TeenAstroAscom_Handle h)
{
  if (!h) return 0;
  AscomContext* ctx = (AscomContext*)h;
  return (ctx->client->setPark() == LX200_VALUEGET || ctx->client->setPark() == LX200_OK) ? 1 : 0;
}

TA_ASCOM_API int TeenAstroAscom_PulseGuide(TeenAstroAscom_Handle h, int direction, int durationMs)
{
  if (!h || durationMs < 1 || durationMs > 30000) return 0;
  static const char dirs[] = {'n', 's', 'e', 'w'};
  if (direction < 0 || direction > 3) return 0;
  AscomContext* ctx = (AscomContext*)h;
  char cmd[32];
  snprintf(cmd, sizeof(cmd), ":Mg%c%d#", dirs[direction], durationMs);
  LX200RETURN ret = ctx->client->set(cmd);
  return (ret == LX200_OK || ret == LX200_VALUESET) ? 1 : 0;
}

TA_ASCOM_API int TeenAstroAscom_MoveAxis(TeenAstroAscom_Handle h, int axis, double rateArcsecPerSec)
{
  if (!h) return 0;
  AscomContext* ctx = (AscomContext*)h;
  char cmd[24];
  snprintf(cmd, sizeof(cmd), ":M%c%.2f#", (axis == 0) ? '1' : '2', rateArcsecPerSec);
  LX200RETURN ret = ctx->client->set(cmd);
  return (ret == LX200_OK || ret == LX200_VALUESET) ? 1 : 0;
}

TA_ASCOM_API int TeenAstroAscom_SyncToEquatorial(TeenAstroAscom_Handle h)
{
  if (!h) return 0;
  AscomContext* ctx = (AscomContext*)h;
  char out[16];
  return (ctx->client->get(":CM#", out, sizeof(out)) == LX200_VALUEGET && strcmp(out, "N/A") == 0) ? 1 : 0;
}

TA_ASCOM_API int TeenAstroAscom_SyncToAltAz(TeenAstroAscom_Handle h)
{
  if (!h) return 0;
  AscomContext* ctx = (AscomContext*)h;
  LX200RETURN ret = ctx->client->set(":CA#");
  return (ret == LX200_OK || ret == LX200_VALUESET) ? 1 : 0;
}

TA_ASCOM_API int TeenAstroAscom_SlewToEquatorial(TeenAstroAscom_Handle h, char* outReply, int outSize)
{
  if (!h || !outReply || outSize < 2) return 0;
  outReply[0] = '\0';
  AscomContext* ctx = (AscomContext*)h;
  LX200RETURN ret = ctx->client->get(":MS#", outReply, outSize);
  return (ret == LX200_VALUEGET) ? 1 : 0;
}

TA_ASCOM_API int TeenAstroAscom_SlewToAltAz(TeenAstroAscom_Handle h, char* outReply, int outSize)
{
  if (!h || !outReply || outSize < 2) return 0;
  outReply[0] = '\0';
  AscomContext* ctx = (AscomContext*)h;
  LX200RETURN ret = ctx->client->get(":MA#", outReply, outSize);
  return (ret == LX200_VALUEGET) ? 1 : 0;
}

TA_ASCOM_API int TeenAstroAscom_SetTargetRA(TeenAstroAscom_Handle h, double raHours)
{
  if (!h) return 0;
  char cmd[32];
  snprintf(cmd, sizeof(cmd), "SrL,%.5f", 15.0 * raHours);
  return TeenAstroAscom_CommandBool(h, cmd, 0);
}

TA_ASCOM_API int TeenAstroAscom_SetTargetDec(TeenAstroAscom_Handle h, double decDeg)
{
  if (!h) return 0;
  char cmd[32];
  const char* sg = (decDeg >= 0) ? "+" : "-";
  snprintf(cmd, sizeof(cmd), "SdL%s%.5f", sg, (decDeg >= 0) ? decDeg : -decDeg);
  return TeenAstroAscom_CommandBool(h, cmd, 0);
}

TA_ASCOM_API int TeenAstroAscom_SetTargetAz(TeenAstroAscom_Handle h, const char* azStr)
{
  if (!h || !azStr) return 0;
  char cmd[64];
  snprintf(cmd, sizeof(cmd), ":Sz%s#", azStr);
  AscomContext* ctx = (AscomContext*)h;
  LX200RETURN ret = ctx->client->set(cmd);
  return (ret == LX200_OK || ret == LX200_VALUESET) ? 1 : 0;
}

TA_ASCOM_API int TeenAstroAscom_SetTargetAlt(TeenAstroAscom_Handle h, const char* altStr)
{
  if (!h || !altStr) return 0;
  char cmd[64];
  snprintf(cmd, sizeof(cmd), ":Sa%s#", altStr);
  AscomContext* ctx = (AscomContext*)h;
  LX200RETURN ret = ctx->client->set(cmd);
  return (ret == LX200_OK || ret == LX200_VALUESET) ? 1 : 0;
}

TA_ASCOM_API int TeenAstroAscom_GetSiteLatitude(TeenAstroAscom_Handle h, double* outLat)
{
  if (!h || !outLat) return 0;
  AscomContext* ctx = (AscomContext*)h;
  return (ctx->client->getLatitude(*outLat) == LX200_VALUEGET) ? 1 : 0;
}

TA_ASCOM_API int TeenAstroAscom_GetSiteLongitude(TeenAstroAscom_Handle h, double* outLon)
{
  if (!h || !outLon) return 0;
  AscomContext* ctx = (AscomContext*)h;
  return (ctx->client->getLongitude(*outLon) == LX200_VALUEGET) ? 1 : 0;
}

TA_ASCOM_API int TeenAstroAscom_SetSiteLatitude(TeenAstroAscom_Handle h, const char* latStr)
{
  if (!h || !latStr) return 0;
  char cmd[64];
  snprintf(cmd, sizeof(cmd), ":St%s#", latStr);
  AscomContext* ctx = (AscomContext*)h;
  LX200RETURN ret = ctx->client->set(cmd);
  return (ret == LX200_OK || ret == LX200_VALUESET) ? 1 : 0;
}

TA_ASCOM_API int TeenAstroAscom_SetSiteLongitude(TeenAstroAscom_Handle h, const char* lonStr)
{
  if (!h || !lonStr) return 0;
  char cmd[64];
  snprintf(cmd, sizeof(cmd), ":Sg%s#", lonStr);
  AscomContext* ctx = (AscomContext*)h;
  LX200RETURN ret = ctx->client->set(cmd);
  return (ret == LX200_OK || ret == LX200_VALUESET) ? 1 : 0;
}

TA_ASCOM_API int TeenAstroAscom_GetUTCTimestamp(TeenAstroAscom_Handle h, double* outSecs)
{
  if (!h || !outSecs) return 0;
  AscomContext* ctx = (AscomContext*)h;
  char out[24];
  if (ctx->client->get(":GXT2#", out, sizeof(out)) != LX200_VALUEGET) return 0;
  *outSecs = strtod(out, nullptr);
  return 1;
}

TA_ASCOM_API int TeenAstroAscom_SetUTCTimestamp(TeenAstroAscom_Handle h, long unixSecs)
{
  if (!h) return 0;
  char cmd[32];
  snprintf(cmd, sizeof(cmd), ":SXT2,%ld#", (long)unixSecs);
  AscomContext* ctx = (AscomContext*)h;
  return (ctx->client->set(cmd) == LX200_OK || ctx->client->set(cmd) == LX200_VALUESET) ? 1 : 0;
}

TA_ASCOM_API int TeenAstroAscom_EnableTracking(TeenAstroAscom_Handle h, int on)
{
  if (!h) return 0;
  AscomContext* ctx = (AscomContext*)h;
  return (ctx->client->enableTracking(on != 0) == LX200_OK) ? 1 : 0;
}

} // extern "C"

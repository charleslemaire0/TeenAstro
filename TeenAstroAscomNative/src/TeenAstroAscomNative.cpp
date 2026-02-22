/*
 * TeenAstroAscomNative.cpp - C API implementation for ASCOM P/Invoke
 *
 * Copyright (C) 2024 TeenAstro by Charles Lemaire
 * GNU General Public License v3
 */
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

} // extern "C"

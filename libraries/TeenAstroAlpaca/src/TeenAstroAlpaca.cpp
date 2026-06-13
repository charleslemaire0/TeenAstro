/*
 * TeenAstroAlpaca.cpp - ASCOM Alpaca server for TeenAstro
 *
 * Copyright (C) 2026 TeenAstro by Charles Lemaire
 * GNU General Public License v3
 */
#include "TeenAstroAlpaca.h"
#include <math.h>
#include <stdio.h>

TeenAstroAlpaca* TeenAstroAlpaca::s_self = nullptr;

// ----------------------------------------------------------------------------
//  Construction / lifecycle
// ----------------------------------------------------------------------------

TeenAstroAlpaca::TeenAstroAlpaca() = default;

const char* TeenAstroAlpaca::uniqueId()
{
  if (m_uniqueId[0]) return m_uniqueId;
  // Build a stable 32-hex-char id from the WiFi MAC.
  uint8_t mac[6] = {0};
  WiFi.macAddress(mac);
  // Repeat the MAC twice to fill 12 bytes / 24 hex chars + 8-char fixed
  // namespace nibble — keeps the string opaque but stable.
  snprintf(m_uniqueId, sizeof(m_uniqueId),
           "TA%02X%02X%02X%02X%02X%02X-Tel0-%02X%02X%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
           mac[0], mac[1], mac[2]);
  return m_uniqueId;
}

void TeenAstroAlpaca::setup(LX200Client& client,
                            TeenAstroMountStatus& mountStatus,
                            uint16_t apiPort)
{
  s_self = this;
  m_apiPort = apiPort;
  m_telescope.begin(client, mountStatus, *this);

  if (m_server)
  {
    m_server->stop();
    delete m_server;
    m_server = nullptr;
  }
  m_server = new WebServerT(apiPort);

  // ASCOM Alpaca routes — onNotFound handles every path so we can dispatch
  // without registering ~50 separate handlers.
  m_server->on("/", HTTP_GET, s_handleRoot);
  m_server->onNotFound(s_handleNotFound);

  m_server->begin();

  // UDP discovery: Alpaca clients broadcast `alpacadiscovery1` on UDP/32227,
  // we reply with our API port in JSON.
  if (m_discoveryUdp.begin(ALPACA_DISCOVERY_PORT))
  {
    m_discoveryStarted = true;
  }
}

void TeenAstroAlpaca::stop()
{
  if (m_server)
  {
    m_server->stop();
    delete m_server;
    m_server = nullptr;
  }
  if (m_discoveryStarted)
  {
    m_discoveryUdp.stop();
    m_discoveryStarted = false;
  }
  m_apiPort = 0;
}

void TeenAstroAlpaca::update()
{
  if (m_server) m_server->handleClient();
  if (m_discoveryStarted) serviceDiscovery();
}


// ----------------------------------------------------------------------------
//  Static trampolines (the WebServer handler API requires C-style fns)
// ----------------------------------------------------------------------------

void TeenAstroAlpaca::s_handleRoot()
{
  if (s_self && s_self->m_server)
    s_self->m_server->send(200, F("text/plain"),
                           F("TeenAstro Alpaca server. See /management/v1/configureddevices"));
}

void TeenAstroAlpaca::s_handleNotFound()
{
  if (!s_self || !s_self->m_server) return;
  String path = s_self->m_server->uri();
  String lower = path;
  lower.toLowerCase();

  if (lower.startsWith(F("/api/v1/")))
    s_self->handleApi();
  else if (lower.startsWith(F("/management/")))
    s_self->handleManagement();
  else if (lower.startsWith(F("/setup")))
    s_self->handleSetup();
  else
    s_self->m_server->send(404, F("text/plain"), F("Not found"));
}


// ----------------------------------------------------------------------------
//  Management API
//    GET /management/apiversions
//    GET /management/v1/description
//    GET /management/v1/configureddevices
// ----------------------------------------------------------------------------

void TeenAstroAlpaca::handleManagement()
{
  String path = m_server->uri();
  path.toLowerCase();

  AlpacaRequest req;
  req.decode(*m_server);
  uint32_t txId = nextServerTransactionId();

  if (path == F("/management/apiversions"))
  {
    sendAlpacaOk(*m_server, req, txId, F("[1]"));
    return;
  }

  if (path == F("/management/v1/description"))
  {
    String body = "{";
    body += F("\"ServerName\":");        body += AlpacaJson::escape("TeenAstro Server");
    body += F(",\"Manufacturer\":");      body += AlpacaJson::escape("TeenAstro");
    body += F(",\"ManufacturerVersion\":"); body += AlpacaJson::escape(AlpacaTelescope::driverVersion());
    body += F(",\"Location\":");          body += AlpacaJson::escape("");
    body += '}';
    sendAlpacaOk(*m_server, req, txId, body);
    return;
  }

  if (path == F("/management/v1/configureddevices"))
  {
    String dev = "{";
    dev += F("\"DeviceName\":");    dev += AlpacaJson::escape(AlpacaTelescope::deviceName());
    dev += F(",\"DeviceType\":");   dev += AlpacaJson::escape("Telescope");
    dev += F(",\"DeviceNumber\":0");
    dev += F(",\"UniqueID\":");     dev += AlpacaJson::escape(uniqueId());
    dev += '}';
    String body = "[";
    body += dev;
    body += ']';
    sendAlpacaOk(*m_server, req, txId, body);
    return;
  }

  m_server->send(404, F("text/plain"), F("Unknown management resource"));
}


// ----------------------------------------------------------------------------
//  /setup pages (HTML, optional per spec)
// ----------------------------------------------------------------------------

void TeenAstroAlpaca::handleSetup()
{
  // Minimal informational page — no settings are exposed via Alpaca because
  // every TeenAstro setting already lives on the wifi configuration UI
  // served on port 80.  Per spec, /setup must exist and return HTML.
  String html;
  html.reserve(512);
  html += F("<!DOCTYPE html><html><head><title>TeenAstro Alpaca</title></head><body>");
  html += F("<h1>TeenAstro Alpaca server</h1>");
  html += F("<p>This server exposes the TeenAstro mount as an ASCOM Alpaca telescope.</p>");
  html += F("<p>Configure the mount (Wi-Fi, site, motors, limits, focuser, ...) on the main web UI on port 80.</p>");
  html += F("<p>Discovery: UDP/");
  html += String(ALPACA_DISCOVERY_PORT);
  html += F(", API: HTTP/");
  html += String(m_apiPort);
  html += F("</p>");
  html += F("</body></html>");
  m_server->send(200, F("text/html"), html);
}


// ----------------------------------------------------------------------------
//  Device API dispatcher
// ----------------------------------------------------------------------------

void TeenAstroAlpaca::handleApi()
{
  AlpacaRequest req;
  req.decode(*m_server);

  if (req.deviceType == F("telescope") && req.deviceNumber == 0)
  {
    m_telescope.dispatch(*m_server, req);
    return;
  }

  // Unknown device type or number.
  sendAlpacaError(*m_server, req, nextServerTransactionId(),
                  AE_NOT_IMPLEMENTED, "Device not present");
}


// ----------------------------------------------------------------------------
//  UDP discovery
//
//  Per https://ascom-standards.org/api/AlpacaDiscovery.yaml , the client
//  broadcasts the literal string "alpacadiscovery1" to UDP port 32227.
//  Every Alpaca server that receives it must reply (unicast back to the
//  sender) with a single-line JSON object containing at least
//  { "AlpacaPort": <port> }.
// ----------------------------------------------------------------------------

void TeenAstroAlpaca::serviceDiscovery()
{
  int avail = m_discoveryUdp.parsePacket();
  if (avail <= 0) return;

  char buf[32];
  int n = m_discoveryUdp.read(buf, sizeof(buf) - 1);
  if (n < 0) n = 0;
  buf[n] = '\0';

  // Be tolerant of extra whitespace and case differences.
  String probe(buf);
  probe.trim();
  probe.toLowerCase();
  if (probe.indexOf("alpacadiscovery1") < 0) return;

  char reply[64];
  snprintf(reply, sizeof(reply), "{\"AlpacaPort\":%u}", (unsigned)m_apiPort);

  m_discoveryUdp.beginPacket(m_discoveryUdp.remoteIP(), m_discoveryUdp.remotePort());
  m_discoveryUdp.write((const uint8_t*)reply, strlen(reply));
  m_discoveryUdp.endPacket();
}

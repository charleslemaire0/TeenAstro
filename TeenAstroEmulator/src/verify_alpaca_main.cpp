/*
 * verify_alpaca_main.cpp - end-to-end test of TeenAstroAlpaca against the
 * MainUnit emulator (mainunit_emu.exe), running over TCP/9997.
 *
 * Usage:
 *   1. Start the MainUnit emulator (mainunit_emu.exe in
 *      TeenAstroEmulator/.pio/build/emu/) — it listens on TCP/9997.
 *   2. Build this binary:
 *        pio run -d TeenAstroEmulator -e verify_alpaca
 *   3. Run it:
 *        TeenAstroEmulator/.pio/build/verify_alpaca/verify_alpaca.exe
 *
 * The program drives AlpacaTelescope::dispatch() with synthetic requests
 * for every common GET / PUT endpoint, captures the JSON response
 * envelope on a mock ESP8266WebServer, and asserts:
 *   - HTTP 200 status
 *   - well-formed Alpaca envelope (`ErrorNumber` field present)
 *   - Value or ErrorNumber matches expectation
 *
 * Each successful assertion prints `[OK]` with the endpoint name; any
 * failure prints `[FAIL]` and the test exits with status 1.
 */

// Force-include order: Arduino must come first so STD types are right.
#include "Arduino.h"
#include "ESP8266WiFi.h"
#include "ESP8266WebServer.h"
#include "WiFiUdp.h"

#include <cstdio>
#include <cstring>
#include <string>
#include <map>

// Real LX200 + mount status (compiled with TEENASTRO_NATIVE_BUILD)
#include "LX200Client.h"
#include "TeenAstroMountStatus.h"

// Real TeenAstroAlpaca library (the thing we want to verify)
#include "TeenAstroAlpaca.h"
#include "AlpacaTelescope.h"
#include "AlpacaResponse.h"

// Win32 TCP transport (already used by TeenAstroAscomNative)
#include "Win32Transport.h"

// ===========================================================================
//  Global singletons / shim instances
// ===========================================================================

WiFiClass WiFi;          // referenced by TeenAstroAlpaca::uniqueId()

// ===========================================================================
//  Tiny test framework
// ===========================================================================

static int g_passed = 0;
static int g_failed = 0;

static bool body_contains(const String& body, const char* needle)
{
  return body.indexOf(needle) >= 0;
}

#define EXPECT(cond, name, body)                                            \
  do {                                                                      \
    bool _ok = (cond);                                                      \
    if (_ok) { ++g_passed; std::printf("[ OK ] %s\n", name); }              \
    else {                                                                  \
      ++g_failed;                                                           \
      std::printf("[FAIL] %s\n        body: %s\n", name, (body).c_str());   \
    }                                                                       \
  } while (0)

// Helper: dispatch a single request and assert a successful envelope.
//
// `assertion` is a string the body must contain (e.g. `"\"Value\":true"` or
// `"\"ErrorNumber\":0"`).
static void run_test(ESP8266WebServer& srv,
                     AlpacaTelescope& tel,
                     const char* name,
                     const String& uri,
                     int method,
                     std::map<std::string, std::string> args,
                     const char* must_contain)
{
  srv.mock_request(uri, method, args);

  AlpacaRequest req;
  req.decode(srv);

  tel.dispatch(srv, req);

  bool envelope_ok = srv.sent && srv.last_code == 200
                  && body_contains(srv.last_body, "\"ErrorNumber\"")
                  && body_contains(srv.last_body, "\"ServerTransactionID\"");
  bool content_ok  = (must_contain == nullptr) || body_contains(srv.last_body, must_contain);

  EXPECT(envelope_ok && content_ok, name, srv.last_body);
}

// ===========================================================================
//  main
// ===========================================================================

int main(int argc, char** argv)
{
  const char* host = "127.0.0.1";
  int         port = 9997;
  if (argc >= 2) host = argv[1];
  if (argc >= 3) port = std::atoi(argv[2]);

  std::printf("==> Connecting to TeenAstro MainUnit emulator at %s:%d ...\n", host, port);
  WinSockTransport transport;
  if (!transport.open(host, port))
  {
    std::printf("[FAIL] could not connect to emulator on %s:%d\n", host, port);
    std::printf("       Make sure mainunit_emu.exe is running.\n");
    return 1;
  }
  transport.setTimeout(2000);
  std::printf("==> Connected.\n");

  LX200Client          client(transport, 200);
  TeenAstroMountStatus mountStatus;
  mountStatus.setClient(client);

  TeenAstroAlpaca alpaca;        // ctor only — no setup() so no real WebServer

  AlpacaTelescope telescope;
  telescope.begin(client, mountStatus, alpaca);

  ESP8266WebServer srv(11111);

  // -------------------------------------------------------------------------
  //  Phase 1 — common metadata (no `Connected` required)
  // -------------------------------------------------------------------------
  std::printf("\n==> Phase 1: common metadata\n");

  run_test(srv, telescope, "GET name",
           "/api/v1/telescope/0/name", HTTP_GET, {{"ClientID","1"},{"ClientTransactionID","1"}},
           "TeenAstro Telescope");
  run_test(srv, telescope, "GET description",
           "/api/v1/telescope/0/description", HTTP_GET, {{"ClientID","1"},{"ClientTransactionID","2"}},
           "TeenAstro");
  run_test(srv, telescope, "GET driverversion",
           "/api/v1/telescope/0/driverversion", HTTP_GET, {{"ClientID","1"},{"ClientTransactionID","3"}},
           "1.0.0");
  run_test(srv, telescope, "GET interfaceversion",
           "/api/v1/telescope/0/interfaceversion", HTTP_GET, {{"ClientID","1"},{"ClientTransactionID","4"}},
           "\"Value\":3");
  run_test(srv, telescope, "GET supportedactions",
           "/api/v1/telescope/0/supportedactions", HTTP_GET, {{"ClientID","1"},{"ClientTransactionID","5"}},
           "\"Value\":[]");

  // GET connected before PUT — should be `false`
  run_test(srv, telescope, "GET connected (initially false)",
           "/api/v1/telescope/0/connected", HTTP_GET, {{"ClientID","1"},{"ClientTransactionID","6"}},
           "\"Value\":false");

  // Property without Connected should return NotConnected (0x407 = 1031)
  run_test(srv, telescope, "GET tracking before connect → NotConnected",
           "/api/v1/telescope/0/tracking", HTTP_GET, {{"ClientID","1"},{"ClientTransactionID","7"}},
           "\"ErrorNumber\":1031");

  // -------------------------------------------------------------------------
  //  Phase 2 — PUT connected = true, then verify state reads
  // -------------------------------------------------------------------------
  std::printf("\n==> Phase 2: connect & state reads\n");

  run_test(srv, telescope, "PUT connected=true",
           "/api/v1/telescope/0/connected", HTTP_PUT,
           {{"Connected","True"},{"ClientID","1"},{"ClientTransactionID","8"}},
           "\"ErrorNumber\":0");

  run_test(srv, telescope, "GET connected (now true)",
           "/api/v1/telescope/0/connected", HTTP_GET, {{"ClientID","1"},{"ClientTransactionID","9"}},
           "\"Value\":true");

  // The emulator initialises with valid mount state, so these should all
  // succeed and return numeric values.  We don't pin specific numbers since
  // the emulator's pose is not deterministic across builds; we just check
  // the envelope is well-formed with ErrorNumber=0.
  run_test(srv, telescope, "GET rightascension",
           "/api/v1/telescope/0/rightascension", HTTP_GET, {{"ClientID","1"},{"ClientTransactionID","10"}},
           "\"ErrorNumber\":0");
  run_test(srv, telescope, "GET declination",
           "/api/v1/telescope/0/declination", HTTP_GET, {{"ClientID","1"},{"ClientTransactionID","11"}},
           "\"ErrorNumber\":0");
  run_test(srv, telescope, "GET altitude",
           "/api/v1/telescope/0/altitude", HTTP_GET, {{"ClientID","1"},{"ClientTransactionID","12"}},
           "\"ErrorNumber\":0");
  run_test(srv, telescope, "GET azimuth",
           "/api/v1/telescope/0/azimuth", HTTP_GET, {{"ClientID","1"},{"ClientTransactionID","13"}},
           "\"ErrorNumber\":0");
  run_test(srv, telescope, "GET siderealtime",
           "/api/v1/telescope/0/siderealtime", HTTP_GET, {{"ClientID","1"},{"ClientTransactionID","14"}},
           "\"ErrorNumber\":0");
  run_test(srv, telescope, "GET utcdate (ISO 8601)",
           "/api/v1/telescope/0/utcdate", HTTP_GET, {{"ClientID","1"},{"ClientTransactionID","15"}},
           "Z\"");

  run_test(srv, telescope, "GET tracking (bool)",
           "/api/v1/telescope/0/tracking", HTTP_GET, {{"ClientID","1"},{"ClientTransactionID","16"}},
           "\"Value\":");
  run_test(srv, telescope, "GET slewing (bool)",
           "/api/v1/telescope/0/slewing", HTTP_GET, {{"ClientID","1"},{"ClientTransactionID","17"}},
           "\"Value\":");
  run_test(srv, telescope, "GET atpark (bool)",
           "/api/v1/telescope/0/atpark", HTTP_GET, {{"ClientID","1"},{"ClientTransactionID","18"}},
           "\"Value\":");
  run_test(srv, telescope, "GET athome (bool)",
           "/api/v1/telescope/0/athome", HTTP_GET, {{"ClientID","1"},{"ClientTransactionID","19"}},
           "\"Value\":");
  run_test(srv, telescope, "GET sideofpier",
           "/api/v1/telescope/0/sideofpier", HTTP_GET, {{"ClientID","1"},{"ClientTransactionID","20"}},
           "\"Value\":");
  run_test(srv, telescope, "GET alignmentmode",
           "/api/v1/telescope/0/alignmentmode", HTTP_GET, {{"ClientID","1"},{"ClientTransactionID","21"}},
           "\"Value\":");
  run_test(srv, telescope, "GET equatorialsystem (Topocentric=1)",
           "/api/v1/telescope/0/equatorialsystem", HTTP_GET, {{"ClientID","1"},{"ClientTransactionID","22"}},
           "\"Value\":1");

  // -------------------------------------------------------------------------
  //  Phase 3 — capability flags
  // -------------------------------------------------------------------------
  std::printf("\n==> Phase 3: capability flags\n");

  run_test(srv, telescope, "GET canpark = true",
           "/api/v1/telescope/0/canpark", HTTP_GET, {{"ClientID","1"},{"ClientTransactionID","30"}},
           "\"Value\":true");
  run_test(srv, telescope, "GET canunpark = true",
           "/api/v1/telescope/0/canunpark", HTTP_GET, {{"ClientID","1"},{"ClientTransactionID","31"}},
           "\"Value\":true");
  run_test(srv, telescope, "GET cansetpark = true",
           "/api/v1/telescope/0/cansetpark", HTTP_GET, {{"ClientID","1"},{"ClientTransactionID","32"}},
           "\"Value\":true");
  run_test(srv, telescope, "GET cansettracking = true",
           "/api/v1/telescope/0/cansettracking", HTTP_GET, {{"ClientID","1"},{"ClientTransactionID","33"}},
           "\"Value\":true");
  run_test(srv, telescope, "GET canslew = true",
           "/api/v1/telescope/0/canslew", HTTP_GET, {{"ClientID","1"},{"ClientTransactionID","34"}},
           "\"Value\":true");
  run_test(srv, telescope, "GET canslewasync = true",
           "/api/v1/telescope/0/canslewasync", HTTP_GET, {{"ClientID","1"},{"ClientTransactionID","35"}},
           "\"Value\":true");
  run_test(srv, telescope, "GET cansync = true",
           "/api/v1/telescope/0/cansync", HTTP_GET, {{"ClientID","1"},{"ClientTransactionID","36"}},
           "\"Value\":true");
  run_test(srv, telescope, "GET canpulseguide = true",
           "/api/v1/telescope/0/canpulseguide", HTTP_GET, {{"ClientID","1"},{"ClientTransactionID","37"}},
           "\"Value\":true");
  run_test(srv, telescope, "GET cansetrightascensionrate = false",
           "/api/v1/telescope/0/cansetrightascensionrate", HTTP_GET, {{"ClientID","1"},{"ClientTransactionID","38"}},
           "\"Value\":false");
  run_test(srv, telescope, "GET cansetdeclinationrate = false",
           "/api/v1/telescope/0/cansetdeclinationrate", HTTP_GET, {{"ClientID","1"},{"ClientTransactionID","39"}},
           "\"Value\":false");
  run_test(srv, telescope, "GET canmoveaxis (Axis=0) = true",
           "/api/v1/telescope/0/canmoveaxis", HTTP_GET,
           {{"Axis","0"},{"ClientID","1"},{"ClientTransactionID","40"}},
           "\"Value\":true");
  run_test(srv, telescope, "GET canmoveaxis (Axis=2) = false",
           "/api/v1/telescope/0/canmoveaxis", HTTP_GET,
           {{"Axis","2"},{"ClientID","1"},{"ClientTransactionID","41"}},
           "\"Value\":false");
  run_test(srv, telescope, "GET axisrates (Axis=0)",
           "/api/v1/telescope/0/axisrates", HTTP_GET,
           {{"Axis","0"},{"ClientID","1"},{"ClientTransactionID","42"}},
           "\"Minimum\":");

  // -------------------------------------------------------------------------
  //  Phase 4 — site (latitude / longitude / elevation)
  // -------------------------------------------------------------------------
  std::printf("\n==> Phase 4: site reads + writes\n");

  run_test(srv, telescope, "GET sitelatitude",
           "/api/v1/telescope/0/sitelatitude", HTTP_GET, {{"ClientID","1"},{"ClientTransactionID","50"}},
           "\"ErrorNumber\":0");
  run_test(srv, telescope, "GET sitelongitude",
           "/api/v1/telescope/0/sitelongitude", HTTP_GET, {{"ClientID","1"},{"ClientTransactionID","51"}},
           "\"ErrorNumber\":0");
  run_test(srv, telescope, "GET siteelevation",
           "/api/v1/telescope/0/siteelevation", HTTP_GET, {{"ClientID","1"},{"ClientTransactionID","52"}},
           "\"ErrorNumber\":0");

  // -------------------------------------------------------------------------
  //  Phase 5 — target setting + read-back
  // -------------------------------------------------------------------------
  std::printf("\n==> Phase 5: target round-trip\n");

  run_test(srv, telescope, "GET targetrightascension before set → ValueNotSet",
           "/api/v1/telescope/0/targetrightascension", HTTP_GET, {{"ClientID","1"},{"ClientTransactionID","60"}},
           "\"ErrorNumber\":1026");

  run_test(srv, telescope, "PUT targetrightascension=12.5",
           "/api/v1/telescope/0/targetrightascension", HTTP_PUT,
           {{"TargetRightAscension","12.5"},{"ClientID","1"},{"ClientTransactionID","61"}},
           "\"ErrorNumber\":0");
  run_test(srv, telescope, "PUT targetdeclination=42.0",
           "/api/v1/telescope/0/targetdeclination", HTTP_PUT,
           {{"TargetDeclination","42.0"},{"ClientID","1"},{"ClientTransactionID","62"}},
           "\"ErrorNumber\":0");

  run_test(srv, telescope, "GET targetrightascension after set",
           "/api/v1/telescope/0/targetrightascension", HTTP_GET, {{"ClientID","1"},{"ClientTransactionID","63"}},
           "\"Value\":12.5");
  run_test(srv, telescope, "GET targetdeclination after set",
           "/api/v1/telescope/0/targetdeclination", HTTP_GET, {{"ClientID","1"},{"ClientTransactionID","64"}},
           "\"Value\":42");

  // Range validation
  run_test(srv, telescope, "PUT targetrightascension=99 → InvalidValue",
           "/api/v1/telescope/0/targetrightascension", HTTP_PUT,
           {{"TargetRightAscension","99"},{"ClientID","1"},{"ClientTransactionID","65"}},
           "\"ErrorNumber\":1025");
  run_test(srv, telescope, "PUT targetdeclination=-200 → InvalidValue",
           "/api/v1/telescope/0/targetdeclination", HTTP_PUT,
           {{"TargetDeclination","-200"},{"ClientID","1"},{"ClientTransactionID","66"}},
           "\"ErrorNumber\":1025");

  // -------------------------------------------------------------------------
  //  Phase 6 — tracking on/off + tracking rate
  // -------------------------------------------------------------------------
  std::printf("\n==> Phase 6: tracking control\n");

  run_test(srv, telescope, "PUT tracking=true",
           "/api/v1/telescope/0/tracking", HTTP_PUT,
           {{"Tracking","True"},{"ClientID","1"},{"ClientTransactionID","70"}},
           "\"ErrorNumber\":0");
  run_test(srv, telescope, "PUT tracking=false",
           "/api/v1/telescope/0/tracking", HTTP_PUT,
           {{"Tracking","False"},{"ClientID","1"},{"ClientTransactionID","71"}},
           "\"ErrorNumber\":0");

  run_test(srv, telescope, "GET trackingrates",
           "/api/v1/telescope/0/trackingrates", HTTP_GET, {{"ClientID","1"},{"ClientTransactionID","72"}},
           "[0,1,2]");
  run_test(srv, telescope, "PUT trackingrate=0 (Sidereal)",
           "/api/v1/telescope/0/trackingrate", HTTP_PUT,
           {{"TrackingRate","0"},{"ClientID","1"},{"ClientTransactionID","73"}},
           "\"ErrorNumber\":0");
  run_test(srv, telescope, "PUT trackingrate=1 (Lunar)",
           "/api/v1/telescope/0/trackingrate", HTTP_PUT,
           {{"TrackingRate","1"},{"ClientID","1"},{"ClientTransactionID","74"}},
           "\"ErrorNumber\":0");
  run_test(srv, telescope, "PUT trackingrate=99 → InvalidValue",
           "/api/v1/telescope/0/trackingrate", HTTP_PUT,
           {{"TrackingRate","99"},{"ClientID","1"},{"ClientTransactionID","75"}},
           "\"ErrorNumber\":1025");

  // -------------------------------------------------------------------------
  //  Phase 7 — write-only methods (we only verify the LX200 round-trip
  //  succeeds, not that the mount actually moves)
  // -------------------------------------------------------------------------
  std::printf("\n==> Phase 7: methods\n");

  run_test(srv, telescope, "PUT abortslew",
           "/api/v1/telescope/0/abortslew", HTTP_PUT,
           {{"ClientID","1"},{"ClientTransactionID","80"}},
           "\"ErrorNumber\":0");
  run_test(srv, telescope, "PUT pulseguide North 100ms",
           "/api/v1/telescope/0/pulseguide", HTTP_PUT,
           {{"Direction","0"},{"Duration","100"},{"ClientID","1"},{"ClientTransactionID","81"}},
           "\"ErrorNumber\":0");
  run_test(srv, telescope, "PUT pulseguide invalid Direction=9",
           "/api/v1/telescope/0/pulseguide", HTTP_PUT,
           {{"Direction","9"},{"Duration","100"},{"ClientID","1"},{"ClientTransactionID","82"}},
           "\"ErrorNumber\":1025");
  run_test(srv, telescope, "PUT moveaxis Axis=0 Rate=0 (stop)",
           "/api/v1/telescope/0/moveaxis", HTTP_PUT,
           {{"Axis","0"},{"Rate","0"},{"ClientID","1"},{"ClientTransactionID","83"}},
           "\"ErrorNumber\":0");
  run_test(srv, telescope, "PUT moveaxis Axis=2 → InvalidValue",
           "/api/v1/telescope/0/moveaxis", HTTP_PUT,
           {{"Axis","2"},{"Rate","1"},{"ClientID","1"},{"ClientTransactionID","84"}},
           "\"ErrorNumber\":1025");

  // -------------------------------------------------------------------------
  //  Phase 8 — unknown member
  // -------------------------------------------------------------------------
  std::printf("\n==> Phase 8: error path\n");

  run_test(srv, telescope, "GET totallyunknown → NotImplemented",
           "/api/v1/telescope/0/totallyunknown", HTTP_GET,
           {{"ClientID","1"},{"ClientTransactionID","90"}},
           "\"ErrorNumber\":1024");

  // -------------------------------------------------------------------------
  //  Summary
  // -------------------------------------------------------------------------
  std::printf("\n==> Summary: %d passed, %d failed\n", g_passed, g_failed);
  transport.close();
  return g_failed == 0 ? 0 : 1;
}

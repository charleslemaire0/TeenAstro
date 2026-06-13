# TeenAstroAlpaca

ASCOM **Alpaca** server library for TeenAstro mounts.

This library implements the [ASCOM Alpaca REST/JSON API](https://ascom-standards.org/api/) on top of the existing TeenAstro `LX200Client` and `TeenAstroMountStatus` so that any Alpaca‑aware application (NINA, Stellarium, KStars/INDI Alpaca bridge, ASCOM Remote, custom scripts, …) can drive the mount over Wi‑Fi without going through the legacy LX200‑over‑TCP socket.

It runs on the TeenAstro Server bridge (Wemos D1 mini / ESP32) **alongside** the existing wifi configuration UI: the wifi UI keeps serving on port 80, the Alpaca API listens on TCP/11111, and the standard UDP/32227 discovery responder lets clients find the device automatically.

## Quick start

```cpp
#include <LX200Client.h>
#include <TeenAstroMountStatus.h>
#include <TeenAstroWifi.h>
#include <TeenAstroAlpaca.h>

TeenAstroWifi         m_wbt;
TeenAstroMountStatus  ta_MountStatus;
LX200Client           lx200(Serial);
TeenAstroAlpaca       m_alpaca;

void setup() {
  Serial.begin(115200);
  ta_MountStatus.setClient(lx200);
  TeenAstroWifi::setClient(lx200);
  m_wbt.setup();
  m_alpaca.setup(lx200, ta_MountStatus);   // default port 11111
}

void loop() {
  m_wbt.update();
  m_alpaca.update();
}
```

The TeenAstroServer sketch (`TeenAstroServer/TeenAstroServer.ino`) already wires this up.

## Endpoints

### Management API (`/management/...`)

| Method | Path                                  | Description                          |
| ------ | ------------------------------------- | ------------------------------------ |
| GET    | `/management/apiversions`             | Returns `[1]`                        |
| GET    | `/management/v1/description`          | Server name / manufacturer / version |
| GET    | `/management/v1/configureddevices`    | Lists the telescope device           |

### Telescope device (`/api/v1/telescope/0/...`)

The device class is fully implemented for the operations supported by the
TeenAstro firmware:

- **Common** — `connected`, `name`, `description`, `driverinfo`, `driverversion`, `interfaceversion`, `supportedactions`, `commandblind`, `commandbool`, `commandstring`
- **Read-only** — `rightascension`, `declination`, `altitude`, `azimuth`, `siderealtime`, `slewing`, `atpark`, `athome`, `ispulseguiding`, `sideofpier`, `alignmentmode`, `equatorialsystem`, `doesrefraction`
- **Target** — `targetrightascension`, `targetdeclination` (GET / PUT)
- **Tracking** — `tracking`, `trackingrate`, `trackingrates`, `rightascensionrate`, `declinationrate`, `guideraterightascension`, `guideratedeclination`, `slewsettletime`
- **Site** — `sitelatitude`, `sitelongitude`, `siteelevation`, `utcdate` (GET / PUT)
- **Capabilities** — every `can*` property advertised correctly for the active mount type
- **Methods** — `abortslew`, `findhome`, `park`, `unpark`, `setpark`, `slewtocoordinates(async)`, `synctocoordinates`, `slewtotarget(async)`, `synctotarget`, `slewtoaltaz(async)`, `synctoaltaz`, `pulseguide`, `moveaxis`, `axisrates`

Properties without a meaningful equivalent on the firmware (`aperturearea`,
`aperturediameter`, `focallength`, write‑side of `*rate` / `*guideraterate`)
return the standard ASCOM error codes (`ValueNotSet` / `NotImplemented`)
so well‑behaved clients negotiate them gracefully.

### Discovery (UDP/32227)

Responds to the literal probe `alpacadiscovery1` with a JSON object
`{"AlpacaPort": <port>}` so Alpaca clients can find the mount on the LAN
without the user typing an IP address.

## Notes / limitations

- The bridge maintains a per‑driver "Connected" boolean to honour the
  ASCOM contract, but the underlying serial link to the MainUnit is
  always live — `connected = false` simply causes property reads to
  fail with `NotConnected (0x407)`.
- Coordinates are reported in the **equinox of date** (Alpaca
  `EquatorialSystem = 1` / Topocentric), matching the LX200 protocol.
- `MoveAxis(rate)` is mapped to TeenAstro's discrete slew‑speed buckets
  (`R0..R4`), so the actual rate may differ from the requested one. The
  granularity is reflected in the single `axisrates` range returned per
  axis.
- All endpoints are dispatched from a single `onNotFound` handler to
  keep flash + RAM usage low on the ESP8266; latency is dominated by
  the LX200 round‑trip to the MainUnit, not by the dispatch loop.

## License

GNU General Public License v3 — same as the rest of the TeenAstro
project.

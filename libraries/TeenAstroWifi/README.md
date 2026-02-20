# TeenAstroWifi

ESP8266/ESP32 web server library for the TeenAstro Smart Hand Controller (SHC). Provides a browser-based interface for telescope control and configuration over WiFi.

Compatible with personal computers, phones, tablets, SkySafari, and the ASCOM driver.

## Features

- **Dashboard** — real-time mount status, position (RA/Dec, Az/Alt), tracking state, parking
- **Control** — slew, sync/goto, tracking mode, speed selection, home/park
- **Configuration pages** — mount type, motors, encoders, rates, limits, site/location, tracking, focuser, WiFi settings
- **Modern UI** — responsive CSS with flexbox layout, CSS variables for theming, card-based panels
- **WiFi modes** — Access Point (AP) and Station (STA) modes

## Architecture

The library generates HTML pages server-side using C++ string building with a shared helper system (`HtmlCommon.h`). All LX200 communication goes through `LX200Client` — no raw command strings in the web layer.

### Source layout

| File | Purpose |
|------|---------|
| `TeenAstroWifi.cpp/.h` | WiFi setup, AP/STA mode, web server registration |
| `Index.cpp` | Dashboard page (status, position, controls) |
| `Control.cpp` | Slew, goto, tracking, speed, park/home actions |
| `Configuration_mount.cpp` | Mount type and geometry settings |
| `Configuration_motors.cpp` | Motor driver configuration |
| `Configuration_encoders.cpp` | Encoder settings |
| `Configuration_speed.cpp` | Slew speed configuration |
| `Configuration_limits.cpp` | Altitude, meridian, and axis limits |
| `Configuration_site.cpp` | Site location and time settings |
| `Configuration_tracking.cpp` | Tracking mode and rate configuration |
| `Configuration_focuser.cpp` | Focuser settings |
| `Configuration_wifi.cpp` | WiFi network settings |
| `Helper.cpp` | HTML generation helpers |
| `HtmlCommon.h` | Shared CSS, HTML builder macros |

## Dependencies

- **LX200Client** — typed mount communication
- **TeenAstroMountStatus** — cached mount state
- **ESP8266WebServer** or **WebServer** (ESP32) — HTTP server

# TeenAstroLX200io

LX200 serial client library for the TeenAstro telescope controller. Provides a typed, object-oriented API for communicating with the main unit over the LX200 protocol.

## Architecture

```
  SHC / WiFi Server / External App
          |
     LX200Client  (this library)
          |
       Stream    (Serial, WiFiClient, etc.)
          |
     MainUnit    (TeenAstroMainUnit firmware)
```

`LX200Client` owns a `Stream&` reference and encapsulates all serial I/O. Any Arduino `Stream` subclass works as the transport (hardware serial, software serial, WiFi socket).

## Main class: `LX200Client`

Constructed with a stream and timeout:

```cpp
LX200Client client(Serial1, 30);  // 30 ms timeout
```

### Command groups

| Group | Examples |
|-------|---------|
| **Position** | `getRaStr()`, `getDecStr()`, `getAzStr()`, `getAltStr()`, `getAxisSteps()` |
| **Time/Date** | `getUTCTime()`, `setUTCTime()`, `getLocalDate()`, `getSiderealStr()` |
| **Tracking** | `enableTracking()`, `setTrackRateSidereal()`, `setTrackRateLunar()`, `getTrackRateRA()` |
| **Navigation** | `moveToTarget()`, `syncGoto()`, `syncGotoAltAz()`, `pushToTarget()` |
| **Movement** | `startMoveNorth()`, `stopSlew()`, `meridianFlip()`, `setSpeed()` |
| **Home/Park** | `homeReset()`, `homeGoto()`, `park()`, `unpark()`, `setPark()` |
| **Alignment** | `alignStart()`, `alignAcceptStar()`, `alignSave()`, `getAlignError()` |
| **Site** | `getLatitude()`, `getLongitude()`, `getSite()`, `setSite()`, `setMount()` |
| **Motors** | per-axis: reverse, backlash, gear ratio, microstepping, current |
| **Encoders** | per-axis: pulse/degree, reverse, calibration, auto-sync |
| **Focuser** | `focuserMoveIn()`, `focuserMoveOut()`, `focuserGotoHome()`, full config |
| **Rotator** | `rotatorCenter()`, `rotatorDeRotateToggle()`, `rotatorMove()` |
| **Rates/Limits** | acceleration, max rate, altitude/meridian/axis limits |
| **Firmware** | `getProductName()`, `getVersionNumber()`, `getBoardVersion()` |

### Higher-level navigation: `LX200Navigation`

Free functions that combine `LX200Client` with the Ephemeris and Catalog libraries:

| Function | Description |
|----------|-------------|
| `SyncGotoLX200()` | Sync/goto RA/Dec with epoch precession (J2000 -> JNow) |
| `SyncGotoCatLX200()` | Sync/goto the currently selected catalog object |
| `SyncGotoPlanetLX200()` | Sync/goto a solar system object by index |
| `SyncSelectedStarLX200()` | Sync the selected alignment star |

## Dependencies

- **TeenAstroCommandDef** — enums, reply types, codec functions
- **TeenAstroFunction** — `gethms()`, `getdms()` for coordinate formatting
- **Ephemeris** — equinox precession (LX200Navigation only)
- **TeenAstroCatalog** — catalog object access (LX200Navigation only)

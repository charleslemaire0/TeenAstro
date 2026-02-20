# TeenAstroMountStatus

Mount state cache and alignment state machine for TeenAstro client applications (SHC, WiFi server). Polls the main unit via `LX200Client`, parses the status string, and exposes structured, rate-limited accessors for all mount state.

## Purpose

Instead of each UI component sending its own LX200 queries, `TeenAstroMountStatus` acts as a **central state store**: it polls the mount periodically (200 ms rate limit), caches results, and provides typed accessors. This avoids redundant serial traffic and keeps all consumers in sync.

## Main class: `TeenAstroMountStatus`

Global instance: `ta_MountStatus`

### Setup

```cpp
ta_MountStatus.setClient(lx200Client);
ta_MountStatus.checkConnection(major, minor);
```

### State updates (rate-limited)

| Method | Caches |
|--------|--------|
| `updateMount()` | Full mount state (tracking, park, pier, errors, GNSS, ...) |
| `updateRaDec()` | RA/Dec strings |
| `updateAzAlt()` | Az/Alt strings |
| `updateTime()` | UTC time/date, sidereal time |
| `updateAxisStep()` | Axis step positions |
| `updateAxisDeg()` | Axis degrees |
| `updateTrackingRate()` | Current RA/Dec tracking rates |
| `updateFocuser()` | Focuser status |

### Mount state queries

| Method | Returns |
|--------|---------|
| `getTrackingState()` | `TRK_OFF`, `TRK_ON`, `TRK_SLEWING` |
| `getParkState()` | `PRK_UNPARKED`, `PRK_PARKED`, `PRK_PARKING`, `PRK_FAILED` |
| `getPierState()` | `PIER_E`, `PIER_W` |
| `getSiderealMode()` | `SID_STAR`, `SID_SUN`, `SID_MOON`, `SID_TARGET` |
| `getMount()` | `MOUNT_TYPE_GEM`, `MOUNT_TYPE_FORK`, `MOUNT_TYPE_ALTAZM`, `MOUNT_TYPE_FORK_ALT` |
| `getError()` | `ERR_NONE` through `ERR_SYNC` |
| `atHome()`, `Parked()`, `isAligned()`, `isAltAz()` | Boolean state checks |
| `hasGNSSBoard()`, `isGNSSValid()`, `isGNSSTimeSync()` | GNSS state |
| `encodersEnable()`, `motorsEnable()` | Enable flags |
| `getLastErrorMessage(buf)` | Human-readable error string |

### Alignment state machine

| Method | Description |
|--------|-------------|
| `startAlign(mode)` | Begin 1/2/3-star alignment |
| `nextStepAlign()` | Advance to next alignment step |
| `addStar()` | Accept current star and advance |
| `stopAlign()` | Cancel alignment |
| `isAligning()`, `isAlignSelect()`, `isAlignSlew()`, `isAlignRecenter()` | State checks |

### Cached position/time accessors

`getRa()`, `getDec()`, `getHa()`, `getAz()`, `getAlt()`, `getUTC()`, `getSidereal()`, axis steps/degrees, tracking rates, and version strings.

## Status string format

The `:GXI#` command returns a fixed-format string where each character index encodes a specific field:

| Index | Field | Encoding |
|-------|-------|----------|
| 0 | Tracking | `0`=off, `1`=on, `2`/`3`=slewing |
| 1 | Sidereal mode | `0`-`3` |
| 2 | Park state | `P`/`p`/`I`/`F` |
| 3 | At home | `H` = yes |
| 4 | Guiding rate | `0`-`4` |
| 12 | Mount type | `E`/`K`/`A`/`k` |
| 13 | Pier side | `E`/`W` |
| 15 | Error code | `0`-`8` |

## Dependencies

- **LX200Client** — serial communication with the main unit
- **Arduino.h** — `millis()` for rate limiting

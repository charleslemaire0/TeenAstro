# TeenAstroMath

Math and coordinate-formatting utility library for the TeenAstro telescope controller. This library merges the former **TeenAstroFunction** (HMS/DMS formatting) into a single place alongside angle normalization, atmospheric refraction, coordinate conversion, and step-distance helpers.

## Coordinate formatting (from TeenAstroFunction)

| Function | Description |
|----------|-------------|
| `gethms(v, h, m, s)` | Split total arc-seconds into hours, minutes, seconds |
| `getdms(v, ispos, d, m, s)` | Split signed arc-seconds into sign, degrees, arcmin, arcsec |

## String-to-integer parsing

| Function | Description |
|----------|-------------|
| `atoi2(a, i)` | Parse string to int16 with bounds check |
| `atoui2(a, i)` | Parse string to uint16 with bounds check |

## Basic math helpers

| Function | Description |
|----------|-------------|
| `frac(v)` | Fractional part (v - floor(v)) |
| `cot(n)` | Cotangent (1 / tan(n)) |

## Angle normalization

| Function | Description |
|----------|-------------|
| `haRange(d)` | Hour angle to [-180, +180] degrees |
| `haRangeRad(d)` | Hour angle to [-PI, +PI] radians |
| `AzRange(d)` | Azimuth to [0, 360) degrees |
| `degRange(d)` | Arbitrary angle to [0, 360) degrees |

## Angular distance

| Function | Description |
|----------|-------------|
| `angDist(h, d, h1, d1)` | Great-circle distance between two equatorial positions (degrees) |

## Step-distance helpers

| Function | Description |
|----------|-------------|
| `distStepAxis1(start, end)` | Step difference for axis 1 (RA / Azimuth) |
| `distStepAxis2(start, end)` | Step difference for axis 2 (Dec / Altitude) |

## Constants

| Macro | Value | Description |
|-------|-------|-------------|
| `DEG_TO_RAD` | 0.01745... | Degrees to radians |
| `RAD_TO_DEG` | 57.2957... | Radians to degrees |
| `HOUR_TO_RAD` | 0.26179... | Hours to radians |
| `RAD_TO_HOUR` | 3.81971... | Radians to hours |
| `Rad` | 57.2957... | Legacy alias for RAD_TO_DEG |

## Types

| Type | Description |
|------|-------------|
| `PoleSide` | Enum: `POLE_NOTVALID`, `POLE_UNDER`, `POLE_OVER` |

## Deprecated / Legacy functions

The following functions are kept for **UniversalMainUnit** compatibility but should not be used in new code:

### Atmospheric refraction [DEPRECATED]

Use `LA3::Topocentric2Apparent` / `LA3::Apparent2Topocentric` (radian-based Meeus formulas in TeenAstroLA3) instead.

| Function | Description |
|----------|-------------|
| `trueRefrac(Alt, P, T)` | Refraction in arcminutes at true altitude (degrees) |
| `Topocentric2Apparent(Alt, P, T)` | Add refraction to topocentric altitude (degrees) |
| `Apparent2Topocentric(Alt, P, T)` | Remove refraction from apparent altitude (degrees) |

### Coordinate conversion [LEGACY]

Use `TeenAstroCoord` classes (`Coord_EQ`, `Coord_HO`) instead.

| Function | Description |
|----------|-------------|
| `EquToHor(HA, Dec, refr, Az, Alt, cosLat, sinLat)` | Equatorial to horizontal (degrees) |
| `HorTopoToEqu(Az, Alt, HA, Dec, cosLat, sinLat)` | Topocentric horizontal to equatorial |
| `HorAppToEqu(Az, Alt, HA, Dec, cosLat, sinLat)` | Apparent horizontal to equatorial |

## Dependencies

None (standard C/C++ math and Arduino core only).

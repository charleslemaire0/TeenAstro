# TeenAstroMath

Math utility library for the TeenAstro telescope controller. Provides astronomical coordinate conversions, atmospheric refraction, angle normalization, and step-distance calculations.

## Functions

### Coordinate conversion

| Function | Description |
|----------|-------------|
| `EquToHor(HA, Dec, refr, Az, Alt, cosLat, sinLat)` | Equatorial -> horizontal (with optional refraction) |
| `HorTopoToEqu(Az, Alt, HA, Dec, cosLat, sinLat)` | Topocentric horizontal -> equatorial |
| `HorAppToEqu(Az, Alt, HA, Dec, cosLat, sinLat)` | Apparent horizontal -> equatorial |

### Atmospheric refraction

| Function | Description |
|----------|-------------|
| `trueRefrac(Alt, P, T)` | Refraction in arcminutes for a given true altitude |
| `Topocentric2Apparent(Alt, ...)` | True -> apparent altitude |
| `Apparent2Topocentric(Alt, ...)` | Apparent -> true altitude |

### Angle normalization

| Function | Description |
|----------|-------------|
| `haRange(d)` | Hour angle to [-180, 180] degrees |
| `haRangeRad(d)` | Hour angle to [-PI, PI] radians |
| `AzRange(d)` | Azimuth to [0, 360) degrees |
| `degRange(d)` | Degrees to [0, 360) |

### Step distance

| Function | Description |
|----------|-------------|
| `distStepAxis1(start, end)` | Step difference for axis 1 (RA), handling wraparound |
| `distStepAxis2(start, end)` | Step difference for axis 2 (Dec) |

### Utilities

| Function | Description |
|----------|-------------|
| `frac(v)` | Fractional part |
| `cot(n)` | Cotangent |
| `angDist(h, d, h1, d1)` | Angular distance between two equatorial positions (degrees) |
| `atoi2(a, i)` | Safe string-to-int16 parse |
| `atoui2(a, i)` | Safe string-to-uint16 parse |

### Constants

| Macro | Value | Description |
|-------|-------|-------------|
| `DEG_TO_RAD` | 0.01745... | Degrees to radians |
| `RAD_TO_DEG` | 57.2957... | Radians to degrees |
| `HOUR_TO_RAD` | 0.26179... | Hours to radians |
| `RAD_TO_HOUR` | 3.81971... | Radians to hours |

## Dependencies

None (standard C/C++ math and Arduino core only).

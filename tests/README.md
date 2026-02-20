# TeenAstro Math Library Unit Tests

Native (desktop) unit tests for the core math libraries, built and run with PlatformIO's `native` platform (no hardware required).

## Prerequisites

PlatformIO CLI and the MinGW toolchain for Windows:

```bash
pip install platformio
pio pkg install -g --tool "platformio/toolchain-gccmingw32"
```

On Windows, the MinGW `bin/` directory must be on PATH:

```powershell
$env:PATH = "$env:USERPROFILE\.platformio\packages\toolchain-gccmingw32\bin;$env:PATH"
```

## Running tests

```bash
# All suites (91 tests)
pio test -d tests

# Individual suites
pio test -d tests --filter test_la3
pio test -d tests --filter test_coord
pio test -d tests --filter test_coordconv

# Python runner (runs all, shows summary)
python tests/run_all_tests.py
python tests/run_all_tests.py test_la3       # single suite
```

## Test suites

### test_la3 (53 tests) — TeenAstroLA3

Linear algebra foundation:

- **modRad** — angle wrapping to [-PI, PI]
- **Vector ops** — dot product, cross product, angle between vectors, norm, normalize
- **Direction cosines** — toDirCos at origin, pole, and arbitrary angles
- **Conversions** — toRad, toDeg, normalizeRads
- **Matrix ops** — identity, copy, transpose, determinant, invert, multiply (matrix×matrix, matrix×vector)
- **Rotation matrices** — single-axis rotations (X, Y, Z at 90°), determinant = 1, multi-rotation composition
- **Euler angle extraction** — round-trip tests for all four decompositions: RzRxRy, RzRyRx, RxRyRz, Rx0RyRx1
- **Atmospheric refraction** — Meeus Saemundsson/Bennett formulas:
  - Round-trip at 5°, 45°, 80° (tolerance ≤ 2–3 arcsec)
  - Disabled refraction leaves altitude unchanged
  - Low altitude produces larger correction than high altitude
  - Zenith correction is ~zero
  - Nautical Almanac reference values: horizon (~28.8 arcmin), 45° (~0.97 arcmin)
  - Pressure/temperature scaling behavior
- **SVD** — identity and rotation matrix decomposition smoke tests

### test_coord (20 tests) — TeenAstroCoord

Coordinate system classes:

- **Construction & accessors** — Coord_EQ (FrE, Dec, Ha, Ra), Coord_HO (FrH, Alt, Az), Coord_IN (Axis1/2/3), Coord_LO (Axis1/2/3)
- **EQ ↔ HO round-trips** — multiple HA/Dec positions at different latitudes
- **HO ↔ IN round-trip** — with identity misalignment matrix
- **EQ ↔ IN round-trip** — through horizontal coordinates
- **EQ ↔ LO round-trip** — with identity transformation
- **Refraction** — ToApparent/ToTopocentric round-trip, idempotency checks
- **EQ → HO → EQ with refraction** — full chain with Meeus formulas
- **Astronomical identities** — zenith star (Dec = Lat, HA = 0 → Alt = 90°), pole star altitude ≈ latitude

### test_coordconv (18 tests) — TeenAstroCoordConv

Mount alignment:

- **Construction** — initial state, reset, clean
- **Identity alignment** — sky coords = axis coords produces T ≈ I
- **Matrix properties** — T × Tinv = I, det(T) = +1, T is orthogonal
- **Serialization** — getT/setT float round-trip, setTinvFromT
- **Reference management** — max 2 references, calculateThirdReference precondition
- **Alignment quality** — known rotation alignment (error ≈ 0), imperfect alignment (error > 0), minimizeAxis1
- **Full-chain integration** — EQ → HO → IN → HO → EQ round-trips with identity alignment, with rotation, with CoordConv, and with refraction

## Architecture

```
tests/
    platformio.ini              PlatformIO project (platform = native, Unity framework)
    shim/
        arduino.h               Arduino API stub for desktop builds
    test/
        test_la3/test_la3.cpp           LA3 tests
        test_coord/test_coord.cpp       Coord tests
        test_coordconv/test_coordconv.cpp   CoordConv tests
    run_all_tests.py            Master runner script
    run_all_tests.bat           Windows batch wrapper
```

Each test file includes library `.cpp` sources directly (single-translation-unit build). The `shim/arduino.h` provides minimal Arduino compatibility (constants, Serial stub) so the library code compiles on desktop without modifications.

## Refraction formulas

The refraction functions use the standard **Meeus Saemundsson/Bennett pair** (*Astronomical Algorithms*, Eq. 16.4 and 16.3), the same formulas used by Stellarium and consistent with the Nautical Almanac:

- **Topocentric → Apparent** (Saemundsson): `R = (1.02 / tan(h + 10.3/(h+5.11)) + 0.0019279) × P/1010 × 283/(273+T)`
- **Apparent → Topocentric** (Bennett): `R = (1/tan(h₀ + 7.31/(h₀+4.4)) + 0.0013515) × P/1010 × 283/(273+T)`

The formulas operate in degrees internally, with radian I/O. The pair is mutually consistent to ~0.4 arcsecond at moderate altitudes and ~2–3 arcseconds near the horizon. Single `tan()` evaluation per direction — no iteration needed.

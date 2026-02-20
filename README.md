TeenAstro an easy to use telescope controller
======  
I was a user of the FS2 And I would like to have a FS3!
All Equatorial and Alt-AZ mount types are supported: German Equatorial, Fork (and variations)

Join our user group: https://groups.io/g/TeenAstro/wiki/home

## Math libraries

The core coordinate and linear algebra libraries live under `libraries/`:

| Library | Purpose |
|---------|---------|
| **TeenAstroLA3** | 3-vector and 3×3 matrix operations, rotation matrices, Euler angle extraction, atmospheric refraction (Meeus Saemundsson/Bennett), SVD |
| **TeenAstroCoord** | Object-oriented coordinate systems: Equatorial (EQ), Horizontal (HO), Instrument (IN), Local Offset (LO) with conversions between them |
| **TeenAstroCoordConv** | Mount alignment: computes transformation matrices from reference stars (Taki method with SVD-based optimal rotation) |

## Unit tests

Native (desktop) unit tests cover the math libraries. Requires PlatformIO with the MinGW toolchain (`pio pkg install -g --tool "platformio/toolchain-gccmingw32"`).

```bash
# Run all test suites (91 tests)
pio test -d tests

# Run a single suite
pio test -d tests --filter test_la3
pio test -d tests --filter test_coord
pio test -d tests --filter test_coordconv

# Or use the runner script
python tests/run_all_tests.py
```

See [tests/README.md](tests/README.md) for details.

## Build

Requires [PlatformIO](https://platformio.org/) (CLI or IDE).

```bash
pio run -d TeenAstroMainUnit
pio run -d TeenAstroSHC
pio run -d TeenAstroServer
```

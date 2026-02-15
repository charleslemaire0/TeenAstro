# TeenAstroFunction

Small utility library for converting astronomical coordinates between compact numeric formats and hour/degree-minute-second components. Used for RA/Dec and Alt/Az formatting in the LX200 protocol and UI input.

## Functions

| Function | Description |
|----------|-------------|
| `gethms(v, h, m, s)` | Splits total arcseconds into hours, minutes, seconds |
| `getdms(v, ispos, d, m, s)` | Splits signed value into sign, degrees, arcminutes, arcseconds |
| `longRa2Ra(Ra, h, m, s)` | Converts internal RA format to hours/minutes/seconds |
| `longDec2Dec(Dec, ispos, deg, min)` | Converts internal Dec format to sign/degrees/arcminutes |

## Usage

```cpp
#include <TeenAstroFunction.h>

uint8_t h, m, s;
gethms(totalArcSeconds, h, m, s);

bool ispos;
uint16_t deg;
uint8_t min, sec;
getdms(signedValue, ispos, deg, min, sec);
```

## Dependencies

None (standard C/C++ types only).

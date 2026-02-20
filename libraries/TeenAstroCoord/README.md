# TeenAstroCoord

Object-oriented coordinate system library for the TeenAstro telescope controller. Models four astronomical/instrument coordinate frames and the transformations between them.

## Coordinate systems

| Class | Components | Description |
|-------|-----------|-------------|
| **Coord_EQ** | FrE, Dec, Ha | Equatorial — Hour Angle and Declination |
| **Coord_HO** | FrH, Alt, Az | Horizontal — Altitude and Azimuth |
| **Coord_IN** | Axis3, Axis2, Axis1 | Instrument — physical mount axis angles |
| **Coord_LO** | Axis3, Axis2, Axis1 | Logical — logical axis angles (pier side, etc.) |

All angles are in **radians**. Each class wraps three Euler rotations internally and uses `TeenAstroLA3` for matrix operations.

## Transformation graph

```
        trafo / trafoinv              Lat + RefrOpt
   EQ <-------------------> LO    EQ <-------------------> HO
                                   HO <-------------------> IN
                                        misalignment matrix
```

- **EQ <-> HO** — latitude and optional atmospheric refraction
- **HO <-> IN** — 3x3 misalignment matrix from the alignment model
- **EQ <-> LO** — 3x3 transformation matrix (mount geometry, pier side)
- **EQ -> IN** — chained via HO: `EQ -> HO -> IN`
- **IN -> EQ** — chained via HO: `IN -> HO -> EQ`

## Refraction

`Coord_HO` tracks whether an altitude is **apparent** (refracted) or **topocentric** (geometric):

- `ToApparent(RefrOpt)` — applies refraction (Meeus Saemundsson)
- `ToTopocentric(RefrOpt)` — removes refraction (Meeus Bennett)

## Usage

```cpp
#include <TeenAstroCoord.hpp>

// Equatorial to Horizontal
Coord_EQ eq(0, dec_rad, ha_rad);
Coord_HO ho = eq.To_Coord_HO(lat_rad, refrOpt);
double alt = ho.Alt();
double az  = ho.Az();

// Horizontal to Instrument (with alignment)
Coord_IN in = ho.To_Coord_IN(misalignment);
```

## Dependencies

- **TeenAstroLA3** — linear algebra, rotation matrices, Euler angle extraction, refraction

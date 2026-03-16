# Coordinate systems (TeenAstroCoord)

Four coordinate classes built on Euler rotation chains: **Coord_EQ**, **Coord_HO**, **Coord_IN**, **Coord_LO**. Base: `Coord3R` with three `SingleRotation` components; rotations applied as R = Rx(E0)·Ry(E1)·Rz(E2).

**Source:** `libraries/TeenAstroCoord/`

---

## Class hierarchy

- **LA3** → **Coord3R** (m_Eulers[3]) → Coord_EQ, Coord_HO, Coord_IN
- **Coord3R** → Coord_LO (Axis1 has **positive** sign in Rz)

---

## Coord_EQ (Equatorial)

**Euler sequence:** R = Rx(FrE)·Ry(Dec)·Rz(−Ha)

- **FrE** — field rotation (often 0)
- **Dec** — declination (radians)
- **Ha** — hour angle; RA = LST − Ha

**Methods:** `FrE()`, `Dec()`, `Ha()`, `Ra(LST)`, `To_Coord_HO(Lat, Opt)`, `To_Coord_IN(Lat, Opt, misalignment)`, `To_Coord_LO(trafo)`.

---

## Coord_HO (Horizontal)

**Euler sequence:** R = Rx(FrH)·Ry(Alt)·Rz(−Az+π)

- **FrH** — field rotation
- **Alt** — altitude (radians)
- **Az** — azimuth (astronomical convention)
- **mIsApparent** — true if refraction applied

**Methods:** `FrH()`, `Alt()`, `Az()`, `ToApparent(opt)`, `ToTopocentric(opt)`, `To_Coord_EQ(Lat)`, `To_Coord_IN(misalignment)`.

---

## Coord_IN (Instrument)

**Euler sequence:** R = Rx(Axis3)·Ry(Axis2)·Rz(−Axis1)

Physical mount axes. The **misalignment** matrix (from alignment model) maps between HO and IN.

**Methods:** `Axis3()`, `Axis2()`, `Axis1()`, `To_Coord_HO(misalignmentinv, Opt)`, `To_Coord_EQ(misalignmentinv, Opt, Lat)`.

---

## Coord_LO (Local / logical)

**Euler sequence:** R = Rx(Axis3)·Ry(Axis2)·Rz(**+**Axis1)

Used with alignment transform T / Tinv for pier-side and logical frame.

**Methods:** `Axis3()`, `Axis2()`, `Axis1()`, `To_Coord_EQ(trafoinv)`.

---

## Conversion summary

| From | To | Key step |
|------|-----|----------|
| EQ | HO | Compose R_EQ with Ry(π/2−Lat), then getEulerRxRyRz; apply refraction if needed |
| HO | EQ | Compose R_HO with Ry(−(π/2−Lat)), extract FrE, Dec, Ha |
| HO | IN | R_IN = R_HO · T (T = alignment matrix) |
| IN | HO | R_HO = R_IN · T⁻¹ |
| EQ | LO | R_LO = R_EQ · T⁻¹ |
| LO | EQ | R_EQ = R_LO · Tinv |

All conversions use `getEulerRxRyRz` to extract angles from the combined rotation matrix.

---

**See also:** [LA3](la3.md) · [Alignment (CoordConv)](alignment.md)

# Mount alignment (CoordConv)

`TeenAstroCoordConv` implements two-star alignment using the **Taki method** (Toshimi Taki, *Sky & Telescope*, Feb 1989). The raw transformation is refined with **SVD** to the nearest proper rotation (det = +1).

**Source:** `libraries/TeenAstroCoordConv/`

---

## CoordConv class

**Public:** `ax1[2]`, `ax2[2]` (reference axis angles), `T[3][3]` (HO→IN), `Tinv[3][3]` (IN→HO), `u[3][3]`, `v[3][3]` (SVD).  
**Methods:** `reset()`, `clean()`, `isReady()`, `getError()`, `getRefs()`, `getT`/`setT` (persistence), `setTinvFromT()`, `addReference(angle1, angle2, axis1, axis2)`, `calculateThirdReference()`, `minimizeAxis1(offset)`, `minimizeAxis2()`.

---

## Taki method

1. **Two reference stars:** For i ∈ {0,1}, record sky (Az, Alt) and instrument (axis1, axis2). Convert to direction cosines: d_HD,i = toDirCos(Az_i, Alt_i), d_AA,i = toDirCos(axis1_i, axis2_i).

2. **Third reference:** d_HD,2 = normalize(d_HD,0 × d_HD,1), d_AA,2 = normalize(d_AA,0 × d_AA,1).

3. **Transformation:** Build 3×3 matrices D_HD, D_AA (rows = the three directions). T = D_AAᵀ·(D_HDᵀ)⁻¹; Tinv = T⁻¹.

4. **Angular error:** anglediff = angle(d_HD,0, d_HD,1) − angle(d_AA,0, d_AA,1) (consistency check).

---

## SVD correction

T from Taki can have det ≠ 1 due to noise. SVD: Tinv = U·Σ·Vᵀ. Replace Σ with D = diag(1, 1, det(U)·det(V)) so R_opt = U·D·Vᵀ has det = +1. Then set Tinv = R_opt and T = Tinvᵀ.

---

## Minimization

- **minimizeAxis1(offset):** Shift axis1 by offset for both refs, recompute third ref and T.
- **minimizeAxis2():** Iterative (5 steps) Newton-like update to reduce anglediff by adjusting axis2; damping 0.8.

---

## Firmware usage

| Operation | Matrix | Conversion |
|-----------|--------|------------|
| Current sky position | T | IN → HO → EQ |
| Goto target | Tinv | EQ → HO → IN |
| Sync | Tinv | EQ → IN (set stepper positions) |
| Tracking rates | Tinv | Δ(EQ) → Δ(IN) / Δt |
| Altitude safety | T | IN → HO |

**EEPROM:** T stored as 9 floats (EE_T11…EE_T33) + EE_Tvalid. On boot, load T and set Tinv via `setTinvFromT()`.

---

## Known cone and perpendicularity

A 2-star alignment estimates the pole (azimuth and altitude) and the axis index. When **Mount error** is on (`knownGeom`, EEPROM use byte exactly 1) and at least one of the stored cone and perpendicularity is non-zero, `alignTwoStarKnownGeom` strips those angles from the two measured axes and writes them into the head. Both values are held, including a zero on the other term. The pole is still estimated. If both stored values are zero, or the feature is off, the session stays the classic Taki path (`minimizeAxis2` then `minimizeAxis1`).

Stored pole azimuth and altitude (`:SXKz` / `:SXKa`) are kept in EEPROM. A 2-star alignment does not substitute them for the pole the stars measure.

**4 Stars** (`:A0,r4#`) and **3+3 Stars** (`:A0,r6#`) are rigid sessions. They estimate head terms from the stars and do not read the stored mount error. From the third star the model is refit before the next goto, so a mount that starts far off tightens up star by star. Axis2 non-perpendicularity may be solved from three stars on one pier side. Cone is solved only when each pier side has at least three stars. **2 Stars Mech.** rebuilds the cold-boot baseline on `:AP#` and does not hold the stored cone or perpendicularity.

The hand controller offers the same **Mount error** item under **Mount** (directly above Refraction) and as the last line of **Align**. The web mount page uses the same order: Off/On, then the cone row, then the perpendicularity row. Each number row is the value, Upload, then the label and the unit (degrees, ±5).

---

**See also:** [Coordinate systems](coord.md) · [MainUnit firmware](../firmware/mainunit.md) · [LX200 commands](../../TeenAstroMainUnit/Commands.md)

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

**See also:** [Coordinate systems](coord.md) · [MainUnit firmware](../firmware/mainunit.md)

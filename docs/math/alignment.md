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

## CH, NP, ID, ME and MA

The terms follow Wallace's TPOINT equatorial convention.

| TPOINT | Meaning | Where it sits in the rotation |
|--------|---------|-------------------------------|
| CH | collimation, a constant east-west arc | `cone`, same sign |
| NP | non-perpendicularity, an east-west arc NP·sin(dec) | `perp`, opposite sign |
| ID | declination index | `idx2`, same sign |
| ME | polar elevation. Positive ME puts the dial pole below the true pole | opposite of `polErrorDeg(PE_EQ_ALT)` |
| MA | polar azimuth of the wedge in the **plumbed** horizontal frame | `polErrorDeg(PE_EQ_AZ)` times cos(latitude) |
| — | azimuth tilt of the wedge (radians on the horizon) | `polErrorDeg(PE_EQ_AZ)`, which is MA / cos(latitude) |

Wallace ME/MA assume a plumbed mount: the polar-axis error is the altitude and azimuth of the mechanical pole in the local horizontal frame. TeenAstro stores that as `setPoleError(dAz, dAlt)` with `MA = dAz·cos(lat)` and `ME = −dAlt`. CH, NP and ID stay in HeadGeom (`cone`, `−perp`, `idx2`).

The classic TPOINT tan/sec formula for MA is a different operator from the plumbed pole tilt used here — see [Extra: TPOINT vs TeenAstro geometry](#extra-tpoint-vs-teenastro-geometry) below. Use the plumbed transfer for inject/export that must match `:GXAa#` / `:GXAz#`. Keep the tan/sec formula only when comparing to OnStep / legacy TPOINT dial math.

Commands, the web page and the hand controller speak CH, NP, ID, ME and MA. The rotations stored in the head and in EEPROM stay the internal angles, so a saved model keeps pointing the same way. `:GXAp#` / `:SXAp,V#` convert NP. `:GXAa#` / `:SXKa,V#` convert ME. `:GXAz#` / `:SXKz,V#` convert MA. The same mechanical tilt also shifts the hour-angle zero by the tilt times sin(latitude); that shift is absorbed by the sync.

A 2-star alignment estimates the pole (the azimuth tilt and the altitude) and the axis index. When **Mount error** is on (`knownGeom`, EEPROM use byte exactly 1) and at least one of the stored CH and NP is non-zero, `alignTwoStarKnownGeom` strips those angles from the two measured axes and writes them into the head. Both values are held, including a zero on the other term. The pole is still estimated. If both stored values are zero, or the feature is off, the session stays the classic Taki path (`minimizeAxis2` then `minimizeAxis1`).

Stored MA and ME (`:SXKz` / `:SXKa`) are kept in EEPROM. EEPROM keeps the azimuth tilt, not MA. A 2-star alignment does not substitute them for the pole the stars measure.

**4 Stars** (`:A0,r4#`) and **3+3 Stars** (`:A0,r6#`) are rigid sessions. They estimate head terms from the stars and do not read the stored mount error. From the third star the model is refit before the next goto, so a mount that starts far off tightens up star by star. NP may be solved from three stars on one pier side. CH is solved only when each pier side has at least three stars. **2 Stars Mech.** rebuilds the cold-boot baseline on `:AP#` and does not hold the stored CH or NP.

The hand controller offers the same **Mount error** item under **Mount** (directly above Refraction) and as the last line of **Align**. The web mount page uses the same order: Off/On, then the CH row, then the NP row. Each number row is the value, Upload, then the name and the unit (degrees, ±5). The sign on each row is Wallace's.

---

## Extra: TPOINT vs TeenAstro geometry

Patrick Wallace’s **TPOINT** equatorial geometrical terms (IH, ID, CH, NP, ME, MA) are the industry language for reporting mount errors. TeenAstro reuses those **names and signs** on the UI and over LX200, but the firmware does **not** implement the classic linearized tan/sec dial patch as its pointing engine. Pointing is an exact chain of rotations: HeadGeom (cone / non-perpendicularity / indices) plus a proper rotation matrix `T` for the polar axis in space.

This section explains what TPOINT actually is, how the two models differ, and why **MA** is the term where the gap shows up.

### What TPOINT was designed to do

TPOINT fits small pointing residuals in **hour angle / declination dial space**. The geometric terms are a short set of nearly orthogonal regressors for least-squares on sparse stars. The goal is practical: shrink pointing error on the observed sky for a nearly aligned telescope.

It was **not** written as a finite rigid-body model of a German equatorial (exact `SO(3)` head + polar axis). “Standard” here means widely used and validated for empirical pointing — not “exact physics of a GEM.”

```
  TPOINT question:     which ΔHA, ΔDec dial patches reduce residual on my stars?
  TeenAstro question:  what is the orientation of the head and polar axis in space?
```

Those coincide for several terms at small angle. They diverge for classic tan/sec **MA**.

### Classic TPOINT tan/sec formulas

Given mount dials \((H_m, \delta_m)\) and terms in radians, the usual first-order correction to true sky is (signs as in TheSkyX Appendix L / Wallace equatorial geometry):

\[
\begin{aligned}
\Delta H &=
  \mathrm{IH}
  + \mathrm{CH}\,\sec\delta
  + \mathrm{NP}\,\tan\delta
  + \mathrm{MA}\,\cos H\,\tan\delta
  - \mathrm{ME}\,\sin H\,\tan\delta \\[0.5em]
\Delta\delta &=
  \mathrm{ID}
  - \mathrm{ME}\,\cos H
  + \mathrm{MA}\,\sin H
\end{aligned}
\]

with \(\sec\delta = 1/\cos\delta\). Then approximately

\[
H_{\mathrm{true}} = H_m + \Delta H,\qquad
\delta_{\mathrm{true}} = \delta_m + \Delta\delta.
\]

Properties of that linear model:

- Valid only for **infinitesimal** (small) errors; it is not a finite rotation.
- Everything lives in **HA/Dec dial coordinates**, not as axes in Euclidean 3-space.
- On the meridian (\(H=0\)): ME shifts dec only (\(\Delta\delta = -\mathrm{ME}\)); MA shifts HA as \(\mathrm{MA}\,\tan\delta\) with no dec shift.
- Terms were chosen to be convenient for regression, not to match one unique mechanical assembly order.

Crosscheck transcription: `tests/pointing_crosscheck/tpoint_eq.py` (`corrections()`).

### TeenAstro exact geometry

TeenAstro separates two layers:

1. **HeadGeom** — finite rotations on the instrument head: collimation (`cone` = CH), non-perpendicularity (`perp` = −NP), declination index (`idx2` = ID), and the hour-angle index absorbed by sync.
2. **Polar axis `T`** — a proper rotation. The mechanical pole is a real direction in the local horizontal frame. `setPoleError(dAz, dAlt)` places that boresight; `polErrorDeg` reads it back.

```
                    true celestial pole
                           *
                          /|
                         / |
                        /  |  ME  (altitude of mechanical pole vs true)
                       /   |
                      /____|______ horizon
                     /  dAz
                    *
           mechanical polar axis

  Plumbed frame (local vertical known):
    dAlt  = altitude error of the polar axis  (TeenAstro pole altitude)
    dAz   = azimuth of that tilt on the horizon

  Wallace names exported by TeenAstro (plumbed transfer):
    ME = −dAlt
    MA =  dAz · cos(latitude)
```

Head terms (sketch of order of ideas, not a full product of matrices):

```
  sky  ──►  polar frame (T)  ──►  RA/Dec axes with
                                  cone (CH), perp (−NP), idx2 (ID)
                                  ──►  instrument pointing
```

So ME/MA in TeenAstro **are** the orientation of a unit vector (the mount pole) in space. After alignment, `T` is that orientation. CH/NP/ID are finite head angles, not tan/sec patches.

### Plumbed transfer (what `:GXAa#` / `:GXAz#` mean)

Wallace’s *names* ME/MA assume a **plumbed** mount: polar error is altitude and azimuth of the mechanical pole in the local horizontal frame. TeenAstro stores the horizon tilts and converts:

\[
\mathrm{MA} = d_{\mathrm{Az}}\cos\phi,\qquad
\mathrm{ME} = -d_{\mathrm{Alt}}
\]

\[
d_{\mathrm{Az}} = \frac{\mathrm{MA}}{\cos\phi},\qquad
d_{\mathrm{Alt}} = -\mathrm{ME}
\]

where \(\phi\) is site latitude. EEPROM keeps \(d_{\mathrm{Az}}\), not MA. The same geometric tilt also shifts hour-angle zero by a term of order \(d_{\mathrm{Az}}\sin\phi\); sync absorbs that index.

This plumbed path is what the firmware and the crosscheck **plumbedgrid** / **plumbedgoto** modes use. On an Alt/Az grid of ~1000 equal-area points, injecting plumbed ME/MA + HeadGeom CH/NP/ID and fitting recovers grid RMS ≈ 0″.

### Where tan/sec MA differs from a pole tilt

Classic MA is **not** “rotate the polar axis in azimuth by \(d_{\mathrm{Az}}=\mathrm{MA}/\cos\phi\)”. It is only the dial operator

\[
\Delta H = \mathrm{MA}\,\cos H\,\tan\delta,\qquad
\Delta\delta = \mathrm{MA}\,\sin H.
\]

Sketch of the mismatch:

```
  Physical MA (plumbed)              Classic tan/sec MA
  -----------------------            -----------------------
  Rotate polar axis about            Add linearized ΔH, Δδ
  local vertical / horizon           in HA–Dec dial space
  by dAz = MA / cos(lat)             (no finite SO(3) pole)

       true pole *                        true pole *
              /                                  :
             / dAz                               :  dial patch
            /                                    :  ΔH ∝ MA cosH tanδ
           * mech pole                           :  Δδ ∝ MA sinH
        (same vector for                         (not the same map
         every sky position)                      as a single pole vector)
```

Consequences:

| | Plumbed MA (TeenAstro `T`) | Tan/sec MA (classic TPOINT) |
|--|----------------------------|-----------------------------|
| Object | one pole direction in space | HA/Dec correction field |
| Valid for | finite angles (exact rotation) | infinitesimal only |
| Fit with HeadGeom + `T` | yes (grid RMS ~ 0) | no — leftovers alias into **NP** |
| Star RMS on 6 align stars | ~0 | can look small |
| True RMS on full sky grid | ~0 | can be **worse** than no model |

Even at ~30″, crosscheck section B (`tests/pointing_crosscheck/compare_onstep.py`) shows tan/sec MA-only: fit-star RMS a few arcseconds, but Alt/Az-grid RMS **higher** than the uncorrected dials, with recovered NP ≈ −(something like) 2×MA. ME / CH / NP / ID at the same scale recover cleanly on the grid.

ME’s tan/sec form is much closer to a real altitude tilt of the pole, which is why ME looks “fine” under both languages and MA does not.

### Side-by-side: same names, different operators

| Term | Classic TPOINT (tan/sec) | TeenAstro (exact) |
|------|--------------------------|-------------------|
| CH | \(\Delta H = \mathrm{CH}\sec\delta\) | HeadGeom `cone`, same sign |
| NP | \(\Delta H = \mathrm{NP}\tan\delta\) | HeadGeom `perp` = −NP |
| ID | \(\Delta\delta = \mathrm{ID}\) | HeadGeom `idx2` |
| ME | \(-\mathrm{ME}\cos H\) in dec, … | `ME = −dAlt` on `T` (close at small angle) |
| MA | \(\mathrm{MA}\cos H\tan\delta\) in HA, … | `MA = dAz·cosφ` on `T` (**not** the tan/sec operator) |
| IH | HA index | absorbed by sync |

### What to use when

- **UI, EEPROM, `:GXAa#` / `:GXAz#` / `:SXKa` / `:SXKz`:** plumbed ME/MA ↔ `setPoleError` / `polErrorDeg`.
- **Physics, polar align, inject/export that must match the mount:** HeadGeom + plumbed pole.
- **OnStep / legacy TPOINT dial comparisons only:** tan/sec `corrections()` in the crosscheck; do not treat that MA as TeenAstro’s exported MA.

### Bench reference

`tests/pointing_crosscheck/compare_onstep.py`:

- **A** — HeadGeom self-consistency on the 1000-point grid (must be ~0″).
- **B** — Wallace tan/sec truth, tiny pole; score = grid RMS (shows MA failure mode).
- **C** — plumbed ME/MA + HeadGeom CH/NP/ID (must be ~0″ grid RMS, export matches inject).

---

**See also:** [Coordinate systems](coord.md) · [MainUnit firmware](../firmware/mainunit.md) · [LX200 commands](../../TeenAstroMainUnit/Commands.md)

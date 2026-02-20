# How tracking is performed in TeenAstro

This document describes how the firmware drives the mount to track the sky (sidereal, solar, lunar, or user-defined rates), how rates are converted into axis motion, and how tracking interacts with guiding and limits.

---

## 1. Overview

TeenAstro uses **target-based tracking**: each axis has a current position (`pos`) and a **target** position in steps. When tracking is on, the target is advanced every **sidereal tick** (1/100 of a sidereal second) by a small fraction of a step. Stepper timers then move the motors so that `pos` follows the target. The rate at which the target moves is derived from the chosen tracking mode (sidereal, solar, lunar, or custom) and from the mount geometry (equatorial vs alt‑az, with optional refraction and alignment).

- **Sidereal clock** – A hardware timer (Timer1) fires at a fixed interval that represents one centi-second of **sidereal** time. That interval is tunable (`siderealClockSpeed`, typically ~997 269.56 µs for 0.01 sidereal seconds). The same timer drives the logical “sidereal tick” used for updating tracking targets and for combining tracking with guiding.
- **Tracking rates** – Requested rates are in **hour angle** (HA) and **declination** (Dec), in “sidereal units”: 1 = one sidereal day per day. They are converted into **axis rates** (axis 1 = RA/HA, axis 2 = Dec) either by a simple equatorial formula or by **compensation** (see below).
- **Step generation** – Two motor timers (Timer3, Timer4) run at intervals derived from the current axis speeds (tracking + guiding). Each tick, the controller compares `target` and `pos`, sets direction, and issues one step. So the mount physically follows the moving target.

---

## 2. Tracking state and modes

Relevant state lives in `MountTracking` (`MountTracking.h`) and in the axis `StatusAxis` structures (`Axis.hpp`):

- **`sideralTracking`** – Whether tracking is enabled (volatile, used from ISR and main loop).
- **`sideralMode`** – Current mode: `SIDM_STAR`, `SIDM_SUN`, `SIDM_MOON`, or `SIDM_TARGET` (user-defined).
- **`RequestedTrackingRateHA`**, **`RequestedTrackingRateDEC`** – Requested rates in “sidereal units” (1 = sidereal, &lt;1 e.g. solar/lunar). Stored and applied when starting tracking or when the user changes rate.
- **`staA1.RequestedTrackingRate`**, **`staA2.RequestedTrackingRate`** – Resulting rates for axis 1 and axis 2 in **hour-arc-seconds per second** (or equivalent), computed from the requested HA/Dec rates and mount type. These are the values used to advance the target and to set the step timers.

Numeric constants (`MountTypes.h`):

- **Sidereal:** 1  
- **Solar:** 0.99726956632  
- **Lunar:** 0.96236513150  

User-defined target rates come from stored RA/DEC drift values (e.g. `:SXRe#` / `:SXRf#`, `:TT#`).

---

## 3. Sidereal clock and tick

- **Timer1** runs at interval `siderealClockSpeed * 0.01` µs (one centi-second of sidereal time). Its ISR:
  - Increments **`rtk.m_lst`** (logical sidereal time in centi-seconds).
  - Updates the **step intervals** for both axes from the current tracking + guiding rate (and backlash), so the motor timers run at the right speed.
  - Applies backlash logic (take-up interval after backlash correction).

- The **main loop** (`Application.cpp`) checks **`rtk.updatesiderealTimer()`** (i.e. whether `m_lst` has changed). When it has, it calls **`mount.onSiderealTick(phase, forceTracking)`** with `phase = m_lst % 100`.

So the “tracking clock” is strictly sidereal: one tick = 1/100 sidereal second. Solar/lunar/custom rates are implemented by moving the target **slower or faster** than one sidereal day per day, not by changing the tick period.

---

## 4. Advancing the target (onSiderealTick)

Inside **`Mount::onSiderealTick()`** (`MountQueriesTracking.cpp`):

1. **If tracking is on** and not in a goto:
   - **`axes.staA1.target += axes.staA1.fstep`**
   - **`axes.staA2.target += axes.staA2.fstep`**
   (with interrupts disabled and skipping while backlash correction is active.)

2. **`fstep`** is the **fractional step per sidereal centi-second** for that axis. It is set in **`applyTrackingRates()`** (called from `computeTrackingRate(true)` and from the tick when `phase == 0`):

   - `staA1.fstep = geoA1.stepsPerCentiSecond * staA1.CurrentTrackingRate`
   - `staA2.fstep = geoA2.stepsPerCentiSecond * staA2.CurrentTrackingRate`

**`stepsPerCentiSecond`** = steps per rotation / 86 400 000 (one rotation per sidereal day, in centi-seconds). So for **CurrentTrackingRate = 1** (sidereal), the target moves by exactly the steps that correspond to one sidereal day per day. For solar or lunar, **RequestedTrackingRateHA** is &lt;1, so the computed axis rates are proportionally smaller and the target advances more slowly.

So: **each sidereal tick, the target is moved by a fraction of a step that corresponds to the current tracking rate.** The steppers then follow this moving target at the combined rate of tracking + guiding.

---

## 5. From requested HA/Dec rate to axis rates (computeTrackingRate)

**`Mount::computeTrackingRate(bool apply)`** turns **RequestedTrackingRateHA** and **RequestedTrackingRateDEC** into **RequestedTrackingRate** for each axis.

- **Equatorial (GEM, Fork), no refraction, no alignment:**  
  Simple conversion:
  - Axis 1 (HA): `RequestedTrackingRate = ±RequestedTrackingRateHA` (sign from hemisphere).
  - Axis 2 (Dec): `RequestedTrackingRate = ±RequestedTrackingRateDEC / 15` (sign from pier side).  
  If **tracking compensation** is RA-only (`TC_RA`), axis 2 rate is forced to 0.

- **Alt‑Az or (equatorial with refraction or alignment):**  
  **Compensation** is used: the code computes where the target would be in HA/Dec a short time (e.g. 60 s) before and after “now”, converts both to instrument angles (axis 1/2) via the alignment and refraction model, and derives **axis rates** from the angular difference over that time. So the mount follows the requested HA/Dec motion in the sky, including refraction and alignment. Again, `TC_RA` can force axis 2 to 0.

After computing **RequestedTrackingRate** for both axes, they are clamped to ±16 (hour-arc-seconds per second). **`applyTrackingRates()`** copies these into **CurrentTrackingRate** and recomputes **fstep** for both axes.

---

## 5a. Compensation in detail: how it works and why it is accurate

Compensation is used whenever the relationship between sky motion (HA, Dec) and axis motion is **not** the simple “axis1 = HA, axis2 = Dec” of a perfectly polar-aligned equatorial: that is, for **alt‑az** mounts, and for **equatorial** mounts when **refraction** and/or an **alignment model** is active. In those cases, the correct axis rates depend on where you are on the sky and on the same geometry and corrections used for goto and sync. The compensation path computes those rates by **numerically differentiating** the full coordinate transformation over a short time window, so the result is consistent with the rest of the pointing system and remains accurate across the sky.

### When compensation is used

- **Alt‑az:** Always. Hour angle and declination map to azimuth and elevation in a position-dependent way (e.g. near the pole the same dHA can require large az/el changes). A single formula cannot give correct axis rates everywhere.
- **Equatorial with refraction:** If `refraction.forTracking` is on, apparent (HA, Dec) is not the same as the unrefracted direction; the mount must track in apparent coordinates, so the EQ→axis chain must include refraction.
- **Equatorial with alignment model:** If `alignment.hasValid` is true, the mount’s real axes do not match an ideal equatorial; the alignment matrix (and its inverse **Tinv**) encode polar misalignment, flexure, and other errors. Tracking must use the same “sky → instrument” map as goto and sync, so axis rates are derived through that map.

### Algorithm (doCompensationCalc + rateFromMovingTarget)

1. **Current (or target) position in the sky**  
   The code gets the current HA and Dec from either the current telescope position or the goto target (`getEqu()` or `getEquTarget()`), and the current pier side. Call this **(HA_now, Dec_now)** in degrees.

2. **Requested drift over a time window**  
   The user has set **RequestedTrackingRateHA** and **RequestedTrackingRateDEC** in “sidereal units” (1 = sidereal). Over **TimeRange** seconds (60 s in the code):
   - **DriftHA** = rateHA × TimeRange × 15 / 3600 (degrees of hour angle; 15° per hour).
   - **DriftDEC** = rateDEC × TimeRange / 3600 (degrees of declination).

3. **Two equatorial positions**  
   We define two points that the object would have if it moved at exactly that rate:
   - **EQ_prev** = (Dec_now − DriftDEC, HA_now − DriftHA)
   - **EQ_next** = (Dec_now + DriftDEC, HA_now + DriftHA)  
   So we have “where the star was” and “where the star will be” **in the sky**, symmetrically around “now”.

4. **Same coordinate chain as goto and sync**  
   **rateFromMovingTarget()** converts both **EQ_prev** and **EQ_next** into **instrument** (axis) space using the **same** pipeline as the rest of the firmware:
   - **EQ → horizon (HO)** with **refraction** if `refraction.forTracking` is enabled (same option as for tracking).
   - **Horizon → instrument (IN)** using **alignment.conv.Tinv** (inverse of the alignment matrix from the pointing model).
   - **IN → steps** via **angle2Step()** and the current pier side.  
   So we get **(axis1_before, axis2_before)** and **(axis1_after, axis2_after)** in steps.

5. **Axis motion in degrees**  
   The code computes the **shortest** axis motion (handling wraparound for axis1):
   - **axis1_delta** = (axis1_after − axis1_before) in **degrees** (using **distStepAxis1** and **stepsPerDegree**).
   - **axis2_delta** = same for axis 2.  
   Wraparound is normalized to [−180°, +180°] so the rate is the actual angular velocity.

6. **Rates in firmware units**  
   The axis motion happened over **2×TimeRange** seconds (from “before” to “after”). So:
   - **A1_trackingRate** = 0.5 × axis1_delta × (3600 / (TimeRange × 15))  
   - **A2_trackingRate** = 0.5 × axis2_delta × (3600 / (TimeRange × 15))  
   The factor 0.5 accounts for the 2×TimeRange span; the rest converts deg/s into the internal “hour-arc-seconds per second” units used for **RequestedTrackingRate**. These values are then clamped to ±16 and written to **staA1.RequestedTrackingRate** and **staA2.RequestedTrackingRate**. If **TC_RA** (RA-only compensation) is set, axis 2 is forced to 0.

So compensation answers: *“Over the next (and previous) 60 seconds, the object will move by this much in HA/Dec; through the same refraction and alignment we use for pointing, that corresponds to this much axis1 and axis2 motion; so these are the axis rates we need.”*

### Why this is accurate

- **Same math as goto and sync**  
  The only way to be consistent with pointing is to use the **same** EQ → IN transformation (refraction + alignment Tinv) for tracking rates. The firmware does that: **rateFromMovingTarget** uses the same **To_Coord_IN(..., alignment.conv.Tinv)** and refraction option as the rest of the mount logic. So whatever the alignment model corrects for (polar error, flexure, etc.) at a given position, the tracking rate at that position is exactly what keeps (HA, Dec) fixed in the sky **according to that model**. There is no separate “tracking model” that could drift relative to goto/sync.

- **No tangent-plane or first-order approximation**  
  We do **not** assume a small patch of sky and use a linearized (tangent-plane) map. We take two full sky positions (EQ_prev, EQ_next), run them through the full nonlinear chain (refraction, alignment, angle2Step), and measure the resulting axis deltas. So the rate is the **exact** finite-difference derivative of the true sky→axis map at the current position. The only approximation is the 60 s window, which is small enough that the rate is effectively instantaneous.

- **Refraction and alignment are fully included**  
  If refraction is on, the EQ→IN path includes it, so we track in **apparent** place. If an alignment model is active, Tinv encodes the real mount geometry; the computed axis rates are exactly those that keep (HA, Dec) fixed in the sky as seen by that model. So tracking stays accurate even with strong refraction or poor polar alignment, as long as the alignment model itself is valid.

- **Refreshed every 100 sidereal ticks**  
  **computeTrackingRate(true)** is called every 100 ticks (about once per sidereal second). So as the mount moves and the “sky → axis” map changes (e.g. different elevation, different part of the alignment fit), the axis rates are recomputed. That keeps compensation accurate over the whole sky and over time, without drift from a single initial linearization.

### Refraction vs. a fixed rate (e.g. King rate)

Using **refraction in the tracking loop** (when compensation is enabled and `refraction.forTracking` is on) is **better than applying a single fixed correction** such as the classical **King rate** (or similar constant or rate-only corrections). Refraction depends strongly on **elevation**: near the horizon it is large and changes quickly with altitude; near the zenith it is small. A fixed “King-style” rate correction cannot match that variation, so the star will drift in apparent position over time, especially at low elevations. By contrast, TeenAstro’s compensation uses the **same refraction model** as goto and sync (elevation- and optionally temperature/pressure-dependent). The computed axis rates therefore keep the object fixed in **apparent** (refracted) place at the current altitude. So tracking with refraction enabled is more accurate than tracking at a single “corrected” sidereal rate, in particular for long exposures or low declinations.

### Tracking compensates for mount misalignment

When an **alignment model** is active (`alignment.hasValid`), tracking **automatically compensates for polar misalignment** (and for flexure or other errors encoded in the model). The axis rates are derived by passing the requested sky motion through **Tinv**, the same inverse alignment matrix used for pointing. So the mount does **not** need to be perfectly polar-aligned for tracking to keep the star fixed: the firmware “knows” how the real axes differ from an ideal equatorial and applies the correct combination of axis 1 and axis 2 rates at each position. That is why, after a successful alignment (e.g. one- or multi-star), tracking stays accurate across the sky even if the polar axis is off by several degrees—the same corrections that make goto land on target also make the tracking rate correct for that geometry.

In short: **compensation is accurate because it reuses the full pointing pipeline (refraction + alignment) and computes axis rates by finite difference over a short window, so tracking and pointing stay mathematically consistent and the rate is exact at the current position.** The simple formula (axis1 = HA rate, axis2 = Dec/15) is used only when that pipeline is not needed (equatorial, no refraction, no alignment).

---

## 6. Step timers and following the target

- **Timer3** (axis 1) and **Timer4** (axis 2) run at a **variable interval** **`interval_Step_Cur`** (µs between steps). This interval is updated in the **Timer1** ISR from the current speed:

  - **Speed** = |tracking rate + guide rate| (or only guide rate when tracking is off).  
  - **interval_Step_Cur** = masterClockSpeed / speed (capped by min/max interval for slew limits).

- In the **Timer3/Timer4** ISRs, each tick:
  1. **updateDeltaTarget()** → **deltaTarget = target − pos**
  2. If **deltaTarget != 0** (or in backlash correction), set direction from the sign of **deltaTarget**, call **move()** to step the motor and update **pos**.

So the **physical position** `pos` is always chasing the **target**; the target moves at the tracking rate (plus any guiding applied in the interval computation). Backlash is handled by a separate take-up phase and interval so that steps are not lost when reversing.

---

## 7. When tracking is updated and (re)started

- **computeTrackingRate(true)** is called:
  - When **starting** tracking (**startSideralTracking()**),
  - Every **100 sidereal ticks** (phase == 0 in **onSiderealTick**), so axis rates are refreshed regularly (e.g. for changing geometry or compensation).
- **applyTrackingRates()** copies **RequestedTrackingRate** → **CurrentTrackingRate** and recomputes **fstep**; the Timer1 ISR then uses the new rates to set **interval_Step_Cur** for the next period.

Tracking is **turned off** during goto, park, home, and when safety checks fail (limits, meridian, under-pole, altitude, motor fault). It can be re-enabled after a short “force tracking” grace period so that brief limit crossings do not immediately stop tracking.

---

## 8. Guiding and backlash

- **Guiding** (ST4, pulse, or recenter) adds an extra rate to one or both axes. In the Timer1 ISR, the **interval** for each axis is computed from **|CurrentTrackingRate + guide rate|** (or only guide rate when tracking is off). So the **target** still advances only at the tracking rate, but the **steppers** run faster or slower to correct for guide commands, and **pos** catches up or backs off relative to the target.
- **Backlash**: when the axis reverses, a fixed number of steps are taken at a dedicated **backlash** interval to take up gear slack. During that time, the tracking target is still advanced; after backlash, the interval is briefly set to a **take-up** value so the axis catches up to the target.

---

## 9. Summary flow

1. **Sidereal clock** (Timer1) ticks every centi-second of sidereal time; it updates axis step intervals from (tracking rate + guide rate) and backlash state.
2. **Main loop** sees a new tick and calls **onSiderealTick()**. If tracking is on, **target += fstep** for each axis. Every 100 ticks, **computeTrackingRate(true)** refreshes axis rates and **fstep**.
3. **Motor timers** (Timer3, Timer4) fire at **interval_Step_Cur**. Each fire: **deltaTarget = target − pos**; if non-zero, one step is issued and **pos** updated. The mount thus follows the moving target at the combined tracking + guiding rate.
4. **Rates** are set by **setTrackingRate()** / **computeTrackingRate()** from the chosen mode (sidereal/solar/lunar/target) and mount type (equatorial vs alt‑az, with or without refraction and alignment), producing axis **RequestedTrackingRate** and then **CurrentTrackingRate** and **fstep**.

This design keeps the sidereal clock uniform, uses a single “target in steps” model for both axes, and integrates guiding and backlash without changing the underlying tracking formulation.

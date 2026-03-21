# How tracking is performed in TeenAstro

This document describes how the firmware drives the mount to track the sky (sidereal, solar, lunar, or user-defined rates), how rates are converted into axis motion, and how tracking interacts with guiding and limits.

---

## 1. Overview

TeenAstro uses **target-based tracking**: each axis has a current position (`pos`) and a **target** position in steps. When tracking is on, the target is advanced every **sidereal tick** (1/100 of a sidereal second) by a small fraction of a step. Stepper timers then move the motors so that `pos` follows the target. The rate at which the target moves is derived from the chosen tracking mode (sidereal, solar, lunar, or custom) and from the mount geometry (equatorial vs alt‑az, with optional refraction and alignment).

- **Sidereal clock** – A hardware timer (Timer1) fires at a fixed interval that represents one centi-second of **sidereal** time. That interval is tunable (`siderealClockSpeed`, typically ~997 269.56 µs for 0.01 sidereal seconds). The same timer drives the logical "sidereal tick" used for updating tracking targets and for combining tracking with guiding.
- **Tracking rates** – Requested rates are in **hour angle** (HA) and **declination** (Dec), in "sidereal units": 1 = one sidereal day per day. They are converted into **axis rates** (axis 1 = RA/HA, axis 2 = Dec) either by a simple equatorial formula or by **compensation** (see below).
- **Step generation** – Two motor timers (Timer3, Timer4) run at intervals derived from the current axis speeds (tracking + guiding). Each tick, the controller compares `target` and `pos`, sets direction, and issues one step. So the mount physically follows the moving target.

---

## 2. Tracking state and modes

Relevant state lives in `MountTracking` (`MountTracking.h`) and in the axis `StatusAxis` structures (`Axis.hpp`):

- **`sideralTracking`** – Whether tracking is enabled (volatile, used from ISR and main loop).
- **`sideralMode`** – Current mode: `SIDM_STAR`, `SIDM_SUN`, `SIDM_MOON`, or `SIDM_TARGET` (user-defined).
- **`RequestedTrackingRateHA`**, **`RequestedTrackingRateDEC`** – Requested rates in "sidereal units" (1 = sidereal, <1 e.g. solar/lunar). Stored and applied when starting tracking or when the user changes rate.
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

So the "tracking clock" is strictly sidereal: one tick = 1/100 sidereal second. Solar/lunar/custom rates are implemented by moving the target **slower or faster** than one sidereal day per day, not by changing the tick period.

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

**`stepsPerCentiSecond`** = steps per rotation / 86 400 000 (one rotation per sidereal day, in centi-seconds). So for **CurrentTrackingRate = 1** (sidereal), the target moves by exactly the steps that correspond to one sidereal day per day. For solar or lunar, **RequestedTrackingRateHA** is <1, so the computed axis rates are proportionally smaller and the target advances more slowly.

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
  **Compensation** is used: the code computes where the target would be in HA/Dec a short time (e.g. 60 s) before and after "now", converts both to instrument angles (axis 1/2) via the alignment and refraction model, and derives **axis rates** from the angular difference over that time. So the mount follows the requested HA/Dec motion in the sky, including refraction and alignment. Again, `TC_RA` can force axis 2 to 0.

After computing **RequestedTrackingRate** for both axes, they are clamped to ±16 (hour-arc-seconds per second). **`applyTrackingRates()`** copies these into **CurrentTrackingRate** and recomputes **fstep** for both axes.

---

## 5a. Compensation in detail

Compensation is used whenever the relationship between sky motion (HA, Dec) and axis motion is **not** the simple "axis1 = HA, axis2 = Dec" of a perfectly polar-aligned equatorial: for **alt‑az** mounts, and for **equatorial** when **refraction** and/or an **alignment model** is active. The compensation path computes axis rates by **numerically differentiating** the full coordinate transformation over a short time window (e.g. 60 s).

**Algorithm (doCompensationCalc + rateFromMovingTarget):**

1. Get current HA, Dec (or target).
2. DriftHA = rateHA × TimeRange × 15/3600 (degrees); DriftDEC = rateDEC × TimeRange/3600.
3. EQ_prev = (Dec_now − DriftDEC, HA_now − DriftHA); EQ_next = (Dec_now + DriftDEC, HA_now + DriftHA).
4. Convert both EQ_prev and EQ_next to instrument (axis) space via **same** pipeline as goto/sync: EQ → HO (with refraction if forTracking) → IN (alignment.conv.Tinv) → steps.
5. axis1_delta, axis2_delta in degrees (shortest path, wraparound handled).
6. A1_trackingRate = 0.5 × axis1_delta × (3600/(TimeRange×15)); same for A2. Clamp to ±16; if TC_RA, axis 2 = 0.

**Why accurate:** Same math as goto and sync; no tangent-plane approximation; refraction and alignment fully included; refreshed every 100 sidereal ticks. Tracking compensates for polar misalignment when an alignment model is active.

---

## 6. Step timers and following the target

- **Timer3** (axis 1) and **Timer4** (axis 2) run at **`interval_Step_Cur`** (µs between steps). This interval is updated in the **Timer1** ISR from the current speed (tracking rate + guide rate; capped by min/max for slew limits).
- In the Timer3/Timer4 ISRs: **updateDeltaTarget()** → deltaTarget = target − pos; if non-zero, set direction, **move()** one step, update **pos**.

So **pos** chases **target**; target moves at tracking rate; guiding changes the step interval so pos catches up or backs off.

---

## 7. When tracking is updated and (re)started

- **computeTrackingRate(true)** when starting tracking and every **100 sidereal ticks** (phase == 0).
- **applyTrackingRates()** copies RequestedTrackingRate → CurrentTrackingRate and recomputes **fstep**.
- Tracking is off during goto, park, home, and on safety failure; can re-enable after a short force-tracking grace period.

---

## 8. Guiding and backlash

- **Guiding:** Interval for each axis from |CurrentTrackingRate + guide rate|; target still advances at tracking rate only.
- **Backlash:** Reversal triggers fixed steps at backlash interval; then take-up interval so axis catches target.

---

## 9. Summary flow

1. Timer1 ticks every sidereal centi-second; updates axis step intervals from (tracking + guide) and backlash.
2. Main loop: onSiderealTick(); if tracking on, target += fstep each axis; every 100 ticks, computeTrackingRate(true).
3. Timer3/Timer4: deltaTarget = target − pos; step and update pos.
4. Rates from setTrackingRate() / computeTrackingRate() (mode and mount type, with or without compensation).

---

**See also:** [MainUnit firmware](mainunit.md) · [Alignment](../math/alignment.md)

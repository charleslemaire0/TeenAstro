# Mount class design: hierarchy, sub-objects, and methods

**Goal:** Turn the current `MountState` struct (and global `mount`) into a `Mount` class with sub-objects and methods that encapsulate behaviour, replacing scattered `mount.*` access and free functions where it adds clarity.

**Scope:** TeenAstro MainUnit only. Axis types (`StatusAxis`, `GeoAxis`, `MotorAxis`, `GuideAxis`, `EncoderAxis`) from Axis.hpp / AxisEncoder.hpp are **unchanged**; the mount class **owns** them and exposes them via accessors or a small axes sub-object.

---

## 0. Units and coordinates

- **Coord types (libraries):** `Coord_EQ`, `Coord_HO`, `Coord_IN` use **radians** internally. EQ = equatorial (RA/Dec or Ha/Dec), HO = horizontal (Az, Alt), IN = instrument (Axis1, Axis2, Axis3).
- **Mount step/angle:** Axis positions and targets are in **steps** (`long`). `stepToAngle` / `angle2Step` use **degrees** for Axis1 and Axis2. For GEM, Axis1 is effectively hour angle (15° per hour).
- **Alignment:** Sky → mount uses **Tinv** (e.g. `To_Coord_IN(..., alignment.conv.Tinv)`). Mount → sky uses **T** (e.g. `getInstr().To_Coord_EQ(alignment.conv.T, ...)`).

---

## 1. Class hierarchy (bullet tree)

```
Mount  (top-level class; single global instance `mount`)
├── ParkHome        (struct)   — park/home and slew-settle state
├── Identity        (struct)   — mount type, name, alignment, meridian/decay options
├── Peripherals     (struct)   — push-to, focuser, GNSS flags
├── Limits          (struct)   — alt limits, meridian limits, under-pole, distance-from-pole
├── TargetCurrent   (struct)   — target (RA/Dec, Alt/Azm, pole side) and current (Alt/Azm)
├── Errors          (struct)   — lastError (and any related)
├── Tracking        (struct)   — sidereal/tracking state, rates, spiral, movingTo, abortSlew
├── Guiding         (struct)   — guiding state, rates, guide axes, pulse/sidereal timing
├── MotorsEncoders  (struct)   — enable flags, motor/encoder axes, intervals, reboot_unit
├── Axes            (struct)   — geoA1/A2, staA1/staA2 (existing types; no API change)
└── Reticule        (struct, optional) — reticuleBrightness (#ifdef RETICULE_LED_PINS)
```

**Naming convention:** Sub-objects are **structs** with **public members** (same as today) for Phase 1. Access: `mount.parkHome()`, `mount.limits()`, etc. — either as **member objects** (e.g. `Mount::ParkHome parkHome;`) so that call sites use `mount.parkHome.parkStatus`, or as **accessor methods** returning references (e.g. `ParkHome& parkHome();`) so that call sites use `mount.parkHome().parkStatus`. The design below uses **member objects** for simplicity and to avoid changing every call site to `mount.parkHome().x` in one go; **method-style accessors** can be added later if desired.

**Recommendation:** Use **member structs** with **public** data (e.g. `ParkHome parkHome;`) so that existing code can migrate from `mount.parkStatus` to `mount.parkHome.parkStatus` with a simple find-replace. No getters for sub-objects in Phase 1.

---

## 2. Sub-objects and members

### 2.1 ParkHome (struct)

| Member | Type | Notes |
|--------|------|--------|
| parkStatus | ParkState | |
| parkSaved | bool | |
| homeSaved | bool | |
| atHome | bool | |
| homeMount | bool | |
| slewSettleDuration | unsigned int | |
| lastSettleTime | unsigned long | |
| settling | bool | |
| backlashStatus | BacklashPhase | |

**Location:** Inline in Mount (e.g. `struct ParkHome { ... }; ParkHome parkHome;`) or in same header as Mount.

**Implementation:** State in MountParkHome.h (struct); full behaviour in MountParkHomeController.

---

### 2.2 Identity (struct)

| Member | Type | Notes |
|--------|------|--------|
| DecayModeTrack | bool | |
| meridianFlip | MeridianFlip | |
| mountType | Mount | |
| mountName | char[maxNumMount][MountNameLen] | |
| isMountTypeFix | bool | |
| maxAlignNumStar | byte | |
| autoAlignmentBySync | bool | |

---

### 2.3 Peripherals (struct)

| Member | Type | Notes |
|--------|------|--------|
| PushtoStatus | Pushto | |
| hasFocuser | bool | |
| hasGNSS | bool | |

---

### 2.4 Limits (struct)

| Member | Type | Notes |
|--------|------|--------|
| minAlt | int | |
| maxAlt | int | |
| minutesPastMeridianGOTOE | long | |
| minutesPastMeridianGOTOW | long | |
| underPoleLimitGOTO | double | |
| distanceFromPoleToKeepTrackingOn | int | |

---

### 2.5 TargetCurrent (struct)

| Member | Type | Notes |
|--------|------|--------|
| newTargetPoleSide | PoleSide | |
| newTargetAlt | double | |
| newTargetAzm | double | |
| newTargetDec | double | |
| newTargetRA | double | |
| currentAzm | double | |
| currentAlt | double | |

**Implementation:** State in MountTargetCurrent.h (struct); usage in MountGotoSync, etc.

---

### 2.6 Errors (struct)

| Member | Type | Notes |
|--------|------|--------|
| lastError | ErrorsTraking | |

---

### 2.7 Tracking (struct)

**Volatile / ISR-used:** sideralTracking, sideralMode, movingTo — keep as `volatile` where used from timer/ISR.

| Member | Type | Notes |
|--------|------|--------|
| siderealClockSpeed | double | |
| trackComp | TrackingCompensation | |
| lastSideralTracking | bool | |
| sideralTracking | volatile bool | |
| sideralMode | volatile SID_Mode | |
| RequestedTrackingRateHA | double | |
| RequestedTrackingRateDEC | double | |
| storedTrakingRateRA | long | |
| storedTrakingRateDEC | long | |
| lastSetTrakingEnable | unsigned long | |
| lastSecurityCheck | unsigned long | |
| abortSlew | bool | |
| doSpiral | bool | |
| SpiralFOV | double | |
| movingTo | volatile bool | |

---

### 2.8 Guiding (struct)

**Volatile:** GuidingState, activeGuideRate, recenterGuideRate — used from ISR/timer.

| Member | Type | Notes |
|--------|------|--------|
| GuidingState | volatile Guiding | |
| lastGuidingState | Guiding | |
| guideRates | double[5] | |
| activeGuideRate | volatile byte | |
| recenterGuideRate | volatile byte | |
| guideA1 | GuideAxis | existing type |
| guideA2 | GuideAxis | existing type |
| pulseGuideRate | float | |
| DegreesForAcceleration | double | |

---

### 2.9 MotorsEncoders (struct)

| Member | Type | Notes |
|--------|------|--------|
| enableMotor | bool | |
| motorA1 | MotorAxis | existing type |
| motorA2 | MotorAxis | existing type |
| enableEncoder | bool | |
| EncodeSyncMode | EncoderSync | |
| encoderA1 | EncoderAxis | existing type |
| encoderA2 | EncoderAxis | existing type |
| minInterval1 | interval | |
| maxInterval1 | interval | |
| minInterval2 | interval | |
| maxInterval2 | interval | |
| reboot_unit | bool | |

---

### 2.10 Axes (struct)

| Member | Type | Notes |
|--------|------|--------|
| geoA1 | GeoAxis | unchanged API |
| geoA2 | GeoAxis | unchanged API |
| staA1 | StatusAxis | unchanged API |
| staA2 | StatusAxis | unchanged API |

Existing code passes `mount.staA1`, `mount.geoA2` etc. to free functions; after refactor use `mount.axes.staA1` or keep a top-level accessor `mount.staA1` that returns `axes.staA1` so call sites need not change.

---

### 2.11 Reticule (struct, optional)

| Member | Type | Notes |
|--------|------|--------|
| reticuleBrightness | int | #ifdef RETICULE_LED_PINS |

---

## 3. Candidate methods

### 3.1 Mount (top-level) — queries

| Method | Signature | Responsibility | Current implementation |
|--------|-----------|----------------|------------------------|
| isParked | `bool isParked() const` | true if parkStatus == PRK_PARKED | Compare mount.parkStatus (Park.cpp, etc.) |
| isAtHome | `bool isAtHome() const` | true if atHome | Compare mount.atHome (mountSim/teenastro maps status to atHome) |
| isSlewing | `bool isSlewing() const` | true if movingTo \|\| GuidingState != GuidingOFF | TelescopeBusy() in Astro.cpp:34–37 |
| getPoleSide | `PoleSide getPoleSide() const` | Pole side from current axis positions | GetPoleSide() in Astro.cpp:19–23; uses setAtMount, geoA2.poleDef |
| getTargetPoleSide | `PoleSide getTargetPoleSide() const` | Pole side from target axis2 | GetTargetPoleSide() in Astro.cpp:26–32; uses getPoleSide(axis2) |
| hasTracking | `bool hasTracking() const` | true if sideralTracking | Direct read of mount.sideralTracking |
| hasGuiding | `bool hasGuiding() const` | true if GuidingState != GuidingOFF | Part of TelescopeBusy() |
| lastError | `ErrorsTraking lastError() const` | Return lastError | Direct read |
| clearError | `void clearError()` | Set lastError to ERRT_NONE | Where lastError is cleared today |
| isAltAZ | `bool isAltAZ() const` | mountType == MOUNT_TYPE_ALTAZM \|\| FORK_ALT | isAltAZ() in Astro.cpp (decl. MainUnitDecl.h) |

### 3.2 Mount — actions / state changes

| Method | Signature | Responsibility | Current implementation |
|--------|-----------|----------------|------------------------|
| setParkStatus | `void setParkStatus(ParkState s)` | Set parkStatus | Direct write in Park.cpp, etc. |
| setTracking | `void setTracking(bool on)` | Set sideralTracking, optionally restore lastSideralTracking | Park/Home and tracking logic |
| setSiderealMode | `void setSiderealMode(SID_Mode m)` | Set sideralMode | Where sideralMode is set |
| setGuidingState | `void setGuidingState(Guiding g)` | Set GuidingState | Guide.cpp, Command_* |
| setTargetRaDec | `void setTargetRaDec(double ra, double dec)` | Set newTargetRA, newTargetDec | Command_M, Goto, etc. |
| setTargetAltAz | `void setTargetAltAz(double alt, double azm)` | Set newTargetAlt, newTargetAzm | Command_M, PushTo |
| setTargetPoleSide | `void setTargetPoleSide(PoleSide s)` | Set newTargetPoleSide | Command_S.cpp:1116, 1121, 1126; Command_others |
| abortSlew | `void abortSlew()` | Set abortSlew = true (and any side effects) | Command_Q, etc. |
| setMeridianFlip | `void setMeridianFlip(MeridianFlip m)` | Set meridianFlip | Command_S / EEPROM |

### 3.3 Mount — limits

| Method | Signature | Responsibility | Current implementation |
|--------|-----------|----------------|------------------------|
| getMeridianEastLimit | `long getMeridianEastLimit() const` | Return minutesPastMeridianGOTOE | Command_G.cpp:513–514 (:GXLE#) |
| getMeridianWestLimit | `long getMeridianWestLimit() const` | Return minutesPastMeridianGOTOW | Command_G.cpp:517–518 (:GXLW#) |
| checkAltitudeLimits | `bool checkAltitudeLimits(double alt) const` | alt within [minAlt, maxAlt] | Logic in Limit.cpp / Goto if any |
| getPoleSideFromAxis2 | `PoleSide getPoleSideFromAxis2(long axis2) const` | Under/over from axis2 steps | getPoleSide(const long &axis2) in Limit.cpp:12–14 |

### 3.4 Mount — initialisation

| Method | Signature | Responsibility | Current implementation |
|--------|-----------|----------------|------------------------|
| init | `void init()` | Default-initialise all sub-objects (or call their inits) | Mount::Mount() in Mount.cpp; initMount() in EEPROM.cpp |

### 3.5 Sub-object methods (optional, Phase 2+)

- **ParkHome:** `bool isParked() const`, `bool isAtHome() const`, `void setParkStatus(ParkState)`.
- **Tracking:** `bool isTracking() const`, `void setTracking(bool)`, `void setSiderealMode(SID_Mode)`.
- **Guiding:** `bool isGuiding() const`, `void setGuidingState(Guiding)`.
- **Limits:** `long getMeridianEastLimit() const`, `long getMeridianWestLimit() const`, `bool checkAltitude(double) const`.

These can delegate to Mount-level methods or live on the sub-structs for clarity.

---

## 4. Preserve existing types and APIs

- **StatusAxis, GeoAxis, MotorAxis, GuideAxis, EncoderAxis:** Do **not** change their public interface (Axis.hpp, AxisEncoder.hpp). Mount only **contains** them (in Axes / MotorsEncoders / Guiding).
- **Physical layout:** Prefer same layout so that code passing `mount.staA1` (or `mount.axes.staA1`) to existing functions (e.g. `staA1.atTarget(false)`) continues to work. If Mount exposes `axes.staA1` and `axes.staA2`, call sites become `mount.axes.staA1`; alternatively Mount can expose `StatusAxis& staA1()` returning `axes.staA1` so that `mount.staA1` still works.
- **Free functions that must remain (or take Mount&):**
  - **Astro.cpp:** GetPoleSide(), GetTargetPoleSide(), TelescopeBusy(), ApplyTrackingRate(), SetTrackingRate(), computeTrackingRate(), RateFromMovingTarget(), do_compensation_calc(), initMaxRate(), SetRates(), SetAcceleration(), enableGuideRate(), enableST4GuideRate(), enableRecenterGuideRate(), resetGuideRate(), isAltAZ(), SafetyCheck(), enable_Axis(), updateRatios(), updateSideral() — either become Mount methods or keep as free functions taking no args (using global `mount`) or `Mount& mount`.
  - **Limit.cpp:** getPoleSide(axis2), checkPole(), checkMeridian(), withinLimit(), setAtMount(), initLimit(), etc. — can take `Mount&` or `const Mount&` and use mount.axes, mount.limits.
  - **Goto.cpp:** StepToAngle, Angle2Step, syncAxis, GotoAxis, SyncInstr, syncEqu, syncAzAlt, predictTarget, goToEqu, goToHor, goTo, Flip — keep as free functions; they can take `Mount&` and use mount.axes, mount.targetCurrent, etc.
  - **Park.cpp, Home.cpp, Guide.cpp, Move.cpp, MoveTo.cpp, Timer.cpp:** Same idea: either become Mount methods or free functions that take `Mount&` and use sub-objects.

---

## 5. Migration and compatibility

### Phase 1: Introduce class and sub-objects (no behaviour change)

1. Define `Mount` class and nested structs (ParkHome, Identity, Peripherals, Limits, TargetCurrent, Errors, Tracking, Guiding, MotorsEncoders, Axes, Reticule) in Mount.h.
2. Replace the single flat `MountState` struct with `Mount` containing these members. Keep a single global `Mount mount;`.
3. Move current members into the appropriate sub-object (e.g. `parkStatus` → `mount.parkHome.parkStatus`). Update all call sites from `mount.parkStatus` to `mount.parkHome.parkStatus` (and similarly for every member).
4. Preserve volatile and layout where needed (e.g. Tracking and Guiding volatiles).
5. Build (pio run -e 240_5160) and fix any compile errors. Run mountSim if available.

**Compatibility:** No API change for Axis types. Call sites only change from `mount.x` to `mount.sub.x`.

### Phase 2: Add methods and replace free-function logic

1. Implement Mount methods (isParked, isAtHome, isSlewing, getPoleSide, getTargetPoleSide, getMeridianEastLimit, getMeridianWestLimit, etc.) by moving or wrapping logic from Astro.cpp, Limit.cpp, Command_G.cpp, etc.
2. Replace calls to GetPoleSide() with mount.getPoleSide(), TelescopeBusy() with mount.isSlewing(), etc., and gradually remove or thin out the free functions.
3. Keep free functions that are complex (e.g. syncEqu, goToHor, predictTarget) as they are; they can take `Mount&` and use mount.axes, mount.limits, etc., if desired.

**Compatibility:** Existing callers of GetPoleSide(), TelescopeBusy(), etc. are updated to use mount.* methods; free functions can remain as thin wrappers that call mount.*.

### Phase 3: Prefer accessors and sub-object access

1. Where direct member access is still used, consider replacing with accessors (e.g. mount.getParkStatus() instead of mount.parkHome.parkStatus) if it improves encapsulation.
2. Optionally add sub-object accessors (e.g. `ParkHome& parkHome() { return parkHome_; }`) if you prefer method style.

**Testing:** After each phase: build 240_5160, run mountSim (connect, status, tracking, goto/park), and any existing unit or integration tests.

---

## 6. Volatile and ISR constraints

- **Used from timer/ISR or in critical sections:** sideralTracking, sideralMode, movingTo, GuidingState, activeGuideRate, recenterGuideRate, and axis positions/speeds (staA1.pos, staA2.pos, etc.). Keep these `volatile` where they are read/written outside main loop.
- Sub-objects that hold volatile members (Tracking, Guiding, Axes) must not be copied by value in a way that could race; prefer references when passing to code that might run from ISR/timer.
- setAtMount(axis1, axis2) and GetPoleSide() use cli/sei around reads of staA1.pos, staA2.pos, staA2.target; any method that reads these should preserve that pattern.

---

## 7. Optional naming and style

### Access convention

**Direct member access** (`mount.parkHome.parkStatus`, `mount.tracking.movingTo`) is the default for code that already has `Mount` in scope. **Getters** (`mount.getParkHome()`, `mount.getLimits()`) are available for consistency and for code that prefers a single access style. New code in the same file as existing code should follow that file's current style; new modules may use either, but prefer direct members for hot paths (e.g. loop/ISR) and getters where it improves readability.

- **Naming (retained):** Existing spelling variants (e.g. Sideral, Traking) are retained for compatibility; do not introduce new variants.
- **Sub-objects:** `mount.parkHome`, `mount.identity`, `mount.peripherals`, `mount.limits`, `mount.targetCurrent`, `mount.errors`, `mount.tracking`, `mount.guiding`, `mount.motorsEncoders`, `mount.axes`, `mount.reticule`.
- **Methods:** `is*` for queries (bool), `get*` for getters, `set*` for setters; `clearError`, `abortSlew` for actions.
- **File layout:** Mount class and nested structs live in Mount.h; `extern Mount mount;` is in Mount.h, definition `Mount mount;` in Mount.cpp.

---

*Document generated as a plan for refactoring MountState into a Mount class with sub-objects and methods. No code changes are implied in this document; use it as the basis for a follow-up implementation task.*

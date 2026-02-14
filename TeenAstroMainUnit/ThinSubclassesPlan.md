# Plan: Improve thin Mount sub-classes

Several Mount sub-types are very thin: they hold state and only one to four trivial methods (one-liners), often with a dedicated .cpp file. This plan proposes ways to reduce fragmentation and clarify roles without breaking the existing design.

---

## 1. Current thin sub-classes

| Type | Data members | Methods (in .cpp) | Behaviour elsewhere |
|------|--------------|-------------------|----------------------|
| **MountErrors** | `lastError` | `lastErrorValue()`, `clearError()` | Error *setting* scattered (MountQueriesTracking, MountMove, MountGuiding, etc.) |
| **MountIdentity** | 5 (mountType, mountName, meridianFlip, etc.) | `isAltAZ()`, `setMeridianFlip()` | EEPROM load/save in EEPROM.cpp; used by Command_*, Mount |
| **MountParkHome** | 9 (parkStatus, atHome, slewSettleDuration, etc.) | `isParked()`, `isAtHome()`, `setParkStatus()` | Park/home *sequences* in MountParkHomeController |
| **MountTargetCurrent** | 7 (newTargetRA/Dec, currentAlt/Azm, etc.) | `setTargetRaDec()`, `setTargetAltAz()`, `setTargetPoleSide()` | Goto/sync/coord logic in MountGotoSync, Command_* |
| **MountTracking** | 14+ (sidereal, rates, spiral, etc.) | `hasTracking()`, `setTracking()`, `setSiderealMode()`, `requestAbortSlew()` | Tracking *logic* (apply rate, safety, spiral) in MountQueriesTracking |

**Already structs (no .cpp):** MountPeripherals, MountAlignment, MountMotorsEncoders, MountReticule — these are fine as data-only.

---

## 2. Improvement strategies

Three directions, from lowest to highest impact:

- **Strategy A — Convert to structs and inline:** Turn the thinnest types into structs; move their one-liner methods onto `Mount` (which already delegates) or inline in the header. Remove the .cpp files. No behaviour move; just fewer files and no “class with only one-liners”.

- **Strategy B — Consolidate related data:** Group related thin data into fewer top-level members (e.g. “identity + peripherals” or “errors as a member + methods on Mount only”). Reduces the number of sub-objects on `Mount` and clarifies “data bag” vs “behaviour” types.

- **Strategy C — Enrich thin classes:** Move related behaviour *into* the thin type so it owns both state and a clear slice of logic (e.g. MountErrors owns all set/clear error; MountIdentity owns load/save from EEPROM). Requires passing dependencies (Mount& or EEPROM) and more refactor.

Recommended order: do **Strategy A** first (low risk, quick win), then consider **B** or **C** as optional follow-ups.

---

## 3. Strategy A — Convert thinnest to structs and inline (recommended first)

Goal: Remove thin .cpp files and avoid “class with only one-liners” by making these types plain structs and moving the trivial methods to Mount or inline.

### 3.1 MountErrors

- **Current:** Class with `lastError`, `lastErrorValue()`, `clearError()` in MountErrors.cpp.
- **Change:** Make `MountErrors` a struct with a single member `ErrorsTraking lastError`. Remove `lastErrorValue()` (call sites use `mount.errors.lastError` or keep `mount.lastError()` on Mount). Move `clearError()` to Mount only (Mount already has `clearError()` delegating to `errors.clearError()`); update that to `errors.lastError = ERRT_NONE` and delete MountErrors.cpp.
- **Call sites:** Replace `mount.errors.lastErrorValue()` with `mount.errors.lastError` or `mount.lastError()`; replace `mount.errors.clearError()` with `mount.clearError()` (or direct assign if preferred). Mount’s `lastError()` and `clearError()` remain the public API.
- **Files:** Remove MountErrors.cpp; change MountErrors.h to a struct; update Mount.h/MountQueriesTracking.cpp and any other references.

### 3.2 MountIdentity

- **Current:** Class with 5 data members and `isAltAZ()`, `setMeridianFlip()` in MountIdentity.cpp.
- **Change:** Make `MountIdentity` a struct (data only). Move `isAltAZ()` to Mount only — Mount already has `isAltAZ()` delegating to `identity.isAltAZ()`; implement it inline in Mount (e.g. in MountQueriesTracking.cpp or a small inline in header). Replace `identity.setMeridianFlip(m)` with direct `identity.meridianFlip = m` at call sites, or keep a single inline `setMeridianFlip` in the struct (in the header). Delete MountIdentity.cpp.
- **Files:** Remove MountIdentity.cpp; MountIdentity.h becomes a struct with optional inline `setMeridianFlip`; update Mount and call sites.

### 3.3 MountParkHome

- **Current:** Class with 9 members and `isParked()`, `isAtHome()`, `setParkStatus()` in MountParkHome.cpp.
- **Change:** Make `MountParkHome` a struct (data only). Mount already has `isParked()`, `isAtHome()`, `setParkStatus()` delegating to `parkHome.*`; implement those on Mount by reading/writing `mount.parkHome` (e.g. `return parkHome.parkStatus == PRK_PARKED`). Remove delegation and delete MountParkHome.cpp. Call sites that use `mount.parkHome.setParkStatus(...)` become `mount.parkHome.parkStatus = ...` or go through `mount.setParkStatus(...)`.
- **Files:** Remove MountParkHome.cpp; MountParkHome.h becomes a struct; implement Mount’s isParked/isAtHome/setParkStatus in MountQueriesTracking.cpp (or Mount.cpp); update any direct parkHome method calls.

### 3.4 MountTargetCurrent

- **Current:** Class with 7 members and three setters in MountTargetCurrent.cpp.
- **Change:** Make `MountTargetCurrent` a struct. Setters are one-liners; either (a) inline them in the header as free functions or member inlines, or (b) remove them and use direct assignment at call sites. Mount’s `setTargetRaDec`, `setTargetAltAz`, `setTargetPoleSide` already delegate to targetCurrent; implement those on Mount by assigning to `targetCurrent.*` and delete MountTargetCurrent.cpp.
- **Files:** Remove MountTargetCurrent.cpp; MountTargetCurrent.h becomes a struct; Mount’s setters (in MountQueriesTracking or Mount.cpp) assign to targetCurrent; update call sites if they called targetCurrent setters directly.

### 3.5 MountTracking

- **Current:** Class with many members and 4 one-liners in MountTracking.cpp: `hasTracking()`, `setTracking()`, `setSiderealMode()`, `requestAbortSlew()`.
- **Change:** Either (a) make MountTracking a struct and implement these four on Mount (Mount already has setTracking, setSiderealMode, etc., delegating to tracking; implement by direct member access and remove MountTracking.cpp), or (b) keep MountTracking as a class but move the four one-liners into the header as inlines so MountTracking.cpp can be deleted. Option (a) is consistent with the other “data only” types; option (b) keeps a minimal “tracking state API” on the type.
- **Files:** Remove MountTracking.cpp; either turn MountTracking into a struct and implement the four methods on Mount, or keep class with inline methods in MountTracking.h.

### 3.6 Verification (Strategy A)

- After each type: build `pio run -e 240_5160`, run any tests.
- Ensure Mount’s public API (isParked, isAtHome, setParkStatus, lastError, clearError, isAltAZ, setTargetRaDec, etc.) unchanged where used from Command/Application.
- Update MountClassDesign.md and README if any “Implementation” or file lists mention the removed .cpp files.

---

## 4. Strategy B — Consolidate related data (optional)

Goal: Fewer top-level members on Mount by grouping “data only” sub-objects.

- **Identity + Peripherals:** Introduce a struct (e.g. `MountIdentityAndPeripherals` or keep two members but document as one “config” group). No need to merge types if the only gain is fewer lines in the doc; merging is useful if you want a single “mount config” block for init/save.
- **Errors:** If Strategy A is done, errors is already a single member; no further consolidation needed.
- **ParkHome + TargetCurrent:** Keep separate; they represent different domains (park/home vs. goto target). Optional: group under a single “MountState” or “MountStatus” that holds parkHome + targetCurrent + errors if you want one “mutable state” object. Low priority.

Recommendation: Apply Strategy B only if you explicitly want fewer top-level members (e.g. “config” and “state” groups). Otherwise Strategy A is enough.

---

## 5. Strategy C — Enrich thin classes (optional, larger refactor)

Goal: Give each thin type a clear behavioural responsibility so it’s no longer “anemic”.

- **MountErrors:** Add `setError(ErrorsTraking e)` and route all assignments to `lastError` through it (e.g. from MountQueriesTracking, MountMove, MountGuiding). Errors becomes “error state + set/clear API”. No new dependencies.
- **MountIdentity:** Add `loadFromEEPROM(...)` / `saveToEEPROM(...)` (or take EEPROM address helper) and move identity load/save from EEPROM.cpp into MountIdentity. Requires passing mount index or EEPROM context; identity then owns “identity data and persistence”.
- **MountParkHome:** Harder to enrich without duplicating MountParkHomeController. Option: move “status queries and status updates” (e.g. setParkStatus, atHome, parkStatus) into ParkHome and have Controller call into it; keep “sequences” (park(), goHome(), sync) in Controller. Clear split: “state + status API” vs “sequences”.
- **MountTargetCurrent:** Add “set from coord” helpers (e.g. `setTargetFromEqu(Coord_EQ)`, `setTargetFromHor(Coord_HO)`) that are currently in Command or Goto. TargetCurrent then owns “target representation and setting from different frames”. May need Mount& or coord conversion.
- **MountTracking:** Already has logic in MountQueriesTracking; could move “apply rate”, “safety check” into MountTracking if given Mount&. Large change; only if you want Tracking to be the single place for all tracking logic.

Recommendation: Do Strategy C only after A (and optionally B). Start with MountErrors (setError) and MountIdentity (load/save) as pilots; leave ParkHome, TargetCurrent, Tracking for later if needed.

---

## 6. Execution order (recommended)

1. **Strategy A.1** — MountErrors → struct, remove .cpp, Mount owns clearError/lastError.
2. **Strategy A.2** — MountIdentity → struct, remove .cpp, Mount owns isAltAZ; direct or inline setMeridianFlip.
3. **Strategy A.3** — MountParkHome → struct, remove .cpp, Mount owns isParked, isAtHome, setParkStatus.
4. **Strategy A.4** — MountTargetCurrent → struct, remove .cpp, Mount owns setTarget*.
5. **Strategy A.5** — MountTracking → struct or inline in header, remove .cpp.
6. Update MountClassDesign.md and README (thin-file list, Implementation notes).
7. **Optional:** Strategy B (consolidate) or Strategy C (enrich) for selected types.

---

## 7. Checklist summary

- [ ] A.1 MountErrors → struct, remove MountErrors.cpp.
- [ ] A.2 MountIdentity → struct, remove MountIdentity.cpp.
- [ ] A.3 MountParkHome → struct, remove MountParkHome.cpp.
- [ ] A.4 MountTargetCurrent → struct, remove MountTargetCurrent.cpp.
- [ ] A.5 MountTracking → struct or inline, remove MountTracking.cpp.
- [ ] Update docs (MountClassDesign, README).
- [ ] Optional B or C as needed.

After each step, run `pio run -e 240_5160` and fix any call sites or includes.

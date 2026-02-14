# TeenAstro Main Unit — Improvement Plan

This plan addresses the points identified in the codebase evaluation. Tasks are grouped by theme: documentation and convention first, then code quality (duplication, const, magic numbers, safety, volatile, naming), then optional follow-ups.

---

## 1. Update README (outdated source layout)

**Goal:** README should reflect the current architecture (Mount class, Mount*.cpp, Application) and remove references to deleted files.

**Actions:**

| Step | Action | File(s) |
|------|--------|--------|
| 1.1 | Replace the **Motion / control** bullet. Remove mention of `Move.cpp`, `MoveTo.cpp`, `Goto.cpp`, `Guide.cpp`, `Limit.cpp`, `Park.cpp`, `Home.cpp`, `ST4.cpp`. | `README.md` |
| 1.2 | Add an **Application** bullet: `Application.h`, `Application.cpp` — setup/loop and lifecycle. | `README.md` |
| 1.3 | Add a **Mount** section describing the Mount class and sub-modules. List the main Mount*.cpp files and their roles (e.g. Mount.cpp = constructor, init, delegation; MountLimits = limits; MountGotoSync = goto/sync; MountParkHome / MountParkHomeController = park/home; MountST4 = ST4; MountGuiding = guiding; MountMove / MountMoveTo = motion; MountQueriesTracking = tracking queries; etc.). Optionally add: “See MountClassDesign.md for the full design.” | `README.md` |
| 1.4 | Update the **Motion / control** line to the current set: `Timer.cpp`, `PushTo.cpp`, `Astro.cpp`, and the Mount motion/goto/guide/limit/park/home/ST4 logic in the various `Mount*.cpp` files. Keep the sentence concise (e.g. “Motion and control are implemented in Mount*.cpp and Timer.cpp; PushTo.cpp and Astro.cpp provide push-to and astro helpers.”). | `README.md` |
| 1.5 | Update **Entry** to mention that `TeenAstroMainUnit.cpp` delegates to `Application::setup()` and `Application::loop()`. | `README.md` |

**Verification:** Read through README; ensure no file name refers to a deleted file; ensure new contributors can find Mount-related code.

---

## 2. Document access style for Mount (direct members vs getters)

**Goal:** One clear convention so future code and reviews stay consistent.

**Actions:**

| Step | Action | File(s) |
|------|--------|--------|
| 2.1 | Choose and document the convention. Suggested wording (to be placed in **MountClassDesign.md** in a new “Access convention” subsection, and optionally summarized in README): “**Direct member access** (`mount.parkHome.parkStatus`, `mount.tracking.movingTo`) is the default for code that already has `Mount` in scope. **Getters** (`mount.getParkHome()`, `mount.getLimits()`) are available for consistency and for code that prefers a single access style. New code in the same file as existing code should follow that file’s current style; new modules may use either, but prefer direct members for hot paths (e.g. loop/ISR) and getters where it improves readability.” | `MountClassDesign.md`, optionally `README.md` |
| 2.2 | Add a short “Access convention” subsection under section 7 (Optional naming and style) in MountClassDesign.md with the chosen rule. | `MountClassDesign.md` |

**Verification:** No code change required in this task; later tasks (e.g. Reticule) can align with this convention.

---

## 3. Group and clarify MainUnitDecl.h

**Goal:** Make cross-module declarations easier to navigate and maintain.

**Actions:**

| Step | Action | File(s) |
|------|--------|--------|
| 3.1 | Add a short file header comment describing the purpose: “Declarations for functions that are implemented in other translation units and used from more than one place. Include from MainUnit.h. For command-specific or mount-specific APIs, prefer including the owning header where possible.” | `MainUnitDecl.h` |
| 3.2 | Group declarations by domain with section comments. Suggested groups: **Lifecycle** (reboot), **Timer** (interval2speed, speed2interval, SetsiderealClockSpeed, beginTimers), **Park / alignment** (saveAlignModel), **EEPROM / mount init** (AutoinitEEPROM, writeDefaultMounts, writeDefaultMount, writeDefaultMountName, initMount, initTransformation, initCelestialPole, initmotor, ReadEEPROMEncoderMotorMode, WriteEEPROMEncoderMotorMode, initencoder, readEEPROMmotorCurrent, readEEPROMmotor, writeDefaultEEPROMmotor, readEEPROMencoder, writeDefaultEEPROMencoder), **Command reply helpers** (replyShortTrue, …), **PushTo** (PushToEqu, PushToHor), **GNSS** (UpdateGnss, GNSSTimeIsValid, …), **Command_others** (Command_E). | `MainUnitDecl.h` |
| 3.3 | (Optional) In a later pass, consider moving declarations to the implementing module headers (e.g. Timer.hpp for timer functions, a small EEPROMDecl.h or similar for EEPROM init) and including those from MainUnitDecl.h or from MainUnit.h. Defer until after 3.1–3.2 are done and the build is green. | — |

**Verification:** Build succeeds; grep for any call site that includes MainUnitDecl.h or MainUnit.h and still finds the needed declarations.

---

## 4. Clarify intent of thin Mount*.cpp files

**Goal:** Make it explicit that some Mount sub-objects are “data + minimal helpers” and that larger behavior lives elsewhere (e.g. MountParkHomeController).

**Actions:**

| Step | Action | File(s) |
|------|--------|--------|
| 4.1 | In **MountParkHome.cpp**, add a brief comment at the top: “MountParkHome holds park/home state and minimal query/setter helpers. Park/home behaviour (park sequence, sync at park, init, etc.) is in MountParkHomeController.” | `MountParkHome.cpp` |
| 4.2 | In **MountTargetCurrent.cpp**, add a brief comment at the top: “MountTargetCurrent holds target and current position data; these setters are the minimal API. Goto/sync and coordinate logic use this data and live in MountGotoSync and related modules.” | `MountTargetCurrent.cpp` |
| 4.3 | (Optional) In **MountClassDesign.md**, in the sub-objects section (2.1 and 2.5), add one line each: “Implementation: state and minimal methods in MountParkHome.cpp; full behaviour in MountParkHomeController.” / “Implementation: state and setters in MountTargetCurrent.cpp; usage in MountGotoSync, etc.” | `MountClassDesign.md` |

**Verification:** Comments read clearly; build unchanged.

---

## 5. Align Reticule access with chosen convention

**Goal:** Use the same access style for reticule as for the rest of the codebase (direct member vs getter), in line with the convention from section 2.

**Actions:**

| Step | Action | File(s) |
|------|--------|--------|
| 5.1 | If the documented convention is **direct members**: add a one-line comment in Application.cpp and Command_others.cpp next to the first use of `mount.reticule.*`: “Reticule: direct member access (see Mount access convention).” No code change. | `Application.cpp`, `Command_others.cpp` |
| 5.2 | If the documented convention is **prefer getters in application/command layer**: switch to `mount.getReticule().reticuleBrightness` (and same for writes) in Application.cpp and Command_others.cpp. Ensure `Mount::getReticule()` is used consistently in those two files. | `Application.cpp`, `Command_others.cpp` |

**Verification:** Build succeeds; style matches the chosen convention.

---

## 6. Code quality

**Goal:** Improve readability, reduce duplication, strengthen const-correctness and safety, and document or fix known quality issues.

**Actions:**

| Step | Action | File(s) |
|------|--------|--------|
| 6.1 | **Reduce duplication — park/home checks.** Replace repeated conditions: use `mount.isParked()` where the intent is “parked”; `mount.isAtHome()` where the intent is “at home”; and `mount.isAtHome() \|\| mount.isParked()` where the intent is “at home or parked”. Audit: Command_G.cpp, Command_S.cpp, Command_GNSS.cpp, Command_others.cpp. This keeps behaviour consistent and centralises future changes. | Command_G.cpp, Command_S.cpp, Command_GNSS.cpp, Command_others.cpp |
| 6.2 | **Const-correctness.** Ensure query methods and getters that do not modify Mount are `const` (Mount already has const overloads for many getters). Audit any new or touched APIs; fix missing `const` on getters and on parameters that are not modified (e.g. `const Mount&` where applicable). | Mount*.h/cpp, call sites |
| 6.3 | **Magic numbers.** Audit Command_*.cpp, Application.cpp, and hot paths (e.g. MountGuiding, MountST4) for literal numbers that represent timeouts, limits, indices, or scaling factors. Replace with named constants (e.g. in an anonymous namespace or a small constants header). Application.cpp already does this; apply the same approach elsewhere where it improves clarity. | Command_*.cpp, Mount*.cpp, Application.cpp |
| 6.4 | **Long functions.** Identify the longest or most branched command handlers and Mount methods (e.g. in Command_S.cpp, Command_G.cpp, MountQueriesTracking.cpp). For each, either: (a) extract helper functions or private methods with clear names, or (b) add a short comment that the function is intentionally long (e.g. “large switch over command variants”) and keep it. Prefer (a) where it improves readability without fragmenting logic. | Command_S.cpp, Command_G.cpp, MountQueriesTracking.cpp |
| 6.5 | **Bounds and safety.** Where user or config data is used (serial buffers, EEPROM indices, array indices, step/angle ranges), ensure bounds are checked before use. Add checks or document assumptions (e.g. “caller guarantees buffer size”) at module boundaries. Focus on command parsing and EEPROM/address code first. | Command.cpp, Command_*.cpp, EEPROM.cpp, EEPROM_address.h |
| 6.6 | **Volatile and ISR usage.** Verify that every variable read or written from timer/ISR code is declared `volatile` where required. Cross-check MountClassDesign.md section 6 and the actual definitions in MountTracking, MountGuiding, and axis types. Add a one-line comment next to any volatile member explaining “used from ISR/timer” if not obvious. | MountTracking.h/cpp, MountGuiding.h/cpp, Axis.hpp, TelTimer.hpp |
| 6.7 | **Naming consistency (optional).** If the codebase has known spelling variants (e.g. “Sideral” vs “Sidereal”, “Traking” vs “Tracking”), either: document them as retained for compatibility and avoid introducing new variants, or plan a single rename pass with find-replace and build verification. Do not rename in the middle of other refactors. | MountTypes.h, MountTracking, MountGuiding, Astro.cpp |

**Verification:** After each step: build with `pio run -e 240_5160` (and optionally other envs). For 6.1, run a quick test or manual check that park/home behaviour is unchanged.

---

## 7. Optional follow-ups (after 1–6)

- **README module map:** If the Mount section in the README is still dense, add a short “Mount module map” table: File → Responsibility (e.g. MountLimits.cpp → limit data and checks; MountGotoSync.cpp → goto and sync; …).
- **Phase 3 (MountClassDesign):** If you later want to encourage getters over direct members in new code, add a “Phase 3 checklist” in MountClassDesign.md with a few high-traffic call sites to migrate as a pilot.
- **MainUnitDecl.h refactor:** After 3.1–3.2, consider moving EEPROM init declarations to EEPROM.cpp’s header (or a dedicated EEPROMDecl.h) and timer declarations to a Timer header, then include those from MainUnitDecl.h to reduce the size of MainUnitDecl.h.

---

## Execution order (recommended)

1. **Section 1** (README) — high impact, no risk to build.
2. **Section 2** (access convention in MountClassDesign.md) — unblocks 5.
3. **Section 4** (comments in thin Mount*.cpp) — quick, low risk.
4. **Section 3** (MainUnitDecl.h comments and grouping) — improves navigation; 3.3 optional.
5. **Section 5** (Reticule) — after 2 is done, choose direct vs getter and apply consistently.
6. **Section 6** (Code quality) — can be done in parallel or after 1–5; 6.1 (park/home) is a good first step; 6.2–6.7 as needed.
7. **Section 7** — as needed.

---

## Checklist summary

- [ ] 1.1–1.5 README updated (source layout, Application, Mount, Entry).
- [ ] 2.1–2.2 Access convention documented in MountClassDesign.md (and optionally README).
- [ ] 3.1–3.2 MainUnitDecl.h header comment and grouping; (optional) 3.3 move declarations.
- [ ] 4.1–4.3 Comments in MountParkHome.cpp, MountTargetCurrent.cpp; (optional) MountClassDesign.md.
- [ ] 5.1 or 5.2 Reticule: comment or switch to getReticule() per convention.
- [ ] 6.1–6.7 Code quality: park/home duplication, const-correctness, magic numbers, long functions, bounds/safety, volatile/ISR, naming (optional).
- [ ] 7 Optional: module map, Phase 3 checklist, MainUnitDecl refactor.

After each section, run `pio run -e 240_5160` to confirm the build still succeeds.

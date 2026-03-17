# Build and App Audit

Audit of the TeenAstro app and build process for **portability** (building on another machine from the repo) and **app bugs**, including a deep review of the planetarium and its painting routines.

---

## 1. Build on another machine - required files

### 1.1 All required files are in the repo

A full clone contains everything needed to build without extra generation steps:

| Requirement | Location | Notes |
|-------------|----------|--------|
| **App assets** | `teenastro_app/assets/` | Catalogs (messier.json, ngc.json, etc.), `stars_mag9.json`, `constellation_lines.json`, `constellation_names.json`, `milky_way.json`, and `assets/icon/` are **committed**. No need to run Python asset tools for a normal build. |
| **LX200 reply lengths** | `libraries/TeenAstroCommandDef/src/CommandReplyLength.h`, `teenastro_app/lib/models/lx200_reply_lengths.dart` | **Generated** from `libraries/TeenAstroCommandDef/data/command_reply_lengths.json` but **committed**. Run `python scripts/generate_reply_lengths.py` only when that JSON changes. |
| **Python tools** | `tools/extract_catalogs.py`, `tools/generate_star_catalog.py`, `tools/convert_stellarium_constellations.py` | Present in repo. Used by `build_app.ps1` only when app assets are missing (e.g. partial clone or deleted assets). |
| **Catalog sources** | `libraries/TeenAstroCatalog/src/catalogs/` | C PROGMEM sources for `extract_catalogs.py`. In repo. |

So: **building on another machine only needs a full clone + the documented prerequisites** (Flutter, Android SDK for APK, .NET/MSBuild for uploader/ASCOM, PlatformIO for firmware, etc.). No hidden "only on one PC" files.

### 1.2 Prerequisites (other machine)

Documented in **`BUILD_SETUP.md`** and **`scripts/BUILD_SCRIPTS.md`**:

- **App (Flutter):** Flutter SDK on PATH (or `-FlutterPath`); for Android: Android SDK + `ANDROID_HOME`; for Windows: Visual Studio with "Desktop development with C++".
- **Firmware:** PlatformIO (`pio` on PATH), optionally MinGW for unit tests (`scripts/setup_build_env.ps1`).
- **Uploader / ASCOM / MSI:** MSBuild (.NET 4.7.2), WiX Toolset 3 for MSI.

### 1.3 Build script portability fix (done)

- **`scripts/build_app.ps1`** previously had **hardcoded paths** for a single user (`C:\Users\charl\...` for `FlutterPath` and `DevelopRoot`). That would fail on another machine.
- **Changes made:**
  - `FlutterPath` default is now **empty** (rely on Flutter on PATH). Callers can pass `-FlutterPath "C:\path\to\flutter\bin"` if needed.
  - `DevelopRoot` is now **`$env:USERPROFILE`** (or `$env:HOME`), so `-FixFlutter` clones Flutter into the current user's home instead of a fixed path.
  - Error message when Flutter is not found now explains PATH / `-FlutterPath` / `-FixFlutter`.
- **`scripts/BUILD_SCRIPTS.md`** was updated to use a generic "from repo root" example instead of a user-specific path.

Other scripts (**`build_release.ps1`**, **`TeenAstroEmulator/installer/build.ps1`**) already use optional `-FlutterPath` and resolve repo root from the script location; no machine-specific paths were found there.

---

## 2. App bugs and behavior

### 2.1 TCP client disconnect order (fixed)

- **File:** `teenastro_app/lib/services/lx200_tcp_client.dart`
- **Issue:** In `disconnect()`, the code cancelled the stream subscription first, then closed the socket. If the subscription's `onDone` ran asynchronously, it could set `_socket = null` before `_socket?.close()` ran, so the socket might not be closed in some cases.
- **Fix:** Close the socket first, then cancel the subscription. That guarantees the connection is closed regardless of `onDone` timing.

### 2.2 Other app code reviewed

- **Timers:** `Timer.periodic` in `dashboard_screen.dart`, `sync_panel.dart`, and `alignment_screen.dart` are all cancelled in `dispose()` (or in the alignment screen's `_reset` / `_abortAlignment` and in the timer callback when done). No timer leak found.
- **Mount state / providers:** `MountStateNotifier` and `mount_state_provider` handle disconnected state and reconnection; `_pollLoop` checks `_client.isConnected` and reconnects with backoff. No obvious null/state bugs in the reviewed paths.
- **LX200 reply lengths:** App uses the generated `lx200_reply_lengths.dart`; as long as the JSON and generated files are in sync (both in repo), builds are consistent.

### 2.3 Optional hardening (not done in this audit)

- **`generate_star_catalog.py`** and **`convert_stellarium_constellations.py`** download from the internet (VizieR, Stellarium). If the repo's committed `stars_mag9.json` / `constellation_lines.json` are ever removed or not present (e.g. partial clone), a build would need network access to regenerate them. Keeping these files in git (current practice) avoids that.

---

## 3. Planetarium audit

Full review of `planetarium_screen.dart` and all supporting models: projection, painting routines, data loading, and user interaction.

### 3.1 Projection (`_Proj`)

**Stereographic zenithal projection** from RA/Dec to Alt/Az to screen. Correct implementation:

- Hour angle: `ha = (lst - raH) * 15 * pi/180` (hours to degrees to radians).
- Standard altitude formula: `sinAlt = sin(dec)*sin(lat) + cos(dec)*cos(lat)*cos(ha)`.
- Standard azimuth formula with correct quadrant disambiguation (`sin(ha) > 0` means west, use `2*pi - az`).
- Stereographic radius: `r = R*cos(alt) / (1 + sin(alt))`. Zenith maps to center, horizon to `r = R`.
- North at top, East to left (correct astronomical convention for zenith-up view).

### 3.2 Painting routines reviewed

| Layer | Method | Status |
|-------|--------|--------|
| Sky background + ground | Direct rect + path clip | Correct |
| Milky Way band | `_drawMilkyWay` | Correct; center+width polygon, clips to sky circle |
| Alt-Az grid | `_drawAltAzGrid` | Correct; concentric circles + radial lines |
| Equatorial grid | `_drawEqGrid` | Correct; RA hour lines and Dec circles |
| Constellation lines | `_drawConstellationLines` | Correct; J2000 to JNow per segment |
| Constellation names | `_drawConstellationNames` | Correct; centered text at J2000 to JNow position |
| Stars | `_drawStars` | Fixed (see 3.3); mag limit, B-V color, radial gradient |
| DSO symbols | `_drawDSOs` | Correct; Stellarium-like symbols per type |
| Planets | `_drawPlanets` | Correct; already equinox-of-date |
| Crosshair | `_drawCrosshair` | Correct; current mount RA/Dec |
| Target marker | `_drawTarget` | Correct; goto target if set |
| Cardinal labels | `_drawCardinal` | Correct; N/E/S/W at horizon edge |

### 3.3 Bugs found and fixed

#### Bug 1: RA/Dec seconds rounding to 60

- **Files:** `planetarium_screen.dart` (`_formatRa/Dec`), `catalog_entry.dart` (`raStr`/`decStr`)
- **Issue:** `.round()` on fractional seconds can produce 60 (e.g. 59.7 rounds to 60), giving "12:59:60".
- **Fix:** Carry logic: if seconds >= 60 increment minutes; if minutes >= 60 increment hours/degrees; if hours >= 24 wrap to 0.

#### Bug 2: `_centerOnObject` doesn't reset vertical pan

- **File:** `planetarium_screen.dart`
- **Issue:** Search-and-select set `_rotation` but not `_panY`, so object wasn't centered vertically.
- **Fix:** Reset `_panY = 0` and persist to `planetariumViewProvider`.

#### Bug 3: Star size clamp made faint-star cutoff unreachable

- **File:** `planetarium_screen.dart`, `_drawStars`
- **Issue:** Radius clamped to `[0.5, 22.0]` but cutoff was `r < 0.3`. Dead code; faintest stars inflated to 0.5px.
- **Fix:** Lowered clamp minimum to 0.15 so faint stars are properly culled at 0.3px.

#### Bug 4: constellation_names.json fallback format mismatch

- **File:** `scripts/build_app.ps1`
- **Issue:** Fallback created `{ "constellations": [] }` (JSON object) but parser expects a top-level JSON array.
- **Fix:** Fallback now creates `[]`.

### 3.4 Astronomy algorithms verified

- **Equinox precession** (`equinox_precession.dart`): Matches Meeus / TeenAstro firmware. Precession, nutation (17-term), and aberration all correct.
- **Planet positions** (`planet_positions.dart`): Keplerian elements (Standish/JPL), Kepler equation, heliocentric to geocentric to equatorial. Earth simplified (zero inclination/node), valid.
- **Julian Date**: Standard Meeus formula.

### 3.5 No issues found

- Data loading: all assets loaded with error handling; static caches prevent redundant loads.
- Gesture handling: pan/zoom/scroll correctly update and persist state.
- Tap hit-testing: correct nearest-within-30px with J2000 to JNow for stars/DSOs.
- `shouldRepaint` always true: correct since sidereal time changes continuously.

---

## 4. Summary

| Area | Status |
|------|--------|
| **Required files for build** | All present in repo; no "only on one machine" dependencies |
| **build_app.ps1 portability** | Fixed: no hardcoded user paths; Flutter from PATH or `-FlutterPath` |
| **BUILD_SCRIPTS.md** | Updated to generic run-from-repo-root example |
| **TCP disconnect** | Fixed: close socket before cancelling subscription |
| **Timers / disposal** | Reviewed; no leaks found |
| **Planetarium projection** | Stereographic zenithal; verified correct |
| **Painting routines** | All 12 layers reviewed; correct |
| **RA/Dec formatting** | Fixed: seconds=60 carry in planetarium + catalog_entry |
| **Center-on-object** | Fixed: panY reset when centering |
| **Star rendering** | Fixed: clamp minimum lowered so faint-star cutoff works |
| **constellation_names.json fallback** | Fixed: build_app.ps1 now creates `[]` (not `{}`) |
| **Astronomy algorithms** | Precession, nutation, aberration, planet positions all verified |

Building on another machine is supported with a full clone and the prerequisites in **BUILD_SETUP.md** and **scripts/BUILD_SCRIPTS.md**.

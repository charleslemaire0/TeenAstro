# Build and App Audit

Audit of the TeenAstro app and build process for **portability** (building on another machine from the repo) and **app bugs**.

---

## 1. Build on another machine – required files

### 1.1 All required files are in the repo

A full clone contains everything needed to build without extra generation steps:

| Requirement | Location | Notes |
|-------------|----------|--------|
| **App assets** | `teenastro_app/assets/` | Catalogs (messier.json, ngc.json, etc.), `stars_mag9.json`, `constellation_lines.json`, `constellation_names.json`, `milky_way.json`, and `assets/icon/` are **committed**. No need to run Python asset tools for a normal build. |
| **LX200 reply lengths** | `libraries/TeenAstroCommandDef/src/CommandReplyLength.h`, `teenastro_app/lib/models/lx200_reply_lengths.dart` | **Generated** from `libraries/TeenAstroCommandDef/data/command_reply_lengths.json` but **committed**. Run `python scripts/generate_reply_lengths.py` only when that JSON changes. |
| **Python tools** | `tools/extract_catalogs.py`, `tools/generate_star_catalog.py`, `tools/convert_stellarium_constellations.py` | Present in repo. Used by `build_app.ps1` only when app assets are missing (e.g. partial clone or deleted assets). |
| **Catalog sources** | `libraries/TeenAstroCatalog/src/catalogs/` | C PROGMEM sources for `extract_catalogs.py`. In repo. |

So: **building on another machine only needs a full clone + the documented prerequisites** (Flutter, Android SDK for APK, .NET/MSBuild for uploader/ASCOM, PlatformIO for firmware, etc.). No hidden “only on one PC” files.

### 1.2 Prerequisites (other machine)

Documented in **`BUILD_SETUP.md`** and **`scripts/BUILD_SCRIPTS.md`**:

- **App (Flutter):** Flutter SDK on PATH (or `-FlutterPath`); for Android: Android SDK + `ANDROID_HOME`; for Windows: Visual Studio with “Desktop development with C++”.
- **Firmware:** PlatformIO (`pio` on PATH), optionally MinGW for unit tests (`scripts/setup_build_env.ps1`).
- **Uploader / ASCOM / MSI:** MSBuild (.NET 4.7.2), WiX Toolset 3 for MSI.

### 1.3 Build script portability fix (done)

- **`scripts/build_app.ps1`** previously had **hardcoded paths** for a single user (`C:\Users\charl\...` for `FlutterPath` and `DevelopRoot`). That would fail on another machine.
- **Changes made:**
  - `FlutterPath` default is now **empty** (rely on Flutter on PATH). Callers can pass `-FlutterPath "C:\path\to\flutter\bin"` if needed.
  - `DevelopRoot` is now **`$env:USERPROFILE`** (or `$env:HOME`), so `-FixFlutter` clones Flutter into the current user’s home instead of a fixed path.
  - Error message when Flutter is not found now explains PATH / `-FlutterPath` / `-FixFlutter`.
- **`scripts/BUILD_SCRIPTS.md`** was updated to use a generic “from repo root” example instead of a user-specific path.

Other scripts (**`build_release.ps1`**, **`TeenAstroEmulator/installer/build_msi.ps1`**) already use optional `-FlutterPath` and resolve repo root from the script location; no machine-specific paths were found there.

---

## 2. App bugs and behavior

### 2.1 TCP client disconnect order (fixed)

- **File:** `teenastro_app/lib/services/lx200_tcp_client.dart`
- **Issue:** In `disconnect()`, the code cancelled the stream subscription first, then closed the socket. If the subscription’s `onDone` ran asynchronously, it could set `_socket = null` before `_socket?.close()` ran, so the socket might not be closed in some cases.
- **Fix:** Close the socket first, then cancel the subscription. That guarantees the connection is closed regardless of `onDone` timing.

### 2.2 Other app code reviewed

- **Timers:** `Timer.periodic` in `dashboard_screen.dart`, `sync_panel.dart`, and `alignment_screen.dart` are all cancelled in `dispose()` (or in the alignment screen’s `_reset` / `_abortAlignment` and in the timer callback when done). No timer leak found.
- **Mount state / providers:** `MountStateNotifier` and `mount_state_provider` handle disconnected state and reconnection; `_pollLoop` checks `_client.isConnected` and reconnects with backoff. No obvious null/state bugs in the reviewed paths.
- **LX200 reply lengths:** App uses the generated `lx200_reply_lengths.dart`; as long as the JSON and generated files are in sync (both in repo), builds are consistent.

### 2.3 Optional hardening (not done in this audit)

- **`generate_star_catalog.py`** and **`convert_stellarium_constellations.py`** download from the internet (VizieR, Stellarium). If the repo’s committed `stars_mag9.json` / `constellation_lines.json` are ever removed or not present (e.g. partial clone), a build would need network access to regenerate them. Keeping these files in git (current practice) avoids that.

---

## 3. Summary

| Area | Status |
|------|--------|
| **Required files for build** | All present in repo; no “only on one machine” dependencies. |
| **build_app.ps1 portability** | Fixed: no hardcoded user paths; Flutter from PATH or `-FlutterPath`; `-FixFlutter` uses `USERPROFILE`. |
| **BUILD_SCRIPTS.md** | Updated to generic run-from-repo-root example. |
| **TCP disconnect** | Fixed: close socket before cancelling subscription. |
| **Timers / disposal** | Reviewed; no leaks found. |

Building on another machine is supported with a full clone and the prerequisites in **BUILD_SETUP.md** and **scripts/BUILD_SCRIPTS.md**.

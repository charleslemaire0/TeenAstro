# TeenAstro build scripts – quick reference

Reminder of which script does what and where outputs go.

**Publishing firmware for the TeenAstro Uploader:** You must follow the procedure in **[README_FIRMWARE_PUBLISH.md](README_FIRMWARE_PUBLISH.md)** (and [FIRMWARE_PUBLISH_PROCEDURE.md](FIRMWARE_PUBLISH_PROCEDURE.md)) and **only** that procedure. Builds go into **X.Y_latest**; promoting to stable copies **X.Y_latest** → **X.Y**. Do not add patch versions to the dropdown or use other paths.

---

## 1. Install build tools (run once)

**Script:** `scripts\setup_build_env.ps1`

- Installs **PlatformIO** (pip) and **MinGW** (for unit tests).
- Adds Python Scripts and MinGW to your user PATH.
- **Does not build** anything.

```powershell
# From repo root:
.\scripts\setup_build_env.ps1
```

Then close and reopen PowerShell so PATH is updated.

---

## 2. Build firmware (MainUnit, SHC, Server, Focuser)

### 2a. Build ALL board variants and languages (release distribution)

**Script:** `build_firmware.py` (repo root)

- Builds **every** board variant and language, renames outputs for the TeenAstro Uploader.
- **Output:** `TeenAstroUploader\TeenAstroUploader\{version}_latest\` (e.g. `1.6_latest`)
- Requires `pio` on PATH; version is set in `RELEASE_VERSION` in the script (currently 1.6).

| Command | Builds |
|--------|--------|
| `python build_firmware.py` | All 13 variants (6 MainUnit + 4 Focuser + 3 SHC languages) |
| `python build_firmware.py --target main` | MainUnit only (6 boards) |
| `python build_firmware.py --target focuser` | Focuser only (4 boards) |
| `python build_firmware.py --target shc` | SHC only (ENGLISH, FRENCH, GERMAN) |
| `python build_firmware.py --list` | List all variants without building |
| `python build_firmware.py --clean` | Clean build dirs before building |

Run from repo root.

### 2b. Build default envs only (quick dev build)

**Script:** `scripts\build_firmware.ps1`

- Builds firmware with PlatformIO. Requires `pio` on PATH (run setup above first).
- **Output:** Each project’s `pio\` folder (e.g. `TeenAstroMainUnit\pio\`, `TeenAstroSHC\pio\`).

| Command | Builds |
|--------|--------|
| `.\build_firmware.ps1` | All: MainUnit, SHC, Server, Focuser |
| `.\build_firmware.ps1 -MainUnit` | MainUnit only |
| `.\build_firmware.ps1 -SHC` | SHC only |
| `.\build_firmware.ps1 -Server` | Server only |
| `.\build_firmware.ps1 -Focuser` | Focuser only |
| `.\build_firmware.ps1 -Tests` | Unit tests only |

Run from repo root or from `scripts\`.

---

## 3. Build Android + Windows app (Flutter)

**Script:** `scripts\build_app.ps1`

- Builds the TeenAstro Flutter app (Android APK and/or Windows).
- **Infrastructure:** Before building, the script checks for generated app assets (catalogs, planetarium star field, constellation lines). If any are missing, it runs the required Python tools (`extract_catalogs.py`, `generate_star_catalog.py`, `convert_stellarium_constellations.py`) so you don't have to remember to run them manually.
- **Output:** Copied to **`Released data\App\`**
  - APK: `Released data\App\teenastro_app-release.apk`
  - Windows: `Released data\App\Windows\` (exe + DLLs)

| Command | Builds |
|--------|--------|
| `.\build_app.ps1` | Android + Windows |
| `.\build_app.ps1 -SkipAndroid` | Windows only |
| `.\build_app.ps1 -SkipWindows` | Android only |
| `.\build_app.ps1 -FixFlutter` | Reinstall Flutter (if broken), then build |

Run from repo root or from `scripts\`. Flutter must be on PATH (or use `-FlutterPath`).

**Android:** To configure your PC so the Android APK can compile (SDK + ANDROID_HOME), see **`scripts\ANDROID_SETUP.md`**. Quick check: `.\scripts\setup_android_sdk.ps1 -Check`. Set SDK path: `.\scripts\setup_android_sdk.ps1 -SdkPath "C:\path\to\sdk"`.

*Alternative:* `teenastro_app\scripts\fix_flutter_and_build.ps1` is a wrapper that calls `scripts\build_app.ps1` (same parameters).

---

## 3b. Build ASCOM Windows installer (Inno Setup)

**Scripts:** `scripts\build_ascom_setup.ps1`, `scripts\build_ascom_setup.bat`

Pipeline: **MSBuild** (Release) → **sign** `ASCOM.TeenAstro.exe` (optional) → **ISCC** → **sign** `TeenAstro Setup 1.6.exe` + verify (optional) → **copy** that installer into `Released data\driver\` (merges; **does not delete** that folder or subfolders like `Archiv`). Optional **`-CopyBinToRelease`** adds all of `bin\Release\*`.

- **Intermediate output:** `TeenAstroASCOM_V7\TeenAstro Setup 1.6.exe` (Inno `OutputBaseFilename`; keep in sync with `TeenAstro Setup.iss`).
- **Release output:** `Released data\driver\` contains **`TeenAstro Setup 1.6.exe`** only (the installer bundles the driver). Use **`-CopyBinToRelease`** if you also want the full `bin\Release\*` tree (DLLs) copied there.

| Command | Effect |
|---------|--------|
| `.\scripts\build_ascom_setup.bat` | Full pipeline (prompts for PFX password if signing) |
| `.\scripts\build_ascom_setup.ps1 -SkipSign` | Build + Inno + `Released data\driver` copy — **unsigned** (used by `build_release.ps1`) |
| `.\scripts\build_ascom_setup.ps1 -SkipBuild` | Inno only (Release binaries already built) |
| `.\scripts\build_ascom_setup.ps1 -SkipCopyToRelease` | Do not refresh `Released data\driver\` |
| `.\scripts\build_ascom_setup.ps1 -CopyBinToRelease` | Also copy full `bin\Release\*` into `Released data\driver\` (DLLs + exe) |
| `.\scripts\build_ascom_setup.ps1 -ISCC "C:\...\ISCC.exe"` | Use a specific Inno compiler |

Signing uses the PFX default path `C:\Users\charl\TeenAstroCodeSigning.pfx` (override with `-PfxPath`), Windows SDK **signtool**, and optional RFC 3161 timestamp. The password is taken from **`TEENASTRO_PFX_PASSWORD`** (environment variable), **`-PfxPassword`** (SecureString), **`-PfxPasswordFile`**, or **`{PfxPath}.password.txt`** if it exists; otherwise you are prompted unless **`-NonInteractive`** is set (then the script fails with instructions). If a password from file/env does not open the PFX, you are prompted again (unless `-NonInteractive`). **`Released data\Run_Ascom_build.bat`** uses **`-NoAutoPasswordFile`** so you type the PFX password at the prompt (set `TEENASTRO_PFX_PASSWORD` to skip). For installer-only signing of an existing EXE, use `TeenAstroASCOM_V7\sign_ASCOM_driver.ps1`.

Requires: **Visual Studio** or **Build Tools** (MSBuild), **Inno Setup**, and for signed builds **Windows SDK** signing tools. The `.iss` may reference ASCOM Developer paths for wizard images; install ASCOM Platform Developer Components if those steps fail.

*Legacy shortcut:* `Released data\Run_Ascom_build.bat` changes to the repo root and runs `scripts\build_ascom_setup.ps1` (pass `-SkipSign` for a non-interactive unsigned build).

---

## 4. Generate LX200 reply lengths (run when JSON changes)

**Script:** `scripts\generate_reply_lengths.py`

- Reads `libraries\TeenAstroCommandDef\data\command_reply_lengths.json`
- Generates `CommandReplyLength.h` (C++) and `lx200_reply_lengths.dart` (Flutter)
- Run when adding or changing fixed-length LX200 reply commands

```powershell
python scripts\generate_reply_lengths.py
```

---

## 5. Build MSI installer (SHC emulator + Uploader + App)

**Script:** `TeenAstroEmulator\installer\build.ps1`

- Builds the SHC emulator (PlatformIO), Firmware Uploader (MSBuild), and TeenAstro App (Flutter).
- Produces a Windows MSI that installs all three with Start Menu shortcuts.
- **Output:** `TeenAstroEmulator\installer\out\TeenAstroEmulator.msi`

| Command | Effect |
|---------|--------|
| `.\TeenAstroEmulator\installer\build.ps1` | Build everything and produce MSI |
| `.\TeenAstroEmulator\installer\build.ps1 -SkipApp` | Skip Flutter app (emulator + uploader only) |
| `.\TeenAstroEmulator\installer\build.ps1 -SkipEmulator` | Reuse previously staged emulator exe |

Requires: PlatformIO, MSBuild, Flutter, WiX Toolset 3. See `TeenAstroEmulator\installer\README.md`.

---

## 6. Build all release artifacts (ASCOM + MSI + App)

**Script:** `scripts\build_release.ps1`

- Runs **`scripts\build_ascom_setup.ps1 -SkipSign`** (ASCOM Release + Inno + copy to `Released data\driver\`), then builds the **MSI** (emulator + uploader + app), then copies the **Flutter Windows app** standalone.
- Collects all outputs under **`release\`**.

| Command | Effect |
|---------|--------|
| `.\scripts\build_release.ps1` | Build everything |
| `.\scripts\build_release.ps1 -SkipASCOM` | Skip ASCOM driver |
| `.\scripts\build_release.ps1 -SkipMSI` | Skip MSI |
| `.\scripts\build_release.ps1 -SkipApp` | Skip standalone app copy |

**Output layout:**

```
Released data\driver\   TeenAstro Setup 1.6.exe (optional: `-CopyBinToRelease` adds bin\Release\*)
release\
  msi\      TeenAstroEmulator.msi
  app\      Flutter Windows app (standalone)
```

Requires: PlatformIO, MSBuild, Inno Setup (ASCOM), Flutter, WiX Toolset 3.

---

## Summary

| Goal | Script | Output location |
|------|--------|-----------------|
| Install pio + MinGW | `scripts\setup_build_env.ps1` | (tools in user profile) |
| Build ALL board/language variants for release | `python build_firmware.py` | `TeenAstroUploader\...\1.6_latest\` |
| Build MainUnit / SHC / Server / Focuser (default envs) | `scripts\build_firmware.ps1` | `TeenAstroMainUnit\pio\` etc. |
| Build Android + Windows app | `scripts\build_app.ps1` | `Released data\App\` |
| Build ASCOM Setup (+ optional signing) | `scripts\build_ascom_setup.bat` or `scripts\build_ascom_setup.ps1` | `TeenAstroASCOM_V7\TeenAstro Setup 1.6.exe`, `Released data\driver\` |
| Generate reply-length tables | `python scripts\generate_reply_lengths.py` | TeenAstroCommandDef, teenastro_app |
| Build MSI (emulator + uploader + app) | `TeenAstroEmulator\installer\build.ps1` | `TeenAstroEmulator\installer\out\` |
| **Build all release artifacts** | **`scripts\build_release.ps1`** | **`release\`** |

Full PC setup (all components, .NET, Flutter): see **`BUILD_SETUP.md`** at repo root.

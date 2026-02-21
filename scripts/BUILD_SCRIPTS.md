# TeenAstro build scripts – quick reference

Reminder of which script does what and where outputs go.

---

## 1. Install build tools (run once)

**Script:** `scripts\setup_build_env.ps1`

- Installs **PlatformIO** (pip) and **MinGW** (for unit tests).
- Adds Python Scripts and MinGW to your user PATH.
- **Does not build** anything.

```powershell
C:\Users\charl\source\repos\charleslemaire0\TeenAstro\scripts\setup_build_env.ps1
```

Then close and reopen PowerShell so PATH is updated.

---

## 2. Build firmware (MainUnit, SHC, Server, Focuser)

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

## Summary

| Goal | Script | Output location |
|------|--------|-----------------|
| Install pio + MinGW | `scripts\setup_build_env.ps1` | (tools in user profile) |
| Build MainUnit / SHC / Server / Focuser | `scripts\build_firmware.ps1` | `TeenAstroMainUnit\pio\` etc. |
| Build Android + Windows app | `scripts\build_app.ps1` | `Released data\App\` |

Full PC setup (all components, .NET, Flutter): see **`BUILD_SETUP.md`** at repo root.

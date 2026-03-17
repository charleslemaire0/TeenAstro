# TeenAstro Emulator -- Windows MSI Installer

Builds a Windows MSI package that installs the **MainUnit emulator** (extended state view / tabbed cockpit), **SHC emulator**, **Firmware Uploader**, and **TeenAstro App** under **Program Files** and adds **Start Menu** shortcuts. There is a single MainUnit emulator (the one with the extended state view); the SHC emulator connects to it.

- **Emulators only** — To build and install only the two emulators (no Firmware Uploader, no TeenAstro App), run:
  ```powershell
  .\TeenAstroEmulator\installer\build_msi.ps1 -EmulatorsOnly
  ```
  This builds only MainUnit and SHC, stages them plus SDL2 and icon, and produces an MSI that installs just the emulators and Start Menu shortcuts. SDL2 is taken from the build output or, if missing, from `C:\SDL2\bin\SDL2.dll`.

- **Firmware Uploader only** — To build and install only the Firmware Uploader (all required files: exe, config, esptool, teensy tools), run:
  ```powershell
  .\TeenAstroEmulator\installer\build_msi.ps1 -UploaderOnly
  ```
  This builds the Uploader with MSBuild, stages the full `TeenAstroUploader\TeenAstroUploader\bin\Release` contents, and produces `TeenAstroUploader.msi` in `installer\.out\`.

## Prerequisites

1. **PlatformIO** -- to build the MainUnit and SHC emulators (`pio run -d TeenAstroEmulator -e emu_mainunit` and `-e emu_shc`).
2. **MSBuild** -- to build the Firmware Uploader (Visual Studio or .NET Framework SDK).
3. **Flutter** -- to build the TeenAstro Windows app (`flutter build windows`).
4. **WiX Toolset 3** -- to build the MSI.
   - Download: https://wixtoolset.org/
   - Install and add the WiX `bin` folder to your PATH (e.g. `C:\Program Files (x86)\WiX Toolset v3.11\bin`),
   **or** set the `WIX` environment variable to the WiX install directory.

## If nothing happens when you run the script

- **Run it from PowerShell** (don’t only double‑click the `.ps1` file). Open PowerShell, `cd` to the repo root, then run:
  ```powershell
  .\TeenAstroEmulator\installer\build_msi.ps1
  ```
  If you double‑click, the window can close as soon as the script exits; when run from an open PowerShell you see all output and errors.

- **“Flutter not found”** — The script needs Flutter to build the TeenAstro App. If you don’t have Flutter or don’t need the app in the MSI, run with **`-SkipApp`**:
  ```powershell
  .\TeenAstroEmulator\installer\build_msi.ps1 -SkipApp
  ```
  That builds the MSI with only the MainUnit emulator, SHC emulator, and Firmware Uploader. To include the app later, install Flutter, add its `bin` folder to PATH (or use `-FlutterPath "C:\path\to\flutter\bin"`), and run without `-SkipApp`.

- **Script won’t run at all** — If PowerShell says scripts are disabled, allow scripts for your user (one time):
  ```powershell
  Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser
  ```
  Then run the script again.

- **Errors now stay visible** — If the script fails (e.g. Flutter not found), it will pause and ask you to press Enter before closing, so you can read the message.

## EXE Icon

The SHC emulator EXE embeds the TeenAstro icon from `installer/icon.ico` (see `installer/embed_icon.py`). The MainUnit emulator uses the same icon at runtime from the staged `icon.ico`. To change the icon, replace `installer/icon.ico` and rebuild.

## Updating the stage directory

The `installer\.stage\` folder is **filled automatically** by the build script; you don’t edit it by hand for normal builds.

| How to update | When to use |
|---------------|-------------|
| **Run the full installer build** | Refresh everything (MainUnit, SHC, Uploader, App) and rebuild the MSI. |
| **Rebuild one component, then run with skip flags** | After changing only one emulator or Uploader/App: rebuild that part, copy the new file(s) into `.stage\`, then run the script with the right `-Skip*` so the rest is left as-is. |

**Full refresh (recommended):**

```powershell
.\TeenAstroEmulator\installer\build_msi.ps1
```

This builds the MainUnit emulator, SHC emulator, Firmware Uploader, and Flutter app, copies them into `installer\.stage\`, then builds the MSI. Anything already in `.stage\` (e.g. `TeenAstroMainUnit.exe`, `TeenAstroSHC.exe`, `SDL2.dll`, `TeenAstroUploader.exe`, Flutter output, `icon.ico`) is overwritten.

**Update only the MainUnit emulator in stage:**

```powershell
# From repo root
pio run -d TeenAstroEmulator -e emu_mainunit
Copy-Item TeenAstroEmulator\.pio\build\emu\mainunit_emu.exe TeenAstroEmulator\installer\.stage\TeenAstroMainUnit.exe -Force

# Rebuild MSI without rebuilding SHC, Uploader or App
.\TeenAstroEmulator\installer\build_msi.ps1 -SkipUploader -SkipApp
# (Note: -SkipEmulator skips building BOTH emulators; to refresh only MainUnit, build above then use -SkipEmulator)
```

**Update only the SHC emulator in stage:**

```powershell
# From repo root
pio run -d TeenAstroEmulator -e emu_shc
Copy-Item TeenAstroEmulator\.pio\build\emu\shc_emu.exe TeenAstroEmulator\installer\.stage\TeenAstroSHC.exe -Force
# Optional: refresh SDL2 if your build outputs it
# Copy-Item TeenAstroEmulator\.pio\build\emu\SDL2.dll TeenAstroEmulator\installer\.stage\SDL2.dll -Force

# Rebuild MSI without rebuilding MainUnit, Uploader or App
.\TeenAstroEmulator\installer\build_msi.ps1 -SkipUploader -SkipApp
```

**Update only the Firmware Uploader in stage:**

Build the Uploader in Visual Studio (Release), then:

```powershell
Copy-Item TeenAstroUploader\TeenAstroUploader\bin\Release\TeenAstroUploader.exe TeenAstroEmulator\installer\.stage\TeenAstroUploader.exe -Force
.\TeenAstroEmulator\installer\build_msi.ps1 -SkipEmulator -SkipApp
```

**Update only the TeenAstro App in stage:**

```powershell
cd teenastro_app
flutter build windows
Copy-Item build\windows\x64\runner\Release\* ..\TeenAstroEmulator\installer\.stage\ -Recurse -Force
cd ..
.\TeenAstroEmulator\installer\build_msi.ps1 -SkipEmulator -SkipUploader
```

`.stage\` and `.out\` are in `.gitignore`; its contents are not committed. The WiX installer expects at least: `TeenAstroMainUnit.exe`, `TeenAstroSHC.exe`, `SDL2.dll`, `TeenAstroUploader.exe`, `icon.ico`, and (if not using `-SkipApp`) the Flutter app files (e.g. `teenastro_app.exe` and its DLLs).

## Build the MSI

From the **repository root**:

```powershell
.\TeenAstroEmulator\installer\build_msi.ps1
```

Or from `TeenAstroEmulator\installer`:

```powershell
.\build_msi.ps1
```

Switches:

| Switch | Effect |
|--------|--------|
| `-SkipEmulator` | Skip building the SHC emulator (reuse staged exe) |
| `-SkipUploader` | Skip building the firmware uploader |
| `-SkipApp` | Skip building the Flutter Windows app |
| `-FlutterPath "..."` | Path to Flutter `bin/` directory |

The script will:

1. Build the SHC emulator with PlatformIO (icon is embedded automatically).
2. Build the Firmware Uploader with MSBuild.
3. Build the TeenAstro App with Flutter.
4. Stage all files into `installer\.stage\`.
5. Use WiX `heat.exe` to harvest the app files, then `candle` and `light` to produce the MSI.

Output: **`TeenAstroEmulator\installer\.out\TeenAstroEmulator.msi`**

## Install

Run the MSI. It installs to **Program Files\TeenAstro Emulator\** and creates Start Menu shortcuts under **TeenAstro**:

- **TeenAstro MainUnit Emulator** -- MainUnit emulator with extended state view (tabbed cockpit). Start this first.
- **TeenAstro SHC Emulator** -- the Smart Hand Controller emulator; connects to the MainUnit.
- **TeenAstro Firmware Uploader** -- download and upload firmware to TeenAstro hardware.
- **TeenAstro App** -- the TeenAstro telescope controller app (if built).

## Build as part of a full release

Use `scripts\build_release.ps1` from the repo root to build the MSI, ASCOM driver, and app together and collect all outputs under `release\`.

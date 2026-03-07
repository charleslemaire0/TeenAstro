# TeenAstro Emulator -- Windows MSI Installer

Builds a Windows MSI package that installs the **SHC emulator**, **Firmware Uploader**, and **TeenAstro App** under **Program Files** and adds **Start Menu** shortcuts.

## Prerequisites

1. **PlatformIO** -- to build the SHC emulator (`pio run -d TeenAstroEmulator -e emu_shc`).
2. **MSBuild** -- to build the Firmware Uploader (Visual Studio or .NET Framework SDK).
3. **Flutter** -- to build the TeenAstro Windows app (`flutter build windows`).
4. **WiX Toolset 3** -- to build the MSI.
   - Download: https://wixtoolset.org/
   - Install and add the WiX `bin` folder to your PATH (e.g. `C:\Program Files (x86)\WiX Toolset v3.11\bin`),
   **or** set the `WIX` environment variable to the WiX install directory.

## EXE Icon

The SHC emulator EXE embeds the TeenAstro icon from `installer/icon.ico`. This is compiled via `installer/app_icon.rc` using **windres** (MinGW) during the PlatformIO build (see `installer/embed_icon.py`). To change the icon, replace `installer/icon.ico` and rebuild.

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
4. Stage all files into `installer\stage\`.
5. Use WiX `heat.exe` to harvest the app files, then `candle` and `light` to produce the MSI.

Output: **`TeenAstroEmulator\installer\out\TeenAstroEmulator.msi`**

## Install

Run the MSI. It installs to **Program Files\TeenAstro Emulator\** and creates Start Menu shortcuts under **TeenAstro**:

- **TeenAstro SHC Emulator** -- the Smart Hand Controller emulator.
- **TeenAstro Firmware Uploader** -- download and upload firmware to TeenAstro hardware.
- **TeenAstro App** -- the TeenAstro telescope controller app.

## Build as part of a full release

Use `scripts\build_release.ps1` from the repo root to build the MSI, ASCOM driver, and app together and collect all outputs under `release\`.

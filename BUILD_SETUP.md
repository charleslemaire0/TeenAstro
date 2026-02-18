# TeenAstro – PC build setup

This document describes how to configure your Windows PC to **compile all elements** in the TeenAstro repository.

## Overview of buildable components

| Component | Build system | Requirements |
|-----------|--------------|--------------|
| **TeenAstroMainUnit** | PlatformIO | PlatformIO CLI, Teensy platform |
| **TeenAstroSHC** | PlatformIO | PlatformIO CLI, ESP8266 |
| **TeenAstroServer** | PlatformIO | PlatformIO CLI, ESP8266 |
| **TeenAstroFocuser** | PlatformIO | PlatformIO CLI, Teensy |
| **UniversalMainUnit** | PlatformIO | PlatformIO CLI, Teensy/ESP32 (custom platform) |
| **Unit tests** (`tests/`) | PlatformIO | PlatformIO CLI, MinGW toolchain |
| **TeenAstroASCOM_V7** | MSBuild | .NET Framework 4.7.2, Visual Studio or Build Tools |
| **TeenAstroUploader** | MSBuild | VB.NET, Visual Studio or Build Tools |
| **CatalogConverter** | MSBuild | VB.NET, Visual Studio or Build Tools |
| **SimpleFocuser** | MSBuild | VB.NET, Visual Studio or Build Tools |
| **teenastro_app** | Flutter | Flutter SDK; for Android APK: Android SDK |

---

## 1. PlatformIO (firmware + native tests)

Required for: **TeenAstroMainUnit**, **TeenAstroSHC**, **TeenAstroServer**, **TeenAstroFocuser**, **UniversalMainUnit**, and **unit tests**.

### Option A: Install via Python (recommended for CLI)

1. Install **Python 3.7+** from [python.org](https://www.python.org/downloads/) (check “Add Python to PATH”).
2. Open a new PowerShell and run:
   ```powershell
   pip install platformio
   ```
3. Install the **MinGW toolchain** (needed for native unit tests on Windows):
   ```powershell
   pio pkg install -g --tool "platformio/toolchain-gccmingw32"
   ```
4. Add MinGW to your user PATH so tests can find `g++.exe`:
   ```powershell
   $pioHome = "$env:USERPROFILE\.platformio"
   $mingwBin = "$pioHome\packages\toolchain-gccmingw32\bin"
   [Environment]::SetEnvironmentVariable("Path", $env:Path + ";$mingwBin", "User")
   ```
   Then close and reopen PowerShell.

### Option B: Install via standalone installer

1. Download **PlatformIO Core** from [platformio.org/install/cli](https://platformio.org/install/cli).
2. Run the installer; it will add `pio` to PATH.
3. Install MinGW and add to PATH as in steps 3–4 above.

### Verify PlatformIO

```powershell
cd c:\Users\clemair\Documents\learn\TeenAstro
pio --version
```

### Build firmware (from repo root)

```powershell
pio run -d TeenAstroMainUnit
pio run -d TeenAstroSHC
pio run -d TeenAstroServer
pio run -d TeenAstroFocuser
```

For **UniversalMainUnit** (custom Teensy/ESP32 platform), from repo root:

```powershell
pio run -d UniversalMainUnit
```

### Run unit tests

```powershell
pio test -d tests
```

To add MinGW to PATH for the **current session** only (if you didn’t set it permanently):

```powershell
$env:PATH = "$env:USERPROFILE\.platformio\packages\toolchain-gccmingw32\bin;$env:PATH"
pio test -d tests
```

---

## 2. .NET (C# and VB.NET)

Required for: **TeenAstroASCOM_V7**, **TeenAstroUploader**, **CatalogConverter**, **SimpleFocuser**.

You need **.NET Framework 4.7.2** and **MSBuild** (Visual Studio or Build Tools).

### Option A: Visual Studio 2022

1. Install [Visual Studio 2022](https://visualstudio.microsoft.com/) (Community is free).
2. In the installer, select workload **“.NET desktop development”**.
3. Ensure **.NET Framework 4.7.2 targeting pack** is installed (Individual components).

### Option B: Build Tools for Visual Studio

1. Download [Build Tools for Visual Studio](https://visualstudio.microsoft.com/visual-cpp-build-tools/).
2. Select **“.NET desktop build tools”** and **.NET Framework 4.7.2 targeting pack**.

### Build from command line

Open **Developer Command Prompt for VS** or a normal prompt with MSBuild on PATH, then:

```powershell
cd c:\Users\clemair\Documents\learn\TeenAstro

# TeenAstroASCOM_V7 (C#)
msbuild TeenAstroASCOM_V7\TeenAstroASCOM_V7.sln -p:Configuration=Release

# TeenAstroUploader (VB.NET)
msbuild TeenAstroUploader\TeenAstroUploader.sln -p:Configuration=Release

# CatalogConverter (VB.NET)
msbuild CatalogConverter\CatConv\CatConv.sln -p:Configuration=Release

# SimpleFocuser (VB.NET)
msbuild SimpleFocuser\SimpleFocuser.sln -p:Configuration=Release
```

Or open each `.sln` in Visual Studio and build from the IDE.

---

## 3. Flutter (Android app)

Required for: **teenastro_app**.

1. Install [Flutter SDK](https://flutter.dev/docs/get-started/install/windows).
2. Run `flutter doctor` and fix any reported issues (Android SDK, etc.).
3. Build the app:
   ```powershell
   cd teenastro_app
   flutter pub get
   flutter build apk
   ```
   Or run on device/emulator: `flutter run`.

### Building the Android APK

To run `flutter build apk` you need the **Android SDK**.

**If Android Studio (e.g. Panda) is already installed:**

1. Open **Android Studio** → **Settings** (Ctrl+Alt+S) → **Appearance & Behavior** → **System Settings** → **Android SDK**.
2. Note the **Android SDK Location** (e.g. `C:\Users\<you>\AppData\Local\Android\Sdk`). If no SDK is installed yet, click **Edit** and complete the setup to install the SDK.
3. Set that path for Flutter (in PowerShell, from repo root or `teenastro_app`):
   ```powershell
   .\teenastro_app\scripts\set_android_sdk.ps1 -SdkPath "C:\Users\<you>\AppData\Local\Android\Sdk"
   ```
   Or set the environment variable yourself:
   ```powershell
   [Environment]::SetEnvironmentVariable("ANDROID_HOME", "C:\path\to\Android\Sdk", "User")
   ```
4. Close and reopen the terminal, then run:
   ```powershell
   flutter doctor --android-licenses   # accept all
   flutter build apk -d teenastro_app  # or: cd teenastro_app; flutter build apk
   ```

**Without Android Studio:** Install the [Android command-line tools](https://developer.android.com/studio#command-tools), set `ANDROID_HOME`, then install components with `sdkmanager` (see Flutter’s Windows Android setup docs).

---

## 4. One-time setup script (PlatformIO + MinGW)

From the repo root you can run the provided script to install PlatformIO (via pip) and the MinGW toolchain, and to add MinGW to your user PATH:

```powershell
.\scripts\setup_build_env.ps1
```

Then close and reopen PowerShell before running `pio run` or `pio test`.

---

## 5. Quick reference – build all PlatformIO firmware

From repo root:

```powershell
pio run -d TeenAstroMainUnit
pio run -d TeenAstroSHC
pio run -d TeenAstroServer
pio run -d TeenAstroFocuser
```

Unit tests:

```powershell
pio test -d tests
```

.NET and Flutter projects must be built separately as described above.

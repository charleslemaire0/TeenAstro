# Build system

---

## Prerequisites

| Component | Build system | Requirements |
|-----------|--------------|--------------|
| TeenAstroMainUnit, SHC, Server, Focuser | PlatformIO | pio, Teensy/ESP8266 |
| UniversalMainUnit | PlatformIO | Custom Teensy/ESP32 platform |
| Unit tests | PlatformIO | MinGW toolchain |
| TeenAstroASCOM_V7 | MSBuild | .NET 4.7.2 |
| TeenAstroUploader, CatalogConverter | MSBuild | VB.NET |
| teenastro_app | Flutter | Flutter SDK; Android: Android SDK + ANDROID_HOME |

**PlatformIO:** `pip install platformio`; `pio pkg install -g --tool "platformio/toolchain-gccmingw32"` (tests).  
**One-shot:** `scripts/setup_build_env.ps1` from repo root.

---

## Build scripts

| Goal | Command | Output |
|------|---------|--------|
| Install pio + MinGW | `scripts/setup_build_env.ps1` | (user profile) |
| All board/language variants | `python build_firmware.py` | TeenAstroUploader/.../1.6_latest/ |
| MainUnit/SHC/Server/Focuser (default) | `scripts/build_firmware.ps1` | */pio/ |
| Android + Windows app | `scripts/build_app.ps1` | Released data/App/ |
| ASCOM Setup (Inno, optional signing) | `scripts/build_ascom_setup.bat` or `scripts/build_ascom_setup.ps1` | `TeenAstroASCOM_V7/TeenAstro Setup 1.6.exe` + `Released data/driver/` |
| Reply-length tables | `python scripts/generate_reply_lengths.py` | TeenAstroCommandDef, teenastro_app |
| MSI (emulator + uploader + app) | `TeenAstroEmulator/installer/build.ps1` | installer/out/ |
| Full release | `scripts/build_release.ps1` | `Released data/driver/`, `release/` (msi, app) |

**build_firmware.py:** `--target main|focuser|shc`, `--list`, `--clean`.  
**build_app.ps1:** `-SkipAndroid`, `-SkipWindows`, `-FixFlutter`.  
**build_ascom_setup.ps1:** `-SkipSign`, `-SkipBuild`, `-SkipCopyToRelease`, `-CopyBinToRelease` (copy all of `bin\Release` into `Released data/driver`), `-ISCC`, `-NonInteractive`; signing: `-PfxPath`, `-PfxPasswordFile`, `-PfxPassword`, `-NoTimestamp`, etc.

---

## Unit tests

```bash
pio test -d tests
pio test -d tests --filter test_la3   # or test_coord, test_coordconv
python tests/run_all_tests.py
```

91 tests across test_la3, test_coord, test_coordconv.

---

## Versioning

Firmware release is **X.Y** only (e.g. 1.6), no patch. RELEASE_VERSION in build_firmware.py. Publish: build → 1.6_latest; promote to stable (1.6) per README_FIRMWARE_PUBLISH.md and FIRMWARE_PUBLISH_PROCEDURE.md.

**Full policy:** [VERSIONING.md](../VERSIONING.md) at repo root.

---

**See also:** [BUILD_SETUP.md](../BUILD_SETUP.md) (repo root) · [scripts/BUILD_SCRIPTS.md](../scripts/BUILD_SCRIPTS.md)

# TeenAstro Release Outputs

This folder collects all distributable artifacts produced by `scripts\build_release.ps1`.

**Build documentation:** [docs/build.md](../docs/build.md) · [scripts/BUILD_SCRIPTS.md](../scripts/BUILD_SCRIPTS.md)

## Layout

```
release/
  ascom/                  ASCOM telescope driver (ASCOM.TeenAstro.exe + deps)
  msi/                    TeenAstro Emulator MSI (SHC + Uploader + App)
  app/                    Flutter Windows app (standalone, also bundled in MSI)
```

## How to build

From the repository root:

```powershell
.\scripts\build_release.ps1
```

See `scripts\BUILD_SCRIPTS.md` for options and prerequisites.

## ASCOM driver

The ASCOM driver (`release\ascom\ASCOM.TeenAstro.exe`) is a COM local server. After copying the files to the target PC, register it once (as Administrator):

```
ASCOM.TeenAstro.exe /regserver
```

To unregister:

```
ASCOM.TeenAstro.exe /unregserver
```

The driver will then appear in the ASCOM Chooser. ASCOM Platform 6 must be installed on the target PC.

## MSI

Run `release\msi\TeenAstroEmulator.msi` to install the SHC emulator, the Firmware Uploader, and the TeenAstro App under **Program Files\TeenAstro Emulator\**.

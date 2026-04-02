# TeenAstro Release Outputs

This folder collects all distributable artifacts produced by `scripts\build_release.ps1`.

**Build documentation:** [docs/build.md](../docs/build.md) · [scripts/BUILD_SCRIPTS.md](../scripts/BUILD_SCRIPTS.md)

## Layout

```
Released data/driver/     TeenAstro Setup 1.6.exe (Windows installer; run it to install the driver). Optional: `build_ascom_setup.ps1 -CopyBinToRelease` also copies `bin\Release\*` for a portable layout.
release/
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

Run **`Released data\driver\TeenAstro Setup 1.6.exe`** on the target PC to install the driver (recommended).

The installed driver is a COM local server (`ASCOM.TeenAstro.exe` under `{cf}\ASCOM\Telescope\` or as chosen by the installer). To register manually if needed (as Administrator), from that folder:

```
ASCOM.TeenAstro.exe /regserver
```

To unregister: `ASCOM.TeenAstro.exe /unregserver`

The driver appears in the ASCOM Chooser after installation. ASCOM Platform 6 must be installed on the target PC.

## MSI

Run `release\msi\TeenAstroEmulator.msi` to install the SHC emulator, the Firmware Uploader, and the TeenAstro App under **Program Files\TeenAstro Emulator\**.

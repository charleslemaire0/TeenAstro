# TeenAstro Firmware Uploader

Download and flash TeenAstro firmware (MainUnit, Focuser, SHC). On firmware **1.5+**, **Auto!** backs up / restores / verifies parameters, and **EEPROM** edits the full set (two mounts on the MainUnit).

User docs: [field manuals](../docs/manuel/) · [wiki draft](../docs/wiki/TeenAstroUploader_for_Windows.md) · groups.io [Uploader](https://groups.io/g/TeenAstro/wiki/8837) / [TeenAstroConfig](https://groups.io/g/TeenAstro/wiki/10404).

## Python uploader (current)

Cross-platform app under [`python/`](python/). See [python/README.md](python/README.md).

**Windows MSI:** `Released data\Run_FirmwareUploader_build.bat` or:

```powershell
powershell -ExecutionPolicy Bypass -File TeenAstroUploader\installer\build.ps1
```

**To publish firmware:** follow [scripts/README_FIRMWARE_PUBLISH.md](../scripts/README_FIRMWARE_PUBLISH.md) and [scripts/FIRMWARE_PUBLISH_PROCEDURE.md](../scripts/FIRMWARE_PUBLISH_PROCEDURE.md). Builds go into **X.Y_latest**; promote to **X.Y** when releasing. No patch versions (e.g. 1.6.1). See [VERSIONING.md](../VERSIONING.md).

## Legacy VB.NET app

The WinForms project in [`TeenAstroUploader/`](TeenAstroUploader/) remains for reference and as a source of bundled Teensy tools for the Windows MSI. New development should target the Python app.

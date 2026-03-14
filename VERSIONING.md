# TeenAstro versioning

## Firmware release version: always X.Y (no patch)

**Firmware releases are never X.Y.Z.** They are always **X.Y** (e.g. **1.6**), not 1.6.1 or 1.6.0 as a “release” name.

- Use **1.6** for the release / distribution folder and build script (`RELEASE_VERSION` in `build_firmware.py`).
- In source, display strings (e.g. `FirmwareNumber`, `FirmwareVersion`, `SHCFirmwareVersionPatch`) may still use three segments (e.g. `1.6.0`) for compatibility, but the **release version** that users see and download is **1.6**.
- Do not introduce a patch segment for firmware releases (no 1.6.1, 1.6.2, etc.).

This applies to MainUnit, SHC, and Focuser firmware releases.

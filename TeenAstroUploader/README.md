# TeenAstro Firmware Uploader

Windows application to upload TeenAstro firmware (MainUnit, Focuser, SHC) to hardware.

**To publish firmware for this Uploader:** you must follow the procedure in **`scripts/README_FIRMWARE_PUBLISH.md`** (and **`scripts/FIRMWARE_PUBLISH_PROCEDURE.md`**) and **only** that procedure. New builds go into **X.Y_latest**; when releasing, promote by copying **X.Y_latest** into **X.Y**. Do not add patch versions (e.g. 1.6.1) to the version dropdown.

Build with MSBuild: see `BUILD_SETUP.md` at repo root.

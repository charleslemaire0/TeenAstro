# Publishing firmware for the TeenAstro Uploader

**You must follow the procedure in [FIRMWARE_PUBLISH_PROCEDURE.md](FIRMWARE_PUBLISH_PROCEDURE.md) and only that procedure.**

The Uploader has one entry per release (e.g. **1.6**) with two options: **Stable** (folder `1.6`) and **Latest** (folder `1.6_latest`). New builds go into **X.Y_latest**; when you release, you promote by copying **X.Y_latest** into **X.Y**. Do not add patch versions (e.g. 1.6.1) to the version dropdown or create 1.6.1_latest folders.

**Build into latest:**
```powershell
python scripts\publish_firmware_for_uploader.py 1.6
```
Then run the git commands the script prints.

**Promote latest to stable (when releasing):**
```powershell
python scripts\promote_firmware_to_stable.py 1.6
```
Then run the git commands the script prints.

See **FIRMWARE_PUBLISH_PROCEDURE.md** for full steps and manual fallback.

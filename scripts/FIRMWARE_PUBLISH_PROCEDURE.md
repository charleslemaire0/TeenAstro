# TeenAstro firmware publish procedure (for the Uploader)

**This is the only procedure to use when publishing firmware so that the TeenAstro Firmware Uploader can offer and flash it. Do not skip steps or use a different workflow.**

---

## How the Uploader works


- The **Firmware Version** dropdown lists **X.Y only** (e.g. **1.6**, **1.5**). There is no 1.6.1 in the list.
- For each version there are two choices:
  - **Stable** → firmware is in folder **`1.6`** (promoted, released).
  - **Latest** → firmware is in folder **`1.6_latest`** (rolling build).
- **New builds** go into **X.Y_latest**. When you are ready to release, you **promote latest to stable** by copying the contents of **X.Y_latest** into **X.Y** and committing.

---

## Prerequisites

- **PlatformIO** (`pio` on PATH). See `BUILD_SETUP.md` or `scripts\setup_build_env.ps1`.

---

## Step 1: Build firmware into “latest”

From the **repository root**:

```powershell
python scripts\publish_firmware_for_uploader.py 1.6
```

Or, using the build script directly (same result):

```powershell
python build_firmware.py --version 1.6
```

- Builds all 13 variants (MainUnit, Focuser, SHC) and writes them into **`TeenAstroUploader\TeenAstroUploader\1.6_latest\`**.
- File names use **X.Y** (e.g. `TeenAstro_1.6_220_TMC260.hex`) so the Uploader finds them when the user selects version **1.6** and **Latest**.
- **No change** to the Uploader UI (the version dropdown already has **1.6**). Do not add 1.6.1 or other patch versions to the dropdown.

After the build, commit the updated **1.6_latest** folder (see git commands printed by the script). Do not add or copy firmware by any other path or name.

---

## Step 2: Promote “latest” to stable (when releasing)

When the build in **1.6_latest** is the one you want to release as **Stable**:

1. **Copy** the contents of **`TeenAstroUploader\TeenAstroUploader\1.6_latest\`** into **`TeenAstroUploader\TeenAstroUploader\1.6\`** (overwrite existing files in **1.6**).
2. **Commit** the updated **1.6** folder:

   ```powershell
   git add "TeenAstroUploader/TeenAstroUploader/1.6/"
   git commit -m "Promote firmware 1.6: latest to stable"
   git push
   ```

You can use the helper script:

```powershell
python scripts\promote_firmware_to_stable.py 1.6
```

It copies **1.6_latest** → **1.6** and prints the `git add` / `git commit` commands.

---

## Procedure (manual steps, if you cannot run the scripts)

1. **Build**  
   From repo root:  
   `python build_firmware.py --version 1.6`  
   Output must be in `TeenAstroUploader\TeenAstroUploader\1.6_latest\` with all 13 files (names like `TeenAstro_1.6_220_TMC260.hex`).

2. **Commit latest**  
   `git add "TeenAstroUploader/TeenAstroUploader/1.6_latest/"`  
   `git commit -m "Build firmware 1.6 into latest"`  
   `git push`

3. **When releasing (promote to stable)**  
   Copy all files from `1.6_latest\` into `1.6\`, then:  
   `git add "TeenAstroUploader/TeenAstroUploader/1.6/"`  
   `git commit -m "Promote firmware 1.6: latest to stable"`  
   `git push`

Do **not** add a patch version (e.g. 1.6.1) to the Uploader version dropdown. Do **not** create folders like **1.6.1_latest**.

---

## Why “only this procedure”?

- The Uploader expects **one** version string per release line (e.g. **1.6**) and two folders: **1.6** (Stable) and **1.6_latest** (Latest). Building into **1.6_latest** and promoting by copying into **1.6** keeps the Uploader in sync. Any other layout or naming breaks version selection or file lookup.

---

## Reference

- Build script: `build_firmware.py` at repo root. See `scripts\BUILD_SCRIPTS.md` for build-only usage.
- Publish script: `scripts\publish_firmware_for_uploader.py` (builds into X.Y_latest).
- Promote script: `scripts\promote_firmware_to_stable.py` (copies X.Y_latest → X.Y).
- Uploader project: `TeenAstroUploader\TeenAstroUploader.sln`.

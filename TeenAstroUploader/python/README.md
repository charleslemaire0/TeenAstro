# TeenAstro Firmware Uploader (Python)

Cross-platform desktop uploader for TeenAstro MainUnit, Focuser, and SHC firmware.

Replaces the legacy VB.NET WinForms app with the same workflow:

- Download Stable + Latest firmware from GitHub (via system `curl`)
- Progress bar while downloading
- Telescope / Focuser / Hand controller tabs
- Auto-detect over serial
- Teensy flash (`teensy_loader_cli`, or Windows `teensy_post_compile`)
- SHC flash (`esptool`) and WiFi update page
- **Auto!** on Telescope/Focuser: backup → flash → restore + verify (firmware **1.5+** only).
- **EEPROM** button: edit MainUnit / Focuser parameters (same groups as the Webserver: Mount, Motors, Speed, Limits, Encoders, Site, Tracking). MainUnit supports **2 mounts**. Write always re-reads to verify.


Firmware cache:

| OS | Path |
|---|---|
| Windows | `%LocalAppData%\TeenAstro\Firmware` |
| macOS | `~/Library/Application Support/TeenAstro/Firmware` |
| Linux | `~/.local/share/TeenAstro/Firmware` |

Parameter backups default under `…/TeenAstro/Config/`.

### Parameters workflow (upgrade / downgrade)

1. Open **Parameters**, choose **Mount** or **Focuser**, select the USB COM port, **Connect**.
2. **Get Data** — reads settings (works on firmware 1.5 and 1.6).
3. **Save…** — writes a `.json` file (mount format matches TAConfig).
4. Flash the new/old firmware on the Firmware tabs.
5. **Load…** the file, **Connect** again, **Write Data**.

Optional: **Write mount type (reboots)** also sends `:S!n#` (board reboots).

**Auto (full)** on the Telescope / Focuser tabs runs the whole loop: backup parameters → upload firmware → wait for reboot → restore parameters. Backups are saved under `…/TeenAstro/Config/auto_*.json`.

## Run from source

```bash
cd TeenAstroUploader/python
python -m venv .venv
# Windows: .venv\Scripts\activate
# Unix:    source .venv/bin/activate
pip install -r requirements.txt
python run_uploader.py
```

## Tests

```bash
pip install -r requirements.txt
python -m pytest tests -q
# or:
python -m unittest discover -s tests -p "test_*.py"
```

(If pytest is not installed: `python -c "from teenastro_uploader.detect import pcb_from_board; assert pcb_from_board(240,2)=='2.4 TMC2130'"`.)

## Windows MSI

From repo root (or `Released data\Run_FirmwareUploader_build.bat`):

```powershell
powershell -ExecutionPolicy Bypass -File TeenAstroUploader\installer\build.ps1
```

Output: `TeenAstroUploader\installer\.out\TeenAstroUploader.msi`  
The installer launches the uploader after a normal (non-silent) install.

## macOS / Linux frozen app

```bash
cd TeenAstroUploader/python
chmod +x packaging/build_unix.sh
./packaging/build_unix.sh
```

Output folder: `dist/TeenAstroUploader/`. Install [`teensy_loader_cli`](https://www.pjrc.com/teensy/loader_cli.html) on PATH or copy it into that folder. On Linux, install Teensy udev rules (see `tools/README.md`).

## Tools

See [`tools/README.md`](tools/README.md).

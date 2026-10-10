# Native flash tools

Place platform flash helpers here (or next to the frozen `TeenAstroUploader` executable).

## Teensy (MainUnit / Focuser)

**Preferred (all platforms):** [`teensy_loader_cli`](https://www.pjrc.com/teensy/loader_cli.html)

- Windows: `teensy_loader_cli.exe`
- macOS / Linux: `teensy_loader_cli`

**Windows fallback:** PJRC tools from Teensyduino (same as the legacy VB uploader):

- `teensy_post_compile.exe`
- `teensy.exe`
- `teensy_reboot.exe`
- `teensy_restart.exe`
- `teensy_gateway.exe`

The Windows MSI build copies these from `TeenAstroUploader/bin/Release` when present.

## ESP8266 (SHC)

`esptool` is installed as a Python dependency and invoked as `python -m esptool`.  
A standalone `esptool.exe` in this folder is also accepted (legacy Arduino CLI style args).

## Linux udev (Teensy)

Without udev rules, flashing may require root. Install PJRC’s udev rules, or copy:

`49-teensy.rules` → `/etc/udev/rules.d/` then `sudo udevadm control --reload-rules`.

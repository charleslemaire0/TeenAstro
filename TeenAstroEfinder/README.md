# TeenAstroEfinder

Plate-solving finder for TeenAstro, based on [AstroKeith/eFinder_Lite](https://github.com/AstroKeith/eFinder_Lite).

The Raspberry Pi Zero 2W attaches to the main unit **Focuser** or **GNSS** UART (3.3 V TTL). The main unit is the master: it probes the port, pushes site and time, requests solves, and applies sync / alignment / GoTo++. The two axis rotations, the Hz cone and the Ve cone, are measured and stored by the main unit (`:eK#`), not by the Pi. `:eO#` on the Pi is only the brightest star against the camera center. The SHC and ASCOM reach the eFinder only through main-unit `:e…#` commands. The Android app can plate-solve with the phone camera and no Pi; see [docs/app/phone_finder.md](../docs/app/phone_finder.md).

When the eFinder sits on the GNSS port it can relay NMEA from a USB GPS dongle so site/time sync keeps working.

## Hardware

- Raspberry Pi Zero 2W
- Camera: Raspberry Pi HQ or ZWO ASI120MM-mini
- Lens: 25 mm or 50 mm, f/1.8 or faster
- Optional USB GPS when using the GNSS port
- Wire TX/RX/GND to the main-unit Focuser or GNSS connector (3.3 V logic)

## Quick start (on the Pi)

```bash
sudo ./install.sh
sudo systemctl start teenastro-efinder
```

Update later with `sudo ./update.sh`.

Config: `~/TeenAstroEfinder/efinder.config` (or `EFINDER_CONFIG`).

- `Camera:SIM` or `Test_mode:1` — synthetic star-field camera (no hardware).
- Configure the guider like the real kit: ASI model, focal length, aperture. Default FOV is **1°** across the long sensor axis; limiting magnitude is estimated from aperture / exposure / QE (AAVSO CCD form).
- Catalog: Hipparcos mag≤9 in `data/bright_stars.csv` (rebuild with `tools/generate_sim_catalog.py`). Fainter stars to the limiting magnitude are filled with a mean-sky density model so a 1° field still has a realistic count.

Example sim keys in `efinder.config`:

```
Camera:SIM
Sim_ASI:ASI120MM Mini
Sim_aperture_mm:50
Sim_fov_deg:1.0
# Sim_focal_mm:275
Exposure:1.0
```

Wiring details: [docs/TeenAstroEfinder.md](../docs/TeenAstroEfinder.md).

## Simulators

**In the PC emulator** an eFinder stub on Serial3 (GNSS) accepts `:eC` pointing and returns solves around that RA/Dec with a synthetic star count.

**Standalone UART sim** (bench-test a real board; renders catalog star fields):

```bash
py -3 efinder_sim.py --port COM5 --baud 9600
py -3 efinder_sim.py --port COM5 --asi "ASI220MM Mini" --aperture 50 --fov 1.0
py -3 efinder_sim.py --port COM5 --asi ASI120MM --focal 275 --aperture 40 --save-images
```

## Link protocol (main unit → Pi)

| Command | Reply |
|---------|--------|
| `:e?#` | `TeenAstroEfinder,<version>#` |
| `:eTyyyymmddhhmmss#` | `1#` set UTC |
| `:eL<lat>,<lonEast>,<elev>#` | `1#` set site |
| `:eC<raHours>,<decDeg>#` | `1#` current mount pointing (JNow) |
| `:eS#` | `1#`/`0#` start solve |
| `:eO#` | `1#`/`0#` start offset cal |
| `:eo#` | `1#` reset offset |
| `:eR#` | `B#` / `N#` / `F<code>#` / `S<raH>,<decD>,<stars>,<ms>#` / `O<dx>,<dy>,<name>#` |
| `:eN0#` `:eN1#` | `1#` NMEA relay off/on |
| `:eN?#` | `1#`/`0#` GPS present |

Baud: the Pi accepts 56000 (focuser) or 9600 (GNSS) after auto-scan.

## Licence

GPL-3. Plate-solving stack (tetra3 / cedar-detect) retains its own licences; see upstream eFinder Lite.

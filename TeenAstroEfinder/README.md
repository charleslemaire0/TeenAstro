# TeenAstroEfinder

Plate-solving finder for TeenAstro, based on [AstroKeith/eFinder_Lite](https://github.com/AstroKeith/eFinder_Lite).

The Raspberry Pi Zero 2W attaches to the main unit **Focuser** or **GNSS** UART (3.3 V TTL). The main unit is the master: it probes the port, pushes site and time, requests solves, and applies sync / alignment / GoTo++. The SHC and ASCOM reach the eFinder only through main-unit `:e…#` commands.

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

Config: `~/TeenAstroEfinder/efinder.config` (or `EFINDER_CONFIG`). Set `Test_mode:1` to exercise the link without a camera.

Wiring details: [docs/TeenAstroEfinder.md](../docs/TeenAstroEfinder.md).

## Link protocol (main unit → Pi)

| Command | Reply |
|---------|--------|
| `:e?#` | `TeenAstroEfinder,<version>#` |
| `:eTyyyymmddhhmmss#` | `1#` set UTC |
| `:eL<lat>,<lonEast>,<elev>#` | `1#` set site |
| `:eS#` | `1#`/`0#` start solve |
| `:eO#` | `1#`/`0#` start offset cal |
| `:eo#` | `1#` reset offset |
| `:eR#` | `B#` / `N#` / `F<code>#` / `S<raH>,<decD>,<stars>,<ms>#` / `O<dx>,<dy>,<name>#` |
| `:eN0#` `:eN1#` | `1#` NMEA relay off/on |
| `:eN?#` | `1#`/`0#` GPS present |

Baud: the Pi accepts 56000 (focuser) or 9600 (GNSS) after auto-scan.

## Licence

GPL-3. Plate-solving stack (tetra3 / cedar-detect) retains its own licences; see upstream eFinder Lite.

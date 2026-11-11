# TeenAstroEfinder wiring

The eFinder (Raspberry Pi Zero 2W + camera) attaches to the main unit like a focuser or GNSS module.

## Connector

Use the **Focuser** port (56000 baud) or the **GNSS** port (9600 baud). The main unit probes both and detects `TeenAstroEfinder` automatically (the Pi may take ~30 s to boot).

| Signal | Notes |
|--------|--------|
| TX / RX | 3.3 V TTL, crossed TX↔RX |
| GND | Common ground |
| 5 V | Optional — confirm the board connector can supply ~1 A for the Pi + camera; otherwise power the Pi separately |

Version 220 boards have no GNSS UART; use the Focuser port only.

## GNSS port + GPS

When the eFinder occupies the GNSS port, plug a USB GPS dongle into the Pi. The eFinder relays NMEA to the main unit so site/time sync still works.

## User interface

Operate from the SHC: **Telescope settings → eFinder** (Solve & Sync, Align, GoTo++, Calibrate, Exposure/Gain, Info). ASCOM and other hosts use the `:e…#` commands on the main unit USB/SHC channel; see [Commands.md](../TeenAstroMainUnit/Commands.md).

## Pi software

See [TeenAstroEfinder/README.md](../TeenAstroEfinder/README.md) and run `install.sh` on the Pi.

## Simulation

There is no real-camera path in the PC emulator. Instead:

- The main unit pushes current RA/Dec to the eFinder link as `:eC<raHours>,<decDeg>#` before each solve.
- `Camera:SIM` / `Test_mode:1` uses `solver/sim_camera.py` to project `data/bright_stars.csv` into a greyscale PNG around that pointing.
- `efinder_sim.py` speaks the same UART protocol for bench-testing a physical main unit.
- The in-process `EfinderStub` in the emulator uses the same `:eC` pointing (no PNG; returns pointing + a density-based star count).

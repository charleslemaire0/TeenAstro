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

The **eFinder** line appears in **Telescope settings** only after the main unit has found the camera. The status row then shows **eF**.

| Item | What it does |
|------|----------------|
| Solve & Sync | Plate-solve and sync the telescope axis |
| Solve | Plate-solve, no sync |
| Align | **2 Stars**, **4 Stars**, and **3+3 Stars** all start from home. The eFinder slews and plate-solves; nothing is centered by hand. **3+3 Stars**, on a German equatorial mount, is three places on the home pier, a flip, then three more. The model is saved. A key press stops it and restores the previous model |
| GoTo++ | After each goto, solve, sync, and slew back to the target once if the error is over the threshold |
| Calibrate | Brightest star against the camera center. This is not the axis calibration |
| Aim | Measure the Hz cone and the Ve cone. Center a bright star in the telescope and sync first |
| Exposure / Gain | Camera settings |
| Info | eFinder version |

The finder axis is not the telescope axis. **Aim** stores two rotations in the telescope frame. Hz is the horizontal cone and Ve is the vertical cone. Later solves, syncs, alignments, and GoTo++ follow the telescope axis. The same two names are used on every mount.

Aim fails when the tube is too close to the pole on an equatorial mount, or too close to the zenith on an alt-azimuth mount.

Hosts use the `:e…#` commands on the main unit. See [Commands.md](../TeenAstroMainUnit/Commands.md).

The Android app can do the same job with the phone camera and no eFinder. See [Phone finder](app/phone_finder.md).

## Pi software

See [TeenAstroEfinder/README.md](../TeenAstroEfinder/README.md) and run `install.sh` on the Pi.

## Simulation

There is no real-camera path in the PC emulator. Instead:

- The main unit pushes current RA/Dec to the eFinder link as `:eC<raHours>,<decDeg>#` before each solve.
- `Camera:SIM` / `Test_mode:1` uses `solver/sim_camera.py` + `solver/optics.py`. Pick an ASI guide camera, guider focal length and aperture; default FOV is **1°**. Limiting magnitude follows the AAVSO atmosphere-limited CCD form (aperture, exposure, QE, read noise).
- Real Hipparcos stars to mag 9 (`data/bright_stars.csv`); fainter stars to LM are density-filled so a 1° field is usable.
- `efinder_sim.py` speaks the same UART protocol for bench-testing a physical main unit.
- The in-process `EfinderStub` in the emulator uses the same `:eC` pointing (no PNG; returns pointing + a density-based star count).

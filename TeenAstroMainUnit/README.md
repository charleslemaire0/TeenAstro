# TeenAstro Main Unit

Firmware for the TeenAstro telescope mount controller. It drives stepper motors and exposes an LX200-style serial command set for alignment, goto, tracking, guiding, and configuration.

## Source layout

- **MainUnit.h** – Mount + cross-module declarations (includes Mount.h and MainUnitDecl.h). Use Command.h / CommandGlobals.h for the command layer.
- **MainUnitDecl.h** – Cross-module function declarations (included from MainUnit.h).
- **Command.cpp** – Defines commandState and rtk (single definitions); dispatcher and reply helpers.
- **Command layer** – `Command.h`, `CommandConstants.h`, `Command.cpp` (dispatcher and reply helpers), and `Command_*.cpp` handlers by lead character (G, S, T, M, F, h, etc.).
- **Application** – `Application.h`, `Application.cpp` (setup and main loop, lifecycle).
- **Mount** – Mount class and sub-modules in `Mount.h` and `Mount*.cpp`: `Mount.cpp` (constructor, init, delegation), `MountLimits.cpp` (limit data and checks), `MountGotoSync.cpp` (goto and sync), `MountParkHomeController.cpp` (park/home behaviour; state in `MountParkHome.h` struct), `MountST4.cpp` (ST4 guiding), `MountGuiding.cpp` (guiding), `MountMove.cpp` / `MountMoveTo.cpp` (motion), `MountQueriesTracking.cpp` (tracking queries and rates), plus `MountAxes.cpp`, `MountLimits.cpp`, `MountGotoSync.cpp`, etc. Data-only sub-objects (errors, identity, parkHome, targetCurrent, tracking) are structs in their .h files; no separate .cpp. Access: direct members by default.
- **Motion / control** – Motion, goto, guide, limit, park, home, and ST4 logic live in the various `Mount*.cpp` files; `Timer.cpp` (sidereal and axis timers), `PushTo.cpp`, and `Astro.cpp` provide push-to and astro helpers.
- **EEPROM** – `EEPROM_address.h` (layout and `getMountAddress`), `EEPROM.cpp` (init, mount/motor/encoder config).
- **Config** – `Config.TeenAstro.h`, `Pins.TeenAstro.h`, `Config.Mount.*.h` (board/mount variants), `FirmwareDef.h` (version, init key).
- **Entry** – `TeenAstroMainUnit.cpp` provides `setup()` and `loop()`, which delegate to `Application::setup()` and `Application::loop()`.

## Documentation

- **[Commands.md](Commands.md)** – Serial command reference (LX200 and TeenAstro extensions).
- **[Tracking.md](Tracking.md)** – How tracking is performed: sidereal clock, target-based motion, rates, compensation, guiding, and backlash.

## Serial protocol

See [Commands.md](Commands.md) for the full list of serial commands: syntax, description, return values, and whether each command is **LX200 standard** (Meade Autostar/LX200) or a **TeenAstro extension**.

## Build

Requires [PlatformIO](https://platformio.org/) (CLI or IDE).

```bash
# Build default env (e.g. 240_5160)
pio run -e 240_5160

# List environments
pio run --list-envs
```

Board and mount variant are selected via `platformio.ini` envs (e.g. `240_5160`, `260_zh-1`). Output goes to `pio/<env>/`.

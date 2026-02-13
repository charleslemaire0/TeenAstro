# TeenAstro Main Unit

Firmware for the TeenAstro telescope mount controller. It drives stepper motors and exposes an LX200-style serial command set for alignment, goto, tracking, guiding, and configuration.

## Source layout

- **Global.h / Globals.cpp** – Shared types, enums, and global variable declarations (Global.h) and definitions (Globals.cpp). Includes EEPROM layout, mount state, and command buffers.
- **MainUnitDecl.h** – Cross-module function declarations (included from Global.h).
- **Command layer** – `Command.h`, `CommandConstants.h`, `Command.cpp` (dispatcher and reply helpers), and `Command_*.cpp` handlers by lead character (G, S, T, M, F, h, etc.).
- **Motion / control** – `Timer.cpp` (sidereal and axis timers), `Move.cpp`, `MoveTo.cpp`, `Goto.cpp`, `Guide.cpp`, `Limit.cpp`, `Park.cpp`, `Home.cpp`, `ST4.cpp`, `PushTo.cpp`, `Astro.cpp`.
- **EEPROM** – `EEPROM_address.h` (layout and `getMountAddress`), `EEPROM.cpp` (init, mount/motor/encoder config).
- **Config** – `Config.TeenAstro.h`, `Pins.TeenAstro.h`, `Config.Mount.*.h` (board/mount variants), `FirmwareDef.h` (version, init key).
- **Entry** – `TeenAstroMainUnit.cpp` provides `setup()` and `loop()`.

## Build

Requires [PlatformIO](https://platformio.org/) (CLI or IDE).

```bash
# Build default env (e.g. 240_5160)
pio run -e 240_5160

# List environments
pio run --list-envs
```

Board and mount variant are selected via `platformio.ini` envs (e.g. `240_5160`, `260_zh-1`). Output goes to `pio/<env>/`.

# TeenAstro PC Emulator

Runs the TeenAstro MainUnit and Smart Hand Controller (SHC) firmware on a desktop PC.

## Layout

- **emu/** – Entry points and cockpit UI: `mainunit_emu.cpp`, `shc_emu.cpp`, `cockpit_sdl.h`, launcher scripts
- **shim/** – Arduino/display/pad stubs for native build (TcpStream, EEPROM, SDL2 display, etc.)
- **src/** – Single translation units: `emu_mainunit_main.cpp`, `emu_shc_main.cpp`

## Build

From the **repository root**:

```bash
# MainUnit (start this first)
pio run -d TeenAstroEmulator -e emu_mainunit

# SHC
pio run -d TeenAstroEmulator -e emu_shc
```

Executables: `TeenAstroEmulator/.pio/build/emu_mainunit/program.exe` and `TeenAstroEmulator/.pio/build/emu_shc/program.exe`.

## Run

1. Start MainUnit: run `program.exe` from `emu_mainunit` build folder (or use `emu/launch_emu.bat` / `emu/launch_emu.ps1` to build and start both).
2. Start SHC: run `program.exe` from `emu_shc` build folder. It connects to `127.0.0.1:9998`.

**Ports**

- **9997** – USB serial (ASCOM / PC clients), localhost
- **9998** – SHC serial, localhost
- **9999** – WiFi/Android (SkySafari, etc.), all interfaces (`0.0.0.0`)

## Prerequisites

- PlatformIO CLI (`pio`)
- SDL2 at `C:\SDL2` on Windows (or adjust `platformio.ini` include/lib paths)
- Copy `SDL2.dll` into both build output folders if needed (launcher scripts do this automatically)

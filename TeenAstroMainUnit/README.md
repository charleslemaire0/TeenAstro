# TeenAstro Main Unit

Firmware for the TeenAstro telescope mount controller. It drives stepper motors and exposes an LX200-style serial command set for alignment, goto, tracking, guiding, and configuration.

## Source layout

- **MainUnit.h** – Mount + cross-module declarations (includes Mount.h and MainUnitDecl.h). Use Command.h / CommandGlobals.h for the command layer.
- **MainUnitDecl.h** – Cross-module function declarations (included from MainUnit.h).
- **Command.cpp** – Defines commandState and rtk (single definitions); dispatcher and reply helpers.
- **Command layer** – `Command.h`, `CommandConstants.h`, `CommandHelpers.h`, `Command.cpp` (dispatcher and reply helpers), and `Command_*.cpp` handlers by lead character (G, S, T, M, F, h, etc.). The SX and GX mega-handlers are split into domain sub-handlers (Motors, Encoders, Rates, Limits, Time, Options, etc.) as static functions within their respective files.
- **CommandHelpers.h** – `AxisRef` struct and `selectAxis()` function: bundles references to one axis's motor, encoder, stepper, geo, and EEPROM offsets. Eliminates all D/R axis-selection duplication across motor and encoder commands.
- **Shared library** – `TeenAstroCommandDef` (in `../libraries/TeenAstroCommandDef/`) provides enums (`CommandEnums.h`), typed structs (`CommandTypes.h`), wire format metadata (`CommandMeta.h` with `Cmd::` namespace and `getReplyType()`), and all value encoding/decoding (`CommandCodec.h`). Both the client (LX200Client) and server (MainUnit) consume this shared library to stay in sync.
- **Application** – `Application.h`, `Application.cpp` (setup and main loop, lifecycle).
- **Mount** – Mount class and sub-modules in `Mount.h` and `Mount*.cpp`: `Mount.cpp` (constructor, init, delegation), `MountLimits.cpp` (limit data and checks), `MountGotoSync.cpp` (goto and sync), `MountParkHomeController.cpp` (park/home behaviour; state in `MountParkHome.h` struct), `MountST4.cpp` (ST4 guiding), `MountGuiding.cpp` (guiding), `MountMove.cpp` / `MountMoveTo.cpp` (motion), `MountQueriesTracking.cpp` (tracking queries and rates), plus `MountAxes.cpp`, `MountLimits.cpp`, `MountGotoSync.cpp`, etc. Data-only sub-objects (errors, identity, parkHome, targetCurrent, tracking) are structs in their .h files; no separate .cpp. Access: direct members by default.
- **Motion / control** – Motion, goto, guide, limit, park, home, and ST4 logic live in the various `Mount*.cpp` files; `Timer.cpp` (sidereal and axis timers), `PushTo.cpp`, and `Astro.cpp` provide push-to and astro helpers.
- **EEPROM** – `EEPROM_address.h` (layout and `getMountAddress`), `EEPROM.cpp` (init, mount/motor/encoder config).
- **Config** – `Config.TeenAstro.h`, `Pins.TeenAstro.h`, `Config.Mount.*.h` (board/mount variants), `FirmwareDef.h` (version, init key).
- **ValueToString.h** – Backward-compatible header; re-exports `<CommandCodec.h>` from the shared library (all encode/decode functions now live there).
- **Entry** – `TeenAstroMainUnit.cpp` provides `setup()` and `loop()`, which delegate to `Application::setup()` and `Application::loop()`.

## Command handler architecture

The command layer uses a two-level dispatch:

1. **Top-level** (`Command.cpp`) — switches on `command[0]` using `Cmd::` constants and delegates to `Command_A()`, `Command_G()`, etc.
2. **Per-letter** (`Command_*.cpp`) — switches on `command[1]` (and deeper) for sub-commands.

For the two largest handlers (GX and SX), the code is further split into **domain sub-handlers** as static functions:

| Domain | GX (Get) | SX (Set) |
|--------|----------|----------|
| Alignment | `Command_GX_Alignment()` | `Command_SX_Alignment()` |
| Encoders | `Command_GX_Encoders()` | `Command_SX_Encoders()` |
| Motors | `Command_GX_Motors()` | `Command_SX_Motors()` |
| Rates | `Command_GX_Rates()` | `Command_SX_Rates()` |
| Limits | `Command_GX_Limits()` | `Command_SX_Limits()` |
| Time | `Command_GX_Time()` | `Command_SX_Time()` |
| Options | `Command_GX_Options()` | `Command_SX_Options()` |
| Refraction | `Command_GX_Refraction()` | `Command_SX_Refraction()` |
| Debug | `Command_GX_Debug()` | — |
| Status | `Command_GX_Status()` | — |
| ASCOM | `Command_GX_ASCOM()` | — |
| Position | `Command_GX_Position()` | — |

### Axis helper (`CommandHelpers.h`)

Motor and encoder commands that operate on either axis use the `AxisRef` struct and `selectAxis()` function to eliminate code duplication:

```cpp
AxisRef* ax = selectAxis(commandState.command[4]); // 'R' → axis1, 'D' → axis2
if (ax) {
    ax->motor.backlashAmount = i;
    XEEPROM.writeUShort(getMountAddress(ax->eeBacklashAmount), ax->motor.backlashAmount);
    ax->stepper.setBacklash_inSteps(ax->motor.backlashAmount, ax->geo.stepsPerArcSecond);
}
```

`AxisRef` bundles references to `MotorAxis`, `EncoderAxis`, `StatusAxis`, `GeoAxis`, and the corresponding EEPROM address constants for one axis.

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

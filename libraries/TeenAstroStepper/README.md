# TeenAstroStepper

Stepper motor driver abstraction for the TeenAstro telescope controller. Provides a single `Driver` class that wraps multiple Trinamic stepper driver chips behind a common API.

## Supported drivers

| Type | Chip | Interface | Max current |
|------|------|-----------|-------------|
| `STEPDIR` | Generic | Step/Dir/Enable pins | N/A |
| `TMC26X` | TMC260/261/262 | SPI-like | 2000 mA |
| `TMC2130` | TMC2130 | SPI | 2000 mA |
| `TMC5160` | TMC5160 | SPI | 3000 mA |
| `TMC2660` | TMC2660 | SPI | 3000 mA |

## Main class: `Driver`

Header-only library (`TeenAstroStepper.h`).

### Initialization

```cpp
Driver driver;
driver.initMotor(
    Driver::TMC5160,   // driver type
    stepsPerRotation,
    enablePin, csPin, dirPin, stepPin,
    currentMA, microstepPower, silentMode
);
```

### API

| Method | Description |
|--------|-------------|
| `initMotor(type, ...)` | Initialize the selected driver with pins, current, microsteps |
| `getMode()` / `setmode(bool)` | Get/set silent (stealthChop/PWM) mode |
| `getCurrent()` / `setCurrent(mA)` | Get/set motor current in milliamps |
| `getMaxCurrent()` | Maximum current for the driver type |
| `setMicrostep(power)` | Set microsteps as power of 2 (e.g. 4 -> 16x) |
| `getSG()` / `setSG(val)` | StallGuard read/threshold (TMC26X only) |

### Driver configuration

- **TMC2130/5160**: Chopper tuning (`tbl(2)`, `toff(5)`, `hstrt(5)`, `hend(3)`), stealthChop threshold (`TPWMTHRS(64)`), 256-microstep interpolation (`intpol(1)`)
- **TMC26X**: SpreadCycle chopper, CoolStep disabled by default, StallGuard threshold 63

## Dependencies

- **TMC26XStepper** — TMC260/261/262 driver library
- **TMCStepper** — TMC2130/5160/2660 driver library (teemuatlut)

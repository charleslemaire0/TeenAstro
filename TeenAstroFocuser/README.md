# TeenAstro Focuser

Firmware for the TeenAstro standalone telescope focuser controller. It provides high-precision stepper-based focus control with temperature compensation, acceleration profiles, Trinamic driver support, and Moonlite/LX200-compatible serial commands.

---

## Hardware & Driver Support

The Focuser firmware runs on PJRC Teensy boards (Teensy 3.1 / 3.2 / LC) and supports multiple hardware revisions and motor drivers:

| Board Revision | Stepper Driver | Description |
|----------------|----------------|-------------|
| **2.2.0** | TMC2130 (SPI) / StepDir | Teensy 3.1 / 3.2 board |
| **2.3.0** | TMC2130 (SPI) / StepDir | Teensy 3.1 / 3.2 board |
| **2.4.0** | TMC2130 (SPI) / TMC5160 (SPI) | Latest hardware revision with silent StealthChop & StallGuard |

### Peripheral Features
- **Trinamic SPI Control**: Configurable microstepping, run/hold currents, and StealthChop silent motion.
- **Temperature Compensation**: DS18B20 1-Wire temperature sensor support with linear expansion coefficient modeling.
- **Real-Time Clock**: DS1302 RTC support for timestamping and timing.
- **EEPROM Storage**: Non-volatile storage of positions, backlash, speeds, and acceleration profiles.

---

## Command Interface

The focuser supports standard Moonlite-compatible commands as well as extended TeenAstro focuser commands (`:FA#`, `:Fa#`, `:FG#`, `:FS#`, etc.):

- **Position & Motion**: `:GP#` (Get Position), `:SP#` (Set Target Position), `:FG#` (Start Move / GoTo), `:FQ#` (Halt Motion), `:FI#` (Step In), `:FO#` (Step Out).
- **Status & Diagnostics**: `:FZ#` (Get Status / Moving flag), `:GT#` (Get Temperature).
- **Configuration & Limits**: Backlash compensation, maximum travel limits, motor speed, and acceleration settings.

---

## Building Firmware

Build with [PlatformIO](https://platformio.org/):

```bash
# Build default environment (240 board with TMC2130)
pio run -d TeenAstroFocuser

# Build specific board variants
pio run -d TeenAstroFocuser -e 240_5160
pio run -d TeenAstroFocuser -e 240_2130
pio run -d TeenAstroFocuser -e 230_2130
pio run -d TeenAstroFocuser -e 220_2130
```

To build all focuser variants as part of the full release distribution:

```bash
python build_firmware.py --target focuser
```

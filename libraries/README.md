# TeenAstro Shared Libraries

This directory contains both TeenAstro-specific core modules and external third-party libraries used across the various firmware targets (MainUnit, Smart Hand Controller, WiFi Server, Focuser, Universal MainUnit, and Desktop Unit Tests).

---

## TeenAstro Core Libraries

| Library | Purpose & Features | Documentation |
|---------|-------------------|---------------|
| **`TeenAstroLA3`** | 3D Linear Algebra, vector/matrix math, Euler angles, Saemundsson/Bennett atmospheric refraction, SVD | [README](TeenAstroLA3/README.md) |
| **`TeenAstroCoord`** | Coordinate frames: Equatorial (EQ), Horizontal (Alt/Az), Instrument (IN), Local Offset (LO) | [README](TeenAstroCoord/README.md) |
| **`TeenAstroCoordConv`** | Multi-star telescope alignment, Taki transformation matrices, SVD non-orthogonality correction | [README](TeenAstroCoordConv/README.md) |
| **`TeenAstroCatalog`** | Compact PROGMEM astronomical catalogs (Messier, NGC, IC, Caldwell, Herschel, Stars, Double/Variable Stars) | [README](TeenAstroCatalog/README.md) |
| **`TeenAstroCommandDef`** | Shared LX200 command opcodes, packet structures, wire format metadata, and binary serialization codecs | [README](TeenAstroCommandDef/README.md) |
| **`TeenAstroLX200io`** | Serial stream parser, LX200 protocol dispatching, and command frame handling | [README](TeenAstroLX200io/README.md) |
| **`TeenAstroMountStatus`** | Mount telemetry structures, tracking state flags, error bitmasks, and binary packing | [README](TeenAstroMountStatus/README.md) |
| **`TeenAstroMath`** | General mathematical utilities, angle normalization, and fast spherical trigonometry | [README](TeenAstroMath/README.md) |
| **`TeenAstroLanguage`** | Multi-language translation tables and localized UI strings (English, French, German) | [README](TeenAstroLanguage/README.md) |
| **`TeenAstroPad`** | Keypad matrix scanning, button debouncing, and long-press event management | [README](TeenAstroPad/README.md) |
| **`TeenAstroStepper`** | Stepper motion abstraction layer interfacing with TeensyStep and motor driver ICs | [README](TeenAstroStepper/README.md) |
| **`TeenAstroWifi`** | WiFi station/AP connection manager, Web server handler, and TCP bridge helpers for ESP8266/ESP32 | [README](TeenAstroWifi/README.md) |
| **`MotorDriver`** | Next-gen motor driver abstraction for Universal MainUnit supporting dynamic Step/Dir & velocity control | [README](MotorDriver/README.md) |
| **`svd3`** | Fast 3×3 Singular Value Decomposition (used by TeenAstroLA3 / CoordConv for optimal matrix orthogonalization) | — |
| **`U8g2ext`** | TeenAstro-specific display extensions and font definitions for U8g2 OLED screens | — |
| **`TeenAstoCustomizations`** | Hardware pinouts and board customization defines | — |

---

## Third-Party Libraries

| Library | Version / Origin | Description |
|---------|------------------|-------------|
| **`TeensyStep`** | [luni64/TeensyStep](https://github.com/luni64/TeensyStep) | High-performance hardware-timer stepper pulse generator for Teensy |
| **`TMCStepper`** | [teemuatlut/TMCStepper](https://github.com/teemuatlut/TMCStepper) | Driver library for Trinamic TMC2130, TMC5160, and TMC2209 stepper ICs |
| **`TMC26XStepper-master`** | [trinamic/TMC26X](https://github.com/trinamic/TMC26X) | Driver for legacy Trinamic TMC260 / TMC262 stepper drivers |
| **`U8g2`** | [olikraus/u8g2](https://github.com/olikraus/u8g2) | Monochrome graphical display driver for SSD1306, SH1106, etc. |
| **`OneButton`** | [mathertel/OneButton](https://github.com/mathertel/OneButton) | Single-pin button state machine for click, double-click, and hold events |
| **`DallasTemperature`** | [milesburton/Arduino-Temperature-Control-Library](https://github.com/milesburton/Arduino-Temperature-Control-Library) | Maxim DS18B20 1-Wire temperature sensor driver |
| **`OneWire`** | [PaulStoffregen/OneWire](https://github.com/PaulStoffregen/OneWire) | 1-Wire protocol communication library |
| **`TinyGPSPlus-1.0.2`** | [mikalhart/TinyGPSPlus](https://github.com/mikalhart/TinyGPSPlus) | NMEA GPS receiver data parser |
| **`Time-master`** | [PaulStoffregen/Time](https://github.com/PaulStoffregen/Time) | Time keeping and Julian date computation |
| **`ephemeris-master`** | [SebastienC2/ephemeris](https://github.com/SebastienC2/ephemeris) | Ephemeris calculation for sun, moon, and solar system planets |
| **`RokkitHash`** | [clemair/RokkitHash](https://github.com/clemair/RokkitHash) | Extremely fast string hashing function for command routing |

---

## Including in PlatformIO Projects

When compiling any TeenAstro sub-project with PlatformIO, `lib_dir = ../libraries` is configured in `platformio.ini`, allowing automatic discovery and dependency resolution of all needed modules.

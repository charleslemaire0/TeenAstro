# System overview & architecture

TeenAstro is an open-source telescope mount controller for German Equatorial (GEM), Fork, Alt-Az, and hybrid mounts. It comprises embedded firmware (Teensy, ESP8266), a Flutter app (Android/Windows), and Windows ASCOM integration.

## Components

| Component | Platform | Language | Role |
|-----------|----------|----------|------|
| TeenAstroMainUnit | Teensy 3/4 | C++ | Mount control, motor ISR, goto, tracking, alignment, EEPROM |
| UniversalMainUnit | Teensy / ESP32 | C++ | Next-gen main unit (custom platform) |
| TeenAstroSHC | ESP8266 | C++ | Hand controller — OLED, buttons, menus, catalogs |
| TeenAstroServer | ESP8266 | C++ | WiFi ↔ serial TCP bridge, web config |
| TeenAstroFocuser | Teensy | C++ | Focuser motor, temperature sensor |
| teenastro_app | Flutter | Dart | Dashboard, planetarium, goto, alignment |
| TeenAstroASCOM_V7 | .NET | C# | ASCOM driver for Stellarium, Cartes du Ciel, etc. |
| TeenAstroUploader | .NET | VB.NET | Firmware flash utility |
| TeenAstroEmulator | PlatformIO | C++ | SHC emulator for desktop testing |

## Data flow (topology)

```
                    ┌─────────────────┐
                    │  Flutter App    │
                    │  (Android/Win)  │
                    └────────┬────────┘
                             │ TCP :9999 (LX200)
                    ┌────────▼────────┐
                    │  WiFi Server    │
                    │  (ESP8266)      │
                    └────────┬────────┘
                             │ Serial (57600)
┌──────────────┐    ┌────────▼────────┐    ┌──────────────┐
│  SHC         ├────┤  MainUnit       ├────┤  Focuser     │
│  (ESP8266)   │    │  (Teensy 3/4)   │    │  (Teensy)   │
└──────────────┘    └────────┬────────┘    └──────────────┘
                             │ USB / Serial
                    ┌────────▼────────┐
                    │  ASCOM / PC     │
                    └────────────────┘
```

- **Protocol:** LX200 (`:CMD#`). MainUnit is the endpoint; Server and SHC are bridges/clients.
- **Sidereal clock:** ~100 Hz (`masterClockSpeed = 1e6`, `siderealClockSpeed ≈ 997269.5625`).
- **Timers:** TIMER3/TIMER4 generate step pulses per axis; main loop does commands, tracking, goto, safety.
- **Polling:** Clients use `:GXAS#` (102 bytes) for state and `:GXCS#` (90 bytes) for config.

## Coordinate pipeline

```
Catalog (J2000 RA/Dec)
    │ precession + nutation + aberration
    ▼
Apparent (JNow RA/Dec)
    │ EQ → HO (latitude, LST)
    ▼
Horizontal (Alt/Az)
    │ Atmospheric refraction (Saemundsson / Bennett)
    ▼
Apparent Horizontal
    │ Alignment matrix T (Taki + SVD)
    ▼
Instrument (Axis1, Axis2, Axis3)
    │ gear, microstep, steps/rotation
    ▼
Stepper positions (steps)
```

## Shared libraries (`libraries/`)

| Library | Purpose |
|---------|---------|
| TeenAstroLA3 | 3-vector and 3×3 matrix, rotations, SVD, atmospheric refraction |
| TeenAstroCoord | Coordinate systems EQ, HO, IN, LO and conversions |
| TeenAstroCoordConv | Mount alignment (Taki method + SVD optimal rotation) |
| TeenAstroMountStatus | Mount state/config; parses GXAS/GXCS |
| TeenAstroStepper | Stepper abstraction (StepDir, TMC5160) |
| TeenAstroLX200io | LX200 transport, client, navigation |
| TeenAstroCatalog | PROGMEM catalogs (Messier, NGC, stars) |
| TeenAstroLanguage | SHC UI strings (EN, FR, DE) |
| TeenAstroWifi | ESP8266 TCP server and web config |
| TeenAstroCommandDef | Command/reply lengths (C++ and Dart) |

---

**Next:** [Math — Linear algebra (LA3)](math/la3.md) · [Coordinate systems](math/coord.md) · [Alignment](math/alignment.md)

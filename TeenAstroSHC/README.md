# TeenAstro Smart Hand Controller (SHC)

ESP8266-based hand controller with OLED display and button pad. Communicates with the TeenAstro MainUnit over serial using the LX200 protocol. Provides on-device setup, catalogs, goto, sync, and alignment.

**Full documentation:** [docs/firmware/shc.md](../docs/firmware/shc.md)

## Build

Requires PlatformIO (Teensy/ESP8266). From repo root:

```bash
pio run -d TeenAstroSHC
```

Language variants (EN, FR, DE) are selected via `platformio.ini` envs.

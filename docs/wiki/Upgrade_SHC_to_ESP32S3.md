# Upgrade SHC from Wemos / Lolin to ESP32‑S3

> Groups.io: https://groups.io/g/TeenAstro/wiki/43152

The hand‑controller PCB that takes a **Wemos D1 Mini** (ESP8266, often sold as Lolin) also accepts a **Lolin S3 Mini** (ESP32‑S3). The S3 module is required for **ASCOM Alpaca** (the ESP8266 1 MB image has no room for it). The rest of the PCB, OLED and buttons stay the same.

## What you need

- A **Lolin S3 Mini** (ESP32‑S3), USB‑C.
- The TeenAstro **S3** hand‑controller firmware: `TeenAstroSHC_<version>_S3_English.bin` (or `_S3_French` / `_S3_German`). That file is a **merged** flash image (bootloader + partitions + app at address `0x0`).
- A USB cable and a computer with [esptool](https://docs.espressif.com/projects/esptool/) (or PlatformIO).

## Hardware swap

1. Power the mount **off**.
2. Remove the Wemos D1 Mini / Lolin ESP8266 from its socket.
3. Insert the **Lolin S3 Mini**.  
   The module has a dual‑row header; **only the outer row** mates with the D1 Mini socket (USB at the bottom of the module, antenna at the top — same orientation as the Wemos). Do not force the inner row into the shield.
4. Leave the OLED and button wiring unchanged. The firmware already maps the S3 outer pins to the same roles as the Wemos (serial to the main unit, I²C display, buttons).

## First flash (USB)

The Lolin S3 Mini enumerates as a USB serial device (CDC). Prefer a direct flash over COM for the first image.

With esptool (replace `COMx` and the bin path):

```text
esptool --chip esp32s3 --port COMx --baud 921600 write_flash 0x0 TeenAstroSHC_<version>_S3_English.bin
```

Or from the repo with PlatformIO:

```text
pio run -d TeenAstroSHC -e esp32s3 -t upload
```

(Use `esp32s3_FRENCH` or `esp32s3_GERMAN` for the other languages.)

**TeenAstroUploader** “Upload over COM!” still targets the ESP8266 chip by default. Until an S3 board choice appears in the Hand controller tab, use esptool or PlatformIO for the first S3 flash. Later updates can use the Webserver **Update Firmware** page with the **application** `.bin` (not the merged image).

## After the upgrade

1. Power on. Confirm the usual TeenAstro splash and menus.
2. Turn Wi‑Fi on (**Telescope Settings → Wifi**), reboot, read **Show IP**.
3. Alpaca is available on TCP **11111** (discovery UDP **32227**). Requirements and client notes: [ASCOM Alpaca](ASCOM_Alpaca.md).
4. The LX200 TCP bridge on port **9999** and the classic ASCOM driver keep working as before.

## Related

- [ASCOM Alpaca](https://groups.io/g/TeenAstro/wiki/43151)
- [TeenAstroUploader — Hand controller](https://groups.io/g/TeenAstro/wiki/43072)
- [WiFi Interface](https://groups.io/g/TeenAstro/wiki/14592)
- [Start here](https://groups.io/g/TeenAstro/wiki/24430)

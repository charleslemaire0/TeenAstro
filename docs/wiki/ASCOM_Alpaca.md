# ASCOM Alpaca

> Groups.io: https://groups.io/g/TeenAstro/wiki/43151

TeenAstro can speak **ASCOM Alpaca** (HTTP/REST + JSON) over Wi‑Fi. Compatible apps (NINA, Stellarium, KStars, ASCOM Remote, custom scripts, …) discover the mount on the LAN and drive it without the classic Windows COM driver or the LX200 TCP bridge on port 9999.

![ASCOM Alpaca](screenshots/ASCOM_Alpaca_logo.png)

## What you need

1. **Hand controller with a Lolin S3 Mini (ESP32‑S3)**  
   The production Wemos D1 Mini (ESP8266) image has no room for Alpaca. Swap the module and flash the **S3** firmware — see [Upgrade SHC to ESP32‑S3](https://groups.io/g/TeenAstro/wiki/43152).

2. **Firmware that includes Alpaca**  
   SHC builds named `TeenAstroSHC_*_S3_English.bin` (or French / German). The web UI stays on port **80**; Alpaca listens on TCP **11111**.

3. **Wi‑Fi on**  
   **Telescope Settings → Wifi → Turn Wifi on**, then reboot. Use Access Point or Station as on the [WiFi Interface](https://groups.io/g/TeenAstro/wiki/14592) page. Read the address with **Show IP**.

4. **Same network as the computer or phone**  
   In Access Point mode join **TeenAstro** (default address **192.168.0.1**). In Station mode use the address from **Show IP**.

5. **An Alpaca‑aware client**  
   Discovery uses UDP **32227** (`alpacadiscovery1`). Most clients find the device automatically; otherwise point them at the IP and port **11111**.

## Ports (summary)

| Service              | Port        |
|----------------------|-------------|
| Web configuration UI | TCP 80      |
| LX200 TCP bridge     | TCP 9999    |
| ASCOM Alpaca API     | TCP 11111   |
| Alpaca discovery     | UDP 32227   |

The classic [ASCOM Driver](https://groups.io/g/TeenAstro/wiki/8102) (COM / IP on 9999) remains available. Alpaca is the direct network path for apps that speak the Alpaca REST API.

## Devices exposed

- **Telescope** — mount goto, sync, tracking, park/home, pulse guide, move axis, site/time.
- **Focuser** — when a focuser is attached to the main unit.

## Related

- [Upgrade SHC to ESP32‑S3](https://groups.io/g/TeenAstro/wiki/43152)
- [WiFi Interface](https://groups.io/g/TeenAstro/wiki/14592)
- [Webserver](https://groups.io/g/TeenAstro/wiki/8105)
- [ASCOM Driver](https://groups.io/g/TeenAstro/wiki/8102)
- Field manuals: [EN](https://github.com/charleslemaire0/TeenAstro/blob/Release_1.6/docs/manuel/Manuel_utilisateur_en.pdf) · [FR](https://github.com/charleslemaire0/TeenAstro/blob/Release_1.6/docs/manuel/Manuel_utilisateur_fr.pdf) · [DE](https://github.com/charleslemaire0/TeenAstro/blob/Release_1.6/docs/manuel/Manuel_utilisateur_de.pdf)

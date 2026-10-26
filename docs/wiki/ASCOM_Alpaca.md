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

- **Telescope** — mount goto, sync, tracking, park/home, pulse guide, move axis, site/time, and plate-solve alignment.
- **Focuser** — when a focuser is attached to the main unit.

## Plate-solve alignment

The Windows ASCOM driver and Alpaca use the same action names. The tube is at home before `AlignStart`. The program slews, plate-solves, and accepts the solved coordinates. It does not recenter. Sync does not add a star of a multi-star session.

| Action | Parameters | Role |
|--------|------------|------|
| `AutoAlign` | | Two-star alignment by sync (**PC Alignment** on the hand controller). Then two syncs of the solved position. |
| `AlignStart` | `2`, `m`, `r3`…`r9` | Start from home. `r4` is **4 Stars**. `r6` is **3+3 Stars**. `m` is **2 Stars Mech.** |
| `AlignAtTarget` | `2`, `m`, `r3`…`r9` | The tube is already on the first star. |
| `AlignAccept` | `star` or `star,raHours,decDeg` | Record that star against the solved coordinates. Right ascension is hours, declination is degrees. |
| `AlignSave` | | Write the model. |
| `AlignAbort` | | Abandon the session. |
| `AlignPolarDone` | | Close **2 Stars Mech.** after the bolt pass. |
| `AlignStatus` | | Stars collected, residual in arcseconds, which of CH, NP and ID were solved, and how many stars lie on each pier side. |
| `AlignErrors` | | CH, NP, ID and the residual in arcseconds, and ME, MA and the two-star error in degrees. |

An alt-azimuth slew is fine. The coordinates given to `AlignAccept` are still the solved right ascension and declination.

ASCOM:

```text
Action("AlignStart", "r6")
Action("AlignAccept", "1,12.345678,45.123456")
Action("AlignSave", "")
```

Repeat `AlignAccept` for stars 1 through 6, each time with that star's solved coordinates.

Alpaca sends the same names:

```text
PUT /api/v1/telescope/0/action
    Action=AlignStart&Parameters=r6

PUT /api/v1/telescope/0/action
    Action=AlignAccept&Parameters=1,12.345678,45.123456

PUT /api/v1/telescope/0/action
    Action=AlignSave
```

`AlignStatus` returns text such as `stars=6;rms=12.500000;terms=-PI;pier=3,3`. In `terms`, `C` is CH, `P` is NP, `I` is ID, and a dash means that term was not solved.

`AlignErrors` returns text such as `ch=1.500000;np=-2.250000;id=0.000000;me=0.209444;ma=-1.500000;rms=12.500000;ae=0.016667`. `ch`, `np`, `id` and `rms` are arcseconds. `me` is ME, `ma` is MA, and `ae` is the two-star error, all in degrees.

A sync once the model is saved only corrects the current pointing. `AutoAlign` again, `AlignStart` again, or **Clear** on the hand controller, resets the model. While the program is accepting stars, leave the hand controller alone.

The same sequence is in the field manuals, chapter 15, **From a computer**.

## Related

- [Upgrade SHC to ESP32‑S3](https://groups.io/g/TeenAstro/wiki/43152)
- [WiFi Interface](https://groups.io/g/TeenAstro/wiki/14592)
- [Webserver](https://groups.io/g/TeenAstro/wiki/8105)
- [ASCOM Driver](https://groups.io/g/TeenAstro/wiki/8102)
- Field manuals: [EN](https://github.com/charleslemaire0/TeenAstro/blob/Release_1.6/docs/manuel/Manuel_utilisateur_en.pdf) · [FR](https://github.com/charleslemaire0/TeenAstro/blob/Release_1.6/docs/manuel/Manuel_utilisateur_fr.pdf) · [DE](https://github.com/charleslemaire0/TeenAstro/blob/Release_1.6/docs/manuel/Manuel_utilisateur_de.pdf)
